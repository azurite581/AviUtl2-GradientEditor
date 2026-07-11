Texture2D<float4> src : register(t0);
SamplerState samp : register(s0);

static const int MARKER_MAX_COUNT = ${MARKER_MAX_COUNT};
static const int GRADIENT_MAX_COUNT = MARKER_MAX_COUNT - 1;

cbuffer constant0 : register(b0) {
    float luma_mode;
    float shift;
    float edge_mode;
    float PAD1;
    float color_space;
    float interp_dir;
    float gradient_w;
    float color_section_count;
    float4 start_col[MARKER_MAX_COUNT];
    float4 stop_col[MARKER_MAX_COUNT];
    float4 pos_and_mid[GRADIENT_MAX_COUNT];
    float alpha_section_count;
    float alpha_blur_width;
    float2 PAD2;
    float4 alpha_pos_and_mid[GRADIENT_MAX_COUNT];
    float4 alpha_value[GRADIENT_MAX_COUNT];
}

float4 psmain(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
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
