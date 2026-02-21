Texture2D<float4> src : register(t0);
SamplerState samp : register(s0);

static const int GRADIENT_MAX_COUNT = 30;
cbuffer constant0 : register(b0) {
    float2 resolution;
    float2 center;
    float radius;
    float gradient_type;
    float is_fit;
    float pad1;
    float2x2 angle;
    float2 pad2;
    float color_space;
    float interp_dir;
    float gradient_w;
    float gradient_count;  // 実際のグラデーションの数
    float4 start_col[GRADIENT_MAX_COUNT];
    float4 stop_col[GRADIENT_MAX_COUNT];
    float4 pos_and_mid[GRADIENT_MAX_COUNT];
}

float4 blend_colors(float4 color1, float4 color2, float t, float color_space, int interp_dir)
{
    float3 col1 = color1.rgb;
    float3 col2 = color2.rgb;
    float alpha1 = color1.a;
    float alpha2 = color2.a;
    float3 result = float3(0.0, 0.0, 0.0);
    float mixed_alpha = max(alpha_mix(alpha1, alpha2, t), 1e-6);
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

float4 makeGradient(float4 col1, float4 col2, float t, float mid, float width, float color_space, int interp_dir)
{
    float half_width = width * 0.5;

    float lower = mid - half_width;
    float upper = mid + half_width;

    t = smoothstep(lower, upper, t);

    float4 result = blend_colors(col1, col2, t, color_space, interp_dir);
    return result;
}

float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target {
    int safe_count = min(gradient_count, GRADIENT_MAX_COUNT);
    if (safe_count <= 0) return float4(1, 1, 1, 1);

    float x = 1.0;

    // 画面の長辺または短辺で正規化
    float aspect = max(resolution.x, resolution.y);

    // 中心からの相対座標
    float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / aspect;

    switch (gradient_type) {
        case 0: // 線形
        {
            if (is_fit <= 0) {
                float2 st = (pos.xy - center) / resolution.y;
                float scale = radius / resolution.y;
                st -= (resolution.xy / (resolution.y * 2.0));
                st = mul(angle, st);
                st += (resolution.xy / (resolution.y * 2.0));
                x = (st.y - 0.5) / scale + 0.5;
            } else {
                // 4隅の絶対座標
                float2 c0 = float2(0.0, 0.0);
                float2 c1 = float2(resolution.x, 0.0);
                float2 c2 = float2(0.0, resolution.y);
                float2 c3 = resolution.xy;

                // 回転後の軸への投影
                float y0 = mul(angle, c0).y;
                float y1 = mul(angle, c1).y;
                float y2 = mul(angle, c2).y;
                float y3 = mul(angle, c3).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float py = mul(angle, (pos.xy - center)).y;
                x = (py - min_y) / max(max_y - min_y, 1e-6);
            }
            break;
        }
        case 1: // 円形
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = (is_fit <= 0) ? radius / max(resolution.x, resolution.y) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            x = length(st) / max(scale * 2.0, 1e-6);
            break;
        }
        case 2: // 矩形
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = (is_fit <= 0) ? radius / max(resolution.x, resolution.y) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            x = (abs(st.x) + abs(st.y)) / max(scale * 2.0, 1e-6);
            break;
        }
        case 3:  // 凸形
        {
            if (is_fit <= 0) {
                float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
                float scale = radius / max(resolution.x, resolution.y);
                st = mul(angle, st);
                x = abs(st.y) / max(scale * 2.0, 1e-6);
                break;
            } else {
                // 4隅の絶対座標
                float2 c0 = float2(0.0, 0.0);
                float2 c1 = float2(resolution.x, 0.0);
                float2 c2 = float2(0.0, resolution.y);
                float2 c3 = resolution.xy;

                // 回転後の軸への投影
                float y0 = mul(angle, c0).y;
                float y1 = mul(angle, c1).y;
                float y2 = mul(angle, c2).y;
                float y3 = mul(angle, c3).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float py = mul(angle, (pos.xy - center)).y;
                float t = (py - min_y) / max(max_y - min_y, 1e-6);
                x = abs(t * 2.0 - 1.0);
                break;
            }
        }
        case 4: // 円形ループ
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            // ループ形状では radius が 0 だとモアレがあまりきれいではないので最低でも 1 にする
            float scale = max(radius, 1.0) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            float w = length(st) / max(scale * 2.0, 1e-6);
            float saw = fmod(w, 2.0);
            x = 1.0 - abs(1.0 - saw);
            break;
        }
        case 5: // 矩形ループ
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = max(radius, 1.0) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            float w = (abs(st.x) + abs(st.y)) / max(scale * 2.0, 1e-6);
            float saw = fmod(w, 2.0);
            x = 1.0 - abs(1.0 - saw);
            break;
        }
        case 6: // 凸形ループ
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = max(radius, 1.0) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            float w = st.y / max(scale * 2.0, 1e-6);
            float saw = w - 2.0 * floor(w / 2.0);  // mod(w, 2.0)
            x = 1.0 - abs(1.0 - saw);
            break;
        }
        default:  // 線形
        {
            float2 st = (pos.xy - center) / resolution.y;
            float scale = radius / resolution.y;
            st -= (resolution.xy / (resolution.y * 2.0));
            st = mul(angle, st);
            st += (resolution.xy / (resolution.y * 2.0));
            x = (st.y - 0.5) / scale + 0.5;
            break;
        }
    }

    x = 1.0 - x;
    float4 out_col = (x <= pos_and_mid[0].x) ? start_col[0] : start_col[safe_count - 1];
    out_col.rgb *= out_col.a;
    x = saturate(x);

    for (int i = 0; i < safe_count; i++) {
        float p_curr = pos_and_mid[i].x;
        float p_next = pos_and_mid[i].y;

        // x が現在の区間内にある場合
        if (x >= p_curr && x < p_next) {
            float dist = p_next - p_curr;

            float t = (x - p_curr) / dist;

            out_col = makeGradient(start_col[i], stop_col[i], t, pos_and_mid[i].z, gradient_w, color_space, interp_dir);
            break;
        }
    }

    return out_col;
}
