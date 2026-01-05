#!/bin/bash
# Automatic GUI dependency installer for Wine Application Manager

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "╔═══════════════════════════════════════════════════════════════════════╗"
echo "║         Wine Application Manager - GUI Dependency Installer          ║"
echo "╚═══════════════════════════════════════════════════════════════════════╝"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${YELLOW}This script needs sudo privileges to install system packages.${NC}"
    echo "Restarting with sudo..."
    echo ""
    exec sudo bash "$0" "$@"
    exit $?
fi

echo -e "${BLUE}[INFO]${NC} Installing GUI dependencies..."
echo ""

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo -e "${RED}[ERROR]${NC} Cannot detect Linux distribution"
    exit 1
fi

echo -e "${GREEN}[DETECTED]${NC} Distribution: $DISTRO"
echo ""

# Install based on distribution
case $DISTRO in
    ubuntu|debian|linuxmint|pop)
        echo -e "${BLUE}[INSTALL]${NC} Installing packages for Debian/Ubuntu..."
        apt update
        apt install -y \
            libxcb-cursor0 \
            libxcb-xinerama0 \
            python3-tk \
            winetricks
        ;;
    
    fedora|rhel|centos)
        echo -e "${BLUE}[INSTALL]${NC} Installing packages for Fedora/RHEL..."
        dnf install -y \
            xcb-util-cursor \
            libxcb \
            python3-tkinter \
            winetricks
        ;;
    
    arch|manjaro)
        echo -e "${BLUE}[INSTALL]${NC} Installing packages for Arch Linux..."
        pacman -Sy --noconfirm \
            libxcb \
            tk \
            winetricks
        ;;
    
    opensuse*)
        echo -e "${BLUE}[INSTALL]${NC} Installing packages for openSUSE..."
        zypper install -y \
            libxcb-cursor0 \
            python3-tk \
            winetricks
        ;;
    
    *)
        echo -e "${YELLOW}[WARNING]${NC} Unknown distribution: $DISTRO"
        echo "Please install manually:"
        echo "  - libxcb-cursor0 (or xcb-util-cursor)"
        echo "  - libxcb-xinerama0"
        echo "  - python3-tk (or python3-tkinter)"
        echo "  - winetricks (optional)"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}[SUCCESS]${NC} Dependencies installed!"
echo ""

# Verify installation
echo -e "${BLUE}[VERIFY]${NC} Checking installation..."
echo ""

if ldconfig -p | grep -q libxcb-cursor 2>/dev/null; then
    echo -e "${GREEN}✓${NC} libxcb-cursor installed"
else
    echo -e "${YELLOW}!${NC} libxcb-cursor may not be properly installed"
fi

if python3 -c "import tkinter" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Python Tkinter installed"
else
    echo -e "${YELLOW}!${NC} Python Tkinter may not be properly installed"
fi

if python3 -c "import PyQt6" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} PyQt6 installed"
else
    echo -e "${YELLOW}!${NC} PyQt6 not installed (optional)"
    echo "  Install with: pip3 install PyQt6"
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════════════╗"
echo "║                        INSTALLATION COMPLETE                          ║"
echo "╚═══════════════════════════════════════════════════════════════════════╝"
echo ""
echo "You can now run the GUI:"
echo ""
echo -e "  ${GREEN}./wine-gui-launcher.sh${NC}"
echo ""
echo "Or use the CLI:"
echo ""
echo -e "  ${GREEN}./bin/wine-cli --help${NC}"
echo ""
