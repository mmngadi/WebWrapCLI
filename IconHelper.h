#pragma once
#include <windows.h>
#include <string>

// ICO file format structures
#pragma pack(push, 1)
typedef struct {
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONDIRENTRY;
#pragma pack(pop)

class IconHelper {
public:
    static HICON LoadIconFromFile(const std::wstring& path);
    static bool ConvertPngToIco(const std::wstring& pngPath, const std::wstring& icoPath);
    static bool IsPngFile(const std::wstring& path);
    static bool IsIcoFile(const std::wstring& path);
    static std::wstring GetConvertedIconPath(const std::wstring& path);
};
