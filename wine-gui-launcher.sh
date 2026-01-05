#!/bin/bash
# Wine Application Manager - Smart GUI Launcher
# Tries PyQt6 first, falls back to Tkinter if needed

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Wine Application Manager - Starting GUI..."
echo ""

# Check if PyQt6 is available
if python3 -c "import PyQt6" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} PyQt6 found - Using advanced GUI"
    
    # Check for Qt platform issues
    if ! ldconfig -p | grep -q libxcb-cursor 2>/dev/null; then
        echo -e "${YELLOW}!${NC} Missing libxcb-cursor0 - Qt may not work"
        echo "  Install: sudo apt install libxcb-cursor0 libxcb-xinerama0"
        echo ""
        echo "Trying PyQt6 GUI anyway..."
        sleep 1
    fi
    
    # Try to run PyQt6 GUI
    if python3 wine_gui_window.py 2>/tmp/wine_gui_error.log; then
        exit 0
    else
        echo -e "${RED}✗${NC} PyQt6 GUI failed"
        echo "Error details in /tmp/wine_gui_error.log"
        echo ""
        echo "Falling back to simple GUI..."
        sleep 1
    fi
fi

# Fallback to Tkinter GUI
echo -e "${GREEN}✓${NC} Using Tkinter GUI (simple interface)"
echo ""

if python3 wine_gui_simple.py; then
    exit 0
else
    echo -e "${RED}✗${NC} GUI failed to start"
    echo ""
    echo "You can still use the command-line interface:"
    echo "  ./bin/wine-cli --help"
    exit 1
fi
