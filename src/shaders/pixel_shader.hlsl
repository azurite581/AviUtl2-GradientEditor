#include "color.hlsli"
#include "spectral.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct Gradient
{
    float4 start_color;
    float4 stop_color;
    float start_x;
    float stop_x;
    float ratio;
    float pad;
};

struct AlphaMarker
{
    float start_pos;
    float stop_pos;
    float start_value;
    float stop_value;
    float mid_ratio;
    float3 pad3;
};

#ifdef MARKER_COUNT
const static int MAX_MARKER_COUNT = MARKER_COUNT;
#else
const static int MAX_MARKER_COUNT = 30;
#endif

cbuffer pixelBuffer : register(b0)
{
    Gradient gradient[MAX_MARKER_COUNT];
    int gradient_count; // 実際のグラデーションの数
    int gradient_type;
    int interp_dir;
    float gradient_w;
    float2 texture_resolution;
    float2 display_resolution;
    AlphaMarker alpha_stops[MAX_MARKER_COUNT];
    int alpha_sec_num;
    float alpha_blur_width;
    float2 pad;
};

Texture2D src : register(t0);
SamplerState samp : register(s0);

#define EPS 1e-6

// 黒付近は通常のlerpにフォールバックする
bool isNearBlack(float3 srgb)
{
    float luma = dot(srgb, float3(0.2126, 0.7152, 0.0722));
    return luma < EPS;
}

float3 safeSpectralMix(float3 col1, float3 col2, float t)
{
    if (isNearBlack(col1) || isNearBlack(col2)) {
        return lerp(col1, col2, t);
    }
    return spectral_mix(col1, col2, t);
}

float smoothPulse(float t, float mid, float width)
{
    float half_width = width * 0.5;

    float lower = mid - half_width;
    float upper = mid + half_width;

    return smoothstep(lower, upper, t);
}

float4 makeGradient(float4 color1, float4 color2, float t, float mid, float width, int color_space, int interp_dir)
{
    t = smoothPulse(t, mid, width);

    float3 col1 = color1.rgb;
    float3 col2 = color2.rgb;
    float alpha1 = color1.a;
    float alpha2 = color2.a;
    float3 result = float3(0.0, 0.0, 0.0);
    float mixed_alpha = max(alpha_mix(alpha1, alpha2, t), 1e-6);
    switch (color_space)
    {
    case 0: // sRGB
    {
        float3 premulti_srgb1 = col1 * alpha1;
        float3 premulti_srgb2 = col2 * alpha2;
        float3 mixed_srgb = lerp(premulti_srgb1, premulti_srgb2, t);
        float3 unpremulti_srgb = mixed_srgb / mixed_alpha;
        result = unpremulti_srgb;
        break;
    }
    case 1: // Linear sRGB
    {
        float3 premulti_linear1 = srgb2linear(col1) * alpha1;
        float3 premulti_linear2 = srgb2linear(col2) * alpha2;
        float3 mixed_linear = lerp(premulti_linear1, premulti_linear2, t);
        float3 unpremulti_linear = mixed_linear / mixed_alpha;
        result = linear2srgb(clamp(unpremulti_linear, 0.0, 1.0));
        break;
    }
    case 2: // HSV
    {
        float3 hsv1 = srgb2hsv(col1);
        float3 hsv2 = srgb2hsv(col2);

        // 片方が透明であってもその色が持っているHueを維持してグラデーションを作るために
        // アルファを掛ける前の彩度で無彩色かどうかを判定する
        bool has_valid_hue1 = hsv1.y > SATURATION_THRESHOLD;
        bool has_valid_hue2 = hsv2.y > SATURATION_THRESHOLD;
        float mixed_hue = adjust_and_mix_hue(hsv1.x, hsv2.x, has_valid_hue1, has_valid_hue2, t, interp_dir);

        // 彩度と明度はアルファを掛けた後で補間する
        float2 sv1 = hsv1.yz * alpha1;
        float2 sv2 = hsv2.yz * alpha2;
        float2 mixed_sv = lerp(sv1, sv2, t);
        mixed_sv /= mixed_alpha;

        // 結果の色を合成
        float3 result_hsv = float3(mixed_hue, mixed_sv);
        result = hsv2srgb(result_hsv);
        break;
    }
    case 3: // HSL
    {
        float3 hsl1 = srgb2hsl(col1);
        float3 hsl2 = srgb2hsl(col2);

        bool has_valid_hue1 = hsl1.y > SATURATION_THRESHOLD;
        bool has_valid_hue2 = hsl2.y > SATURATION_THRESHOLD;
        float mixed_hue = adjust_and_mix_hue(hsl1.x, hsl2.x, has_valid_hue1, has_valid_hue2, t, interp_dir);

        float2 sl1 = hsl1.yz * alpha1;
        float2 sl2 = hsl2.yz * alpha2;
        float2 mixed_sl = lerp(sl1, sl2, t);
        mixed_sl /= mixed_alpha;

        float3 result_hsl = float3(mixed_hue, mixed_sl);
        result = hsl2srgb(result_hsl);
        break;
    }
    case 4: // L*a*b* (CIELAB)
    {
        // D50基準
        float3 lab1 = linear2d50lab(srgb2linear(col1));
        float3 lab2 = linear2d50lab(srgb2linear(col2));
        float3 premulti_lab1 = lab1 * alpha1;
        float3 premulti_lab2 = lab2 * alpha2;
        float3 mixed_lab = lerp(premulti_lab1, premulti_lab2, t);
        float3 unpremulti_lab = mixed_lab / mixed_alpha;
        result = linear2srgb(clamp(d50lab2linear(unpremulti_lab), 0.0, 1.0));
        break;
    }
    case 5: // LCh
    {
        // D50基準
        float3 lch1 = linear2d50lch(srgb2linear(col1));
        float3 lch2 = linear2d50lch(srgb2linear(col2));

        bool has_valid_hue1 = lch1.y > CHROMA_THRESHOLD;
        bool has_valid_hue2 = lch2.y > CHROMA_THRESHOLD;
        float mixed_hue = adjust_and_mix_hue(lch1.z, lch2.z, has_valid_hue1, has_valid_hue2, t, interp_dir);

        float2 lc1 = lch1.xy * alpha1;
        float2 lc2 = lch2.xy * alpha2;
        float2 mixed_lc = lerp(lc1, lc2, t);
        mixed_lc = mixed_lc / mixed_alpha;

        mixed_lc.x = clamp(mixed_lc.x, 0.0, 100.0);
        mixed_lc.y = max(mixed_lc.y, 0.0);

        float3 result_lch = float3(mixed_lc.x, mixed_lc.y, mixed_hue);
        result = linear2srgb(clamp(d50lch2linear(result_lch), 0.0, 1.0));
        break;
    }
    case 6: // Oklab
    {
        float3 oklab1 = linear2oklab(srgb2linear(col1));
        float3 oklab2 = linear2oklab(srgb2linear(col2));
        float3 premulti_oklab1 = oklab1 * alpha1;
        float3 premulti_oklab2 = oklab2 * alpha2;
        float3 mixed_oklab = lerp(premulti_oklab1, premulti_oklab2, t);
        float3 unpremulti_oklab = mixed_oklab / mixed_alpha;
        result = linear2srgb(clamp(oklab2linear(unpremulti_oklab), 0.0, 1.0));
        break;
    }
    case 7: // OkLCh
    {
        float3 oklch1 = linear2oklch(srgb2linear(col1));
        float3 oklch2 = linear2oklch(srgb2linear(col2));

        bool has_valid_hue1 = oklch1.y > CHROMA_THRESHOLD;
        bool has_valid_hue2 = oklch2.y > CHROMA_THRESHOLD;
        float mixed_hue = adjust_and_mix_hue(oklch1.z, oklch2.z, has_valid_hue1, has_valid_hue2, t, interp_dir);

        float2 lc1 = oklch1.xy * alpha1;
        float2 lc2 = oklch2.xy * alpha2;
        float2 mixed_lc = lerp(lc1, lc2, t);
        mixed_lc = mixed_lc / mixed_alpha;

        mixed_lc.x = clamp(mixed_lc.x, 0.0, 1.0);
        mixed_lc.y = max(mixed_lc.y, 0.0);

        float3 result_oklch = float3(mixed_lc.x, mixed_lc.y, mixed_hue);
        result = linear2srgb(clamp(oklch2linear(result_oklch), 0.0, 1.0));
        break;
    }
    case 8:  // Kubelka-Munk
    {
        result = safeSpectralMix(col1, col2, t);
        break;
    }
    }

    return float4(result, mixed_alpha);
}

float4 alphaBlend(float4 src, float4 dst)
{
    float3 rgb = lerp(src.rgb, dst.rgb, dst.a);
    float a = src.a + dst.a * (1 - src.a);
    return float4(rgb, a);
}

float4 drawCheckerBoard(float2 pixel_pos, float2 checker_size, float3 color1, float3 color2)
{
    int2 cell = int2(pixel_pos / checker_size);
    bool is_even = ((cell.x + cell.y) & 1) == 0;
    float3 color = is_even ? color1 : color2;
    return float4(color, 1.0);
}

float4 psmain(VS_OUTPUT input) : SV_Target
{
    float x = input.uv.x;

    float2 scale = float2(texture_resolution / display_resolution);
    float2 display_uv_pos = float2(input.pos.xy / scale);
    float4 transparent_checker_col = drawCheckerBoard(display_uv_pos, 8, float3(0.75, 0.75, 0.75), float3(0.90, 0.90, 0.90));

    float4 gradient_col = float4(0, 0, 0, 0);
    if (x <= gradient[0].start_x) {
        gradient_col = gradient[0].start_color;
    } else if (x >= gradient[gradient_count - 1].stop_x) {
        gradient_col = gradient[gradient_count - 1].stop_color;
    } else {
        for (int i = 0; i < gradient_count; i++) {
            float p_curr = gradient[i].start_x;
            float p_next = gradient[i].stop_x;

            // x が現在の区間内にある場合
            if (x >= p_curr && x < p_next) {
                float dist = p_next - p_curr;
                float t = (x - p_curr) / dist;

                gradient_col = makeGradient(gradient[i].start_color, gradient[i].stop_color, t, gradient[i].ratio, gradient_w, gradient_type, interp_dir);
                break;
            }
        }
    }

    if (x <= alpha_stops[0].start_pos) {
        gradient_col.a *= alpha_stops[0].start_value;
    } else if (x >= alpha_stops[alpha_sec_num - 1].stop_pos) {
        gradient_col.a *= alpha_stops[alpha_sec_num - 1].stop_value;
    } else {
        for (int j = 0; j < alpha_sec_num; j++) {
            float p_curr = alpha_stops[j].start_pos;
            float p_next = alpha_stops[j].stop_pos;

            if (x >= p_curr && x < p_next) {
                float dist = p_next - p_curr;
                float t = (x - p_curr) / dist;
                float alpha_value = lerp(alpha_stops[j].start_value, alpha_stops[j].stop_value, smoothPulse(t, alpha_stops[j].mid_ratio, alpha_blur_width));
                gradient_col.a *= alpha_value;
                break;
            }
        }
    }

    return alphaBlend(transparent_checker_col, gradient_col);
}
