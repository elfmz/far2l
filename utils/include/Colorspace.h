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
