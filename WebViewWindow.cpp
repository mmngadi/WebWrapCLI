#include "WebViewWindow.h"
#include "IconHelper.h"
#include <wrl.h>
#include <wrl/event.h>
#include <iostream>

#define WM_LOADING_TIMER 1

WebViewWindow::WebViewWindow(const std::wstring& title,
    const std::wstring& iconPath,
    const std::wstring& url)
    : m_title(title), m_iconPath(iconPath), m_url(url)
{
    // Generate unique window class name to avoid conflicts
    static int instanceCounter = 0;
    wchar_t className[256];
    swprintf_s(className, L"WebWrapWindowClass_%d_%p", instanceCounter++, this);
    m_className = className;
    
    // Load icon BEFORE creating window if provided
    if (!m_iconPath.empty()) {
        // Convert to absolute path first
        wchar_t absolutePath[MAX_PATH];
        DWORD result = GetFullPathNameW(m_iconPath.c_str(), MAX_PATH, absolutePath, nullptr);
        std::wstring absIconPath = (result > 0) ? std::wstring(absolutePath) : m_iconPath;
        
        std::wcout << L"Loading icon from: " << absIconPath << L"\n";
        
        // Validate icon file exists
        if (GetFileAttributesW(absIconPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            // Get the converted icon path (handles PNG to ICO conversion)
            std::wstring iconPath = IconHelper::GetConvertedIconPath(absIconPath);
            
            std::wcout << L"Converted icon path: " << iconPath << L"\n";
            
            if (!iconPath.empty() && GetFileAttributesW(iconPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                // Load large icon (32x32)
                m_hIconLarge = (HICON)LoadImageW(
                    nullptr,
                    iconPath.c_str(),
                    IMAGE_ICON,
                    32,
                    32,
                    LR_LOADFROMFILE);
                
                // Load small icon (16x16 for title bar)
                m_hIconSmall = (HICON)LoadImageW(
                    nullptr,
                    iconPath.c_str(),
                    IMAGE_ICON,
                    16,
                    16,
                    LR_LOADFROMFILE);
                
                if (m_hIconLarge && m_hIconSmall) {
                    std::wcout << L"✓ Icons loaded successfully (Large: " << m_hIconLarge 
                               << L", Small: " << m_hIconSmall << L")\n";
                } else {
                    std::wcerr << L"✗ Failed to load icons from file: " << absIconPath << L"\n";
                    DWORD error = GetLastError();
                    std::wcerr << L"  Error code: " << error << L"\n";
                    std::wcerr << L"  Icon path: " << iconPath << L"\n";
                    if (!m_hIconLarge) std::wcerr << L"  Large icon failed\n";
                    if (!m_hIconSmall) std::wcerr << L"  Small icon failed\n";
                }
            } else {
                std::wcerr << L"✗ Failed to get converted icon path or file doesn't exist\n";
            }
        } else {
            std::wcerr << L"✗ Icon file not found: " << absIconPath << L"\n";
        }
    }

    // Register window class with icon using WNDCLASSEXW for small icon support
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = m_className.c_str();
    wc.hIcon = m_hIconLarge;        // Set large class icon
    wc.hIconSm = m_hIconSmall;      // Set small class icon (16x16 for title bar)
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassExW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::wcerr << L"✗ Failed to register window class. Error: " << error << L"\n";
        }
    } else {
        std::wcout << L"✓ Window class registered: " << m_className << L"\n";
    }

    // Create the native window
    m_hWnd = CreateWindowExW(
        0, m_className.c_str(), m_title.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 768, nullptr, nullptr, GetModuleHandle(nullptr), this);

    if (!m_hWnd) {
        std::wcerr << L"Error: Failed to create window.\n";
        return;
    }

    // Set icons on the window instance as well (belt and suspenders approach)
    if (m_hIconLarge && m_hIconSmall) {
        std::wcout << L"Setting window icons via WM_SETICON...\n";
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIconLarge);
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSmall);
        std::wcout << L"✓ Window icons set\n";
    } else {
        std::wcout << L"No custom icons to set\n";
    }

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
    InitWebView();
}

WebViewWindow::~WebViewWindow() {
    // Clean up WebView2 resources in proper order
    if (m_webview) {
        m_webview.Reset();
    }
    if (m_controller) {
        m_controller->Close();
        m_controller.Reset();
    }
    
    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
    
    // Unregister window class
    if (!m_className.empty()) {
        UnregisterClassW(m_className.c_str(), GetModuleHandle(nullptr));
    }
    
    // Clean up icon resources
    if (m_hIconLarge) {
        DestroyIcon(m_hIconLarge);
        m_hIconLarge = nullptr;
    }
    if (m_hIconSmall) {
        DestroyIcon(m_hIconSmall);
        m_hIconSmall = nullptr;
    }
}

void WebViewWindow::InitWebView() {
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result)) {
                    std::wcerr << L"Error: Failed to create WebView2 environment. HRESULT: 0x" 
                               << std::hex << result << std::dec << L"\n";
                    std::wcerr << L"Make sure WebView2 Runtime is installed.\n";
                    PostQuitMessage(-1);
                    return result;
                }

                if (!env) {
                    std::wcerr << L"Error: WebView2 environment is null.\n";
                    PostQuitMessage(-1);
                    return E_FAIL;
                }

                HRESULT hr = env->CreateCoreWebView2Controller(
                    m_hWnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result)) {
                                std::wcerr << L"Error: Failed to create WebView2 controller. HRESULT: 0x" 
                                           << std::hex << result << std::dec << L"\n";
                                PostQuitMessage(-1);
                                return result;
                            }

                            if (!controller) {
                                std::wcerr << L"Error: WebView2 controller is null.\n";
                                PostQuitMessage(-1);
                                return E_FAIL;
                            }

                            m_controller = controller;
                            HRESULT hr = m_controller->get_CoreWebView2(&m_webview);
                            
                            if (FAILED(hr) || !m_webview) {
                                std::wcerr << L"Error: Failed to get CoreWebView2 interface. HRESULT: 0x" 
                                           << std::hex << hr << std::dec << L"\n";
                                PostQuitMessage(-1);
                                return hr;
                            }

                            RECT bounds;
                            GetClientRect(m_hWnd, &bounds);
                            m_controller->put_Bounds(bounds);

                            // Hide the WebView2 initially while loading
                            m_controller->put_IsVisible(FALSE);

                            // Add NavigationCompleted event handler
                            EventRegistrationToken token;
                            m_webview->add_NavigationCompleted(
                                Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                        OnNavigationCompleted();
                                        return S_OK;
                                    }).Get(), &token);

                            // Navigate to the URL
                            hr = m_webview->Navigate(m_url.c_str());
                            if (FAILED(hr)) {
                                std::wcerr << L"Error: Failed to navigate to URL: " << m_url 
                                           << L". HRESULT: 0x" << std::hex << hr << std::dec << L"\n";
                                // Show error and stop loading
                                m_isLoading = false;
                                InvalidateRect(m_hWnd, nullptr, TRUE);
                            } else {
                                m_webviewInitialized = true;
                                std::wcout << L"Successfully navigating to: " << m_url << L"\n";
                            }

                            return S_OK;
                        }).Get());
                
                if (FAILED(hr)) {
                    std::wcerr << L"Error: Failed to initiate controller creation. HRESULT: 0x" 
                               << std::hex << hr << std::dec << L"\n";
                    PostQuitMessage(-1);
                }

                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        std::wcerr << L"Error: Failed to initiate WebView2 environment creation. HRESULT: 0x" 
                   << std::hex << hr << std::dec << L"\n";
        std::wcerr << L"Please ensure WebView2 Runtime is installed.\n";
        PostQuitMessage(-1);
    }
}

void WebViewWindow::OnNavigationCompleted() {
    std::wcout << L"Navigation completed. Showing content...\n";
    
    // Stop loading state
    m_isLoading = false;
    
    // Show the WebView2 control
    if (m_controller) {
        m_controller->put_IsVisible(TRUE);
    }
    
    // Redraw the window to remove loading screen
    InvalidateRect(m_hWnd, nullptr, TRUE);
}

void WebViewWindow::DrawLoadingScreen(HDC hdc, const RECT& rect) {
    // Calculate center of window
    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;
    
    // Fill background with white
    HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(hdc, &rect, whiteBrush);
    
    // Draw loading text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 100, 100));
    
    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    std::wstring loadingText = L"Loading...";
    
    RECT textRect = rect;
    textRect.top = centerY - 10;
    DrawTextW(hdc, loadingText.c_str(), -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void WebViewWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WebViewWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WebViewWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<WebViewWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
    }
    else {
        self = reinterpret_cast<WebViewWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (self) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            if (self->m_isLoading) {
                // Draw loading screen
                RECT rect;
                GetClientRect(hWnd, &rect);
                self->DrawLoadingScreen(hdc, rect);
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            // Prevent flicker during loading animation
            if (self->m_isLoading) {
                return 1;
            }
            break;
            
        case WM_SIZE: {
            if (self->m_controller) {
                RECT bounds;
                GetClientRect(hWnd, &bounds);
                self->m_controller->put_Bounds(bounds);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}