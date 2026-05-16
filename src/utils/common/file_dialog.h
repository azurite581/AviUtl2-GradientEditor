#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <variant>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <shobjidl.h>
#include <windows.h>

#include <expected>
#include <filesystem>
#include <string>
#include <vector>
#include <variant>

inline std::wstring hresultToString(HRESULT hr)
{
    LPWSTR buffer = nullptr;

    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    if (size == 0) {
        return L"Unknown error";
    }

    std::wstring message(buffer, size);

    LocalFree(buffer);

    return message;
}

enum class FileDialogResult {
    FD_OKAY,
    FD_CANCEL,
    FD_ERROR
};

struct OpenFileDialogResult {
    FileDialogResult result;
    std::variant<std::vector<std::filesystem::path>, std::monostate, std::wstring> value;
};

inline OpenFileDialogResult openFiles(const HWND owner = nullptr)
{
    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED);

    if (FAILED(hr)) {
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    IFileOpenDialog* dialog = nullptr;

    hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&dialog));

    if (FAILED(hr)) {
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    COMDLG_FILTERSPEC filters[] = {
        {L"GRDファイル (*.grd;*.GRD)",
         L"*.grd;*.GRD"}};

    dialog->SetFileTypes(
        static_cast<UINT>(std::size(filters)),
        filters);

    DWORD options = 0;

    hr = dialog->GetOptions(&options);

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();

        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    hr = dialog->SetOptions(
        options |
        FOS_ALLOWMULTISELECT |
        FOS_FILEMUSTEXIST);

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    hr = dialog->Show(owner);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_CANCEL, std::monostate{}};
    }

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    IShellItemArray* items = nullptr;

    hr = dialog->GetResults(&items);

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    DWORD count = 0;

    hr = items->GetCount(&count);

    if (FAILED(hr)) {
        items->Release();
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    std::vector<std::filesystem::path> paths;
    for (DWORD i = 0; i < count; ++i) {
        IShellItem* item = nullptr;

        hr = items->GetItemAt(i, &item);

        if (FAILED(hr)) {
            continue;
        }

        PWSTR path = nullptr;

        hr = item->GetDisplayName(
            SIGDN_FILESYSPATH,
            &path);

        if (SUCCEEDED(hr)) {
            paths.emplace_back(path);
            CoTaskMemFree(path);
        }

        item->Release();
    }

    items->Release();
    dialog->Release();

    CoUninitialize();

    return { FileDialogResult::FD_OKAY, paths };
}

struct WriteFileDialogResult {
    FileDialogResult result;
    std::variant<std::filesystem::path, std::monostate, std::wstring> value;
};

inline WriteFileDialogResult writeFile(const HWND owner = nullptr)
{
    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED);

    if (FAILED(hr)) {
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    IFileSaveDialog* dialog = nullptr;

    hr = CoCreateInstance(
        CLSID_FileSaveDialog,
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&dialog));

    if (FAILED(hr)) {
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    COMDLG_FILTERSPEC filters[] = {
        {L"GRDファイル (*.grd;*.GRD)", L"*.grd;*.GRD"}};

    dialog->SetFileTypes(_countof(filters), filters);
    dialog->SetDefaultExtension(L"grd");

    hr = dialog->Show(owner);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_CANCEL, std::monostate{}};
    }

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    IShellItem* item = nullptr;

    hr = dialog->GetResult(&item);

    if (FAILED(hr)) {
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    PWSTR path = nullptr;

    hr = item->GetDisplayName(
        SIGDN_FILESYSPATH,
        &path);

    if (FAILED(hr)) {
        item->Release();
        dialog->Release();
        CoUninitialize();
        return { FileDialogResult::FD_ERROR, hresultToString(hr)};
    }

    std::filesystem::path out_path = path;

    CoTaskMemFree(path);

    item->Release();
    dialog->Release();

    CoUninitialize();

    return { FileDialogResult::FD_OKAY, out_path };
}

#endif
