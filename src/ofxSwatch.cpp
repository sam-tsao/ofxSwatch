#include "ofxSwatch.h"

void ofxSwatch::setup(float w, float h, int internalformat, int numSamples) {
	gradientShader.setupShaderFromSource(GL_VERTEX_SHADER,getVertexShader());
	gradientShader.setupShaderFromSource(GL_FRAGMENT_SHADER,getFragmentShader());
	gradientShader.bindDefaults();
	gradientShader.linkProgram();

	width = w; height = h;
	swatchFbo.allocate(w, h, internalformat, numSamples);
}

void  ofxSwatch::addColor(ofColor color, int rowIndex) {
	row[rowIndex].colors.push_back(color);
}

void  ofxSwatch::addColors(vector<ofColor> colors, int rowIndex) {
	for (ofColor c : colors) {
		row[rowIndex].colors.push_back(c);
	}
}

void ofxSwatch::addPalette() {
	Row r;
	r.rowMode = swatchMode::PALETTE;
	row.push_back(r);
}

void ofxSwatch::addGradient() {
	Row r;
	r.rowMode = swatchMode::GRADIENT;
	row.push_back(r);
}
void ofxSwatch::addPalette(vector<ofColor> colors) {
	Row r;
	for (ofColor c : colors) {
		r.colors.push_back(c);
	}
	r.rowMode = swatchMode::PALETTE;
	row.push_back(r);
}

void ofxSwatch::addGradient(vector<ofColor> colors) {
	Row r;
	for (ofColor c : colors) {
		r.colors.push_back(c);
	}
	r.rowMode = swatchMode::GRADIENT;
	row.push_back(r);
}

void  ofxSwatch::createSwatch() {
	int numRows = row.size();
	float rowHeight = height / (float)numRows;
	swatchFbo.begin();
	for (int ROW = 0; ROW < numRows; ROW++) {
		int numVertices = row[ROW].colors.size();
		float vertDist = (float)width / (float)(numVertices - 1);
		if (row[ROW].rowMode == swatchMode::GRADIENT) {
			/*
			The gradient is created by assigning colors to vertices
			and letting OpenGL interpolate 
			*/
			ofMesh mesh;
			mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
			for (int x = 0; x < numVertices; x++) {
				for (int y = 0; y < 2; y++) {
					mesh.addVertex({ vertDist*x, rowHeight*(y + ROW),0 });
					mesh.addColor(row[ROW].colors[x]);
				}
			}
			gradientShader.begin();
			mesh.draw();
			gradientShader.end();
		}
		else if (row[ROW].rowMode == swatchMode::PALETTE) {
			float paletteWidth = (float)width / (float)(numVertices);
			for (int x = 0; x < numVertices; x++) {
				ofPushStyle();
				ofSetColor(row[ROW].colors[x]);
				ofDrawRectangle(
					x*paletteWidth,
					ROW*rowHeight,
					paletteWidth,
					rowHeight);
				ofPopStyle();
				
			}
		}
	}
	swatchFbo.end();

	swatchFbo.readToPixels(swatchPix);
}

ofColor ofxSwatch::sample(float val, int rowIndex) {
	int x = floor(val * width);
	if (x == (int)width) //make sure X is within bounds
		x = width - 1;
	int y = floor(rowIndex * (height / (float)row.size()));
	return swatchPix.getColor(x, y);
}

ofTexture & ofxSwatch::getTexture() {
	return swatchFbo.getTexture();
}

void ofxSwatch::draw() {
	swatchFbo.draw(0, 0);
}

string ofxSwatch::getVertexShader() {
	string shader = R"(
#version 460

uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec4 color;
out vec4 v_color;
vec3 rgb2xyz( vec3 c ) {
    vec3 tmp;
    tmp.x = ( c.r > 0.04045 ) ? pow( ( c.r + 0.055 ) / 1.055, 2.4 ) : c.r / 12.92;
    tmp.y = ( c.g > 0.04045 ) ? pow( ( c.g + 0.055 ) / 1.055, 2.4 ) : c.g / 12.92,
    tmp.z = ( c.b > 0.04045 ) ? pow( ( c.b + 0.055 ) / 1.055, 2.4 ) : c.b / 12.92;
    return 100.0 * tmp *
        mat3( 0.4124, 0.3576, 0.1805,
              0.2126, 0.7152, 0.0722,
              0.0193, 0.1192, 0.9505 );
}

vec3 xyz2lab( vec3 c ) {
    vec3 n = c / vec3( 95.047, 100, 108.883 );
    vec3 v;
    v.x = ( n.x > 0.008856 ) ? pow( n.x, 1.0 / 3.0 ) : ( 7.787 * n.x ) + ( 16.0 / 116.0 );
    v.y = ( n.y > 0.008856 ) ? pow( n.y, 1.0 / 3.0 ) : ( 7.787 * n.y ) + ( 16.0 / 116.0 );
    v.z = ( n.z > 0.008856 ) ? pow( n.z, 1.0 / 3.0 ) : ( 7.787 * n.z ) + ( 16.0 / 116.0 );
    return vec3(( 116.0 * v.y ) - 16.0, 500.0 * ( v.x - v.y ), 200.0 * ( v.y - v.z ));
}

vec3 rgb2lab(vec3 c) {
    vec3 lab = xyz2lab( rgb2xyz( c ) );
    return vec3( lab.x / 100.0, 0.5 + 0.5 * ( lab.y / 127.0 ), 0.5 + 0.5 * ( lab.z / 127.0 ));
}

vec3 lab2xyz( vec3 c ) {
    float fy = ( c.x + 16.0 ) / 116.0;
    float fx = c.y / 500.0 + fy;
    float fz = fy - c.z / 200.0;
    return vec3(
         95.047 * (( fx > 0.206897 ) ? fx * fx * fx : ( fx - 16.0 / 116.0 ) / 7.787),
        100.000 * (( fy > 0.206897 ) ? fy * fy * fy : ( fy - 16.0 / 116.0 ) / 7.787),
        108.883 * (( fz > 0.206897 ) ? fz * fz * fz : ( fz - 16.0 / 116.0 ) / 7.787)
    );
}

vec3 xyz2rgb( vec3 c ) {
    vec3 v =  c / 100.0 * mat3( 
        3.2406, -1.5372, -0.4986,
        -0.9689, 1.8758, 0.0415,
        0.0557, -0.2040, 1.0570
    );
    vec3 r;
    r.x = ( v.r > 0.0031308 ) ? (( 1.055 * pow( v.r, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.r;
    r.y = ( v.g > 0.0031308 ) ? (( 1.055 * pow( v.g, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.g;
    r.z = ( v.b > 0.0031308 ) ? (( 1.055 * pow( v.b, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.b;
    return r;
}

vec3 lab2rgb(vec3 c) {
    return xyz2rgb( lab2xyz( vec3(100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5)) ) );
}
void main(){
	gl_Position = modelViewProjectionMatrix * position;
	vec3 col = rgb2lab(color.rgb);
	v_color = vec4(col,1.);
}
	)";

	return shader;
}

string ofxSwatch::getFragmentShader() {
	string shader = R"(
#version 460

in vec4 v_color;
vec3 rgb2xyz( vec3 c ) {
    vec3 tmp;
    tmp.x = ( c.r > 0.04045 ) ? pow( ( c.r + 0.055 ) / 1.055, 2.4 ) : c.r / 12.92;
    tmp.y = ( c.g > 0.04045 ) ? pow( ( c.g + 0.055 ) / 1.055, 2.4 ) : c.g / 12.92,
    tmp.z = ( c.b > 0.04045 ) ? pow( ( c.b + 0.055 ) / 1.055, 2.4 ) : c.b / 12.92;
    return 100.0 * tmp *
        mat3( 0.4124, 0.3576, 0.1805,
              0.2126, 0.7152, 0.0722,
              0.0193, 0.1192, 0.9505 );
}

vec3 xyz2lab( vec3 c ) {
    vec3 n = c / vec3( 95.047, 100, 108.883 );
    vec3 v;
    v.x = ( n.x > 0.008856 ) ? pow( n.x, 1.0 / 3.0 ) : ( 7.787 * n.x ) + ( 16.0 / 116.0 );
    v.y = ( n.y > 0.008856 ) ? pow( n.y, 1.0 / 3.0 ) : ( 7.787 * n.y ) + ( 16.0 / 116.0 );
    v.z = ( n.z > 0.008856 ) ? pow( n.z, 1.0 / 3.0 ) : ( 7.787 * n.z ) + ( 16.0 / 116.0 );
    return vec3(( 116.0 * v.y ) - 16.0, 500.0 * ( v.x - v.y ), 200.0 * ( v.y - v.z ));
}

vec3 rgb2lab(vec3 c) {
    vec3 lab = xyz2lab( rgb2xyz( c ) );
    return vec3( lab.x / 100.0, 0.5 + 0.5 * ( lab.y / 127.0 ), 0.5 + 0.5 * ( lab.z / 127.0 ));
}

vec3 lab2xyz( vec3 c ) {
    float fy = ( c.x + 16.0 ) / 116.0;
    float fx = c.y / 500.0 + fy;
    float fz = fy - c.z / 200.0;
    return vec3(
         95.047 * (( fx > 0.206897 ) ? fx * fx * fx : ( fx - 16.0 / 116.0 ) / 7.787),
        100.000 * (( fy > 0.206897 ) ? fy * fy * fy : ( fy - 16.0 / 116.0 ) / 7.787),
        108.883 * (( fz > 0.206897 ) ? fz * fz * fz : ( fz - 16.0 / 116.0 ) / 7.787)
    );
}

vec3 xyz2rgb( vec3 c ) {
    vec3 v =  c / 100.0 * mat3( 
        3.2406, -1.5372, -0.4986,
        -0.9689, 1.8758, 0.0415,
        0.0557, -0.2040, 1.0570
    );
    vec3 r;
    r.x = ( v.r > 0.0031308 ) ? (( 1.055 * pow( v.r, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.r;
    r.y = ( v.g > 0.0031308 ) ? (( 1.055 * pow( v.g, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.g;
    r.z = ( v.b > 0.0031308 ) ? (( 1.055 * pow( v.b, ( 1.0 / 2.4 ))) - 0.055 ) : 12.92 * v.b;
    return r;
}

vec3 lab2rgb(vec3 c) {
    return xyz2rgb( lab2xyz( vec3(100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5)) ) );
}
void main(){
	gl_FragColor = vec4(lab2rgb(v_color.rgb),1.);
			}
	)";
	return shader;
}
