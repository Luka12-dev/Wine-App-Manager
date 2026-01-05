# Installing Dependencies for Wine Application Manager GUI

## The Problem

The GUI requires Qt platform plugins that are missing. You're seeing this error:
```
qt.qpa.plugin: Could not load the Qt platform plugin "xcb"
```

## Solution - Install Missing Dependencies

Run these commands to install everything needed:

### Option 1: Quick Install (Recommended)

```bash
# Install Qt dependencies
sudo apt install libxcb-cursor0 libxcb-xinerama0

# Install Python Tkinter (fallback GUI)
sudo apt install python3-tk

# Optional: Install Winetricks
sudo apt install winetricks
```

### Option 2: Complete Install (All dependencies)

```bash
sudo apt install -y \
    python3 \
    python3-pip \
    python3-tk \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    libxcb-shape0 \
    wine \
    wine64 \
    winetricks

pip3 install PyQt6
```

## After Installing Dependencies

Once you've installed the dependencies, you can run:

### For PyQt6 GUI (Advanced):
```bash
./wine-gui-launcher.sh
```

### For Tkinter GUI (Simple):
```bash
python3 wine_gui_simple.py
```

### For CLI (Always Works):
```bash
./bin/wine-cli --help
./bin/wine-cli run /path/to/program.exe
```

## Step-by-Step Guide

1. **Install Qt dependencies** (this fixes the xcb error):
   ```bash
   sudo apt install libxcb-cursor0 libxcb-xinerama0
   ```

2. **Install Tkinter** (for fallback GUI):
   ```bash
   sudo apt install python3-tk
   ```

3. **Test the GUI**:
   ```bash
   ./wine-gui-launcher.sh
   ```

4. **If still having issues**, use the CLI:
   ```bash
   ./bin/wine-cli version
   ./bin/wine-cli --help
   ```

## Verification

After installing, verify dependencies:
```bash
./check_dependencies.sh
```

This will show you what's installed and what's missing.

## Distribution-Specific Commands

### Ubuntu/Debian:
```bash
sudo apt install libxcb-cursor0 libxcb-xinerama0 python3-tk
```

### Fedora/RHEL:
```bash
sudo dnf install xcb-util-cursor libxcb python3-tkinter
```

### Arch Linux:
```bash
sudo pacman -S libxcb python-pyqt6 tk
```

## CLI Usage (No Dependencies Required)

The CLI application works without any GUI dependencies:

```bash
# Run an executable
./bin/wine-cli run /path/to/program.exe

# Create a Wine prefix
./bin/wine-cli prefix-create myprefix

# List all prefixes
./bin/wine-cli prefix-list

# Show help
./bin/wine-cli --help
```

The CLI is fully functional and doesn't require any GUI libraries!
