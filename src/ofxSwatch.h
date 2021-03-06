/*
AUTHOR: Samuel Cho
YEAR: 2021
Title: ofxSwatch
Description: Texture Based Color Utility for OpenFrameworks
https://samuelcho.de
*/

#pragma once
#include "ofMain.h"

// Determines whether to use interpolation or to use blocks
// options: GRADIENT, PALETTE
enum  swatchMode { GRADIENT, PALETTE };

struct Row {
	vector<ofColor> colors;
	swatchMode rowMode = swatchMode::GRADIENT;
};

class ofxSwatch {
public:
	float width, height;
	ofFbo swatchFbo;
	ofPixels swatchPix;
	ofShader gradientShader;
	vector<Row> row;

	void setup(float w, float h, int internalformat = GL_RGBA, int numSamples = 0);

	//individually add color to a row
	void addColor(ofColor color, int rowIndex);
	//add a list of colors to a row
	void addColors(vector<ofColor> colors, int rowIndex);

	//add a palette to the swatch
	void addPalette();
	//add a gradient to the swatch
	void addGradient();

	//add a palette to the swatch with a list of colors
	void addPalette(vector<ofColor> colors);
	//add a gradient to the swatch with a list of colors
	void addGradient(vector<ofColor> colors);

	//generates a texture with the given colors
	void createSwatch();

	//samples the texture for the color using a value from 0.0-1.0
	ofColor sample(float val, int rowIndex);

	ofTexture & getTexture();
	void draw();

	string getVertexShader();
	string getFragmentShader();
};