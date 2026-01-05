#!/bin/bash
# Wait for apt to be available, then install dependencies

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "═══════════════════════════════════════════════════════════════════════"
echo "           Waiting for Package Manager to be Available"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

# Check if apt is locked
while fuser /var/lib/dpkg/lock-frontend >/dev/null 2>&1; do
    echo -e "${YELLOW}⏳${NC} Package manager is locked. Waiting..."
    echo "    (Close Software Updater or other package managers)"
    sleep 5
done

echo -e "${GREEN}✓${NC} Package manager is available!"
echo ""

# Now install dependencies
echo "Installing GUI dependencies..."
echo ""

sudo apt install -y libxcb-cursor0 libxcb-xinerama0 python3-tk python3-venv

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ Installation complete!${NC}"
    echo ""
    echo "Now you can run the GUI:"
    echo "  ./run_gui.sh"
    echo ""
else
    echo ""
    echo -e "${RED}✗ Installation failed${NC}"
    echo ""
    echo "Try manually:"
    echo "  sudo apt install libxcb-cursor0 libxcb-xinerama0 python3-tk"
    echo ""
fi
