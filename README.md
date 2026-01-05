# Wine Application Manager

A comprehensive Wine application manager for Linux with both CLI and GUI interfaces.

## Features

- **Dual Interface**: Command-line (C++) and graphical (PyQt6) interfaces
- **Wine Prefix Management**: Create, delete, switch between multiple Wine prefixes
- **Process Monitoring**: Real-time monitoring of Wine processes
- **Configuration Manager**: Advanced Wine configuration options
- **Winetricks Integration**: Easy installation of Windows components
- **Application Shortcuts**: Quick access to frequently used applications
- **Logging System**: Comprehensive logging and debugging capabilities
- **Registry Management**: Wine registry editing and management
- **Built-in Tools**: Access to Wine tools (winecfg, regedit, etc.)

## Requirements

### Build Dependencies
- g++ (C++17 support)
- make or cmake
- pthread

### Runtime Dependencies
- Wine (any recent version)
- Python 3.6+
- PyQt6 (for GUI)

## Installation

### Quick Install

```bash
# Clone or extract the source
cd wine-application-manager

# Make build script executable
chmod +x build.sh

# Build everything
./build.sh

# Install system-wide (requires sudo)
sudo ./build.sh install
```

### Manual Build

Using Make:
```bash
make all
make gui
sudo make install
```

Using CMake:
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

## Usage

### GUI Application

Launch the graphical interface:
```bash
wine-gui
```

Features:
- Browse and run Windows executables
- Manage Wine prefixes
- Configure Wine settings
- Monitor running processes
- Install components via Winetricks
- View logs in real-time

### CLI Application

Command-line interface:
```bash
wine-cli [OPTIONS] COMMAND [ARGS...]
```

#### Common Commands

Run an executable:
```bash
wine-cli run /path/to/program.exe
```

Execute and wait:
```bash
wine-cli exec /path/to/installer.exe
```

Manage prefixes:
```bash
wine-cli prefix-create gaming
wine-cli prefix-list
wine-cli prefix-switch gaming
```

Install components:
```bash
wine-cli install d3dx9
wine-cli list-components
```

Manage shortcuts:
```bash
wine-cli shortcut-add notepad /path/to/notepad.exe
wine-cli shortcut-list
wine-cli shortcut-run notepad
```

View processes:
```bash
wine-cli list-processes
wine-cli kill <PID>
```

Show information:
```bash
wine-cli info
wine-cli version
wine-cli logs 50
```

#### CLI Options

- `-h, --help` - Show help message
- `-v, --verbose` - Enable verbose output
- `-q, --quiet` - Suppress output
- `-c, --config DIR` - Set configuration directory
- `-p, --prefix PATH` - Set Wine prefix path
- `-a, --arch ARCH` - Set architecture (win32/win64/auto)

## Configuration

Configuration files are stored in `~/.config/wineapp/`:
- `wine.conf` - Main Wine configuration
- `shortcuts.conf` - Application shortcuts
- `logs/wineapp.log` - Application logs

Wine prefixes are stored in `~/.local/share/wineprefixes/`

## Architecture

### C++ Components

- **wine_wrapper.hpp/cpp** - Core Wine wrapper library
- **wine_executor.cpp** - Wine process execution
- **wine_wrapper_impl.cpp** - Prefix and registry management
- **wine_utils.cpp** - Utility functions
- **wine_app_manager.cpp** - Application manager
- **wine_cli.cpp** - Command-line interface

### Python Components

- **wine_gui.py** - GUI core classes
- **wine_gui_main.py** - GUI tab widgets
- **wine_gui_window.py** - Main window implementation

## Development

### Building for Development

```bash
# Build with debug symbols
make clean
CXXFLAGS="-g -O0" make all

# Run tests
./build.sh test

# Count lines of code
./build.sh count
```

### Project Structure

```
.
├── wine_wrapper.hpp          # Main header file
├── wine_wrapper.cpp          # Configuration and logging
├── wine_wrapper_impl.cpp     # Prefix manager and monitor
├── wine_executor.cpp         # Wine executor
├── wine_utils.cpp            # Utility functions
├── wine_app_manager.cpp      # Application manager
├── wine_cli.cpp              # CLI application
├── wine_gui.py               # GUI core
├── wine_gui_main.py          # GUI tabs
├── wine_gui_window.py        # Main window
├── Makefile                  # Make build system
├── CMakeLists.txt            # CMake build system
├── build.sh                  # Build script
└── README.md                 # This file
```

## Uninstallation

```bash
sudo ./build.sh uninstall
```

Or with make:
```bash
sudo make uninstall
```

## License

This project is open source.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Support

For issues and questions, please check the documentation or open an issue.
