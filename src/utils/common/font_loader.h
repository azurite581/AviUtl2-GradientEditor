#ifndef FONT_LOADER_H
#define FONT_LOADER_H

// clang-format off
#include <format>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// clang-format off

#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <expected>
#include <format>

#pragma comment(lib, "dwrite.lib")

struct FontData {
    std::vector<unsigned char> bytes;
    UINT32 face_index;  // TTC 内のインデックス
};

inline std::expected<FontData, std::wstring>
getFontDataByName(const std::wstring& font_name)
{
    using Microsoft::WRL::ComPtr;

    ComPtr<IUnknown> unknown;
    if (FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            &unknown)))
        return std::unexpected(L"Failed to create DWrite factory");

    ComPtr<IDWriteFactory> factory;
    if (FAILED(unknown.As(&factory)))
        return std::unexpected(L"Failed to get IDWriteFactory");

    ComPtr<IDWriteFontCollection> collection;
    if (FAILED(factory->GetSystemFontCollection(&collection)))
        return std::unexpected(L"Failed to get font collection");

    UINT32 index = 0;
    BOOL exists  = FALSE;
    if (FAILED(collection->FindFamilyName(font_name.c_str(), &index, &exists)))
        return std::unexpected(L"FindFamilyName failed");
    if (!exists)
        return std::unexpected(std::format(L"Font '{}' not found", font_name));

    ComPtr<IDWriteFontFamily> family;
    if (FAILED(collection->GetFontFamily(index, &family)))
        return std::unexpected(L"GetFontFamily failed");

    ComPtr<IDWriteFont> font;
    if (FAILED(family->GetFirstMatchingFont(
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            &font)))
        return std::unexpected(L"GetFirstMatchingFont failed");

    ComPtr<IDWriteFontFace> font_face;
    if (FAILED(font->CreateFontFace(&font_face)))
        return std::unexpected(L"CreateFontFace failed");

    UINT32 number_of_files = 0;
    font_face->GetFiles(&number_of_files, nullptr);
    if (number_of_files == 0)
        return std::unexpected(std::format(L"No font files for '{}'", font_name));
    UINT32 face_index = font_face->GetIndex();

    std::vector<ComPtr<IDWriteFontFile>> font_files(number_of_files);
    if (FAILED(font_face->GetFiles(&number_of_files, font_files[0].GetAddressOf())))
        return std::unexpected(L"GetFiles failed");

    ComPtr<IDWriteFontFileLoader> loader;
    if (FAILED(font_files[0]->GetLoader(&loader)))
        return std::unexpected(L"GetLoader failed");

    ComPtr<IDWriteLocalFontFileLoader> local_loader;
    if (FAILED(loader.As(&local_loader)))
        return std::unexpected(
            std::format(L"Not a local font file loader for '{}'", font_name));

    const void* ref_key      = nullptr;
    UINT32      ref_key_size = 0;
    if (FAILED(font_files[0]->GetReferenceKey(&ref_key, &ref_key_size)))
        return std::unexpected(L"GetReferenceKey failed");

    ComPtr<IDWriteFontFileStream> stream;
    if (FAILED(loader->CreateStreamFromKey(ref_key, ref_key_size, &stream)))
        return std::unexpected(L"CreateStreamFromKey failed");

    UINT64 file_size = 0;
    if (FAILED(stream->GetFileSize(&file_size)))
        return std::unexpected(L"GetFileSize failed");

    const void* fragment_start = nullptr;
    void*       context        = nullptr;
    if (FAILED(stream->ReadFileFragment(&fragment_start, 0, file_size, &context)))
        return std::unexpected(L"ReadFileFragment failed");

    std::vector<unsigned char> buffer(static_cast<size_t>(file_size));
    memcpy(buffer.data(), fragment_start, buffer.size());
    stream->ReleaseFileFragment(context);

    return FontData{buffer, face_index};
}

#endif  // !FONT_LOADER_H
