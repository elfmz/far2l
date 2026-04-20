#pragma once

enum class ContrastLevel {
    Good,
    Warning,
    Bad
};

struct RGB { double r, g, b; }; // 0..1
struct LAB { double L, a, b; }; // L: 0–100, a/b: approx -128..127
struct iRGB { unsigned char r, g, b; }; // 0..255
struct HSL { double h, s, l; };


struct HoverResult {  RGB bg_hover, fg_hover; };

// helper functions
RGB toRGB(int r, int g, int b);

// Convert RGB → HSL

void RGBtoHSL(const RGB& c, double& h, double& s, double& l);
double HueToRGB(double p, double q, double t);
RGB HSLtoRGB(double h, double s, double l);

// Convert RGB → LAB

LAB RGBtoLAB(const RGB& c);
RGB LABtoRGB(const LAB& lab);

// Convert RGB → XYZ → LAB

void RGBtoXYZ(RGB& in, double& X, double& Y, double& Z);
LAB XYZtoLAB(double X, double Y, double Z);
void LABtoXYZ(const LAB& in, double& X, double& Y, double& Z);
iRGB XYZtoRGB(double X, double Y, double Z);

// Contrast computations

double wcagComputeContrast(const RGB& fg, const RGB& bg);
double deltaE2000(const LAB& fg, const LAB& bg);

// converters

RGB toRGB(int r, int g, int b);
RGB toRGB(const iRGB& c);
iRGB toIRGB(const RGB& c);
iRGB toIRGB(int r, int g, int b);
HSL toHSL(double h, double s, double l);

// Service functions

const char* contrastToString(ContrastLevel lvl);
ContrastLevel ComputeContrast(const RGB& fg, const RGB& bg, RGB& newFg);
RGB ComputeRaiseColor_HSL(const RGB& bg, const RGB& line);
RGB ComputeRaiseColor_LAB(const RGB& bg, const RGB& line);
HoverResult ComputeHoverColors(const RGB& bg, const RGB& fg);

iRGB ConvertForPrintLAB(const iRGB& in, const iRGB& bg);

HoverResult ComputeControlAccent(const RGB& fg, const RGB& bg);

bool IsNearBlack(int r, int g, int b, double threshold = 32.0);
bool IsNearWhite(int r, int g, int b, double threshold = 32.0);
bool IsNearBlack(RGB c, double threshold = 32.0);
bool IsNearWhite(RGB c, double threshold = 32.0);

iRGB SoftenBlackish_LAB(const RGB& c);

RGB SoftenToDisabledState_LAB(const RGB& cc, 
	double L_center = 60.0, /* target mid-gray center */
	double L_strength = 0.5,  // how strongly to pull L toward center
	double C_strength = 0.7);  // how strongly to desaturate
RGB SoftenToHoverState_LAB(const RGB& cc,
	const RGB& tint = { 0.0, 0.5, 1.0 }, // default light blue
	double L_boost = 0.10,   // +10% brightness
	double C_boost = 0.20,   // +20% chroma
	double tint_max = 0.40,  // max tint for pure black/white
	double tint_min = 0.10,  // min tint for slightly neutral colors
	double C_neutral = 20.0); // chroma threshold for "neutral"
RGB SoftenToFocusedState_LAB(const RGB& cc,
	const RGB& focusTint = { 0.4, 0.1, 1.0 }, // subtle blue-violet
	double L_boost = 0.05,   // +5% brightness
	double C_boost = 0.30,   // +30% chroma
	double tint_max = 0.25,  // max tint for pure neutrals
	double tint_min = 0.05,  // min tint for slightly neutral colors
	double C_neutral = 45.0); // chroma threshold for "neutral"
RGB SoftenToPressedState_LAB(const RGB& fg,
	const RGB& bgC,          // background LAB
	double L_push = 0.25,   // how strongly to push toward background L*
	double C_reduce = 0.40, // reduce chroma by 40%
	double neutral_tint = 0.20, // tint neutrals toward bg
	double C_neutral = 15.0);    // threshold for “neutral”
