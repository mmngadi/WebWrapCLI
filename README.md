# WebWrapCLI

A Windows desktop application that wraps web URLs into native Windows applications using Microsoft Edge WebView2.

## Overview

WebWrapCLI allows you to turn any web application into a standalone Windows desktop application with custom branding. It creates a native window that hosts a WebView2 control, providing a seamless desktop experience for web-based applications.

## Features

- **Web-to-Desktop Wrapping**: Display any web URL or local HTML file in a native Windows window
- **Custom Branding**: Set custom window titles and application icons (.ico or .png)
- **Desktop Shortcuts**: Create desktop shortcuts to your web apps
- **WebView2 Integration**: Uses Microsoft Edge WebView2 for modern web standards support
- **CLI Interface**: Simple command-line interface for easy automation
- **Loading Screen**: Minimalist loading screen with "Loading..." text while content loads for seamless UX
- **PNG Icon Support**: Automatically converts PNG images to ICO format for icons
- **Local File Support**: Open local HTML files using file:// protocol

## Requirements

- **Operating System**: Windows 10 or later
- **WebView2 Runtime**: Microsoft Edge WebView2 Runtime ([Download](https://developer.microsoft.com/en-us/microsoft-edge/webview2/))
- **Build Tools**: Visual Studio 2022 with C++ Desktop Development workload
- **Windows SDK**: Windows 10 SDK or later

## Usage

### Command-Line Syntax

```cmd
ww.exe --target <url> [options]
```

### Required Arguments

- `--target <url>` - The URL or local HTML file to display
  - Web URLs: `http://` or `https://`
  - Local files: `file:///C:/path/to/file.html`

### Optional Arguments

- `--name <name>` - Window title and shortcut name (default: "Web App")
- `--icon <path>` - Path to custom icon file (.ico or .png format)
- `-s` - Create desktop shortcut only (does not launch the window)
- `--debug` - Show console window for debugging output
- `--help` - Display help information

### Examples

#### Basic Usage
```cmd
ww.exe --target https://example.com
```

#### With Custom Name and Icon (ICO)
```cmd
ww.exe --target https://github.com --name "GitHub" --icon github.ico
```

#### With PNG Icon (Automatically Converted)
```cmd
ww.exe --target https://example.com --name "Example" --icon logo.png
```

#### Create Desktop Shortcut Only (No Window Launch)
```cmd
ww.exe --target https://mail.google.com --name "Gmail" --icon gmail.ico -s
```

#### Open Local HTML File
```cmd
ww.exe --target file:///C:/projects/myapp/index.html --name "My Local App"
```

#### Launch Window and Create Shortcut
```cmd
# First create the shortcut
ww.exe --target https://example.com --name "Example" --icon app.png -s

# Then launch the window separately
ww.exe --target https://example.com --name "Example" --icon app.png
```

#### Debug Mode (Show Console)
```cmd
# Show console window to see debug output and error messages
ww.exe --target https://example.com --name "Example" --debug
```

## Building the Project

### Prerequisites

1. Install Visual Studio 2022 with "Desktop development with C++" workload
2. Ensure NuGet Package Manager is installed

### Build Steps

#### Using Visual Studio

1. Open `WebWrapCLI.sln` in Visual Studio
2. Right-click the solution and select "Restore NuGet Packages"
3. Select your desired configuration (Debug/Release) and platform (x64 recommended)
4. Build the solution (Ctrl+Shift+B)

#### Using MSBuild (Command Line)

```cmd
# Navigate to project directory
cd WebWrapCLI

# Restore NuGet packages
nuget restore WebWrapCLI.vcxproj -PackagesDirectory ..\packages

# Build for x64 Release
msbuild WebWrapCLI.vcxproj /p:Configuration=Release /p:Platform=x64
```

### NuGet Dependencies

The project uses the following NuGet packages (automatically restored):

- **Microsoft.Web.WebView2** (v1.0.3595.46) - WebView2 SDK
- **Microsoft.Windows.CppWinRT** (v2.0.250303.1) - C++/WinRT support
- **Microsoft.Windows.ImplementationLibrary** (v1.0.250325.1) - WIL helpers

## Code Fixes Applied

This version includes several important fixes and improvements:

### 1. Fixed ANSI/Unicode String Issues
- **Problem**: Mixing wide-character strings with ANSI functions in `ShortcutHelper.cpp`
- **Solution**: Explicitly use wide-character Windows API functions (`GetModuleFileNameW`, `IShellLinkW`, etc.)

### 2. Fixed COM Initialization
- **Problem**: Multiple COM initialization calls causing conflicts
- **Solution**: Single `CoInitializeEx` in main, removed redundant calls in `ShortcutHelper`

### 3. Fixed Hardcoded Paths
- **Problem**: Desktop path was hardcoded as `C:\Users\Public\Desktop\`
- **Solution**: Use `SHGetFolderPathW` API to get proper desktop path for current user

### 4. Added Comprehensive Error Handling
- Validation of all file paths before use
- HRESULT checking with error messages for all COM/WebView2 operations
- User-friendly error messages with diagnostic codes
- URL format validation

### 5. Improved Resource Management
- Added destructor to `WebViewWindow` for proper cleanup
- Proper COM resource release order
- Icon validation before loading

### 6. Enhanced User Experience
- Help command (`--help`) with usage information
- Default window name if not specified
- Informative console output during operation
- Warning messages for non-critical issues

### 7. Fixed String Conversions
- **Problem**: Using `winrt::hstring` incompatible with `std::wstring`
- **Solution**: Implemented proper UTF-8 to wide-string conversion using Windows API

### 8. WebView2 Include Path
- **Note**: IDE may show errors for `WebView2.h` not found, but MSBuild will resolve this correctly through NuGet package .targets files
- The include path is automatically added during build by the Microsoft.Web.WebView2 NuGet package

## Project Structure

```
WebWrapCLI/
├── main.cpp                 - Entry point and CLI argument parsing
├── WebViewWindow.h/cpp      - WebView2 window implementation
├── ShortcutHelper.h/cpp     - Desktop shortcut creation
├── IconHelper.h/cpp         - Icon loading utilities
├── WebWrapCLI.vcxproj      - Visual Studio project file
├── packages.config          - NuGet package configuration
└── README.md               - This file
```

## Icon Format Support

### Supported Formats

- **.ico files**: Loaded directly
- **.png files**: Automatically converted to .ico format with scaling to fit

### PNG Conversion Details

- PNG images are automatically scaled to fit standard icon sizes (16x16 to 256x256)
- Aspect ratio is preserved during scaling
- Transparent backgrounds are maintained
- High-quality bicubic interpolation is used for smooth scaling
- Temporary .ico file is created in the system temp directory

### Best Practices

- Use square images for best results (e.g., 256x256, 512x512)
- PNG format is recommended for source images
- Transparent backgrounds work well
- Higher resolution source images produce better results

## Known Issues

- **IntelliSense Errors**: Visual Studio IntelliSense may show errors for `WebView2.h` include, but the project will compile successfully with MSBuild as the NuGet package provides the correct include paths at build time.

## Troubleshooting

### "WebView2 Runtime not found" Error
- Download and install the [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)

### "Failed to create WebView2 environment" Error
- Ensure WebView2 Runtime is installed
- Check that the application has proper permissions
- Try running as administrator

### Shortcut Creation Fails
- Verify you have write permissions to the Desktop folder
- Check that the icon file path is valid and accessible
- Ensure the target URL is properly formatted

### Can't See Error Messages

**Solution:**
- Use the `--debug` flag to show the console window
```cmd
ww.exe --target URL --name "App" --debug
```

### Icon Not Displaying
- Verify the icon file is in .ico or .png format
- Check the file path is correct and accessible
- Ensure the icon file is not corrupted
- For PNG files, ensure GDI+ can load the image

### Local File Not Loading

**Solutions:**
1. Verify the file path is correct
2. Use proper file:// URL format: `file:///C:/path/to/file.html`
3. Ensure the HTML file exists at the specified location
4. Check that linked resources (CSS, JS, images) use relative paths or are accessible

## License

This project is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! Areas for improvement:

- Support for additional command-line options (window size, position, etc.)
- Configuration file support
- System tray integration
- Multiple window instances
- Custom user agent strings
- JavaScript injection support
- Persistent debug logging to file

## Version History

### v1.2 (Current)
- **PNG icon support**: Automatically converts PNG images to ICO format
- **Local file support**: Open local HTML files with file:// URLs
- **Changed -s flag behavior**: Now creates shortcut only without launching window
- **Debug mode**: Added --debug flag to show console window for debugging
- Console window hidden by default for cleaner UX
- Icon validation for .ico and .png formats
- Enhanced URL validation with file path checking
- Improved error messages and user guidance

### v1.1
- Added loading screen with "Loading..." text
- Seamless transition from loading to content
- NavigationCompleted event handling
- Improved user experience during page load
- Minimalist, text-only static display
- Ultra-lightweight implementation

### v1.0
- Initial release with core functionality
- WebView2 integration
- Desktop shortcut creation
- Custom icon and title support
- Comprehensive error handling