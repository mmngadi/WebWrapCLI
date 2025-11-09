#pragma once
#include <string>

class ShortcutHelper {
public:
    static void CreateShortcut(const std::wstring& name,
        const std::wstring& iconPath,
        const std::wstring& targetUrl);
};
