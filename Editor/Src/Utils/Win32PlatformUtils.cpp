//
// Created by johnk on 2026/7/11.
//

#include <Editor/Utils/PlatformUtils.h>

#if PLATFORM_WINDOWS

#include <shobjidl.h>

namespace Editor::Internal {
    static std::wstring Utf8ToWide(const std::string& inString)
    {
        if (inString.empty()) {
            return {};
        }

        const int length = MultiByteToWideChar(CP_UTF8, 0, inString.data(), static_cast<int>(inString.size()), nullptr, 0);
        std::wstring result(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, inString.data(), static_cast<int>(inString.size()), result.data(), length);
        return result;
    }

    static std::string WideToUtf8(const wchar_t* inString)
    {
        const int length = WideCharToMultiByte(CP_UTF8, 0, inString, -1, nullptr, 0, nullptr, nullptr);
        if (length <= 1) {
            return {};
        }

        std::string result(static_cast<size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, inString, -1, result.data(), length, nullptr, nullptr);
        result.resize(static_cast<size_t>(length - 1));
        return result;
    }
}

namespace Editor {
    std::optional<std::string> PlatformUtils::SelectDirectory(const std::string& inTitle, const std::string& inInitialDirectory)
    {
        const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        const bool shouldUninitialize = SUCCEEDED(initializeResult);
        if (FAILED(initializeResult) && initializeResult != RPC_E_CHANGED_MODE) {
            return std::nullopt;
        }

        std::optional<std::string> result;
        IFileOpenDialog* dialog = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
            FILEOPENDIALOGOPTIONS options = 0;
            if (SUCCEEDED(dialog->GetOptions(&options))) {
                dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
            }

            const std::wstring title = Internal::Utf8ToWide(inTitle);
            dialog->SetTitle(title.c_str());

            if (!inInitialDirectory.empty()) {
                IShellItem* initialFolder = nullptr;
                const std::wstring initialDirectory = Internal::Utf8ToWide(inInitialDirectory);
                if (SUCCEEDED(SHCreateItemFromParsingName(initialDirectory.c_str(), nullptr, IID_PPV_ARGS(&initialFolder)))) {
                    dialog->SetFolder(initialFolder);
                    initialFolder->Release();
                }
            }

            if (SUCCEEDED(dialog->Show(nullptr))) {
                IShellItem* selectedFolder = nullptr;
                if (SUCCEEDED(dialog->GetResult(&selectedFolder))) {
                    PWSTR selectedPath = nullptr;
                    if (SUCCEEDED(selectedFolder->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath))) {
                        result = Internal::WideToUtf8(selectedPath);
                        CoTaskMemFree(selectedPath);
                    }
                    selectedFolder->Release();
                }
            }
            dialog->Release();
        }

        if (shouldUninitialize) {
            CoUninitialize();
        }
        return result;
    }
}

#endif
