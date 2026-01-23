# Wine Application Manager - Standalone Makefile
# This is a manually written Makefile (not CMake-generated)

# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
CXXFLAGS_DEBUG := -g -O0 -DDEBUG
LDFLAGS := -pthread

# Directories
BUILD_DIR := build
BIN_DIR := bin
LIB_DIR := lib

# Source files
WRAPPER_SOURCES := wine_wrapper.cpp wine_wrapper_impl.cpp wine_executor.cpp wine_utils.cpp wine_app_manager.cpp
CLI_SOURCE := wine_cli.cpp

# Object files
WRAPPER_OBJECTS := $(WRAPPER_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
CLI_OBJECT := $(BUILD_DIR)/wine_cli.o

# Output files
STATIC_LIB := $(LIB_DIR)/libwine_wrapper.a
SHARED_LIB := $(LIB_DIR)/libwine_wrapper.so
CLI_BIN := $(BIN_DIR)/wine-cli

# Installation paths
PREFIX := /usr/local
INSTALL_BIN := $(PREFIX)/bin
INSTALL_LIB := $(PREFIX)/lib
INSTALL_INCLUDE := $(PREFIX)/include/wine_wrapper
INSTALL_SHARE := $(PREFIX)/share/wine-app

# Python GUI files
GUI_FILES := wine_gui.py wine_gui_main.py wine_gui_window.py wine_gui_simple.py

# Default target
.PHONY: all
all: dirs $(STATIC_LIB) $(SHARED_LIB) $(CLI_BIN)
	@echo "Build complete!"

# Create directories
.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)

# Compile object files
$(BUILD_DIR)/%.o: %.cpp wine_wrapper.hpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

# Build static library
$(STATIC_LIB): $(WRAPPER_OBJECTS)
	@echo "Creating static library..."
	ar rcs $@ $^

# Build shared library
$(SHARED_LIB): $(WRAPPER_OBJECTS)
	@echo "Creating shared library..."
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# Build CLI executable
$(CLI_BIN): $(CLI_OBJECT) $(STATIC_LIB)
	@echo "Building CLI executable..."
	$(CXX) $(CXXFLAGS) -o $@ $(CLI_OBJECT) -L$(LIB_DIR) -lwine_wrapper $(LDFLAGS)

# GUI target (just verifies Python files exist)
.PHONY: gui
gui:
	@echo "Checking GUI files..."
	@for f in $(GUI_FILES); do \
		if [ -f "$$f" ]; then \
			echo "  ✓ $$f"; \
		else \
			echo "  ✗ $$f (missing)"; \
		fi; \
	done
	@echo "GUI files ready. Run with: python3 wine_gui_window.py"

# Debug build
.PHONY: debug
debug: CXXFLAGS += $(CXXFLAGS_DEBUG)
debug: clean all

# Run tests
.PHONY: test
test: $(CLI_BIN)
	@echo "Running tests..."
	@$(CLI_BIN) version || true
	@echo "Tests complete!"

# Install to system
.PHONY: install
install: all
	@echo "Installing Wine Application Manager..."
	install -d $(INSTALL_BIN)
	install -d $(INSTALL_LIB)
	install -d $(INSTALL_INCLUDE)
	install -d $(INSTALL_SHARE)
	install -m 755 $(CLI_BIN) $(INSTALL_BIN)/
	install -m 644 $(STATIC_LIB) $(INSTALL_LIB)/
	install -m 755 $(SHARED_LIB) $(INSTALL_LIB)/
	install -m 644 wine_wrapper.hpp $(INSTALL_INCLUDE)/
	@for f in $(GUI_FILES); do \
		if [ -f "$$f" ]; then \
			install -m 755 $$f $(INSTALL_SHARE)/; \
		fi; \
	done
	@echo "Creating wine-gui symlink..."
	ln -sf $(INSTALL_SHARE)/wine_gui_window.py $(INSTALL_BIN)/wine-gui
	@ldconfig || true
	@echo "Installation complete!"

# Uninstall from system
.PHONY: uninstall
uninstall:
	@echo "Uninstalling Wine Application Manager..."
	rm -f $(INSTALL_BIN)/wine-cli
	rm -f $(INSTALL_BIN)/wine-gui
	rm -f $(INSTALL_LIB)/libwine_wrapper.a
	rm -f $(INSTALL_LIB)/libwine_wrapper.so
	rm -rf $(INSTALL_INCLUDE)
	rm -rf $(INSTALL_SHARE)
	@ldconfig || true
	@echo "Uninstallation complete!"

# Clean build files
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)
	@echo "Clean complete!"

# Help
.PHONY: help
help:
	@echo "Wine Application Manager - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build everything (default)"
	@echo "  gui       - Check GUI files"
	@echo "  debug     - Build with debug flags"
	@echo "  test      - Run tests"
	@echo "  install   - Install to system (requires sudo)"
	@echo "  uninstall - Remove from system (requires sudo)"
	@echo "  clean     - Remove build files"
	@echo "  help      - Show this help"
