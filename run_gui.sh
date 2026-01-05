#!/bin/bash
# Smart GUI launcher - tries best available option

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "Wine Application Manager - Starting GUI..."
echo ""

# Try PyQt6 with venv first
if [ -f "venv/bin/python" ]; then
    echo -e "${GREEN}→${NC} Trying PyQt6 GUI (virtual environment)..."
    if venv/bin/python wine_gui_window.py 2>/dev/null; then
        exit 0
    fi
    echo -e "${YELLOW}!${NC} PyQt6 failed, trying next option..."
    echo ""
fi

# Try PyQt6 with system Python
if python3 -c "import PyQt6" 2>/dev/null; then
    echo -e "${GREEN}→${NC} Trying PyQt6 GUI (system Python)..."
    if python3 wine_gui_window.py 2>/dev/null; then
        exit 0
    fi
    echo -e "${YELLOW}!${NC} PyQt6 failed, trying next option..."
    echo ""
fi

# Try Tkinter GUI
if python3 -c "import tkinter" 2>/dev/null; then
    echo -e "${GREEN}→${NC} Starting Tkinter GUI (simple interface)..."
    python3 wine_gui_simple.py
    exit 0
fi

# All failed
echo -e "${RED}✗${NC} No GUI available"
echo ""
echo "To fix this, run:"
echo "  ./fix_and_run_gui.sh"
echo ""
echo "Or use the CLI:"
echo "  ./bin/wine-cli --help"
exit 1
