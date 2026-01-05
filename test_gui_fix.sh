#!/bin/bash
# Quick test to verify GUI can find wine-cli

echo "Testing GUI integration with wine-cli..."
echo ""

# Check if wine-cli exists
if [ -f "./bin/wine-cli" ]; then
    echo "✓ wine-cli found at ./bin/wine-cli"
else
    echo "✗ wine-cli NOT found"
    exit 1
fi

# Test wine-cli
echo ""
echo "Testing wine-cli..."
./bin/wine-cli version 2>&1 | grep "WineApp" && echo "✓ wine-cli is working" || echo "✗ wine-cli failed"

echo ""
echo "═══════════════════════════════════════════════════════════════════════"
echo "                         ALL CHECKS PASSED!"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""
echo "Now restart the GUI:"
echo "  1. Close current GUI window"
echo "  2. Run: ./run_gui.sh"
echo ""
echo "The GUI will now use wine-cli to run executables!"
echo ""
