// Gradient Editor v0.4.3 以前を使用していたプロジェクトファイルを v0.5.0 以降の形式に変換するツール
// 使い方：
//   1. このファイルを exe としてビルドする。
//   2. v0.4.3 以前を使用していたプロジェクトファイル（aup2）をビルドした exe にドラッグ&ドロップする。
//      変換に成功すると変換後のプロジェクトファイルが同一階層のディレクトリに出力される。

// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// clang-format off

#include <shellapi.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>


namespace fs = std::filesystem;

// double を小数点以下2桁の文字列にする
static std::string fmt2(double v)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
}

// キーバリューをパース
static bool parseKV(const std::string& line, std::string& key, std::string& val)
{
    auto pos = line.find('=');
    if (pos == std::string::npos) {
        key = "";
        val = "";
        return false;
    }
    key = line.substr(0, pos);
    val = line.substr(pos + 1);
    return true;
}

static std::string getValue(const std::unordered_map<std::string, std::string>& dict, const std::string& key, const std::string& def)
{
    auto it = dict.find(key);
    if (it != dict.end()) {
        return it->second;
    }
    return def;
}

static std::string convertGradientSection(const std::string& section, const std::string& script_name)
{
    std::unordered_map<std::string, std::string> kv;
    std::vector<std::string> lines;

    std::istringstream reader(section);
    std::string line;
    while (std::getline(reader, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
        std::string k, v;
        if (parseKV(line, k, v)) {
            kv[k] = v;
        }
    }

    auto mc_it = kv.find(u8"マーカー数");
    if (mc_it == kv.end()) {
        return section;
    }
    int mc = 0;
    try {
        mc = std::stoi(mc_it->second);
    } catch (...) {
        return section;
    }

    // 色
    std::string color_str;
    for (int n = 1; n <= mc; n++) {
        if (n > 1) color_str += ",";
        color_str += "0x" + getValue(kv, u8"色" + std::to_string(n), "000000");
    }

    // 色の透明度（透明度N / 100）
    std::string alpha_str;
    for (int n = 1; n <= mc; n++) {
        if (n > 1) alpha_str += ",";
        std::string alpha_val = getValue(kv, u8"透明度" + std::to_string(n), "0.00");
        try {
            double v = std::stod(alpha_val);
            alpha_str += fmt2(v / 100.0);
        } catch (...) {
            alpha_str += "0.00";
        }
    }

    // 位置（位置N / 100、昇順ソート）
    std::vector<double> positions;
    for (int n = 1; n <= mc; n++) {
        std::string pos_val = getValue(kv, u8"位置" + std::to_string(n), "0.00");
        try {
            double v = std::stod(pos_val);
            positions.push_back(v / 100.0);
        } catch (...) {
            positions.push_back(0.00);
        }
    }
    std::sort(positions.begin(), positions.end());
    std::string pos_str;
    for (size_t i = 0; i < positions.size(); i++) {
        if (i > 0) pos_str += ",";
        pos_str += fmt2(positions[i]);
    }

    // 中間点（中間点N / 100、マーカー数-1 個）
    std::string mid_str;
    for (int n = 1; n < mc; n++) {
        if (n > 1) mid_str += ",";
        std::string mid_val = getValue(kv, u8"中間点" + std::to_string(n), "0.50");
        try {
            double v = std::stod(mid_val);
            mid_str += fmt2(v / 100.0);
        } catch (...) {
            mid_str += "0.00";
        }
    }

    // ヘッダー行
    std::string header = lines.empty() ? "" : lines[0];

    // v0.5.0 以降の形式に変換
    std::ostringstream out_sb;
    out_sb << header << "\n";
    out_sb << u8"effect.name=" << script_name << "\n";
    out_sb << u8"強さ=" << getValue(kv, u8"強さ", "100.00") << "\n";

    if (script_name == "MultiGradient@GradientEditor") {
        out_sb << u8"中心X=" << getValue(kv, u8"中心X", "0.00") << "\n";
        out_sb << u8"中心Y=" << getValue(kv, u8"中心Y", "0.00") << "\n";
        out_sb << u8"Group=1\n";
        out_sb << u8"角度=" << getValue(kv, u8"角度", "90.0") << "\n";
        out_sb << u8"幅=" << getValue(kv, u8"幅", "100") << "\n";
        out_sb << u8"背景透明度=" << getValue(kv, u8"背景透明度", "0.00") << "\n";
        out_sb << u8"形状=" << getValue(kv, u8"形状", u8"線形") << "\n";
    } else {
        out_sb << u8"背景透明度=" << getValue(kv, u8"背景透明度", "0.00") << "\n";
        out_sb << u8"ルーマ=" << getValue(kv, u8"ルーマ", "Rec. 601") << "\n";
    }

    out_sb << u8"シフト=" << getValue(kv, u8"シフト", "0.00") << "\n";
    out_sb << u8"境界モード=" << getValue(kv, u8"境界モード", u8"境界色") << "\n";
    out_sb << u8"合成モード=" << getValue(kv, u8"合成モード", u8"通常") << "\n";

    if (script_name == "MultiGradient@GradientEditor") {
        out_sb << u8"幅をオブジェクトに合わせる=" << getValue(kv, u8"幅をオブジェクトに合わせる", "1") << "\n";
    }

    out_sb << u8"グラデーションデータ.hide=1\n";
    out_sb << u8"色=" << color_str << "\n";
    out_sb << u8"色.hide=1\n";
    out_sb << u8"色の透明度=" << alpha_str << "\n";
    out_sb << u8"位置=" << pos_str << "\n";
    out_sb << u8"位置.hide=1\n";
    out_sb << u8"中間点=" << mid_str << "\n";
    out_sb << u8"中間点.hide=1\n";
    out_sb << u8"マーカー数=" << mc << "\n";
    out_sb << u8"ぼかし幅=" << getValue(kv, u8"ぼかし幅", "100") << "\n";
    out_sb << u8"色空間=" << getValue(kv, u8"色空間", "Linear sRGB") << "\n";
    out_sb << u8"補間経路=" << getValue(kv, u8"補間経路", "短経路") << "\n";
    out_sb << u8"アルファ値=1.00,1.00\n";
    out_sb << u8"アルファ位置=0.00,1.00\n";
    out_sb << u8"アルファ中間点=0.50\n";
    out_sb << u8"アルファマーカー数=2\n";
    out_sb << u8"アルファぼかし幅=100\n";

    return out_sb.str();
}

static bool processFile(const std::wstring& input_path_str)
{
    fs::path input_path(input_path_str);

    fs::path parent_dir  = input_path.parent_path();
    std::wstring stem   = input_path.stem().wstring();
    std::wstring ext    = input_path.extension().wstring();
    fs::path output_path = parent_dir / (stem + L"_converted" + ext);

    // バイナリで読み込んで UTF-8 として処理
    std::ifstream ifs(input_path, std::ios::binary);
    if (!ifs) {
        std::wstring msg = L"読み込み失敗:\n" + input_path.wstring();
        MessageBoxW(NULL, msg.c_str(), L"エラー", MB_ICONERROR);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    // UTF-8 BOM（EF BB BF）の除去
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3);
    }

    std::string result;
    std::string current;
    std::string script_name = "MultiGradient@GradientEditor";
    bool in_gradient_section         = false;
    bool in_project_section   = false;

    std::istringstream reader(content);
    std::string line;
    while (std::getline(reader, line)) {
        std::string trimmed = line;
        if (!trimmed.empty() && trimmed.back() == '\r') {
            trimmed.pop_back();
        }

        // 新しいセクションヘッダー検出
        if (!trimmed.empty() && trimmed[0] == '[') {
            if (!current.empty()) {
                if (in_gradient_section)
                    result += convertGradientSection(current, script_name);
                else
                    result += current;
            }
            current    = trimmed + "\n";
            in_gradient_section = false;

            // [project] セクションに入ったか判定
            if (trimmed == "[project]") {
                in_project_section = true;
            } else {
                in_project_section = false;
            }
            continue;
        }

        // [project] 内の file = から始まる行の末尾（拡張子の前）に _converted を挿入
        if (in_project_section && trimmed.rfind("file=", 0) == 0) {
            std::string original_path_str = trimmed.substr(5);
            auto dot                    = original_path_str.rfind('.');
            if (dot != std::string::npos) {
                original_path_str.insert(dot, "_converted");
            } else {
                original_path_str += "_converted";
            }
            trimmed = "file=" + original_path_str;
        }

        current += trimmed + "\n";

        if (trimmed == "effect.name=MultiGradient@GradientEditor") {
            script_name = "MultiGradient@GradientEditor";
            in_gradient_section  = true;
        } else if (trimmed == "effect.name=GradientMap@GradientEditor") {
            script_name = "GradientMap@GradientEditor";
            in_gradient_section  = true;
        }
    }

    if (!current.empty()) {
        if (in_gradient_section)
            result += convertGradientSection(current, script_name);
        else
            result += current;
    }

    // 書き出し
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) {
        std::wstring msg = L"書き出し失敗:\n" + output_path.wstring();
        MessageBoxW(NULL, msg.c_str(), L"エラー", MB_ICONERROR);
        return false;
    }
    ofs.write(result.c_str(), result.size());
    ofs.close();

    return true;
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return 1;
    }

    if (argc < 2) {
        MessageBoxW(NULL,
                    L"aup2 ファイルをこの exe にドラッグ＆ドロップしてください。\n",
                    L"Gradient Editor Project Converter", MB_ICONINFORMATION);
        LocalFree(argv);
        return 0;
    }

    int ok = 0, fail = 0;
    std::wstring success_list, fail_list;

    for (int i = 1; i < argc; i++) {
        std::wstring path = argv[i];
        if (processFile(path)) {
            ok++;
            fs::path p(path);
            fs::path parent_dir = p.parent_path();
            std::wstring stem  = p.stem().wstring();
            std::wstring ext   = p.extension().wstring();
            fs::path outPath   = parent_dir / (stem + L"_converted" + ext);
            success_list += L"  " + outPath.wstring() + L"\n";
        } else {
            fail++;
            fail_list += L"  " + path + L"\n";
        }
    }

    std::wstring msg;
    if (ok > 0) msg += L"変換成功 (" + std::to_wstring(ok) + L" 件):\n" + success_list;
    if (fail > 0) msg += L"\n変換失敗 (" + std::to_wstring(fail) + L" 件):\n" + fail_list;

    MessageBoxW(NULL, msg.c_str(), L"Gradient Editor Project Converter",
                fail > 0 ? MB_ICONWARNING : MB_ICONINFORMATION);

    LocalFree(argv);
    return fail > 0 ? 1 : 0;
}
