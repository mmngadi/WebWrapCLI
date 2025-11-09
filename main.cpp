#include <iostream>
#include <string>
#include "WebViewWindow.h"
#include "ShortcutHelper.h"
#include <algorithm>

// Simple struct to hold CLI options
struct Options {
    std::wstring target;
    std::wstring name;
    std::wstring icon;
    bool createShortcut = false;
    bool debugMode = false;
};

// Convert std::string to std::wstring
std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Print usage information
void printUsage() {
    std::wcout << L"WebWrapCLI - Wrap web applications as native Windows apps\n\n";
    std::wcout << L"Usage: ww.exe --target <url> [options]\n\n";
    std::wcout << L"Required Arguments:\n";
    std::wcout << L"  --target <url>    URL or local HTML file to display\n";
    std::wcout << L"                    - Web URLs: http:// or https://\n";
    std::wcout << L"                    - Local files: file:///C:/path/to/file.html\n\n";
    std::wcout << L"Optional Arguments:\n";
    std::wcout << L"  --name <name>     Window title and shortcut name (default: \"Web App\")\n";
    std::wcout << L"  --icon <path>     Path to icon file (.ico or .png)\n";
    std::wcout << L"  -s                Create desktop shortcut only (don't launch window)\n";
    std::wcout << L"  --debug           Show console window for debugging\n";
    std::wcout << L"  --help            Show this help message\n\n";
    std::wcout << L"Example:\n";
    std::wcout << L"  ww.exe --target https://example.com --name \"My App\" --icon app.ico -s\n";
    std::wcout << L"  ww.exe --target file:///C:/dev/myapp/index.html --name \"Local App\"\n";
    std::wcout << L"  ww.exe --target https://github.com --icon github.png\n";
}

// Parse CLI arguments
Options parseArgs(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h" || arg == "/?") {
            printUsage();
            exit(0);
        }
        else if (arg == "--target" && i + 1 < argc) {
            opts.target = stringToWString(argv[++i]);
        }
        else if (arg == "--name" && i + 1 < argc) {
            opts.name = stringToWString(argv[++i]);
        }
        else if (arg == "--icon" && i + 1 < argc) {
            opts.icon = stringToWString(argv[++i]);
        }
        else if (arg == "-s") {
            opts.createShortcut = true;
        }
        else if (arg == "--debug") {
            opts.debugMode = true;
        }
        else {
            std::wcerr << L"Warning: Unknown argument or missing value: " 
                       << stringToWString(arg) << L"\n";
        }
    }
    return opts;
}

// Validate URL format (basic check)
bool isValidUrl(const std::wstring& url) {
    if (url.empty()) return false;
    
    // Basic validation - check if it starts with http:// or https://
    if (url.find(L"http://") == 0 || url.find(L"https://") == 0) {
        return true;
    }
    
    // Allow file:// URLs (file:///C:/path/to/file.html)
    if (url.find(L"file://") == 0) {
        return true;
    }
    
    return false;
}

// Validate and normalize file path for file:// URLs
bool validateFilePath(const std::wstring& url) {
    if (url.find(L"file://") != 0) {
        return true; // Not a file URL, other validation applies
    }
    
    // Extract the file path from file:// URL
    std::wstring filePath = url.substr(7); // Skip "file://"
    
    // Remove leading slash if it's a Windows path (file:///C:/...)
    if (filePath.length() > 2 && filePath[0] == L'/' && filePath[2] == L':') {
        filePath = filePath.substr(1);
    }
    
    // Check if file exists
    if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Error: Local file not found: " << filePath << L"\n";
        return false;
    }
    
    return true;
}

// Validate icon file format
bool isValidIconFile(const std::wstring& iconPath) {
    if (iconPath.empty()) return true;
    
    // Get file extension
    size_t dotPos = iconPath.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        std::wcerr << L"Error: Icon file has no extension\n";
        return false;
    }
    
    std::wstring ext = iconPath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    
    if (ext != L".ico" && ext != L".png") {
        std::wcerr << L"Error: Icon must be .ico or .png format\n";
        std::wcerr << L"Provided: " << iconPath << L"\n";
        return false;
    }
    
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Get command line arguments
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // Convert wide char arguments to char for parseArgs
    char** argvA = new char*[argc];
    for (int i = 0; i < argc; i++) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
        argvA[i] = new char[size];
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argvA[i], size, NULL, NULL);
    }
    
    // Parse arguments first to check for debug flag
    Options opts = parseArgs(argc, argvA);
    
    // Create/show console if debug mode is enabled
    if (opts.debugMode) {
        AllocConsole();
        FILE* pFile;
        freopen_s(&pFile, "CONOUT$", "w", stdout);
        freopen_s(&pFile, "CONOUT$", "w", stderr);
        freopen_s(&pFile, "CONIN$", "r", stdin);
        std::wcout.clear();
        std::wcerr.clear();
        std::wcin.clear();
    }
    
    // Initialize COM for shell operations
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Show help if no arguments provided
    if (argc < 2) {
        printUsage();
        
        // Clean up
        for (int i = 0; i < argc; i++) delete[] argvA[i];
        delete[] argvA;
        LocalFree(argv);
        return -1;
    }

    // Validate required arguments
    if (opts.target.empty()) {
        std::wcerr << L"Error: --target [url] is required.\n\n";
        printUsage();
        
        // Clean up
        for (int i = 0; i < argc; i++) delete[] argvA[i];
        delete[] argvA;
        LocalFree(argv);
        return -1;
    }

    // Validate URL format
    if (!isValidUrl(opts.target)) {
        std::wcerr << L"Error: Invalid URL format. URL must start with http://, https://, or file://\n";
        std::wcerr << L"Provided URL: " << opts.target << L"\n";
        std::wcerr << L"\nExamples:\n";
        std::wcerr << L"  https://example.com\n";
        std::wcerr << L"  http://localhost:3000\n";
        std::wcerr << L"  file:///C:/path/to/file.html\n";
        
        // Clean up
        for (int i = 0; i < argc; i++) delete[] argvA[i];
        delete[] argvA;
        LocalFree(argv);
        return -1;
    }
    
    // Validate file path if it's a file:// URL
    if (!validateFilePath(opts.target)) {
        // Clean up
        for (int i = 0; i < argc; i++) delete[] argvA[i];
        delete[] argvA;
        LocalFree(argv);
        return -1;
    }

    // Set default name if not provided
    if (opts.name.empty()) {
        opts.name = L"Web App";
    }

    // Validate icon path and format if provided
    if (!opts.icon.empty()) {
        // Check format
        if (!isValidIconFile(opts.icon)) {
            std::wcerr << L"Continuing without custom icon...\n";
            opts.icon.clear();
        }
        // Check file exists
        else if (GetFileAttributesW(opts.icon.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::wcerr << L"Warning: Icon file not found: " << opts.icon << L"\n";
            std::wcerr << L"Continuing without custom icon...\n";
            opts.icon.clear();
        }
    }

    // If -s flag is provided, only create shortcut without launching window
    if (opts.createShortcut) {
        std::wcout << L"Creating desktop shortcut...\n";
        std::wcout << L"Name: " << opts.name << L"\n";
        std::wcout << L"Target: " << opts.target << L"\n";
        if (!opts.icon.empty()) {
            std::wcout << L"Icon: " << opts.icon << L"\n";
        }
        
        ShortcutHelper::CreateShortcut(opts.name, opts.icon, opts.target);
        
        std::wcout << L"Shortcut created. Exiting without launching window.\n";
    }
    else {
        // Launch the window
        std::wcout << L"Initializing WebView2 window...\n";
        std::wcout << L"Title: " << opts.name << L"\n";
        std::wcout << L"Target URL: " << opts.target << L"\n";
        if (!opts.icon.empty()) {
            std::wcout << L"Icon: " << opts.icon << L"\n";
        }

        // Pass the URL into the WebViewWindow constructor
        WebViewWindow window(opts.name, opts.icon, opts.target);

        // Run the message loop (navigation happens inside async callback in WebViewWindow)
        window.RunMessageLoop();
    }
    
    // Cleanup COM
    CoUninitialize();
    
    // Clean up command line arguments
    for (int i = 0; i < argc; i++) delete[] argvA[i];
    delete[] argvA;
    LocalFree(argv);
    
    return 0;
}