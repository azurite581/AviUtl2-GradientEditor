Texture2D<float4> src : register(t0);
SamplerState samp : register(s0);

static const int MARKER_MAX_COUNT = ${MARKER_MAX_COUNT};
static const int GRADIENT_MAX_COUNT = MARKER_MAX_COUNT - 1;

cbuffer constant0 : register(b0) {
    float2 resolution;
    float2 center;
    float width;
    float gradient_type;
    float is_fit;
    float shift;
    float edge_mode;
    float width_unit;
    float2 PAD1;
    float2x2 angle;
    float2 PAD2;
    float color_space;
    float interp_dir;
    float gradient_w;
    float color_section_count;
    float4 start_col[MARKER_MAX_COUNT];
    float4 stop_col[MARKER_MAX_COUNT];
    float4 pos_and_mid[GRADIENT_MAX_COUNT];
    float alpha_section_count;
    float alpha_blur_width;
    float2 PAD3;
    float4 alpha_pos_and_mid[GRADIENT_MAX_COUNT];
    float4 alpha_value[GRADIENT_MAX_COUNT];
}

float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float x = 1.0;

    // 画面の長辺または短辺で正規化
    float aspect = max(resolution.x, resolution.y);

    // 中心からの相対座標
    float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / aspect;

    switch (gradient_type) {
        case 0:  // 線形
        {
            if (is_fit <= 0 && width_unit <= 0) {
                float2 st = (pos.xy - center) / resolution.y;
                float scale = width / resolution.y;

                st -= resolution.xy / (resolution.y * 2.0);
                st = mul(angle, st);
                st += resolution.xy / (resolution.y * 2.0);

                x = (st.y - 0.5) / scale + 0.5;
            } else {
                // 4隅を回転後の軸へ投影
                float y0 = mul(angle, float2(0.0, 0.0)).y;
                float y1 = mul(angle, float2(resolution.x, 0.0)).y;
                float y2 = mul(angle, float2(0.0, resolution.y)).y;
                float y3 = mul(angle, resolution.xy).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float gradient_min;
                float gradient_range;
                if (is_fit > 0) {
                    gradient_min = min_y;
                    gradient_range = max_y - min_y;
                } else {
                    gradient_range = (max_y - min_y) * (width / 100.0);

                    float gradient_center = (min_y + max_y) * 0.5;
                    gradient_min = gradient_center - gradient_range * 0.5;
                }

                float py = mul(angle, (pos.xy - center)).y;

                x = (py - gradient_min) / max(gradient_range, EPS);
            }
            break;
        }
        case 1:  // 円形
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = (is_fit <= 0) ? ((width_unit <= 0) ? width / max(resolution.x, resolution.y) : min(resolution.x, resolution.y) * (width / 100.0) / max(resolution.x, resolution.y)) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            x = length(st) / max(scale * 2.0, EPS);
            break;
        }
        case 2:  // 矩形
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = (is_fit <= 0) ? ((width_unit <= 0) ? width / max(resolution.x, resolution.y) : min(resolution.x, resolution.y) * (width / 100.0) / max(resolution.x, resolution.y)) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            x = (abs(st.x) + abs(st.y)) / max(scale * 2.0, EPS);
            break;
        }
        case 3:  // 凸形
        {
            if (is_fit <= 0 && width_unit <= 0) {
                float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
                float scale = width / max(resolution.x, resolution.y);
                st = mul(angle, st);

                x = abs(st.y) / max(scale * 2.0, EPS);
            } else {
                // 4隅を回転後の軸へ投影
                float y0 = mul(angle, float2(0.0, 0.0)).y;
                float y1 = mul(angle, float2(resolution.x, 0.0)).y;
                float y2 = mul(angle, float2(0.0, resolution.y)).y;
                float y3 = mul(angle, resolution.xy).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float py = mul(angle, (pos.xy - center)).y;
                float gradient_range = max_y - min_y;

                if (is_fit <= 0) {
                    gradient_range *= width / 100.0;
                }

                float gradient_center = (min_y + max_y) * 0.5;
                float gradient_min = gradient_center - gradient_range * 0.5;

                float t = (py - gradient_min) / max(gradient_range, EPS);
                x = abs(t * 2.0 - 1.0);
            }

            break;
        }
        case 4:  // 円形ループ
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            // ループ形状では width が 0 だとモアレがあまりきれいではないので、最低でも 1 にする
            float scale = (is_fit <= 0) ? ((width_unit <= 0) ? max(width, 1.0) / max(resolution.x, resolution.y) : max(min(resolution.x, resolution.y) * (width / 100.0), 1.0) / max(resolution.x, resolution.y)) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            float w = length(st) / max(scale * 2.0, EPS);
            float saw = fmod(w, 2.0);
            x = 1.0 - abs(1.0 - saw);
            break;
        }
        case 5:  // 矩形ループ
        {
            float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
            float scale = (is_fit <= 0) ? ((width_unit <= 0) ? max(width, 1.0) / max(resolution.x, resolution.y) : max(min(resolution.x, resolution.y) * (width / 100.0), 1.0) / max(resolution.x, resolution.y)) : min(resolution.x, resolution.y) / max(resolution.x, resolution.y);
            st = mul(angle, st);
            float w = (abs(st.x) + abs(st.y)) / max(scale * 2.0, EPS);
            float saw = fmod(w, 2.0);
            x = 1.0 - abs(1.0 - saw);
            break;
        }
        case 6:  // 凸形ループ
        {
            if (is_fit <= 0 && width_unit <= 0) {
                float2 st = ((pos.xy - center) * 2.0 - resolution.xy) / max(resolution.x, resolution.y);
                float scale = max(width, 1.0) / max(resolution.x, resolution.y);
                st = mul(angle, st);
                float w = st.y / max(scale * 2.0, EPS);
                float saw = w - 2.0 * floor(w / 2.0);  // mod(w, 2.0)
                x = 1.0 - abs(1.0 - saw);
            } else {
                // 4隅を回転後の軸へ投影
                float y0 = mul(angle, float2(0.0, 0.0)).y;
                float y1 = mul(angle, float2(resolution.x, 0.0)).y;
                float y2 = mul(angle, float2(0.0, resolution.y)).y;
                float y3 = mul(angle, resolution.xy).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float py = mul(angle, (pos.xy - center)).y;
                float gradient_range = max_y - min_y;

                if (is_fit <= 0) {
                    gradient_range *= width / 100.0;
                }

                float gradient_center = (min_y + max_y) * 0.5;
                float gradient_min = gradient_center - gradient_range * 0.5;

                float t = (py - gradient_min) / max(gradient_range, EPS);
                float saw = t - 2.0 * floor(t / 2.0);  // mod(t, 2.0)
                x = 1.0 - abs(1.0 - saw);
            }
            break;
        }
        default:  // 線形
        {
            if (is_fit <= 0 && width_unit <= 0) {
                float2 st = (pos.xy - center) / resolution.y;
                float scale = width / resolution.y;

                st -= resolution.xy / (resolution.y * 2.0);
                st = mul(angle, st);
                st += resolution.xy / (resolution.y * 2.0);

                x = (st.y - 0.5) / scale + 0.5;
            } else {
                // 4隅を回転後の軸へ投影
                float y0 = mul(angle, float2(0.0, 0.0)).y;
                float y1 = mul(angle, float2(resolution.x, 0.0)).y;
                float y2 = mul(angle, float2(0.0, resolution.y)).y;
                float y3 = mul(angle, resolution.xy).y;

                float min_y = min(min(y0, y1), min(y2, y3));
                float max_y = max(max(y0, y1), max(y2, y3));

                float gradient_min;
                float gradient_range;
                if (is_fit > 0) {
                    gradient_min = min_y;
                    gradient_range = max_y - min_y;
                } else {
                    gradient_range = (max_y - min_y) * (width / 100.0);

                    float gradient_center = (min_y + max_y) * 0.5;
                    gradient_min = gradient_center - gradient_range * 0.5;
                }

                float py = mul(angle, (pos.xy - center)).y;

                x = (py - gradient_min) / max(gradient_range, EPS);
            }
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

    x = 1.0 - x;

    int grad_sec_n = (int)color_section_count;
    float4 out_col = (x <= pos_and_mid[0].x) ? start_col[0] : start_col[grad_sec_n];

    // 区間ごとにグラデーションを作る
    [loop]
    for (int i = 0; i < grad_sec_n; ++i) {
        float2 pos = pos_and_mid[i].xy;

        // x が現在の区間内にある場合
        if (pos.x <= x && x < pos.y) {
            float t = (x - pos.x) / (pos.y - pos.x);
            out_col = makeGradient(
                start_col[i],
                stop_col[i],
                t,
                pos_and_mid[i].z,
                gradient_w,
                color_space,
                interp_dir
            );
            break;
        }
    }

    int alpha_sec_n = (int)alpha_section_count;
    float alpha = (x <= alpha_pos_and_mid[0].x) ? alpha_value[0].x : alpha_value[alpha_sec_n - 1].y;

    [loop]
    for (int j = 0; j < alpha_sec_n; ++j) {
        float2 pos = alpha_pos_and_mid[j].xy;

        if (pos.x <= x && x < pos.y) {
            float t = (x - pos.x) / (pos.y - pos.x);
            alpha = lerp(
                alpha_value[j].x,
                alpha_value[j].y,
                smoothPulse(t, alpha_pos_and_mid[j].z, alpha_blur_width)
            );
            break;
        }
    }

    out_col.a *= alpha;
    out_col.rgb *= out_col.a;

    return out_col;
}
