/*
printersupport.cpp

Print capabilities from wxWidgets (if any).
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "printersupport.hpp"

////////////////////////////
// Bridge to backend

bool PrinterSupport::IsReducedHTMLSupported()
{
	return WINPORT(PrintIsHTMLSupported)();
}

bool PrinterSupport::IsPrinterSetupDialogSupported()
{
	return WINPORT(PrintIsSettingsDialogSupported)();
}

bool PrinterSupport::IsPrintPreviewSupported()
{
	return WINPORT(PrintIsPreviewSupported)();
}

void PrinterSupport::PrintText(const std::wstring& jobName, const std::wstring& text) 
{
	WINPORT(PrintTextFragment)(jobName.c_str(), text.c_str());
}

void PrinterSupport::PrintReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	WINPORT(PrintHtmlFragment)(jobName.c_str(), text.c_str());
}

void PrinterSupport::PrintTextFile(const std::wstring& fileName)
{
	WINPORT(PrintTextFile)(fileName.c_str());
}

void PrinterSupport::PrintHtmlFile(const std::wstring& fileName)
{
	WINPORT(PrintHtmlFile)(fileName.c_str());
}

void PrinterSupport::ShowPreviewForText(const std::wstring& jobName, const std::wstring& text)
{
	WINPORT(PrintPreviewTextFragment)(jobName.c_str(), text.c_str());
}

void PrinterSupport::ShowPreviewForReducedHTML(const std::wstring& jobName, const std::wstring& text)
{
	WINPORT(PrintPreviewHtmlFragment)(jobName.c_str(), text.c_str());
}

void PrinterSupport::ShowPreviewForTextFile(const std::wstring& fileName)
{
	WINPORT(PrintPreviewTextFile)(fileName.c_str());
}

void PrinterSupport::ShowPreviewForHtmlFile(const std::wstring& fileName)
{
	WINPORT(PrintPreviewHtmlFile)(fileName.c_str());
}

void PrinterSupport::ShowPrinterSetupDialog()
{
	WINPORT(PrintSettingsDialog)();
}

/////////////////////////
// Color spaces

struct LAB {
    double L, a, b; // L: 0–100, a/b: approx -128..127
};

// Helper: clamp
static double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

static double rgbToX(unsigned char rgbV) {
   double v = rgbV / 255.0;
   return (v > 0.04045) ? pow((v + 0.055) / 1.055, 2.4) : (v / 12.92);
}

// Convert RGB (0–255) → XYZ
static void RGBtoXYZ(const FarTrueColor& in, double& X, double& Y, double& Z)
{
    double r = rgbToX(in.R);
    double g = rgbToX(in.G);
    double b = rgbToX(in.B);

    // sRGB D65
    X = r * 0.4124 + g * 0.3576 + b * 0.1805;
    Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    Z = r * 0.0193 + g * 0.1192 + b * 0.9505;
}

static double xyzToL(double t) {
	return (t > 0.008856) ? pow(t, 1.0/3.0) : (7.787 * t + 16.0/116.0);
}

static double labToX(double t) {
    double t3 = t * t * t;
    return (t3 > 0.008856) ? t3 : ((t - 16.0/116.0) / 7.787);
}

// Convert XYZ → LAB
static LAB XYZtoLAB(double X, double Y, double Z)
{
    // Reference white D65
    const double Xr = 0.95047;
    const double Yr = 1.00000;
    const double Zr = 1.08883;

    double fx = xyzToL(X / Xr);
    double fy = xyzToL(Y / Yr);
    double fz = xyzToL(Z / Zr);

    LAB out;
    out.L = 116.0 * fy - 16.0;
    out.a = 500.0 * (fx - fy);
    out.b = 200.0 * (fy - fz);
    return out;
}

// Convert LAB → XYZ
static void LABtoXYZ(const LAB& in, double& X, double& Y, double& Z)
{
    const double Xr = 0.95047;
    const double Yr = 1.00000;
    const double Zr = 1.08883;

    double fy = (in.L + 16.0) / 116.0;
    double fx = fy + in.a / 500.0;
    double fz = fy - in.b / 200.0;

    X = Xr * labToX(fx);
    Y = Yr * labToX(fy);
    Z = Zr * labToX(fz);
}

static double xyzToR(double v) {
    return (v > 0.0031308) ? (1.055 * pow(v, 1.0/2.4) - 0.055) : (12.92 * v);
}

// Convert XYZ → RGB
static FarTrueColor XYZtoRGB(double X, double Y, double Z)
{
    double r =  3.2406*X - 1.5372*Y - 0.4986*Z;
    double g = -0.9689*X + 1.8758*Y + 0.0415*Z;
    double b =  0.0557*X - 0.2040*Y + 1.0570*Z;

    double rr = clamp(xyzToR(r) * 255.0, 0, 255);
    double gg = clamp(xyzToR(g) * 255.0, 0, 255);
    double bb = clamp(xyzToR(b) * 255.0, 0, 255);

    FarTrueColor out;
    out.R = (unsigned char)rr;
    out.G = (unsigned char)gg;
    out.B = (unsigned char)bb;

    return out;
}

FarTrueColor ColorspaceSupport::ConvertForPrintLAB(const FarTrueColor& in, const FarTrueColor& bg) {
    // Step 1: RGB → LAB
    double X, Y, Z;
    RGBtoXYZ(in, X, Y, Z);
    LAB lab = XYZtoLAB(X, Y, Z);

    RGBtoXYZ(bg, X, Y, Z);
    LAB labBg = XYZtoLAB(X, Y, Z);

    double deltaL = lab.L - labBg.L;

    const double Lwhite = 100.0; 
    const double k = 0.8;

    // Step 2: invert brightness (or darken)
    lab.L = Lwhite - k * fabs(deltaL);

	// Clamp to reasonable range 
	if (lab.L > 70) lab.L = 70; // avoid too light; too dark is OK 
	if (deltaL < 0) {
    	// foreground is darker than background
	    // ensure printed text is still dark enough
    	lab.L = std::min(lab.L, 50.0);
	}

    // Step 3: reduce chroma slightly (avoid neon colors)
    lab.a *= 0.7;
    lab.b *= 0.7;

    // Step 4: LAB → RGB
    LABtoXYZ(lab, X, Y, Z);
    return XYZtoRGB(X, Y, Z);
}
