#ifndef COLOR_HLSL
#define COLOR_HLSL

#define PI 3.14159265358979323846
#define TAU (2.0 * PI)

#define SATURATION_THRESHOLD 0
#define CHROMA_THRESHOLD 0.02  // 無彩色か判定するのに使う誤差を考慮した閾値

static const float3 D65_WHITE = float3(0.95047, 1.0, 1.08883);
static const float3 D50_WHITE = float3(0.96422, 1.0, 0.82521);

float mod(float x, float y) {
    return x - y * floor(x / y);
}

float alpha_mix(float alpha1, float alpha2, float t) {
    return clamp(lerp(alpha1, alpha2, t), 0.0, 1.0);
}

// ----------------------------------------------------------------------------
// linear sRGB
// ----------------------------------------------------------------------------
// 参考: https://w.wiki/DBwx
float gamma_decode(float x) {
    return x <= 0.04045 ? x / 12.92 : pow(abs((x + 0.055) / 1.055), 2.4);
}

float3 srgb2linear(float3 x) {
    return float3(gamma_decode(x.r), gamma_decode(x.g), gamma_decode(x.b));
    //return pow(abs(x), float3(2.2));  // 近似
}

// 参考: https://w.wiki/DBx3
float gamma_encode(float x) {
    return x <= 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}

float3 linear2srgb(float3 x) {
    return float3(gamma_encode(x.r), gamma_encode(x.g), gamma_encode(x.b));
    //return pow(abs(x), float3(1.0 / 2.2));  // 近似
}

// ----------------------------------------------------------------------------
// HSV
// ----------------------------------------------------------------------------
// 参考: https://w.wiki/DD6A
float3 srgb2hsv(float3 rgb) {
    float max_val = max(rgb.r, max(rgb.g, rgb.b));
    float min_val = min(rgb.r, min(rgb.g, rgb.b));
    float chroma = max_val - min_val;

    float hue = 0;
    float saturation = 0;

    if (chroma == 0) hue = 0;
    else if (max_val == rgb.r) hue = mod((rgb.g - rgb.b) / chroma, 6.0);
    else if (max_val == rgb.g) hue = (rgb.b - rgb.r) / chroma + 2.0;
    else hue = (rgb.r - rgb.g) / chroma + 4.0;
    hue *= 60.0;

    if (max_val == 0) saturation = 0;
    else saturation = chroma / max_val;

    return float3(radians(hue), saturation, max_val);
}

// 参考: https://w.wiki/DD66
float hsv2srgb_f(float3 hsv, float n) {
    float k = mod((n + degrees(hsv.x) / 60.0), 6.0);
    return hsv.z - hsv.z * hsv.y * max(0.0, min(k, min(4.0 - k, 1.0)));
}
float3 hsv2srgb(float3 hsv) {
    return float3(
        hsv2srgb_f(hsv, 5.0),
        hsv2srgb_f(hsv, 3.0),
        hsv2srgb_f(hsv, 1.0)
    );
}

// ----------------------------------------------------------------------------
// HSL
// ----------------------------------------------------------------------------
// 参考: https://w.wiki/DD6A
float3 srgb2hsl(float3 rgb) {
    float max_val = max(rgb.r, max(rgb.g, rgb.b));
    float min_val = min(rgb.r, min(rgb.g, rgb.b));
    float chroma = max_val - min_val;

    float hue = 0;
    if (chroma == 0) hue = 0;
    else if (max_val == rgb.r) hue = mod((rgb.g - rgb.b) / chroma, 6.0);
    else if (max_val == rgb.g) hue = (rgb.b - rgb.r) / chroma + 2.0;
    else hue = (rgb.r - rgb.g) / chroma + 4.0;
    hue *= 60.0;

    float lightness = (max_val + min_val) / 2.0;

    float saturation = 0.0;
    if (lightness != 1.0 && lightness != 0.0) {
        saturation = chroma / (1.0 - abs(2.0 * lightness - 1.0));
    }
    return float3 (radians(hue), saturation, lightness);
}

// 参考: https://w.wiki/DD6D
float hsl2rgb_f(float3 hsl, float n) {
    float k = mod((n + degrees(hsl.x) / 30.0), 12.0);
    float a = hsl.y * min(hsl.z, 1.0 - hsl.z);
    return hsl.z - a * max(-1.0, min(k - 3.0, min(9.0 - k, 1.0)));
}

float3 hsl2srgb(float3 hsl) {
    return float3(
        hsl2rgb_f(hsl, 0.0),
        hsl2rgb_f(hsl, 8.0),
        hsl2rgb_f(hsl, 4.0)
    );
}

float hue_mix(float hue1, float hue2, float t, int interp_dir) {
    float diff = hue2 - hue1;
    switch (interp_dir) {
    case 0:
        if (diff > PI) hue1 += TAU;
        else if (diff < -PI) hue2 += TAU;
        break;
    case 1:
        if (0.0 < diff && diff < PI) hue1 += TAU;
        else if (-PI < diff && diff <= 0.0) hue2 += TAU;
        break;
    }
    float angle = lerp(hue1, hue2, t);
    return mod(mod(angle, TAU) + TAU, TAU);
}

// 各色が無彩色かどうかに基づいてHueを調整し混合する
float adjust_and_mix_hue(float h1, float h2, bool has_valid_hue1, bool has_valid_hue2, float t, int interp_dir) {
    if (has_valid_hue1 && !has_valid_hue2) {
        h2 = h1;
    } else if (!has_valid_hue1 && has_valid_hue2) {
        h1 = h2;
    } else if (!has_valid_hue1 && !has_valid_hue2) {
        h1 = 0.0;
        h2 = 0.0;
    }
    return hue_mix(h1, h2, t, interp_dir);
}

// HSLも同じ補間を行う
float3 hsv_mix(float3 a, float3 b, float x, int interp_dir) {
    bool has_valid_hue1 = a.y > SATURATION_THRESHOLD;
    bool has_valid_hue2 = b.y > SATURATION_THRESHOLD;
    float hue = adjust_and_mix_hue(a.x, b.x, has_valid_hue1, has_valid_hue2, x, interp_dir);
    float3 hsv = float3(hue, lerp(a.yz, b.yz, x));
    return hsv;
}

// ----------------------------------------------------------------------------
// XYZ
// ----------------------------------------------------------------------------
// 参考: http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
float3 linear2d50xyz(float3 linear_rgb) {
    float x = (0.4360747 * linear_rgb.r + 0.3850649 * linear_rgb.g + 0.1430804 * linear_rgb.b);
    float y = (0.2225045 * linear_rgb.r + 0.7168786 * linear_rgb.g + 0.0606169 * linear_rgb.b);
    float z = (0.0139322 * linear_rgb.r + 0.0971045 * linear_rgb.g + 0.7141733 * linear_rgb.b);
    return float3(x, y, z);
}

float3 d50xyz2linear(float3 xyz) {
    float r = (xyz.x *  3.1338561 + xyz.y * -1.6168667 + xyz.z * -0.4906146);
    float g = (xyz.x * -0.9787684 + xyz.y *  1.9161415 + xyz.z *  0.0334540);
    float b = (xyz.x *  0.0719453 + xyz.y * -0.2289914 + xyz.z *  1.4052427);
    return float3(r, g, b);
}

float3 linear2d65xyz(float3 linear_rgb) {
    float x = (0.4124564 * linear_rgb.r + 0.3575761 * linear_rgb.g + 0.1804375 * linear_rgb.b);
    float y = (0.2126729 * linear_rgb.r + 0.7151522 * linear_rgb.g + 0.0721750 * linear_rgb.b);
    float z = (0.0193339 * linear_rgb.r + 0.1191920 * linear_rgb.g + 0.9503041 * linear_rgb.b);
    return float3(x, y, z);
}

float3 d65xyz2linear(float3 xyz) {
    float r = (xyz.x *  3.2404542 + xyz.y * -1.5371385 + xyz.z * -0.4985314);
    float g = (xyz.x * -0.9692660 + xyz.y *  1.8760108 + xyz.z *  0.0415560);
    float b = (xyz.x *  0.0556434 + xyz.y * -0.2040259 + xyz.z *  1.0572252);
    return float3(r, g, b);
}

// ----------------------------------------------------------------------------
// CIELAB
// ----------------------------------------------------------------------------
// 参考: http://www.brucelindbloom.com/Eqn_XYZ_to_Lab.html
float xyz2lab_f(float x) {
    return x > 0.008856 ? pow(abs(x), 0.333333333) : (903.3 * x + 16.0) / 116.0;
}

float3 d50xyz2lab(float3 xyz) {
    float3 xyz_scaled = xyz / D50_WHITE;
    xyz_scaled = float3(
        xyz2lab_f(xyz_scaled.x),
        xyz2lab_f(xyz_scaled.y),
        xyz2lab_f(xyz_scaled.z)
    );
    return float3(
        (116.0 * xyz_scaled.y) - 16.0,
         500.0 * (xyz_scaled.x - xyz_scaled.y),
         200.0 * (xyz_scaled.y - xyz_scaled.z)
    );
}

float3 d65xyz2lab(float3 xyz) {
    float3 xyz_scaled = xyz / D65_WHITE;
    xyz_scaled = float3(
        xyz2lab_f(xyz_scaled.x),
        xyz2lab_f(xyz_scaled.y),
        xyz2lab_f(xyz_scaled.z)
    );
    return float3(
        (116.0 * xyz_scaled.y) - 16.0,
         500.0 * (xyz_scaled.x - xyz_scaled.y),
         200.0 * (xyz_scaled.y - xyz_scaled.z)
    );
}

// 参考: http://www.brucelindbloom.com/Eqn_Lab_to_XYZ.html
float lab2xyz_f(float x) {
    return pow(abs(x), 3.0) > 0.008856 ? pow(abs(x), 3.0) : (116.0 * x - 16.0) / 903.3;
}

float3 lab2d50xyz(float3 lab) {
    float f = (lab.x + 16.0) / 116.0;
    float y = lab.x > 0.008856 * 903.3 ? pow(abs((lab.x + 16.0) / 116.0), 3.0) : lab.x / 903.3;
    return D50_WHITE * float3(
        lab2xyz_f(f + lab.y / 500.0),
        y,
        lab2xyz_f(f - lab.z / 200.0)
    );
}

float3 lab2d65xyz(float3 lab) {
    float f = (lab.x + 16.0) / 116.0;
    float y = lab.x > 0.008856 * 903.3 ? pow(abs((lab.x + 16.0) / 116.0), 3.0) : lab.x / 903.3;
    return D65_WHITE * float3(
        lab2xyz_f(f + lab.y / 500.0),
        y,
        lab2xyz_f(f - lab.z / 200.0)
    );
}

float3 linear2d50lab(float3 linear_rgb) {
    float3 xyz = linear2d50xyz(linear_rgb);
    return d50xyz2lab(xyz);
}

float3 d50lab2linear(float3 lab) {
    float3 xyz = lab2d50xyz(lab);
    return d50xyz2linear(xyz);
}

float3 linear2d65lab(float3 linear_rgb) {
    float3 xyz = linear2d65xyz(linear_rgb);
    return d65xyz2lab(xyz);
}

float3 d65lab2linear(float3 lab) {
    float3 xyz = lab2d65xyz(lab);
    return d65xyz2linear(xyz);
}

// ----------------------------------------------------------------------------
// CIELCH
// ----------------------------------------------------------------------------
// 参考: http://www.brucelindbloom.com/Eqn_Lab_to_LCH.html
float3 lab2lch(float3 lab) {
    float chroma = length(float2(lab.y, lab.z));
    float hue = 0.0;
    // 無彩色でない場合のみHueを計算
    if (chroma > CHROMA_THRESHOLD) {
        hue = atan2(lab.z, lab.y);
        hue = hue < 0.0 ? hue + TAU : hue;
    }
    return float3(
        lab.x,
        chroma,
        hue
    );
}

// 参考: http://www.brucelindbloom.com/Eqn_LCH_to_Lab.html
float3 lch2lab(float3 lch) {
    return float3(
        lch.x,
        lch.y * cos(lch.z),
        lch.y * sin(lch.z)
    );
}

float3 linear2d50lch(in float3 linear_rgb) {
    float3 xyz = linear2d50xyz(linear_rgb);
    float3 lab = d50xyz2lab(xyz);
    return lab2lch(lab);
}

float3 d50lch2linear(in float3 lch) {
    float3 lab = lch2lab(lch);
    float3 xyz = lab2d50xyz(lab);
    return d50xyz2linear(xyz);
}

float3 linear2d65lch(in float3 linear_rgb) {
    float3 xyz = linear2d65xyz(linear_rgb);
    float3 lab = d65xyz2lab(xyz);
    return lab2lch(lab);
}

float3 d65lch2linear(in float3 lch) {
    float3 lab = lch2lab(lch);
    float3 xyz = lab2d65xyz(lab);
    return d65xyz2linear(xyz);
}

float3 lch_mix(float3 a, float3 b, float x, int interp_dir) {
    float lightness = lerp(a.x, b.x, x);
    float chroma = lerp(a.y, b.y, x);
    bool has_valid_hue1 = a.y > CHROMA_THRESHOLD;
    bool has_valid_hue2 = b.y > CHROMA_THRESHOLD;
    float hue = adjust_and_mix_hue(a.z, b.z, has_valid_hue1, has_valid_hue2, x, interp_dir);
    lightness = clamp(lightness, 0.0, 100.0);
    chroma = max(chroma, 0.0);
    return float3(lightness, chroma, hue);
}

// ----------------------------------------------------------------------------
// Oklab
// ----------------------------------------------------------------------------
float cbrt(float x) {
    return sign(x) * pow(abs(x), 1.0 / 3.0);
}

// 参考: https://bottosson.github.io/posts/oklab/#converting-from-linear-srgb-to-oklab
float3 linear2oklab(float3 linear_rgb) {
    float l = 0.4122214708 * linear_rgb.r + 0.5363325363 * linear_rgb.g + 0.0514459929 * linear_rgb.b;
	float m = 0.2119034982 * linear_rgb.r + 0.6806995451 * linear_rgb.g + 0.1073969566 * linear_rgb.b;
	float s = 0.0883024619 * linear_rgb.r + 0.2817188376 * linear_rgb.g + 0.6299787005 * linear_rgb.b;

    float l_ = cbrt(l);
    float m_ = cbrt(m);
    float s_ = cbrt(s);

    return float3(
        0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
        1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
        0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    );
}

// 参考: https://bottosson.github.io/posts/oklab/#converting-from-linear-srgb-to-oklab
float3 oklab2linear(float3 oklab) {
    float l_ = oklab.x + 0.3963377774 * oklab.y + 0.2158037573 * oklab.z;
    float m_ = oklab.x - 0.1055613458 * oklab.y - 0.0638541728 * oklab.z;
    float s_ = oklab.x - 0.0894841775 * oklab.y - 1.2914855480 * oklab.z;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return float3(
		 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
		-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
		-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    );
}

// ----------------------------------------------------------------------------
// Oklch
// ----------------------------------------------------------------------------
float3 linear2oklch(float3 linear_rgb) {
    float3 oklab = linear2oklab(linear_rgb);
    return lab2lch(oklab);
}

float3 oklch2linear(float3 oklch) {
    float3 oklab = lch2lab(oklch);
    return oklab2linear(oklab);
}

#endif  // COLOR_HLSL
