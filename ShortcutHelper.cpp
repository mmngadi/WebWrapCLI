#include "ShortcutHelper.h"
#include "IconHelper.h"
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlguid.h>
#include <objbase.h>
#include <strsafe.h>
#include <iostream>

void ShortcutHelper::CreateShortcut(const std::wstring& name,
    const std::wstring& iconPath,
    const std::wstring& targetUrl) {
    
    // Don't call CoInitialize here as it's already initialized in main via CoInitializeEx()
    
    IShellLinkW* pLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (LPVOID*)&pLink);
    
    if (FAILED(hr)) {
        std::wcerr << L"Error: Failed to create shell link instance. HRESULT: 0x" 
                   << std::hex << hr << std::dec << L"\n";
        return;
    }

    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        std::wcerr << L"Error: Failed to get module file name.\n";
        pLink->Release();
        return;
    }

    // Build arguments string with absolute icon path
    std::wstring args = L"--target \"" + targetUrl + L"\"";
    if (!name.empty()) {
        args += L" --name \"" + name + L"\"";
    }
    
    // Convert icon path to absolute before adding to arguments
    std::wstring absoluteIconPath;
    if (!iconPath.empty()) {
        wchar_t absPath[MAX_PATH];
        DWORD result = GetFullPathNameW(iconPath.c_str(), MAX_PATH, absPath, nullptr);
        if (result > 0 && result < MAX_PATH) {
            absoluteIconPath = absPath;
            args += L" --icon \"" + absoluteIconPath + L"\"";
        } else {
            // Fallback to original path if conversion fails
            absoluteIconPath = iconPath;
            args += L" --icon \"" + iconPath + L"\"";
        }
    }

    pLink->SetPath(exePath);
    pLink->SetArguments(args.c_str());
    
    if (!absoluteIconPath.empty()) {
        // Validate icon path exists before setting
        if (GetFileAttributesW(absoluteIconPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            // Get the ICO path (converts PNG if needed)
            std::wstring finalIconPath = IconHelper::GetConvertedIconPath(absoluteIconPath);
            
            if (!finalIconPath.empty() && GetFileAttributesW(finalIconPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                pLink->SetIconLocation(finalIconPath.c_str(), 0);
                
                if (IconHelper::IsPngFile(absoluteIconPath)) {
                    std::wcout << L"Converted PNG icon to ICO format for shortcut\n";
                    std::wcout << L"Absolute icon path in shortcut: " << absoluteIconPath << L"\n";
                }
            } else {
                std::wcerr << L"Warning: Failed to convert icon for shortcut\n";
            }
        } else {
            std::wcerr << L"Warning: Icon file not found: " << absoluteIconPath << L"\n";
        }
    }

    IPersistFile* pFile = nullptr;
    hr = pLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pFile);
    
    if (SUCCEEDED(hr)) {
        // Get the Desktop folder path properly using Windows API
        wchar_t desktopPath[MAX_PATH];
        hr = SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktopPath);
        
        if (SUCCEEDED(hr)) {
            std::wstring shortcutPath = std::wstring(desktopPath) + L"\\" + name + L".lnk";
            hr = pFile->Save(shortcutPath.c_str(), TRUE);
            
            if (SUCCEEDED(hr)) {
                std::wcout << L"Shortcut created successfully at: " << shortcutPath << L"\n";
            } else {
                std::wcerr << L"Error: Failed to save shortcut. HRESULT: 0x" 
                           << std::hex << hr << std::dec << L"\n";
            }
        } else {
            std::wcerr << L"Error: Failed to get desktop folder path. HRESULT: 0x" 
                       << std::hex << hr << std::dec << L"\n";
        }
        
        pFile->Release();
    } else {
        std::wcerr << L"Error: Failed to query IPersistFile interface. HRESULT: 0x" 
                   << std::hex << hr << std::dec << L"\n";
    }
    
    pLink->Release();
    
    // Don't call CoUninitialize here as COM is managed in main
}