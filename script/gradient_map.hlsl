Texture2D<float4> src : register(t0);
SamplerState samp : register(s0);

#define EPS 1e-6
static const int MARKER_MAX_COUNT = 30;
static const int GRADIENT_MAX_COUNT = MARKER_MAX_COUNT - 1;

cbuffer constant0 : register(b0) {
    float luma_mode;
    float shift;
    float edge_mode;
    float PAD1;
    float color_space;
    float interp_dir;
    float gradient_w;
    float marker_count;
    float4 start_col[MARKER_MAX_COUNT];
    float4 stop_col[MARKER_MAX_COUNT];
    float4 pos_and_mid[GRADIENT_MAX_COUNT];
    float alpha_marker_count;
    float3 PAD2;
    float4 alpha_pos_and_value[GRADIENT_MAX_COUNT];
}

float4 blend_colors(float4 color1, float4 color2, float t, float color_space, int interp_dir)
{
    float3 col1 = color1.rgb;
    float3 col2 = color2.rgb;
    float alpha1 = color1.a;
    float alpha2 = color2.a;
    float3 result = float3(0.0, 0.0, 0.0);
    float mixed_alpha = max(alpha_mix(alpha1, alpha2, t), EPS);
    switch (color_space) {
        case 0:  // sRGB
        {
            float3 premulti_srgb1 = col1 * alpha1;
            float3 premulti_srgb2 = col2 * alpha2;
            float3 mixed_srgb = lerp(premulti_srgb1, premulti_srgb2, t);
            float3 unpremulti_srgb = mixed_srgb / mixed_alpha;
            result = unpremulti_srgb;
            break;
        }
        case 1:  // Linear sRGB
        {
            float3 premulti_linear1 = srgb2linear(col1) * alpha1;
            float3 premulti_linear2 = srgb2linear(col2) * alpha2;
            float3 mixed_linear = lerp(premulti_linear1, premulti_linear2, t);
            float3 unpremulti_linear = mixed_linear / mixed_alpha;
            result = linear2srgb(clamp(unpremulti_linear, 0.0, 1.0));
            break;
        }
        case 2:  // HSV
        {
            float3 hsv1 = srgb2hsv(col1);
            float3 hsv2 = srgb2hsv(col2);

            // 片方が透明であってもその色が持っているHueを維持してグラデーションを作るために、
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
        case 3:  // HSL
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
        case 4:  // L*a*b* (CIELAB)
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
        case 5:  // LCh
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
        case 6:  // Oklab
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
        case 7:  // OkLCh
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
    }
    return float4(result * mixed_alpha, mixed_alpha);
}

float smoothPulse(float t, float mid, float width)
{
    float half_width = width * 0.5;

    float lower = mid - half_width;
    float upper = mid + half_width;

    return smoothstep(lower, upper, t);
}

float4 makeGradient(float4 col1, float4 col2, float t, float mid, float width, float color_space, int interp_dir)
{
    return blend_colors(col1, col2, smoothPulse(t, mid, width), color_space, interp_dir);
}

float4 unpremulti(float4 col)
{
    return col.a > 0.0 ? float4(col.rgb / col.a, 1.0) : float4(col.rgb, 1.0);
}

float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target {
    float4 tex_col = src.Sample(samp, uv);

    // アンチエイリアス等でアルファが1未満の場合、RGBがPre-multipliedだと輝度が低く判定されてしまうため、
    // アルファで除算して元の色の輝度を取得する
    float3 unpremul_col = unpremulti(tex_col).rgb;

    float x;
    switch ((int)luma_mode) {
        case 0:  // Rec. 601
        {
            x = dot(unpremul_col, float3(0.299, 0.587, 0.114));
            break;
        }
        case 1:  // Rec. 701
        {
            x = dot(unpremul_col, float3(0.2126, 0.7152, 0.0722));
            break;
        }
        default:
        {
            x = dot(unpremul_col, float3(0.299, 0.587, 0.114));
            break;
        }
    }

    switch ((int)edge_mode) {
        case 0:  // 境界色
        {
            x = clamp(x + shift, 0.0, 1.0);
            break;
        }
        case 1:  // ループ
        {
            x = frac(x + shift);
            break;
        }
        case 2:  // ミラー
        {
            x = abs(frac((x + shift) * 0.5 + 0.5) * 2.0 - 1.0);
            break;
        }
        default:
        {
            x = clamp(x + shift, 0.0, 1.0);
            break;
        }
    }

    int safe_marker_count = min(marker_count, MARKER_MAX_COUNT);
    int safe_gradient_count = clamp(safe_marker_count - 1, 1, MARKER_MAX_COUNT - 1);

    float4 out_col = (x <= pos_and_mid[0].x) ? start_col[0] : start_col[safe_marker_count - 1];
    out_col.rgb *= out_col.a;
    x = saturate(x);

    for (int i = 0; i < safe_gradient_count; i++) {
        float p_curr = pos_and_mid[i].x;
        float p_next = pos_and_mid[i].y;

        // luma が現在の区間内にある場合
        if (p_curr <= x && x < p_next) {
            float dist = p_next - p_curr;
            float t = (x - p_curr) / dist;
            out_col = makeGradient(start_col[i], stop_col[i], t, pos_and_mid[i].z, gradient_w, color_space, interp_dir);
            break;
        }
    }

    float alpha = 1.0;
    if (x < alpha_pos_and_value[0].x) {
        alpha = alpha_pos_and_value[0].z;
        out_col.a *= alpha;
        out_col.rgb *= out_col.a;
        return out_col;
    } else if (x >= alpha_pos_and_value[(int)alpha_marker_count - 2].y) {
        alpha = alpha_pos_and_value[(int)alpha_marker_count - 2].w;
        out_col.a *= alpha;
        out_col.rgb *= out_col.a;
        return out_col;
    }
    for (int j = 0; j < (int)alpha_marker_count - 1; j++) {
        float p_curr = alpha_pos_and_value[j].x;  // 区間の開始位置
        float p_next = alpha_pos_and_value[j].y;  // 区間の終了位置

        // x が現在の区間内にある場合
        if (p_curr <= x && x < p_next) {
            float dist = p_next - p_curr;
            float t = (x - p_curr) / dist;
            alpha = lerp(alpha_pos_and_value[j].z, alpha_pos_and_value[j].w, t);
            out_col.a *= alpha;
            out_col.rgb *= out_col.a;
            break;
        }
    }

    return out_col;
}
