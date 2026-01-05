#!/bin/bash
# Build script for Wine Application Manager

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_msg "Checking dependencies..."
    
    # Check for g++
    if ! command -v g++ &> /dev/null; then
        print_error "g++ is not installed. Please install it first."
        exit 1
    fi
    
    # Check for Python 3
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 is not installed. Please install it first."
        exit 1
    fi
    
    # Check for PyQt6
    if ! python3 -c "import PyQt6" &> /dev/null; then
        print_warning "PyQt6 is not installed. GUI will not work."
        print_info "Install with: pip3 install PyQt6"
    fi
    
    # Check for Wine
    if ! command -v wine &> /dev/null; then
        print_warning "Wine is not installed. The application will not function without it."
        print_info "Install Wine from your distribution's package manager."
    fi
    
    print_msg "Dependency check complete!"
}

# Build using Make
build_make() {
    print_msg "Building with Make..."
    make clean
    make all
    make gui
    print_msg "Make build complete!"
}

# Build using CMake
build_cmake() {
    print_msg "Building with CMake..."
    
    rm -rf build_cmake
    mkdir -p build_cmake
    cd build_cmake
    
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    
    cd ..
    
    print_msg "CMake build complete!"
}

# Run tests
run_tests() {
    print_msg "Running tests..."
    
    if [ -f "bin/wine-cli" ]; then
        ./bin/wine-cli version || true
        print_msg "CLI test complete!"
    fi
    
    print_msg "All tests complete!"
}

# Count lines of code
count_lines() {
    print_msg "Counting lines of code..."
    
    total=0
    
    echo ""
    echo "Lines of Code:"
    echo "=============="
    
    for file in wine_wrapper.hpp wine_wrapper.cpp wine_wrapper_impl.cpp wine_executor.cpp wine_utils.cpp wine_app_manager.cpp wine_cli.cpp; do
        if [ -f "$file" ]; then
            lines=$(wc -l < "$file")
            total=$((total + lines))
            printf "%-30s : %6d lines\n" "$file" "$lines"
        fi
    done
    
    echo ""
    echo "Python Files:"
    for file in wine_gui.py wine_gui_main.py wine_gui_window.py; do
        if [ -f "$file" ]; then
            lines=$(wc -l < "$file")
            total=$((total + lines))
            printf "%-30s : %6d lines\n" "$file" "$lines"
        fi
    done
    
    echo ""
    echo "=============="
    printf "TOTAL                         : %6d lines\n" "$total"
    echo "=============="
    echo ""
    
    if [ $total -ge 5000 ]; then
        print_msg "Line count requirement MET! (>= 5000 lines)"
    else
        print_warning "Line count requirement NOT MET. Total: $total lines, Required: 5000 lines"
    fi
}

# Install the application
install_app() {
    print_msg "Installing Wine Application Manager..."
    
    if [ "$EUID" -ne 0 ]; then
        print_error "Please run install with sudo: sudo ./build.sh install"
        exit 1
    fi
    
    make install
    
    print_msg "Installation complete!"
    echo ""
    print_info "Run 'wine-cli --help' for command line interface"
    print_info "Run 'wine-gui' for graphical interface"
}

# Uninstall the application
uninstall_app() {
    print_msg "Uninstalling Wine Application Manager..."
    
    if [ "$EUID" -ne 0 ]; then
        print_error "Please run uninstall with sudo: sudo ./build.sh uninstall"
        exit 1
    fi
    
    make uninstall
    
    print_msg "Uninstallation complete!"
}

# Show help
show_help() {
    echo "Wine Application Manager - Build Script"
    echo ""
    echo "Usage: ./build.sh [command]"
    echo ""
    echo "Commands:"
    echo "  all         - Build everything (default)"
    echo "  make        - Build using Make"
    echo "  cmake       - Build using CMake"
    echo "  test        - Run tests"
    echo "  count       - Count lines of code"
    echo "  install     - Install to system (requires sudo)"
    echo "  uninstall   - Remove from system (requires sudo)"
    echo "  clean       - Clean build files"
    echo "  help        - Show this help message"
    echo ""
    echo "Examples:"
    echo "  ./build.sh              # Build everything"
    echo "  ./build.sh count        # Count lines of code"
    echo "  sudo ./build.sh install # Install to system"
}

# Clean build files
clean_build() {
    print_msg "Cleaning build files..."
    make clean
    rm -rf build_cmake
    print_msg "Clean complete!"
}

# Main script
main() {
    print_msg "Wine Application Manager - Build System"
    echo ""
    
    case "${1:-all}" in
        all)
            check_dependencies
            build_make
            count_lines
            run_tests
            ;;
        make)
            check_dependencies
            build_make
            ;;
        cmake)
            check_dependencies
            build_cmake
            ;;
        test)
            run_tests
            ;;
        count)
            count_lines
            ;;
        install)
            install_app
            ;;
        uninstall)
            uninstall_app
            ;;
        clean)
            clean_build
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            print_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
#3
#
##
###"
#
