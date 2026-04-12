#include "Colorspace.h"

#include <algorithm>
#include <cmath>

//////////////////
/// HSL colorspace
//////////////////

// Convert RGB → HSL
void RGBtoHSL(const RGB& c, double& h, double& s, double& l)
{
    double maxv = std::max({c.r, c.g, c.b});
    double minv = std::min({c.r, c.g, c.b});
    l = (maxv + minv) * 0.5;

    if (maxv == minv) {
        h = s = 0.0;
        return;
    }

    double d = maxv - minv;
    s = (l > 0.5) ? d / (2.0 - maxv - minv) : d / (maxv + minv);

    if (maxv == c.r)
        h = (c.g - c.b) / d + (c.g < c.b ? 6.0 : 0.0);
    else if (maxv == c.g)
        h = (c.b - c.r) / d + 2.0;
    else
        h = (c.r - c.g) / d + 4.0;

    h /= 6.0;
}

HSL toHSL(double h, double s, double l) {
	return HSL {h,s,l};
}

// Helper for HSL → RGB
double HueToRGB(double p, double q, double t)
{
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
    return p;
}

// Convert HSL → RGB
RGB HSLtoRGB(double h, double s, double l)
{
    RGB out;
    if (s == 0.0) {
        out.r = out.g = out.b = l;
        return out;
    }

    double q = (l < 0.5) ? l * (1.0 + s) : (l + s - l * s);
    double p = 2.0 * l - q;

    out.r = HueToRGB(p, q, h + 1.0/3.0);
    out.g = HueToRGB(p, q, h);
    out.b = HueToRGB(p, q, h - 1.0/3.0);

    return out;
}

///////////////////
/// LAB colorspace
///////////////////

// Gamma correction
static double InvGamma(double x)
{
    return (x <= 0.04045) ? (x / 12.92) : std::pow((x + 0.055) / 1.055, 2.4);
}

static double Gamma(double x)
{
    return (x <= 0.0031308) ? (12.92 * x) : (1.055 * std::pow(x, 1.0/2.4) - 0.055);
}

// RGB → LAB
LAB RGBtoLAB(const RGB& c)
{
    double r = InvGamma(c.r);
    double g = InvGamma(c.g);
    double b = InvGamma(c.b);

    // RGB → XYZ (D65)
    double X = r*0.4124 + g*0.3576 + b*0.1805;
    double Y = r*0.2126 + g*0.7152 + b*0.0722;
    double Z = r*0.0193 + g*0.1192 + b*0.9505;

    // Normalize by D65 white point
    X /= 0.95047;
    Y /= 1.00000;
    Z /= 1.08883;

    auto f = [](double t) {
        return (t > 0.008856) ? std::cbrt(t) : (7.787 * t + 16.0/116.0);
    };

    LAB out;
    out.L = 116.0 * f(Y) - 16.0;
    out.a = 500.0 * (f(X) - f(Y));
    out.b = 200.0 * (f(Y) - f(Z));
    return out;
}

// LAB → RGB
RGB LABtoRGB(const LAB& lab)
{
    double fy = (lab.L + 16.0) / 116.0;
    double fx = fy + lab.a / 500.0;
    double fz = fy - lab.b / 200.0;

    auto invf = [](double t) {
        double t3 = t*t*t;
        return (t3 > 0.008856) ? t3 : ((t - 16.0/116.0) / 7.787);
    };

    double X = invf(fx) * 0.95047;
    double Y = invf(fy);
    double Z = invf(fz) * 1.08883;

    // XYZ → RGB
    double r =  3.2406*X - 1.5372*Y - 0.4986*Z;
    double g = -0.9689*X + 1.8758*Y + 0.0415*Z;
    double b =  0.0557*X - 0.2040*Y + 1.0570*Z;

    // Gamma correction
    r = Gamma(r);
    g = Gamma(g);
    b = Gamma(b);

    return { std::clamp(r,0.0,1.0), std::clamp(g,0.0,1.0), std::clamp(b,0.0,1.0) };
}

////////////////////
/// WCAG contrast
////////////////////

// WCAG relative luminance
static double srgbToLinear(double c) {
    if (c <= 0.04045)
        return c / 12.92;
    return std::pow((c + 0.055) / 1.055, 2.4);
}

static double relativeLuminance(const RGB& c) {
    double R = srgbToLinear(c.r);
    double G = srgbToLinear(c.g);
    double B = srgbToLinear(c.b);

    // Standard Rec. 709 luminance
    return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}

// ------------------------------------------------------------
// WCAG contrast ratio
// ------------------------------------------------------------
double wcagComputeContrast(const RGB& fg, const RGB& bg) {
    double L1 = relativeLuminance(fg);
    double L2 = relativeLuminance(bg);
    double Lmax = std::max(L1, L2);
    double Lmin = std::min(L1, L2);
    return (Lmax + 0.05) / (Lmin + 0.05);
}

// ------------------------------------------------------------
// Method A: ΔE (simple ΔE76)
// ------------------------------------------------------------
static double deltaE76(const LAB& c1, const LAB& c2) {
    double dL = c1.L - c2.L;
    double da = c1.a - c2.a;
    double db = c1.b - c2.b;
    return std::sqrt(dL*dL + da*da + db*db);
}

// ------------------------------------------------------------
// Method A: ΔL*
// ------------------------------------------------------------
static double deltaL(const LAB& c1, const LAB& c2) {
    return std::abs(c1.L - c2.L);
}

// ------------------------------------------------------------
// CIEDE2000 ΔE implementation
// ------------------------------------------------------------
double deltaE2000(const LAB& c1, const LAB& c2)
{
    // Step 1: Calculate C1, C2
    double C1 = std::sqrt(c1.a * c1.a + c1.b * c1.b);
    double C2 = std::sqrt(c2.a * c2.a + c2.b * c2.b);

    // Step 2: Mean C
    double Cbar = (C1 + C2) * 0.5;

    // Step 3: G factor
    double Cbar7 = std::pow(Cbar, 7.0);
    double G = 0.5 * (1.0 - std::sqrt(Cbar7 / (Cbar7 + std::pow(25.0, 7.0))));

    // Step 4: a' values
    double a1p = (1.0 + G) * c1.a;
    double a2p = (1.0 + G) * c2.a;

    // Step 5: C' values
    double C1p = std::sqrt(a1p * a1p + c1.b * c1.b);
    double C2p = std::sqrt(a2p * a2p + c2.b * c2.b);

    // Step 6: h' values
    auto hp_f = [](double x, double y) {
        if (x == 0 && y == 0) return 0.0;
        double h = std::atan2(y, x);
        if (h < 0) h += 2.0 * M_PI;
        return h;
    };

    double h1p = hp_f(a1p, c1.b);
    double h2p = hp_f(a2p, c2.b);

    // Step 7: ΔL', ΔC', ΔH'
    double dLp = c2.L - c1.L;
    double dCp = C2p - C1p;

    double dhp;
    if (C1p * C2p == 0)
        dhp = 0;
    else {
        double dh = h2p - h1p;
        if (dh > M_PI) dh -= 2.0 * M_PI;
        if (dh < -M_PI) dh += 2.0 * M_PI;
        dhp = dh;
    }

    double dHp = 2.0 * std::sqrt(C1p * C2p) * std::sin(dhp * 0.5);

    // Step 8: Means
    double Lbarp = (c1.L + c2.L) * 0.5;
    double Cbarp = (C1p + C2p) * 0.5;

    double hbarp;
    if (C1p * C2p == 0)
        hbarp = h1p + h2p;
    else {
        double dh = std::abs(h1p - h2p);
        if (dh > M_PI) {
            if (h1p + h2p < 2.0 * M_PI)
                hbarp = (h1p + h2p + 2.0 * M_PI) * 0.5;
            else
                hbarp = (h1p + h2p - 2.0 * M_PI) * 0.5;
        } else {
            hbarp = (h1p + h2p) * 0.5;
        }
    }

    // Step 9: T factor
    double T =
        1.0
        - 0.17 * std::cos(hbarp - M_PI / 6.0)
        + 0.24 * std::cos(2.0 * hbarp)
        + 0.32 * std::cos(3.0 * hbarp + M_PI / 30.0)
        - 0.20 * std::cos(4.0 * hbarp - 63.0 * M_PI / 180.0);

    // Step 10: SL, SC, SH
    double Sl = 1.0 + (0.015 * std::pow(Lbarp - 50.0, 2.0)) /
                        std::sqrt(20.0 + std::pow(Lbarp - 50.0, 2.0));
    double Sc = 1.0 + 0.045 * Cbarp;
    double Sh = 1.0 + 0.015 * Cbarp * T;

    // Step 11: Δθ, RC, RT
    double deltaTheta = 30.0 * M_PI / 180.0 *
                        std::exp(-std::pow((hbarp * 180.0 / M_PI - 275.0) / 25.0, 2.0));

    double Rc = 2.0 * std::sqrt(std::pow(Cbarp, 7.0) /
                                (std::pow(Cbarp, 7.0) + std::pow(25.0, 7.0)));

    double Rt = -std::sin(2.0 * deltaTheta) * Rc;

    // Step 12: Final ΔE2000
    double dE = std::sqrt(
        std::pow(dLp / Sl, 2.0) +
        std::pow(dCp / Sc, 2.0) +
        std::pow(dHp / Sh, 2.0) +
        Rt * (dCp / Sc) * (dHp / Sh)
    );

    return dE;
}

// Convert enum to text
const char* contrastToString(ContrastLevel lvl) 
{
    switch (lvl) {
        case ContrastLevel::Good:    return "Good";
        case ContrastLevel::Warning: return "Borderline";
        case ContrastLevel::Bad:     return "Poor";
    }
    return "Unknown";
}

//////////////////////
/// RGB -> XYZ -> LAB
//////////////////////

static double clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(v, hi));
}

static double rgbToX(unsigned char rgbV) {
   double v = rgbV / 255.0;
   return (v > 0.04045) ? pow((v + 0.055) / 1.055, 2.4) : (v / 12.92);
}

// Convert RGB (0–255) → XYZ
void RGBtoXYZ(RGB& in, double& X, double& Y, double& Z)
{
    double r = rgbToX(in.r);
    double g = rgbToX(in.g);
    double b = rgbToX(in.b);

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
LAB XYZtoLAB(double X, double Y, double Z)
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
void LABtoXYZ(const LAB& in, double& X, double& Y, double& Z)
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
iRGB XYZtoRGB(double X, double Y, double Z)
{
    double r =  3.2406*X - 1.5372*Y - 0.4986*Z;
    double g = -0.9689*X + 1.8758*Y + 0.0415*Z;
    double b =  0.0557*X - 0.2040*Y + 1.0570*Z;

    double rr = clamp(xyzToR(r) * 255.0, 0, 255);
    double gg = clamp(xyzToR(g) * 255.0, 0, 255);
    double bb = clamp(xyzToR(b) * 255.0, 0, 255);

    iRGB out;
    out.r = (unsigned char)rr;
    out.g = (unsigned char)gg;
    out.b = (unsigned char)bb;

    return out;
}

///////////////////////////
/// High-level functiuons
///////////////////////////

/* HSL: fast way to compute raise color for bevels */
RGB ComputeRaiseColor_HSL(const RGB& bg, const RGB& line)
{
    double hb, sb, lb;
    double hl, sl, ll;

    RGBtoHSL(bg, hb, sb, lb);
    RGBtoHSL(line, hl, sl, ll);

    // Opposite lightness shift
    double shift = 0.25; // 20% is a good UI default

    double newL = (ll < lb) ? ll + shift : ll - shift;
    newL = std::clamp(newL, 0.0, 1.0);

    return HSLtoRGB(hl, sl, newL);
}

/* LAB: make raise color for bevels */

RGB ComputeRaiseColor_LAB(const RGB& bg, const RGB& line)
{
    LAB Lbg = RGBtoLAB(bg);
    LAB Lln = RGBtoLAB(line);

    LAB out = Lln;

    // Opposite lightness shift
    double shift = 25.0; // LAB L is 0..100, so 12 is ~12%
    out.L = (Lln.L < Lbg.L) ? (Lln.L + shift) : (Lln.L - shift);
    out.L = std::clamp(out.L, 0.0, 100.0);

    return LABtoRGB(out);
}

/* hover pair: (fg, bg) -> (hover_bg, hover_fg) */

HoverResult ComputeHoverColors(const RGB& bg, const RGB& fg)
{
    HoverResult out;

    // Convert background to HSL
    HSL hsl;
    RGBtoHSL(bg, hsl.h, hsl.s, hsl.l);

    // --- Background hover modification ---
    const double lighten = 0.12;   // +12% lightness
    const double desat   = 0.08;   // -8% saturation

    hsl.l = std::min(1.0, hsl.l + lighten);
    hsl.s = std::max(0.0, hsl.s - desat);

    out.bg_hover = HSLtoRGB(hsl.h, hsl.s, hsl.l);

    // --- Foreground adjustment for readability ---
    RGB fg_new = fg;

    double contrast = wcagComputeContrast(fg_new, out.bg_hover);

    const double minContrast = 4.0; // WCAG-ish

    if (contrast < minContrast)
    {
        // Convert fg to HSL
        HSL fgh;
        RGBtoHSL(fg_new, fgh.h, fgh.s, fgh.l);

        // Move foreground lightness opposite to background
        if (hsl.l > 0.5)
            fgh.l = std::max(0.0, fgh.l - 0.20); // darken text
        else
            fgh.l = std::min(1.0, fgh.l + 0.20); // lighten text

        fg_new = HSLtoRGB(fgh.h, fgh.s, fgh.l);
    }

    out.fg_hover = fg_new;
    return out;
}

// ------------------------------------------------------------
// Final evaluation function
// ------------------------------------------------------------
ContrastLevel ComputeContrast(const RGB& fg, const RGB& bg, RGB& newFg) {
    // --- Method A: Lab-based perceptual contrast ---
    LAB labFg = RGBtoLAB(fg);   // you already have this
    LAB labBg = RGBtoLAB(bg);

    newFg = fg;

    double dL  = deltaL(labFg, labBg);
    double dE  = deltaE2000(labFg, labBg); // or deltaE76(labFg, labBg);

    // --- Method B: WCAG contrast ratio ---
    double ratio = wcagComputeContrast(fg, bg);

    // --------------------------------------------------------
    // Thresholds (tweak to taste)
    // --------------------------------------------------------
    bool goodLab   = (dL >= 40.0) || (dE >= 50.0);
    bool warnLab   = (dL >= 25.0) || (dE >= 30.0);

    // for E2000 it will be
	//   ΔE2000 ≥ 30 → good
	//   ΔE2000 ≥ 20 → warning
	//   ΔE2000 < 20 → poor

    bool goodWcag  = ratio >= 7.0;
    bool warnWcag  = ratio >= 4.5;

    // --------------------------------------------------------
    // Combine both methods
    // --------------------------------------------------------
    if (goodLab && goodWcag)
        return ContrastLevel::Good;

    //if (warnLab || warnWcag)
    //    return ContrastLevel::Warning;

    double targetDeltaE = 30;
    for (int i = 0; i < 50; ++i) {
        double dE = deltaE2000(labFg, labBg);
        if (dE >= targetDeltaE) {
            newFg = LABtoRGB(labFg);
            return ContrastLevel::Good;
        }

        double diff = targetDeltaE - dE;
        double step = std::clamp(diff * 0.25, 0.5, 3.0);

        if (labBg.L > labFg.L)
            labFg.L -= step;   // darken foreground
        else
            labFg.L += step;   // lighten foreground

        labFg.L = std::clamp(labFg.L, 0.0, 100.0);
    }

    // no way to lighten -> try to make darken
    targetDeltaE = 20;
    labFg = RGBtoLAB(fg);
    for (int i = 0; i < 50; ++i) {
        double dE = deltaE2000(labFg, labBg);
        if (dE >= targetDeltaE) {
            newFg = LABtoRGB(labFg);
            return ContrastLevel::Good;
        }

        double diff = targetDeltaE - dE;
        double step = std::clamp(diff * 0.25, 0.5, 3.0);

        if (labBg.L > 0.5)
            labFg.L -= step;   // darken foreground
        else
            labFg.L += step;   // lighten foreground

        labFg.L = std::clamp(labFg.L, 0.0, 100.0);
    }

    // keep unchanged
    // newFg = LABtoRGB(labFg);
    return ContrastLevel::Bad;
}

/* print lab: (fg, bg) -> (new_fg, white) */

iRGB ConvertForPrintLAB(const iRGB& afg, const iRGB& abg) {
    // Step 1: RGB → LAB
    RGB fg = toRGB(afg);
    RGB bg = toRGB(abg);

    LAB lab = RGBtoLAB(fg);
    LAB labBg = RGBtoLAB(bg);

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
    RGB cc = LABtoRGB(lab);
    return toIRGB(cc);
}

RGB toRGB(const iRGB& c) {
	return toRGB(c.r, c.g, c.b);
}

iRGB toIRGB(const RGB& c) {
	iRGB a;
	a.r = (unsigned char)(c.r * 255.0);
	a.g = (unsigned char)(c.g * 255.0);
	a.b = (unsigned char)(c.b * 255.0);
	return a;
}

iRGB toIRGB(int r, int g, int b) {
	iRGB a;
	a.r = (unsigned char)(r & 0xFF);
	a.g = (unsigned char)(g & 0xFF);
	a.b = (unsigned char)(b & 0xFF);
	return a;
}


RGB toRGB(int r, int g, int b) {
	RGB a;

    a.r = (r & 0x00FF) / 255.0;
    a.g = (g & 0x00FF) / 255.0;;
    a.b = (b & 0x00FF) / 255.0;;

    return a;
}

// Compute luminance (perceptual)
double Luminance(const RGB& c)
{
    return c.r * 0.299 + c.g * 0.587 + c.b * 0.114;
}

// Main function: compute accent background + readable foreground
HoverResult ComputeControlAccent(const RGB& fg, const RGB& bg)
{
    double lum_bg = Luminance(bg);
    bool dark_theme = (lum_bg < 0.3) || IsNearBlack(bg);

    RGB accent = dark_theme
        ? toRGB( 215, 140, 0 )    // toRGB(80, 140, 255)   // bright blue for dark theme / toRGB( 215, 140, 0 ) orange toRGB(0x7F, 0x4F, 0x8C) violet
        : toRGB( 0, 120, 255);    // Windows-like blue for light theme

    HSL h;
    RGBtoHSL(accent, h.h, h.s, h.l);

    // Boost saturation
    h.s = std::clamp(h.s * 1.40, 0.0, 1.0);

    // Adjust lightness depending on theme
    if (dark_theme)
        h.l = std::clamp(h.l * 1.40, 0.0, 1.0);  // brighten
    else
        h.l = std::clamp(h.l * 0.70, 0.0, 1.0);  // darken

    // now we need a correction for corner cases

    // 1. Ensure minimum saturation (avoid grayish accents)
    if (h.s < 0.25)
        h.s = 0.25;

    // 2. Avoid near-white and near-black
    if (h.l > 0.92) h.l = 0.92;
    if (h.l < 0.08) h.l = 0.08;

    RGB glyph_bg = HSLtoRGB(h.h, h.s, h.l);

    // 3. Ensure contrast with background
    lum_bg = Luminance(bg);
    double lum_ac = Luminance(glyph_bg);

    if (std::abs(lum_bg - lum_ac) < 0.235)
    {
        // If too close, push lightness away from background
        if (lum_bg > 0.5)
            h.l = std::max(0.0, h.l - 0.25);
        else
            h.l = std::min(1.0, h.l + 0.25);

        glyph_bg = HSLtoRGB(h.h, h.s, h.l);
    }

    // 4. Ensure contrast with foreground text
    double lum_fg = Luminance(fg);
    lum_ac = Luminance(glyph_bg);

    if (std::abs(lum_fg - lum_ac) < 0.235)
    {
        // Push away from text color
        if (lum_fg > 0.5)
            h.l = std::max(0.0, h.l - 0.20);
        else
            h.l = std::min(1.0, h.l + 0.20);

        glyph_bg = HSLtoRGB(h.h, h.s, h.l);
    }

    double lum_gb = Luminance(glyph_bg);
    RGB glyph_fg = (lum_gb > 0.6)
        ? toRGB( 0, 0, 0 )
        : toRGB( 255, 255, 255 );

    return HoverResult { glyph_bg, glyph_fg };
}

bool IsNearBlack(int r, int g, int b, double threshold)
{
    double L = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    return L < 32.0;
}

bool IsNearWhite(int r, int g, int b, double threshold)
{
    double L = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    return L > (255.0 - threshold);
}

bool IsNearBlack(RGB c, double threshold) {
	iRGB i = toIRGB(c);
	return IsNearBlack(i.r, i.g, i.b, threshold);
}

bool IsNearWhite(RGB c, double threshold) {
	iRGB i = toIRGB(c);
	return IsNearWhite(i.r, i.g, i.b, threshold);
}

double Chroma(const LAB& lab)
{
    return std::sqrt(lab.a*lab.a + lab.b*lab.b);
}


iRGB SoftenBlackish_LAB(const RGB& cc) {
    LAB lab = RGBtoLAB(cc);

    double L_min = 10.0;
    double L_max = 90.0;

    if (lab.L <= L_max && lab.L >= L_min)
        return toIRGB(cc);

    double C_neutral = 10.0;
    double L_target = lab.L < L_min ? 60.0 : 40.0;

    double C = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    double k = 1.0 - std::min(C / C_neutral, 1.0);
    if (k <= 0.0)
        return toIRGB(cc); // colorful bright color: keep it

    // You can use linear or quadratic; here: quadratic for smoother feel
    double t = lab.L < L_min ? (L_min - lab.L) / L_min : (lab.L - L_max) / (100.0 - L_max); // how far into "white"/"black" we are
    double strength = k * t * t; // stronger for true white, weaker near threshold

    lab.L = lab.L + (L_target - lab.L) * strength;

    RGB c2 = LABtoRGB(lab);
    return toIRGB(c2);
}

RGB SoftenToDisabledState_LAB(const RGB& cc,
                     double L_center,   // target mid-gray center
                     double L_strength,  // how strongly to pull L toward center
                     double C_strength)  // how strongly to desaturate
{
    LAB lab = RGBtoLAB(cc);

    // 1. Reduce chroma (desaturate)
    double C = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    if (C > 0.0)
    {
        double scale = 1.0 - C_strength; // e.g. 0.3 keeps 30% of chroma
        lab.a *= scale;
        lab.b *= scale;
    }

    // 2. Pull L toward a neutral mid-gray
    //    L_center = 60 is a good universal disabled tone
    lab.L = lab.L + (L_center - lab.L) * L_strength;

    return LABtoRGB(lab);
}

RGB SoftenToHoverState_LAB(const RGB& cc,
	const RGB& tintR, // default light blue
	double L_boost,   // +10% brightness
	double C_boost,   // +20% chroma
	double tint_max,  // max tint for pure black/white
	double tint_min,  // min tint for slightly neutral colors
	double C_neutral) // chroma threshold for "neutral"
{
    LAB lab = RGBtoLAB(cc);
    LAB tint = RGBtoLAB(tintR);

    // 1. Brightness boost
    lab.L = lab.L + (100.0 - lab.L) * L_boost;

    // 2. Chroma boost
    double C = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    if (C > 0.0)
    {
        double scale = 1.0 + C_boost;
        lab.a *= scale;
        lab.b *= scale;
    }

    // 3. Tint injection for neutral colors
    //    Stronger tint for lower chroma
    double chromaFactor = std::min(C / C_neutral, 1.0); // 0 = neutral, 1 = colorful
    double tint_strength =
        tint_max - (tint_max - tint_min) * chromaFactor;

    if (tint_strength > 0.0)
    {
        lab.L = lab.L + (tint.L - lab.L) * tint_strength;
        lab.a = lab.a + (tint.a - lab.a) * tint_strength;
        lab.b = lab.b + (tint.b - lab.b) * tint_strength;
    }
    return LABtoRGB(lab);
}

RGB SoftenToFocusedState_LAB(const RGB& cc,
	const RGB& focusTintR, // subtle blue-violet
	double L_boost,   // +5% brightness
	double C_boost,   // +30% chroma
	double tint_max,  // max tint for pure neutrals
	double tint_min,  // min tint for slightly neutral colors
	double C_neutral) // chroma threshold for "neutral"
{
    LAB lab = RGBtoLAB(cc);
    LAB focusTint = RGBtoLAB(focusTintR);

    // 1. Slight brightness boost (less than hover)
    lab.L = lab.L + (100.0 - lab.L) * L_boost;

    // 2. Stronger chroma boost (focus should feel crisp)
    double C = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    if (C > 0.0)
    {
        double scale = 1.0 + C_boost;
        lab.a *= scale;
        lab.b *= scale;
    }

    // 3. Tint injection for neutral colors (but less playful than hover)
    double chromaFactor = std::min(C / C_neutral, 1.0);
    double tint_strength =
        tint_max - (tint_max - tint_min) * chromaFactor;

    if (tint_strength > 0.0)
    {
        lab.L = lab.L + (focusTint.L - lab.L) * tint_strength;
        lab.a = lab.a + (focusTint.a - lab.a) * tint_strength;
        lab.b = lab.b + (focusTint.b - lab.b) * tint_strength;
    }
    return LABtoRGB(lab);
}

RGB SoftenToPressedState_LAB(const RGB& fg,
	const RGB& bgC,          // background LAB
	double L_push,   // how strongly to push toward background L*
	double C_reduce, // reduce chroma by 40%
	double neutral_tint, // tint neutrals toward bg
	double C_neutral)    // threshold for “neutral”
{
    LAB lab = RGBtoLAB(fg);
    LAB bg = RGBtoLAB(bgC);

    // 1. Move L* toward background (darken on light bg, lighten on dark bg)
    lab.L = lab.L + (bg.L - lab.L) * L_push;

    // 2. Reduce chroma (pressed = less colorful)
    double C = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    if (C > 0.0)
    {
        double scale = 1.0 - C_reduce; // e.g. 0.6 keeps 60% of chroma
        lab.a *= scale;
        lab.b *= scale;
    }

    // 3. If color is neutral-ish, tint toward background hue
    double chromaFactor = std::min(C / C_neutral, 1.0);
    double tint_strength = neutral_tint * (1.0 - chromaFactor);

    if (tint_strength > 0.0)
    {
        lab.L = lab.L + (bg.L - lab.L) * tint_strength;
        lab.a = lab.a + (bg.a - lab.a) * tint_strength;
        lab.b = lab.b + (bg.b - lab.b) * tint_strength;
    }
    return LABtoRGB(lab);
}
