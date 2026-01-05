#!/usr/bin/env python3
"""
Wine Application Manager - Main Window Implementation
"""

import sys
import os
from pathlib import Path

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QTextEdit, QTabWidget, QMessageBox,
    QMenuBar, QMenu, QToolBar, QStatusBar, QSystemTrayIcon, QStyle
)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QIcon, QAction, QTextCursor

from wine_gui import (
    WineConfig, ProcessMonitorThread, WineExecutorThread,
    PrefixManagerDialog
)
from wine_gui_main import ConfigurationTab, ApplicationsTab, ProcessesTab


class LogTab(QWidget):
    """Log viewer tab"""
    
    def __init__(self):
        super().__init__()
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setStyleSheet("QTextEdit { font-family: monospace; background-color: #1e1e1e; color: #d4d4d4; }")
        layout.addWidget(self.log_text)
        
        button_layout = QHBoxLayout()
        
        clear_btn = QPushButton("Clear Log")
        clear_btn.clicked.connect(self.clear_log)
        button_layout.addWidget(clear_btn)
        
        save_btn = QPushButton("Save Log")
        save_btn.clicked.connect(self.save_log)
        button_layout.addWidget(save_btn)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        self.setLayout(layout)
    
    def append_log(self, message: str, color: str = "#d4d4d4"):
        """Append message to log"""
        cursor = self.log_text.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        
        timestamp = os.popen('date "+%Y-%m-%d %H:%M:%S"').read().strip()
        formatted_message = f'<span style="color: #888;">[{timestamp}]</span> <span style="color: {color};">{message}</span>'
        
        cursor.insertHtml(formatted_message + "<br>")
        self.log_text.setTextCursor(cursor)
        self.log_text.ensureCursorVisible()
    
    def clear_log(self):
        """Clear log"""
        self.log_text.clear()
    
    def save_log(self):
        """Save log to file"""
        from PyQt6.QtWidgets import QFileDialog
        
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Log File", "", "Log Files (*.log);;Text Files (*.txt);;All Files (*)"
        )
        
        if file_path:
            try:
                with open(file_path, 'w') as f:
                    f.write(self.log_text.toPlainText())
                QMessageBox.information(self, "Success", "Log saved successfully")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save log: {str(e)}")


class WinetricksTab(QWidget):
    """Winetricks management tab"""
    
    def __init__(self, config: WineConfig):
        super().__init__()
        self.config = config
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        info_label = QLabel("Install Windows components and libraries using Winetricks")
        info_label.setStyleSheet("QLabel { color: #666; margin-bottom: 10px; }")
        layout.addWidget(info_label)
        
        from PyQt6.QtWidgets import QListWidget
        
        categories_layout = QHBoxLayout()
        
        left_layout = QVBoxLayout()
        left_layout.addWidget(QLabel("Common Components:"))
        
        self.common_list = QListWidget()
        common_items = [
            'dotnet40', 'dotnet45', 'dotnet46', 'dotnet48',
            'vcrun2005', 'vcrun2008', 'vcrun2010', 'vcrun2012', 'vcrun2013', 'vcrun2015',
            'd3dx9', 'd3dx10', 'd3dx11_43', 'd3dcompiler_43', 'd3dcompiler_47',
            'dxvk', 'vkd3d', 'quartz', 'devenum', 'wmp10',
            'msxml3', 'msxml4', 'msxml6', 'xact', 'xna31', 'xna40',
            'physx', 'vcrun6', 'mfc42', 'gdiplus', 'corefonts'
        ]
        self.common_list.addItems(common_items)
        left_layout.addWidget(self.common_list)
        
        categories_layout.addLayout(left_layout)
        
        right_layout = QVBoxLayout()
        right_layout.addWidget(QLabel("Fonts:"))
        
        self.fonts_list = QListWidget()
        font_items = [
            'corefonts', 'tahoma', 'liberation', 'lucida', 'consolas',
            'courier', 'times', 'arial', 'georgia', 'impact', 'trebuchet'
        ]
        self.fonts_list.addItems(font_items)
        right_layout.addWidget(self.fonts_list)
        
        categories_layout.addLayout(right_layout)
        
        layout.addLayout(categories_layout)
        
        button_layout = QHBoxLayout()
        
        install_common_btn = QPushButton("Install Selected Component")
        install_common_btn.clicked.connect(lambda: self.install_selected(self.common_list))
        button_layout.addWidget(install_common_btn)
        
        install_font_btn = QPushButton("Install Selected Font")
        install_font_btn.clicked.connect(lambda: self.install_selected(self.fonts_list))
        button_layout.addWidget(install_font_btn)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        status_label = QLabel("Status: Ready")
        status_label.setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }")
        layout.addWidget(status_label)
        self.status_label = status_label
        
        self.setLayout(layout)
    
    def install_selected(self, list_widget):
        """Install selected component"""
        current_item = list_widget.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a component to install")
            return
        
        component = current_item.text()
        
        reply = QMessageBox.question(
            self, "Confirm Installation",
            f"Install {component}? This may take several minutes.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.install_component(component)
    
    def install_component(self, component: str):
        """Install winetricks component"""
        self.status_label.setText(f"Status: Installing {component}...")
        self.status_label.setStyleSheet("QLabel { color: #FF9800; font-weight: bold; }")
        
        import subprocess
        
        try:
            env = os.environ.copy()
            env['WINEPREFIX'] = self.config.config['wine_prefix']
            
            process = subprocess.Popen(
                ['winetricks', '-q', component],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            
            process.wait()
            
            if process.returncode == 0:
                self.status_label.setText(f"Status: {component} installed successfully")
                self.status_label.setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; }")
                QMessageBox.information(self, "Success", f"{component} installed successfully")
            else:
                self.status_label.setText(f"Status: Failed to install {component}")
                self.status_label.setStyleSheet("QLabel { color: #f44336; font-weight: bold; }")
                QMessageBox.critical(self, "Error", f"Failed to install {component}")
        except Exception as e:
            self.status_label.setText(f"Status: Error - {str(e)}")
            self.status_label.setStyleSheet("QLabel { color: #f44336; font-weight: bold; }")
            QMessageBox.critical(self, "Error", f"Installation error: {str(e)}")


class ToolsTab(QWidget):
    """Wine tools tab"""
    
    def __init__(self, config: WineConfig):
        super().__init__()
        self.config = config
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        from PyQt6.QtWidgets import QGroupBox, QGridLayout
        
        wine_tools_group = QGroupBox("Wine Tools")
        wine_tools_layout = QGridLayout()
        
        winecfg_btn = QPushButton("Wine Configuration (winecfg)")
        winecfg_btn.clicked.connect(lambda: self.run_tool('winecfg'))
        wine_tools_layout.addWidget(winecfg_btn, 0, 0)
        
        regedit_btn = QPushButton("Registry Editor (regedit)")
        regedit_btn.clicked.connect(lambda: self.run_tool('wine', 'regedit'))
        wine_tools_layout.addWidget(regedit_btn, 0, 1)
        
        taskmgr_btn = QPushButton("Task Manager (taskmgr)")
        taskmgr_btn.clicked.connect(lambda: self.run_tool('wine', 'taskmgr'))
        wine_tools_layout.addWidget(taskmgr_btn, 1, 0)
        
        explorer_btn = QPushButton("File Explorer (explorer)")
        explorer_btn.clicked.connect(lambda: self.run_tool('wine', 'explorer'))
        wine_tools_layout.addWidget(explorer_btn, 1, 1)
        
        cmd_btn = QPushButton("Command Prompt (cmd)")
        cmd_btn.clicked.connect(lambda: self.run_tool('wine', 'cmd'))
        wine_tools_layout.addWidget(cmd_btn, 2, 0)
        
        notepad_btn = QPushButton("Notepad")
        notepad_btn.clicked.connect(lambda: self.run_tool('wine', 'notepad'))
        wine_tools_layout.addWidget(notepad_btn, 2, 1)
        
        wine_tools_group.setLayout(wine_tools_layout)
        layout.addWidget(wine_tools_group)
        
        maintenance_group = QGroupBox("Maintenance Tools")
        maintenance_layout = QGridLayout()
        
        wineboot_btn = QPushButton("Update Wine Prefix (wineboot -u)")
        wineboot_btn.clicked.connect(self.run_wineboot_update)
        maintenance_layout.addWidget(wineboot_btn, 0, 0)
        
        kill_wine_btn = QPushButton("Kill Wine Server")
        kill_wine_btn.clicked.connect(self.kill_wineserver)
        maintenance_layout.addWidget(kill_wine_btn, 0, 1)
        
        cleanup_btn = QPushButton("Clean Temp Files")
        cleanup_btn.clicked.connect(self.cleanup_temp)
        maintenance_layout.addWidget(cleanup_btn, 1, 0)
        
        uninstaller_btn = QPushButton("Uninstall Programs")
        uninstaller_btn.clicked.connect(lambda: self.run_tool('wine', 'uninstaller'))
        maintenance_layout.addWidget(uninstaller_btn, 1, 1)
        
        maintenance_group.setLayout(maintenance_layout)
        layout.addWidget(maintenance_group)
        
        info_group = QGroupBox("System Information")
        info_layout = QVBoxLayout()
        
        self.info_text = QTextEdit()
        self.info_text.setReadOnly(True)
        self.info_text.setMaximumHeight(150)
        info_layout.addWidget(self.info_text)
        
        refresh_info_btn = QPushButton("Refresh Information")
        refresh_info_btn.clicked.connect(self.refresh_info)
        info_layout.addWidget(refresh_info_btn)
        
        info_group.setLayout(info_layout)
        layout.addWidget(info_group)
        
        layout.addStretch()
        
        self.setLayout(layout)
        self.refresh_info()
    
    def run_tool(self, *args):
        """Run Wine tool"""
        import subprocess
        
        env = os.environ.copy()
        env['WINEPREFIX'] = self.config.config['wine_prefix']
        
        try:
            subprocess.Popen(list(args), env=env)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run tool: {str(e)}")
    
    def run_wineboot_update(self):
        """Run wineboot to update prefix"""
        import subprocess
        
        env = os.environ.copy()
        env['WINEPREFIX'] = self.config.config['wine_prefix']
        
        try:
            subprocess.run(['wineboot', '-u'], env=env, check=True)
            QMessageBox.information(self, "Success", "Wine prefix updated successfully")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to update prefix: {str(e)}")
    
    def kill_wineserver(self):
        """Kill wineserver"""
        import subprocess
        
        env = os.environ.copy()
        env['WINEPREFIX'] = self.config.config['wine_prefix']
        
        try:
            subprocess.run(['wineserver', '-k'], env=env, check=True)
            QMessageBox.information(self, "Success", "Wine server terminated")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to kill wineserver: {str(e)}")
    
    def cleanup_temp(self):
        """Clean temporary files"""
        import shutil
        
        prefix = self.config.config['wine_prefix']
        temp_dirs = [
            os.path.join(prefix, 'drive_c', 'windows', 'temp'),
            os.path.join(prefix, 'drive_c', 'users', 'Public', 'Temp'),
            os.path.join(prefix, 'drive_c', 'windows', 'Installer')
        ]
        
        cleaned = 0
        for temp_dir in temp_dirs:
            if os.path.exists(temp_dir):
                for item in os.listdir(temp_dir):
                    item_path = os.path.join(temp_dir, item)
                    try:
                        if os.path.isfile(item_path):
                            os.unlink(item_path)
                            cleaned += 1
                        elif os.path.isdir(item_path):
                            shutil.rmtree(item_path)
                            cleaned += 1
                    except Exception:
                        pass
        
        QMessageBox.information(self, "Success", f"Cleaned {cleaned} temporary items")
    
    def refresh_info(self):
        """Refresh system information"""
        import subprocess
        
        info = []
        
        try:
            wine_version = subprocess.check_output(['wine', '--version'], universal_newlines=True).strip()
            info.append(f"Wine Version: {wine_version}")
        except:
            info.append("Wine Version: Not found")
        
        info.append(f"Wine Prefix: {self.config.config['wine_prefix']}")
        info.append(f"Architecture: {self.config.config['architecture']}")
        
        if os.path.exists(self.config.config['wine_prefix']):
            size = self.get_directory_size(self.config.config['wine_prefix'])
            info.append(f"Prefix Size: {size / (1024*1024):.2f} MB")
        
        self.info_text.setPlainText('\n'.join(info))
    
    def get_directory_size(self, path):
        """Get directory size"""
        total = 0
        for dirpath, dirnames, filenames in os.walk(path):
            for filename in filenames:
                filepath = os.path.join(dirpath, filename)
                if os.path.exists(filepath):
                    total += os.path.getsize(filepath)
        return total


class WineApplicationWindow(QMainWindow):
    """Main application window"""
    
    def __init__(self):
        super().__init__()
        self.config = WineConfig()
        self.process_monitor = ProcessMonitorThread()
        self.executor_thread = None
        self.init_ui()
        self.setup_connections()
    
    def init_ui(self):
        """Initialize UI"""
        self.setWindowTitle("Wine Application Manager")
        self.setGeometry(100, 100, 1000, 700)
        
        self.create_menu_bar()
        self.create_tool_bar()
        self.create_status_bar()
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        layout = QVBoxLayout()
        
        self.tabs = QTabWidget()
        
        self.applications_tab = ApplicationsTab(self.config, self)
        self.tabs.addTab(self.applications_tab, "Applications")
        
        self.config_tab = ConfigurationTab(self.config)
        self.tabs.addTab(self.config_tab, "Configuration")
        
        self.processes_tab = ProcessesTab()
        self.tabs.addTab(self.processes_tab, "Processes")
        
        self.log_tab = LogTab()
        self.tabs.addTab(self.log_tab, "Log")
        
        self.winetricks_tab = WinetricksTab(self.config)
        self.tabs.addTab(self.winetricks_tab, "Winetricks")
        
        self.tools_tab = ToolsTab(self.config)
        self.tabs.addTab(self.tools_tab, "Tools")
        
        layout.addWidget(self.tabs)
        
        central_widget.setLayout(layout)
        
        self.apply_theme()
    
    def create_menu_bar(self):
        """Create menu bar"""
        menubar = self.menuBar()
        
        file_menu = menubar.addMenu("File")
        
        run_action = QAction("Run Executable...", self)
        run_action.setShortcut("Ctrl+R")
        run_action.triggered.connect(self.run_executable_dialog)
        file_menu.addAction(run_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("Exit", self)
        exit_action.setShortcut("Ctrl+Q")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        prefix_menu = menubar.addMenu("Prefix")
        
        manage_prefix_action = QAction("Manage Prefixes...", self)
        manage_prefix_action.triggered.connect(self.manage_prefixes)
        prefix_menu.addAction(manage_prefix_action)
        
        tools_menu = menubar.addMenu("Tools")
        
        winecfg_action = QAction("Wine Configuration", self)
        winecfg_action.triggered.connect(lambda: self.run_wine_tool('winecfg'))
        tools_menu.addAction(winecfg_action)
        
        help_menu = menubar.addMenu("Help")
        
        about_action = QAction("About", self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
    
    def create_tool_bar(self):
        """Create tool bar"""
        toolbar = QToolBar()
        toolbar.setMovable(False)
        self.addToolBar(toolbar)
        
        run_action = QAction("Run", self)
        run_action.triggered.connect(self.run_executable_dialog)
        toolbar.addAction(run_action)
        
        toolbar.addSeparator()
        
        prefix_action = QAction("Prefixes", self)
        prefix_action.triggered.connect(self.manage_prefixes)
        toolbar.addAction(prefix_action)
    
    def create_status_bar(self):
        """Create status bar"""
        self.statusBar().showMessage("Ready")
    
    def setup_connections(self):
        """Setup signal connections"""
        self.process_monitor.process_update.connect(self.on_process_update)
        self.process_monitor.process_finished.connect(self.on_process_finished)
        self.process_monitor.start()
    
    def execute_wine_program(self, exe_path: str, args: list):
        """Execute Wine program"""
        self.log_tab.append_log(f"Starting: {exe_path}", "#4CAF50")
        self.statusBar().showMessage(f"Running: {os.path.basename(exe_path)}")
        
        self.executor_thread = WineExecutorThread(exe_path, args, self.config.config)
        self.executor_thread.output_ready.connect(self.on_output_ready)
        self.executor_thread.error_ready.connect(self.on_error_ready)
        self.executor_thread.process_started.connect(self.on_process_started)
        self.executor_thread.process_finished.connect(self.on_executor_finished)
        self.executor_thread.start()
    
    def on_output_ready(self, text: str):
        """Handle process output"""
        self.log_tab.append_log(text.strip(), "#d4d4d4")
    
    def on_error_ready(self, text: str):
        """Handle process error"""
        self.log_tab.append_log(text.strip(), "#f44336")
    
    def on_process_started(self, pid: int):
        """Handle process started"""
        self.log_tab.append_log(f"Process started with PID: {pid}", "#4CAF50")
        self.process_monitor.add_process(pid, "Wine Application")
    
    def on_executor_finished(self, exit_code: int):
        """Handle executor finished"""
        self.log_tab.append_log(f"Process exited with code: {exit_code}", "#FF9800")
        self.statusBar().showMessage("Ready")
    
    def on_process_update(self, info: dict):
        """Handle process update"""
        pass
    
    def on_process_finished(self, pid: int, exit_code: int):
        """Handle process finished"""
        self.log_tab.append_log(f"Process {pid} finished with code {exit_code}", "#888")
    
    def run_executable_dialog(self):
        """Show run executable dialog"""
        from PyQt6.QtWidgets import QFileDialog
        
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Executable", "", "Windows Executables (*.exe *.msi);;All Files (*)"
        )
        
        if file_path:
            self.execute_wine_program(file_path, [])
    
    def manage_prefixes(self):
        """Show prefix manager dialog"""
        dialog = PrefixManagerDialog(self.config, self)
        dialog.exec()
    
    def run_wine_tool(self, tool: str):
        """Run Wine tool"""
        import subprocess
        
        env = os.environ.copy()
        env['WINEPREFIX'] = self.config.config['wine_prefix']
        
        try:
            subprocess.Popen([tool], env=env)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to run {tool}: {str(e)}")
    
    def show_about(self):
        """Show about dialog"""
        QMessageBox.about(
            self, "About Wine Application Manager",
            "Wine Application Manager v1.0\n\n"
            "A comprehensive GUI for managing Wine applications on Linux.\n\n"
            "Features:\n"
            "- Run Windows executables\n"
            "- Manage Wine prefixes\n"
            "- Install components via Winetricks\n"
            "- Monitor running processes\n"
            "- Configure Wine settings\n\n"
            "Built with PyQt6 and C++"
        )
    
    def apply_theme(self):
        """Apply application theme"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f5f5f5;
            }
            QTabWidget::pane {
                border: 1px solid #ddd;
                background-color: white;
            }
            QTabBar::tab {
                background-color: #e0e0e0;
                padding: 8px 16px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: white;
                border-bottom: 2px solid #2196F3;
            }
            QPushButton {
                padding: 6px 12px;
                background-color: #2196F3;
                color: white;
                border: none;
                border-radius: 3px;
            }
            QPushButton:hover {
                background-color: #1976D2;
            }
            QPushButton:pressed {
                background-color: #0D47A1;
            }
            QGroupBox {
                font-weight: bold;
                border: 1px solid #ddd;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }
        """)
    
    def closeEvent(self, event):
        """Handle window close event"""
        self.process_monitor.stop()
        self.process_monitor.wait()
        event.accept()


def main():
    """Main entry point"""
    app = QApplication(sys.argv)
    app.setApplicationName("Wine Application Manager")
    
    window = WineApplicationWindow()
    window.show()
    
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
