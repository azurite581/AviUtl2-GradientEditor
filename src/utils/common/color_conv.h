#ifndef COLOR_CONV_H
#define COLOR_CONV_H

#include <cmath>
#include <cstdint>
#include <numbers>
#include <algorithm>

namespace color_conv {
template <typename Vec4>
constexpr Vec4 u32Rgba2Vec4Rgba(const uint32_t rgba)
{
    float inv_255 = 1.0f / 255.0f;
    float r       = static_cast<float>((rgba >> 24) & 0xFF) * inv_255;
    float g       = static_cast<float>((rgba >> 16) & 0xFF) * inv_255;
    float b       = static_cast<float>((rgba >> 8) & 0xFF) * inv_255;
    float a       = static_cast<float>(rgba & 0xFF) * inv_255;
    return Vec4{r, g, b, a};
}

template <typename Vec4>
constexpr Vec4 u32Rgb2Vec4Rgba(const uint32_t rgb, const uint32_t a = 0xFF)
{
    float inv_255 = 1.0f / 255.0f;
    float r       = static_cast<float>((rgb >> 16) & 0xFF) * inv_255;
    float g       = static_cast<float>((rgb >> 8) & 0xFF) * inv_255;
    float b       = static_cast<float>(rgb & 0xFF) * inv_255;
    float a_f     = static_cast<float>(a & 0xFF) * inv_255;
    return Vec4{r, g, b, a_f};
}

template <typename Vec4>
constexpr uint32_t vec4Rgba2u32Rgba(const Vec4& rgba)
{
    uint32_t r = static_cast<uint32_t>(rgba.x) & 0xFF;
    uint32_t g = static_cast<uint32_t>(rgba.y) & 0xFF;
    uint32_t b = static_cast<uint32_t>(rgba.z) & 0xFF;
    uint32_t a = static_cast<uint32_t>(rgba.w) & 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

template <typename Vec4>
constexpr uint32_t vec4normRgba2u32Rgba(const Vec4& rgba)
{
    uint32_t r = static_cast<uint32_t>(rgba.x * 255.0f + 0.5f) & 0xFF;
    uint32_t g = static_cast<uint32_t>(rgba.y * 255.0f + 0.5f) & 0xFF;
    uint32_t b = static_cast<uint32_t>(rgba.z * 255.0f + 0.5f) & 0xFF;
    uint32_t a = static_cast<uint32_t>(rgba.w * 255.0f + 0.5f) & 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

constexpr uint32_t u32Rgb2u32Rgba(const uint32_t rgb, const uint32_t a = 0xFF)
{
    return (rgb << 8) | (a & 0xFF);
}

constexpr uint32_t u32Bgr2u32Abgr(const uint32_t bgr, const uint32_t a = 0xFF)
{
    uint8_t b = (bgr >> 16) & 0xFF;
    uint8_t g = (bgr >> 8) & 0xFF;
    uint8_t r = bgr & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

constexpr uint32_t u32Rgb2u32Bgr(const uint32_t rgb)
{
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;
    return (b << 16) | (g << 8) | r;
}

constexpr uint32_t u32Rgba2u32Abgr(const uint32_t rgba)
{
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    uint8_t a = rgba & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

struct RGBA {
    uint8_t r, g, b, a;
};

inline RGBA hexRgba2rgba(const uint32_t rgba)
{
    return {
        static_cast<uint8_t>((rgba >> 24) & 0xFF),
        static_cast<uint8_t>((rgba >> 16) & 0xFF),
        static_cast<uint8_t>((rgba >> 8)  & 0xFF),
        static_cast<uint8_t>(rgba & 0xFF),
    };
}

inline double gammaDecord(const double x)
{
    return x <= 0.04045 ? x / 12.92 : std::pow(std::abs((x + 0.055) / 1.055), 2.4);
}

inline double gammaEncode(const double x) {
    return x <= 0.0031308 ? 12.92 * x : 1.055 * std::pow(std::abs(x), 1.0 / 2.4) - 0.055;
}

struct sRGB {
    double r, g, b;
};

struct Linear {
    double r, g, b;
};

inline Linear srgb2linear(const sRGB& srgb) {
    return Linear{gammaDecord(srgb.r), gammaDecord(srgb.g), gammaDecord(srgb.b)};
}

inline sRGB linear2srgb(const Linear& linear) {
    return sRGB{gammaEncode(linear.r), gammaEncode(linear.g), gammaEncode(linear.b)};
}

struct HSV {
    double h, s, v;  // h == ラジアン
};

// 参考: https://w.wiki/DD66
inline sRGB hsv2srgb(const HSV& hsv) {
    auto hsv2srgb_f = [](const HSV& hsv, const double n) {
        double k = std::fmod((n + (hsv.h  * 180.0 / std::numbers::pi) / 60.0), 6.0);
        return hsv.v - hsv.v * hsv.s * (std::max)(0.0, (std::min)(k, (std::min)(4.0 - k, 1.0)));
    };

    return sRGB{
        hsv2srgb_f(hsv, 5.0),
        hsv2srgb_f(hsv, 3.0),
        hsv2srgb_f(hsv, 1.0)
    };
}

struct XYZ {
    double x, y, z;
};

inline XYZ linear2d50xyz(const Linear& linear) {
    double x = (0.4360747 * linear.r + 0.3850649 * linear.g + 0.1430804 * linear.b);
    double y = (0.2225045 * linear.r + 0.7168786 * linear.g + 0.0606169 * linear.b);
    double z = (0.0139322 * linear.r + 0.0971045 * linear.g + 0.7141733 * linear.b);
    return XYZ{x, y, z};
}

inline Linear d50xyz2linear(const XYZ& xyz) {
    double r = (xyz.x *  3.1338561 + xyz.y * -1.6168667 + xyz.z * -0.4906146);
    double g = (xyz.x * -0.9787684 + xyz.y *  1.9161415 + xyz.z *  0.0334540);
    double b = (xyz.x *  0.0719453 + xyz.y * -0.2289914 + xyz.z *  1.4052427);
    return Linear{r, g, b};
}

struct Lab {
    double l, a, b;
};

inline static constexpr double D50_WHITE[3] = {0.96422, 1.0, 0.82521};

// 参考: http://www.brucelindbloom.com/Eqn_XYZ_to_Lab.html
inline Lab d50xyz2lab(const XYZ& xyz) {
    auto xyz2lab_f = [](const double x) {
        return x > 0.008856 ? std::pow(x, 0.333333333) : (903.3 * x + 16.0) / 116.0;
    };

    XYZ xyz_scaled = { xyz.x / D50_WHITE[0], xyz.y / D50_WHITE[1], xyz.z / D50_WHITE[2] };
    xyz_scaled = {
        xyz2lab_f(xyz_scaled.x),
        xyz2lab_f(xyz_scaled.y),
        xyz2lab_f(xyz_scaled.z)
    };
    return Lab(
        (116.0 * xyz_scaled.y) - 16.0,
         500.0 * (xyz_scaled.x - xyz_scaled.y),
         200.0 * (xyz_scaled.y - xyz_scaled.z)
    );
}

// 参考: http://www.brucelindbloom.com/Eqn_Lab_to_XYZ.html
inline XYZ lab2d50xyz(const Lab& lab) {
    double f = (lab.l + 16.0) / 116.0;
    double y = lab.l > 0.008856 * 903.3 ? std::pow(std::abs((lab.l + 16.0) / 116.0), 3.0) : lab.l / 903.3;
    auto lab2xyz_f = [](const double x) {
        return std::pow(x, 3.0) > 0.008856 ? std::pow(x, 3.0) : (116.0 * x - 16.0) / 903.3;
    };

    return {
        D50_WHITE[0] * lab2xyz_f(f + lab.a / 500.0),
        D50_WHITE[1] * y,
        D50_WHITE[2] * lab2xyz_f(f - lab.b / 200.0)
    };
}

inline Lab srgb2d50lab(const sRGB& srgb) {
    const auto linear = srgb2linear(srgb);
    const auto xyz = linear2d50xyz(linear);
    return d50xyz2lab(xyz);
}

inline sRGB d50lab2srgb(const Lab& lab) {
    const auto xyz = lab2d50xyz(lab);
    const auto linear= d50xyz2linear(xyz);
    return linear2srgb(linear);
}

}  // namespace color_conv

#endif  // !COLOR_CONV_H
