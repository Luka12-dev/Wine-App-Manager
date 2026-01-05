# Wine Application Manager - Quick Start Guide

## 🚀 Getting Started

You have two options: CLI (works now) or GUI (needs dependencies).

### Option 1: Use CLI (Works Immediately - No Dependencies!)

The CLI is fully functional right now:

```bash
# Show help
./bin/wine-cli --help

# Run a Windows executable
./bin/wine-cli run /path/to/program.exe

# Create a Wine prefix
./bin/wine-cli prefix-create gaming

# List all commands
./bin/wine-cli --help
```

**CLI Features:**
- ✅ Run Windows executables
- ✅ Manage Wine prefixes
- ✅ Install components
- ✅ Monitor processes
- ✅ Configure settings
- ✅ Create shortcuts
- ✅ View logs

### Option 2: Install GUI Dependencies

To use the graphical interface, install dependencies:

```bash
# Automatic installation (recommended)
./install_gui_deps.sh

# Then run the GUI
./wine-gui-launcher.sh
```

**Or manual installation:**
```bash
sudo apt install libxcb-cursor0 libxcb-xinerama0 python3-tk winetricks
```

---

## 📋 Common CLI Commands

### Running Applications
```bash
# Run an executable
./bin/wine-cli run /path/to/app.exe

# Run with arguments
./bin/wine-cli run /path/to/app.exe --arg1 --arg2

# Run and wait for completion
./bin/wine-cli exec /path/to/installer.exe
```

### Managing Prefixes
```bash
# Create a new prefix
./bin/wine-cli prefix-create myprefix

# List all prefixes
./bin/wine-cli prefix-list

# Switch to a prefix
./bin/wine-cli prefix-switch myprefix

# Show prefix info
./bin/wine-cli prefix-info myprefix

# Delete a prefix
./bin/wine-cli prefix-delete myprefix
```

### Application Shortcuts
```bash
# Add a shortcut
./bin/wine-cli shortcut-add notepad /path/to/notepad.exe

# List shortcuts
./bin/wine-cli shortcut-list

# Run from shortcut
./bin/wine-cli shortcut-run notepad

# Remove shortcut
./bin/wine-cli shortcut-remove notepad
```

### Process Management
```bash
# List running Wine processes
./bin/wine-cli list-processes

# Kill a specific process
./bin/wine-cli kill <PID>

# Kill all Wine processes
./bin/wine-cli killall
```

### System Information
```bash
# Show version
./bin/wine-cli version

# Show system info
./bin/wine-cli info

# View logs
./bin/wine-cli logs 50

# Show configuration
./bin/wine-cli config-show
```

### Installing Components
```bash
# Install a component (e.g., DirectX)
./bin/wine-cli install d3dx9

# List available components
./bin/wine-cli list-components
```

---

## 🎯 Example Workflow

### 1. Create a gaming prefix
```bash
./bin/wine-cli prefix-create gaming
./bin/wine-cli prefix-switch gaming
```

### 2. Install DirectX
```bash
./bin/wine-cli install d3dx9
```

### 3. Add a game shortcut
```bash
./bin/wine-cli shortcut-add mygame /path/to/game.exe
```

### 4. Run the game
```bash
./bin/wine-cli shortcut-run mygame
```

### 5. Monitor processes
```bash
./bin/wine-cli list-processes
```

---

## 🔧 Configuration

Configuration is stored in `~/.config/wineapp/`

### Change Wine prefix
```bash
./bin/wine-cli -p /custom/path/to/prefix run app.exe
```

### Set architecture
```bash
./bin/wine-cli -a win32 run app.exe    # 32-bit
./bin/wine-cli -a win64 run app.exe    # 64-bit
```

### Enable verbose output
```bash
./bin/wine-cli -v run app.exe
```

---

## 📚 All Available Commands

- `run` - Run an executable
- `exec` - Execute and wait for completion
- `kill` - Kill a process by PID
- `killall` - Kill all Wine processes
- `list-processes` - List running Wine processes
- `prefix-create` - Create a new Wine prefix
- `prefix-delete` - Delete a Wine prefix
- `prefix-list` - List all Wine prefixes
- `prefix-switch` - Switch to a Wine prefix
- `prefix-info` - Show prefix information
- `install` - Install a winetricks component
- `list-components` - List available components
- `shortcut-add` - Add application shortcut
- `shortcut-remove` - Remove application shortcut
- `shortcut-list` - List application shortcuts
- `shortcut-run` - Run application from shortcut
- `config-show` - Show current configuration
- `version` - Show version information
- `info` - Show system information
- `logs` - Show recent log entries

---

## 🐛 Troubleshooting

### GUI won't start?
Use the CLI - it works without any GUI dependencies!

```bash
./bin/wine-cli --help
```

### Need to install GUI dependencies?
```bash
./install_gui_deps.sh
```

### Wine not working?
Make sure Wine is installed:
```bash
sudo apt install wine wine64
```

---

## 📦 Project Stats

- **Total Code:** 5,631 lines
- **C++ Components:** 3,964 lines
- **Python GUI:** 1,667 lines
- **CLI Commands:** 20+
- **Build Status:** ✅ Working
- **CLI Status:** ✅ Fully Functional

---

## 🎉 You're Ready!

Start using the CLI right now:

```bash
./bin/wine-cli version
./bin/wine-cli info
```

Or install GUI dependencies and get the full experience:

```bash
./install_gui_deps.sh
./wine-gui-launcher.sh
```

Enjoy! 🍷
