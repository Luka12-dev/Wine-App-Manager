#!/bin/bash
# Dependency checker for Wine Application Manager

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    if [ $2 -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $1"
    else
        echo -e "${RED}✗${NC} $1"
    fi
}

echo "=== Wine Application Manager - Dependency Check ==="
echo ""

# Check Python
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    print_status "Python 3: $PYTHON_VERSION" 0
else
    print_status "Python 3: NOT FOUND" 1
    echo "  Install: sudo apt install python3"
fi

# Check PyQt6
if python3 -c "import PyQt6" 2>/dev/null; then
    print_status "PyQt6: Installed" 0
else
    print_status "PyQt6: NOT FOUND" 1
    echo "  Install: pip3 install PyQt6"
fi

# Check Qt platform plugins
echo ""
echo "Checking Qt Platform Plugins..."

# Check for xcb-cursor library
if ldconfig -p | grep -q libxcb-cursor; then
    print_status "libxcb-cursor: Installed" 0
else
    print_status "libxcb-cursor: NOT FOUND" 1
    echo "  Install: sudo apt install libxcb-cursor0"
fi

# Check for other Qt dependencies
QT_DEPS=(
    "libxcb-icccm4"
    "libxcb-image0"
    "libxcb-keysyms1"
    "libxcb-randr0"
    "libxcb-render-util0"
    "libxcb-xinerama0"
    "libxcb-xfixes0"
    "libxcb-shape0"
)

MISSING_DEPS=()

for dep in "${QT_DEPS[@]}"; do
    if ldconfig -p | grep -q ${dep%%[0-9]*}; then
        print_status "$dep: Installed" 0
    else
        print_status "$dep: NOT FOUND" 1
        MISSING_DEPS+=("$dep")
    fi
done

# Check Wine
echo ""
if command -v wine &> /dev/null; then
    WINE_VERSION=$(wine --version)
    print_status "Wine: $WINE_VERSION" 0
else
    print_status "Wine: NOT FOUND" 1
    echo "  Install: sudo apt install wine"
fi

# Check winetricks
if command -v winetricks &> /dev/null; then
    print_status "Winetricks: Installed" 0
else
    print_status "Winetricks: NOT FOUND (optional)" 1
    echo "  Install: sudo apt install winetricks"
fi

# Summary
echo ""
echo "=== Installation Commands ==="
echo ""

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "Install missing Qt dependencies:"
    echo "sudo apt install ${MISSING_DEPS[@]}"
    echo ""
fi

if ! python3 -c "import PyQt6" 2>/dev/null; then
    echo "Install PyQt6:"
    echo "pip3 install PyQt6"
    echo ""
fi

if ! command -v wine &> /dev/null; then
    echo "Install Wine:"
    echo "sudo apt install wine wine64 wine32"
    echo ""
fi

echo "Or install everything at once:"
echo "sudo apt install python3 python3-pip libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-xinerama0 libxcb-xfixes0 libxcb-shape0 wine wine64 winetricks"
echo "pip3 install PyQt6"
