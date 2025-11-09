#include "IconHelper.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <gdiplus.h>
#include <shlwapi.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Gdiplus;

// GDI+ initialization helper
class GdiplusInit {
public:
    GdiplusInit() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~GdiplusInit() {
        GdiplusShutdown(gdiplusToken);
    }
private:
    ULONG_PTR gdiplusToken;
};

bool IconHelper::IsPngFile(const std::wstring& path) {
    std::wstring ext = PathFindExtensionW(path.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext == L".png";
}

bool IconHelper::IsIcoFile(const std::wstring& path) {
    std::wstring ext = PathFindExtensionW(path.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext == L".ico";
}

bool IconHelper::ConvertPngToIco(const std::wstring& pngPath, const std::wstring& icoPath) {
    static GdiplusInit gdiplusInit;

    // Load the PNG image
    Bitmap* pngBitmap = Bitmap::FromFile(pngPath.c_str());
    if (!pngBitmap || pngBitmap->GetLastStatus() != Ok) {
        std::wcerr << L"Error: Failed to load PNG file: " << pngPath << L"\n";
        if (pngBitmap) delete pngBitmap;
        return false;
    }

    // Get original dimensions
    UINT width = pngBitmap->GetWidth();
    UINT height = pngBitmap->GetHeight();

    // Determine target size (scale to fit common icon sizes)
    UINT targetSize = 256;
    if (width <= 16 || height <= 16) targetSize = 16;
    else if (width <= 32 || height <= 32) targetSize = 32;
    else if (width <= 48 || height <= 48) targetSize = 48;
    else if (width <= 64 || height <= 64) targetSize = 64;
    else if (width <= 128 || height <= 128) targetSize = 128;

    // Create a square bitmap with the target size
    Bitmap* iconBitmap = new Bitmap(targetSize, targetSize, PixelFormat32bppARGB);
    Graphics* graphics = Graphics::FromImage(iconBitmap);
    
    if (!graphics) {
        delete pngBitmap;
        delete iconBitmap;
        return false;
    }

    // Set high quality scaling
    graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics->SetSmoothingMode(SmoothingModeHighQuality);
    graphics->SetPixelOffsetMode(PixelOffsetModeHighQuality);

    // Calculate scaling to fit within square while maintaining aspect ratio
    float scale = min((float)targetSize / width, (float)targetSize / height);
    UINT scaledWidth = (UINT)(width * scale);
    UINT scaledHeight = (UINT)(height * scale);
    UINT offsetX = (targetSize - scaledWidth) / 2;
    UINT offsetY = (targetSize - scaledHeight) / 2;

    // Clear with transparent background
    graphics->Clear(Color(0, 0, 0, 0));

    // Draw the scaled image
    Rect destRect(offsetX, offsetY, scaledWidth, scaledHeight);
    graphics->DrawImage(pngBitmap, destRect);

    // Get HICON from bitmap
    HICON hIcon = NULL;
    iconBitmap->GetHICON(&hIcon);

    // Clean up GDI+ objects
    delete graphics;
    delete iconBitmap;
    delete pngBitmap;

    if (!hIcon) {
        std::wcerr << L"Error: Failed to create icon from PNG\n";
        return false;
    }

    // Save as ICO file
    HANDLE hFile = CreateFileW(icoPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error: Failed to create ICO file: " << icoPath << L"\n";
        DestroyIcon(hIcon);
        return false;
    }

    // Write ICO file header
    ICONDIR iconDir;
    iconDir.idReserved = 0;
    iconDir.idType = 1;
    iconDir.idCount = 1;

    DWORD written;
    WriteFile(hFile, &iconDir, sizeof(ICONDIR), &written, NULL);

    // Get icon info
    ICONINFO iconInfo;
    GetIconInfo(hIcon, &iconInfo);

    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

    // Write ICONDIRENTRY
    ICONDIRENTRY iconEntry;
    iconEntry.bWidth = (BYTE)targetSize;
    iconEntry.bHeight = (BYTE)targetSize;
    iconEntry.bColorCount = 0;
    iconEntry.bReserved = 0;
    iconEntry.wPlanes = 1;
    iconEntry.wBitCount = 32;
    iconEntry.dwBytesInRes = 0; // Will calculate
    iconEntry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);

    // Calculate image data size
    DWORD imageSize = sizeof(BITMAPINFOHEADER) + (targetSize * targetSize * 4);
    iconEntry.dwBytesInRes = imageSize;

    WriteFile(hFile, &iconEntry, sizeof(ICONDIRENTRY), &written, NULL);

    // Write BITMAPINFOHEADER
    BITMAPINFOHEADER bmih;
    ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = targetSize;
    bmih.biHeight = targetSize * 2; // Double height for mask
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    WriteFile(hFile, &bmih, sizeof(BITMAPINFOHEADER), &written, NULL);

    // Get bitmap bits
    HDC hdc = GetDC(NULL);
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = targetSize;
    bmi.bmiHeader.biHeight = -((LONG)targetSize); // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE* bits = new BYTE[targetSize * targetSize * 4];
    GetDIBits(hdc, iconInfo.hbmColor, 0, targetSize, bits, &bmi, DIB_RGB_COLORS);

    // Write color data (bottom-up for ICO format)
    for (int y = targetSize - 1; y >= 0; y--) {
        WriteFile(hFile, bits + (y * targetSize * 4), targetSize * 4, &written, NULL);
    }

    delete[] bits;
    ReleaseDC(NULL, hdc);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    CloseHandle(hFile);
    DestroyIcon(hIcon);

    return true;
}

HICON IconHelper::LoadIconFromFile(const std::wstring& path) {
    if (path.empty()) {
        return nullptr;
    }

    // Convert to absolute path
    wchar_t absolutePath[MAX_PATH];
    DWORD result = GetFullPathNameW(path.c_str(), MAX_PATH, absolutePath, nullptr);
    if (result == 0) {
        std::wcerr << L"Error: Failed to get absolute path for: " << path << L"\n";
        return nullptr;
    }

    std::wstring absPath(absolutePath);

    // Validate file exists
    if (GetFileAttributesW(absPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Error: Icon file does not exist: " << absPath << L"\n";
        return nullptr;
    }

    // Check if it's a PNG file
    if (IsPngFile(absPath)) {
        // Create persistent ICO file with unique name based on source PNG
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        
        // Generate unique filename based on absolute PNG path hash
        size_t pathHash = std::hash<std::wstring>{}(absPath);
        wchar_t hashStr[32];
        swprintf_s(hashStr, L"%zu", pathHash);
        std::wstring icoPath = std::wstring(tempPath) + L"webwrap_icon_" + hashStr + L".ico";

        // Convert PNG to ICO (only if not already converted)
        if (GetFileAttributesW(icoPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            if (!ConvertPngToIco(absPath, icoPath)) {
                std::wcerr << L"Error: Failed to convert PNG to ICO format\n";
                return nullptr;
            }
        }

        // Load the converted ICO with specific size for better quality
        HICON hIcon = (HICON)LoadImageW(
            nullptr,
            icoPath.c_str(),
            IMAGE_ICON,
            32,  // Width - standard icon size
            32,  // Height - standard icon size
            LR_LOADFROMFILE);

        if (!hIcon) {
            DWORD error = GetLastError();
            std::wcerr << L"Error: Failed to load converted icon. Error code: " << error << L"\n";
            std::wcerr << L"ICO path: " << icoPath << L"\n";
            return nullptr;
        }

        return hIcon;
    }
    else if (IsIcoFile(absPath)) {
        // Load ICO file directly with specific size using absolute path
        HICON hIcon = (HICON)LoadImageW(
            nullptr,
            absPath.c_str(),
            IMAGE_ICON,
            32,  // Width - standard icon size
            32,  // Height - standard icon size
            LR_LOADFROMFILE);

        if (!hIcon) {
            DWORD error = GetLastError();
            std::wcerr << L"Error: Failed to load icon from file: " << absPath 
                       << L". Error code: " << error << L"\n";
            return nullptr;
        }

        return hIcon;
    }
    else {
        std::wcerr << L"Error: Unsupported icon file format. Please use .ico or .png files.\n";
        return nullptr;
    }
}

std::wstring IconHelper::GetConvertedIconPath(const std::wstring& path) {
    if (path.empty()) {
        return L"";
    }

    // Convert to absolute path
    wchar_t absolutePath[MAX_PATH];
    DWORD result = GetFullPathNameW(path.c_str(), MAX_PATH, absolutePath, nullptr);
    if (result == 0) {
        return path;  // Fallback to original path
    }

    std::wstring absPath(absolutePath);

    // If it's already an ICO file, return the absolute path
    if (IsIcoFile(absPath)) {
        return absPath;
    }

    // If it's a PNG file, return the converted ICO path
    if (IsPngFile(absPath)) {
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        
        // Generate unique filename based on absolute PNG path hash
        size_t pathHash = std::hash<std::wstring>{}(absPath);
        wchar_t hashStr[32];
        swprintf_s(hashStr, L"%zu", pathHash);
        std::wstring icoPath = std::wstring(tempPath) + L"webwrap_icon_" + hashStr + L".ico";
        
        // Convert if not already done
        if (GetFileAttributesW(icoPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            ConvertPngToIco(absPath, icoPath);
        }
        
        return icoPath;
    }

    return absPath;
}