#pragma once
#include <windows.h>
#include <string>
#include <wrl.h>
#include <WebView2.h>

class WebViewWindow {
public:
    WebViewWindow(const std::wstring& title,
        const std::wstring& iconPath,
        const std::wstring& url);
    
    ~WebViewWindow();

    void RunMessageLoop();

private:
    HWND m_hWnd = nullptr;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webview;
    std::wstring m_title;
    std::wstring m_iconPath;
    std::wstring m_url;
    std::wstring m_className;
    bool m_webviewInitialized = false;
    bool m_isLoading = true;
    HICON m_hIconLarge = nullptr;
    HICON m_hIconSmall = nullptr;

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void InitWebView();
    void DrawLoadingScreen(HDC hdc, const RECT& rect);
    void OnNavigationCompleted();
};
