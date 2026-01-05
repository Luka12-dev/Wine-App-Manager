#!/bin/bash
# Complete GUI fix and run script

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "═══════════════════════════════════════════════════════════════════════"
echo "           Wine Application Manager - Complete GUI Setup"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

# Step 1: Install system dependencies
echo -e "${BLUE}Step 1: Installing system dependencies...${NC}"
echo ""
echo "This will install:"
echo "  - libxcb-cursor0 (Qt cursor support)"
echo "  - libxcb-xinerama0 (Qt multi-monitor support)"
echo "  - python3-tk (Tkinter fallback GUI)"
echo "  - python3-venv (Python virtual environments)"
echo ""

read -p "Press ENTER to install (requires sudo) or Ctrl+C to cancel..."

sudo apt update
sudo apt install -y libxcb-cursor0 libxcb-xinerama0 python3-tk python3-venv python3-pip

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to install dependencies${NC}"
    echo "Please run manually:"
    echo "  sudo apt install libxcb-cursor0 libxcb-xinerama0 python3-tk python3-venv"
    exit 1
fi

echo -e "${GREEN}✓ System dependencies installed!${NC}"
echo ""

# Step 2: Create virtual environment (optional but recommended)
echo -e "${BLUE}Step 2: Setting up Python environment...${NC}"
echo ""

if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    
    echo "Installing PyQt6 in virtual environment..."
    source venv/bin/activate
    pip install --upgrade pip
    pip install PyQt6
    deactivate
    
    echo -e "${GREEN}✓ Virtual environment created!${NC}"
else
    echo -e "${YELLOW}Virtual environment already exists${NC}"
fi

echo ""

# Step 3: Verify installation
echo -e "${BLUE}Step 3: Verifying installation...${NC}"
echo ""

# Check libxcb
if ldconfig -p | grep -q libxcb-cursor; then
    echo -e "${GREEN}✓${NC} libxcb-cursor0 installed"
else
    echo -e "${RED}✗${NC} libxcb-cursor0 missing"
fi

if ldconfig -p | grep -q libxcb-xinerama; then
    echo -e "${GREEN}✓${NC} libxcb-xinerama0 installed"
else
    echo -e "${RED}✗${NC} libxcb-xinerama0 missing"
fi

# Check Tkinter
if python3 -c "import tkinter" 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Python Tkinter installed"
else
    echo -e "${RED}✗${NC} Python Tkinter missing"
fi

# Check PyQt6 in venv
if [ -f "venv/bin/python" ]; then
    if venv/bin/python -c "import PyQt6" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} PyQt6 installed in venv"
    else
        echo -e "${YELLOW}!${NC} PyQt6 not in venv (will use system)"
    fi
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════════"
echo -e "                    ${GREEN}Installation Complete!${NC}"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""
echo "You can now run:"
echo ""
echo "  Option 1 (PyQt6 GUI):"
echo "    ./run_gui.sh"
echo ""
echo "  Option 2 (Tkinter GUI - Simple):"
echo "    python3 wine_gui_simple.py"
echo ""
echo "  Option 3 (CLI - Always works):"
echo "    ./bin/wine-cli --help"
echo ""
