#!/usr/bin/env python3
"""
Wine Application Manager - Main GUI Window
"""

import sys
import os
import subprocess
import threading
from pathlib import Path
from datetime import datetime

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QLineEdit, QTextEdit, QListWidget, QTreeWidget,
    QTreeWidgetItem, QTabWidget, QGroupBox, QComboBox, QSpinBox,
    QCheckBox, QFileDialog, QMessageBox, QDialog, QDialogButtonBox,
    QTableWidget, QTableWidgetItem, QProgressBar, QSplitter, QFrame,
    QScrollArea, QGridLayout, QMenuBar, QMenu, QToolBar, QStatusBar,
    QSystemTrayIcon, QStyle
)
from PyQt6.QtCore import Qt, QTimer, QSize
from PyQt6.QtGui import QIcon, QAction, QFont, QColor, QPalette, QTextCursor

from wine_gui import (
    WineConfig, ProcessMonitorThread, WineExecutorThread,
    AddShortcutDialog, PrefixManagerDialog
)


class ConfigurationTab(QWidget):
    """Configuration tab widget"""
    
    def __init__(self, config: WineConfig):
        super().__init__()
        self.config = config
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll_widget = QWidget()
        scroll_layout = QVBoxLayout()
        
        prefix_group = QGroupBox("Wine Prefix Settings")
        prefix_layout = QGridLayout()
        
        prefix_layout.addWidget(QLabel("Wine Prefix:"), 0, 0)
        self.prefix_edit = QLineEdit(self.config.config['wine_prefix'])
        prefix_layout.addWidget(self.prefix_edit, 0, 1)
        prefix_browse = QPushButton("Browse...")
        prefix_browse.clicked.connect(self.browse_prefix)
        prefix_layout.addWidget(prefix_browse, 0, 2)
        
        prefix_layout.addWidget(QLabel("Wine Binary:"), 1, 0)
        self.wine_binary_edit = QLineEdit(self.config.config['wine_binary'])
        prefix_layout.addWidget(self.wine_binary_edit, 1, 1, 1, 2)
        
        prefix_layout.addWidget(QLabel("Architecture:"), 2, 0)
        self.arch_combo = QComboBox()
        self.arch_combo.addItems(['auto', 'win32', 'win64'])
        self.arch_combo.setCurrentText(self.config.config['architecture'])
        prefix_layout.addWidget(self.arch_combo, 2, 1, 1, 2)
        
        prefix_group.setLayout(prefix_layout)
        scroll_layout.addWidget(prefix_group)
        
        display_group = QGroupBox("Display Settings")
        display_layout = QGridLayout()
        
        self.virtual_desktop_check = QCheckBox("Enable Virtual Desktop")
        self.virtual_desktop_check.setChecked(self.config.config['enable_virtual_desktop'])
        display_layout.addWidget(self.virtual_desktop_check, 0, 0, 1, 3)
        
        display_layout.addWidget(QLabel("Resolution:"), 1, 0)
        self.resolution_edit = QLineEdit(self.config.config['virtual_desktop_resolution'])
        display_layout.addWidget(self.resolution_edit, 1, 1, 1, 2)
        
        display_layout.addWidget(QLabel("Graphics Driver:"), 2, 0)
        self.graphics_combo = QComboBox()
        self.graphics_combo.addItems(['x11', 'wayland'])
        self.graphics_combo.setCurrentText(self.config.config['graphics_driver'])
        display_layout.addWidget(self.graphics_combo, 2, 1, 1, 2)
        
        display_group.setLayout(display_layout)
        scroll_layout.addWidget(display_group)
        
        performance_group = QGroupBox("Performance Settings")
        performance_layout = QGridLayout()
        
        self.csmt_check = QCheckBox("Enable CSMT (Command Stream Multithreading)")
        self.csmt_check.setChecked(self.config.config['enable_csmt'])
        performance_layout.addWidget(self.csmt_check, 0, 0, 1, 3)
        
        self.esync_check = QCheckBox("Enable ESYNC (Event Synchronization)")
        self.esync_check.setChecked(self.config.config['enable_esync'])
        performance_layout.addWidget(self.esync_check, 1, 0, 1, 3)
        
        self.fsync_check = QCheckBox("Enable FSYNC (Fast Synchronization)")
        self.fsync_check.setChecked(self.config.config['enable_fsync'])
        performance_layout.addWidget(self.fsync_check, 2, 0, 1, 3)
        
        self.dxvk_check = QCheckBox("Enable DXVK (DirectX to Vulkan)")
        self.dxvk_check.setChecked(self.config.config['enable_dxvk'])
        performance_layout.addWidget(self.dxvk_check, 3, 0, 1, 3)
        
        performance_layout.addWidget(QLabel("Nice Level:"), 4, 0)
        self.nice_spin = QSpinBox()
        self.nice_spin.setRange(-20, 19)
        self.nice_spin.setValue(self.config.config['nice_level'])
        performance_layout.addWidget(self.nice_spin, 4, 1, 1, 2)
        
        performance_group.setLayout(performance_layout)
        scroll_layout.addWidget(performance_group)
        
        audio_group = QGroupBox("Audio Settings")
        audio_layout = QGridLayout()
        
        audio_layout.addWidget(QLabel("Audio Driver:"), 0, 0)
        self.audio_combo = QComboBox()
        self.audio_combo.addItems(['alsa', 'pulse', 'oss', 'jack'])
        self.audio_combo.setCurrentText(self.config.config['audio_driver'])
        audio_layout.addWidget(self.audio_combo, 0, 1, 1, 2)
        
        audio_group.setLayout(audio_layout)
        scroll_layout.addWidget(audio_group)
        
        debug_group = QGroupBox("Debug Settings")
        debug_layout = QGridLayout()
        
        self.debug_check = QCheckBox("Enable Debug Output")
        self.debug_check.setChecked(self.config.config['debug_output'])
        debug_layout.addWidget(self.debug_check, 0, 0, 1, 3)
        
        self.capture_stdout_check = QCheckBox("Capture Standard Output")
        self.capture_stdout_check.setChecked(self.config.config['capture_stdout'])
        debug_layout.addWidget(self.capture_stdout_check, 1, 0, 1, 3)
        
        self.capture_stderr_check = QCheckBox("Capture Standard Error")
        self.capture_stderr_check.setChecked(self.config.config['capture_stderr'])
        debug_layout.addWidget(self.capture_stderr_check, 2, 0, 1, 3)
        
        debug_group.setLayout(debug_layout)
        scroll_layout.addWidget(debug_group)
        
        scroll_layout.addStretch()
        
        button_layout = QHBoxLayout()
        save_btn = QPushButton("Save Configuration")
        save_btn.clicked.connect(self.save_configuration)
        button_layout.addWidget(save_btn)
        
        reset_btn = QPushButton("Reset to Defaults")
        reset_btn.clicked.connect(self.reset_configuration)
        button_layout.addWidget(reset_btn)
        
        button_layout.addStretch()
        scroll_layout.addLayout(button_layout)
        
        scroll_widget.setLayout(scroll_layout)
        scroll.setWidget(scroll_widget)
        
        layout.addWidget(scroll)
        self.setLayout(layout)
    
    def browse_prefix(self):
        """Browse for Wine prefix directory"""
        directory = QFileDialog.getExistingDirectory(self, "Select Wine Prefix Directory")
        if directory:
            self.prefix_edit.setText(directory)
    
    def save_configuration(self):
        """Save configuration"""
        self.config.config['wine_prefix'] = self.prefix_edit.text()
        self.config.config['wine_binary'] = self.wine_binary_edit.text()
        self.config.config['architecture'] = self.arch_combo.currentText()
        self.config.config['enable_virtual_desktop'] = self.virtual_desktop_check.isChecked()
        self.config.config['virtual_desktop_resolution'] = self.resolution_edit.text()
        self.config.config['graphics_driver'] = self.graphics_combo.currentText()
        self.config.config['enable_csmt'] = self.csmt_check.isChecked()
        self.config.config['enable_esync'] = self.esync_check.isChecked()
        self.config.config['enable_fsync'] = self.fsync_check.isChecked()
        self.config.config['enable_dxvk'] = self.dxvk_check.isChecked()
        self.config.config['nice_level'] = self.nice_spin.value()
        self.config.config['audio_driver'] = self.audio_combo.currentText()
        self.config.config['debug_output'] = self.debug_check.isChecked()
        self.config.config['capture_stdout'] = self.capture_stdout_check.isChecked()
        self.config.config['capture_stderr'] = self.capture_stderr_check.isChecked()
        
        self.config.save_config()
        
        QMessageBox.information(self, "Success", "Configuration saved successfully")
    
    def reset_configuration(self):
        """Reset configuration to defaults"""
        reply = QMessageBox.question(
            self, "Confirm Reset",
            "Are you sure you want to reset all settings to defaults?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.config.config = {
                'wine_prefix': os.path.join(Path.home(), '.wine'),
                'wine_binary': 'wine',
                'architecture': 'auto',
                'enable_virtual_desktop': False,
                'virtual_desktop_resolution': '1024x768',
                'enable_csmt': True,
                'enable_dxvk': False,
                'enable_esync': True,
                'enable_fsync': False,
                'audio_driver': 'alsa',
                'graphics_driver': 'x11',
                'nice_level': 0,
                'debug_output': False,
                'capture_stdout': True,
                'capture_stderr': True
            }
            self.config.save_config()
            
            self.prefix_edit.setText(self.config.config['wine_prefix'])
            self.wine_binary_edit.setText(self.config.config['wine_binary'])
            self.arch_combo.setCurrentText(self.config.config['architecture'])
            self.virtual_desktop_check.setChecked(self.config.config['enable_virtual_desktop'])
            self.resolution_edit.setText(self.config.config['virtual_desktop_resolution'])
            self.graphics_combo.setCurrentText(self.config.config['graphics_driver'])
            self.csmt_check.setChecked(self.config.config['enable_csmt'])
            self.esync_check.setChecked(self.config.config['enable_esync'])
            self.fsync_check.setChecked(self.config.config['enable_fsync'])
            self.dxvk_check.setChecked(self.config.config['enable_dxvk'])
            self.nice_spin.setValue(self.config.config['nice_level'])
            self.audio_combo.setCurrentText(self.config.config['audio_driver'])
            self.debug_check.setChecked(self.config.config['debug_output'])
            self.capture_stdout_check.setChecked(self.config.config['capture_stdout'])
            self.capture_stderr_check.setChecked(self.config.config['capture_stderr'])


class ApplicationsTab(QWidget):
    """Applications tab widget"""
    
    def __init__(self, config: WineConfig, parent=None):
        super().__init__(parent)
        self.config = config
        self.parent_window = parent
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        top_layout = QHBoxLayout()
        
        self.exe_edit = QLineEdit()
        self.exe_edit.setPlaceholderText("Enter .exe file path or select from shortcuts...")
        top_layout.addWidget(self.exe_edit)
        
        browse_btn = QPushButton("Browse...")
        browse_btn.clicked.connect(self.browse_executable)
        top_layout.addWidget(browse_btn)
        
        run_btn = QPushButton("Run")
        run_btn.setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 5px 15px; }")
        run_btn.clicked.connect(self.run_executable)
        top_layout.addWidget(run_btn)
        
        layout.addLayout(top_layout)
        
        args_layout = QHBoxLayout()
        args_layout.addWidget(QLabel("Arguments:"))
        self.args_edit = QLineEdit()
        self.args_edit.setPlaceholderText("Command line arguments (optional)")
        args_layout.addWidget(self.args_edit)
        layout.addLayout(args_layout)
        
        shortcuts_group = QGroupBox("Application Shortcuts")
        shortcuts_layout = QVBoxLayout()
        
        self.shortcuts_list = QListWidget()
        self.shortcuts_list.itemDoubleClicked.connect(self.shortcut_double_clicked)
        shortcuts_layout.addWidget(self.shortcuts_list)
        
        shortcuts_btn_layout = QHBoxLayout()
        
        add_shortcut_btn = QPushButton("Add Shortcut")
        add_shortcut_btn.clicked.connect(self.add_shortcut)
        shortcuts_btn_layout.addWidget(add_shortcut_btn)
        
        remove_shortcut_btn = QPushButton("Remove Shortcut")
        remove_shortcut_btn.clicked.connect(self.remove_shortcut)
        shortcuts_btn_layout.addWidget(remove_shortcut_btn)
        
        run_shortcut_btn = QPushButton("Run Selected")
        run_shortcut_btn.clicked.connect(self.run_shortcut)
        shortcuts_btn_layout.addWidget(run_shortcut_btn)
        
        shortcuts_btn_layout.addStretch()
        shortcuts_layout.addLayout(shortcuts_btn_layout)
        
        shortcuts_group.setLayout(shortcuts_layout)
        layout.addWidget(shortcuts_group)
        
        recent_group = QGroupBox("Recent Applications")
        recent_layout = QVBoxLayout()
        
        self.recent_list = QListWidget()
        recent_layout.addWidget(self.recent_list)
        
        recent_group.setLayout(recent_layout)
        layout.addWidget(recent_group)
        
        self.setLayout(layout)
        self.refresh_shortcuts()
    
    def browse_executable(self):
        """Browse for executable file"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Executable", "", "Windows Executables (*.exe *.msi);;All Files (*)"
        )
        if file_path:
            self.exe_edit.setText(file_path)
    
    def run_executable(self):
        """Run the selected executable"""
        exe_path = self.exe_edit.text()
        
        if not exe_path:
            QMessageBox.warning(self, "Warning", "Please select an executable file")
            return
        
        if not os.path.exists(exe_path):
            QMessageBox.critical(self, "Error", f"File not found: {exe_path}")
            return
        
        args = self.args_edit.text().split() if self.args_edit.text() else []
        
        # Use wine-cli for execution
        script_dir = os.path.dirname(os.path.abspath(__file__))
        cli_path = os.path.join(script_dir, 'bin', 'wine-cli')
        
        if os.path.exists(cli_path):
            # Use CLI for better integration
            if self.parent_window:
                self.parent_window.log_tab.append_log(f"Using wine-cli to run: {exe_path}", "#4CAF50")
            self.run_via_cli(cli_path, exe_path, args)
        else:
            # Fallback to direct execution
            if self.parent_window:
                self.parent_window.execute_wine_program(exe_path, args)
        
        self.add_to_recent(exe_path)
    
    def run_via_cli(self, cli_path, exe_path, args):
        """Run executable via wine-cli"""
        try:
            cmd = [cli_path, 'run', exe_path] + args
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True
            )
            
            if self.parent_window:
                self.parent_window.log_tab.append_log(f"Process started with PID: {process.pid}", "#4CAF50")
                
                # Read output in background
                def read_output():
                    for line in process.stdout:
                        if line.strip():
                            self.parent_window.log_tab.append_log(line.strip(), "#d4d4d4")
                    process.wait()
                    self.parent_window.log_tab.append_log(f"Process exited with code: {process.returncode}", "#FF9800")
                
                import threading
                thread = threading.Thread(target=read_output, daemon=True)
                thread.start()
        except Exception as e:
            if self.parent_window:
                self.parent_window.log_tab.append_log(f"Error: {str(e)}", "#f44336")
    
    def add_shortcut(self):
        """Add new shortcut"""
        dialog = AddShortcutDialog(self)
        if dialog.exec() == QDialog.DialogCode.Accepted:
            name, path = dialog.get_values()
            if name and path:
                self.config.add_shortcut(name, path)
                self.refresh_shortcuts()
    
    def remove_shortcut(self):
        """Remove selected shortcut"""
        current_item = self.shortcuts_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a shortcut to remove")
            return
        
        name = current_item.text().split(" -> ")[0]
        
        reply = QMessageBox.question(
            self, "Confirm Remove",
            f"Remove shortcut '{name}'?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.config.remove_shortcut(name)
            self.refresh_shortcuts()
    
    def run_shortcut(self):
        """Run selected shortcut"""
        current_item = self.shortcuts_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a shortcut to run")
            return
        
        name = current_item.text().split(" -> ")[0]
        path = self.config.get_shortcut(name)
        
        if path:
            self.exe_edit.setText(path)
            self.run_executable()
    
    def shortcut_double_clicked(self, item):
        """Handle double click on shortcut"""
        name = item.text().split(" -> ")[0]
        path = self.config.get_shortcut(name)
        if path:
            self.exe_edit.setText(path)
    
    def refresh_shortcuts(self):
        """Refresh shortcuts list"""
        self.shortcuts_list.clear()
        for name in self.config.list_shortcuts():
            path = self.config.get_shortcut(name)
            self.shortcuts_list.addItem(f"{name} -> {path}")
    
    def add_to_recent(self, path: str):
        """Add to recent applications"""
        for i in range(self.recent_list.count()):
            if self.recent_list.item(i).text() == path:
                return
        
        self.recent_list.insertItem(0, path)
        
        if self.recent_list.count() > 10:
            self.recent_list.takeItem(10)


class ProcessesTab(QWidget):
    """Processes tab widget"""
    
    def __init__(self):
        super().__init__()
        self.init_ui()
    
    def init_ui(self):
        """Initialize UI"""
        layout = QVBoxLayout()
        
        self.process_table = QTableWidget()
        self.process_table.setColumnCount(5)
        self.process_table.setHorizontalHeaderLabels(['PID', 'Name', 'Status', 'Start Time', 'Actions'])
        self.process_table.horizontalHeader().setStretchLastSection(True)
        layout.addWidget(self.process_table)
        
        button_layout = QHBoxLayout()
        
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self.refresh_processes)
        button_layout.addWidget(refresh_btn)
        
        kill_btn = QPushButton("Kill Selected")
        kill_btn.clicked.connect(self.kill_selected)
        button_layout.addWidget(kill_btn)
        
        killall_btn = QPushButton("Kill All Wine")
        killall_btn.setStyleSheet("QPushButton { background-color: #f44336; color: white; }")
        killall_btn.clicked.connect(self.kill_all_wine)
        button_layout.addWidget(killall_btn)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        self.setLayout(layout)
        
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self.refresh_processes)
        self.refresh_timer.start(2000)
    
    def refresh_processes(self):
        """Refresh process list"""
        import subprocess
        
        try:
            output = subprocess.check_output(['ps', 'aux'], universal_newlines=True)
            wine_processes = [line for line in output.split('\n') if 'wine' in line.lower()]
            
            self.process_table.setRowCount(len(wine_processes))
            
            for i, line in enumerate(wine_processes):
                parts = line.split()
                if len(parts) >= 11:
                    pid = parts[1]
                    cpu = parts[2]
                    mem = parts[3]
                    start_time = parts[8]
                    command = ' '.join(parts[10:])
                    
                    self.process_table.setItem(i, 0, QTableWidgetItem(pid))
                    self.process_table.setItem(i, 1, QTableWidgetItem(command[:50]))
                    self.process_table.setItem(i, 2, QTableWidgetItem(f"CPU: {cpu}% MEM: {mem}%"))
                    self.process_table.setItem(i, 3, QTableWidgetItem(start_time))
                    
                    kill_btn = QPushButton("Kill")
                    kill_btn.clicked.connect(lambda checked, p=pid: self.kill_process(p))
                    self.process_table.setCellWidget(i, 4, kill_btn)
        except Exception as e:
            pass
    
    def kill_selected(self):
        """Kill selected process"""
        current_row = self.process_table.currentRow()
        if current_row >= 0:
            pid_item = self.process_table.item(current_row, 0)
            if pid_item:
                self.kill_process(pid_item.text())
    
    def kill_process(self, pid: str):
        """Kill process by PID"""
        try:
            os.kill(int(pid), 15)
            QMessageBox.information(self, "Success", f"Sent termination signal to process {pid}")
            QTimer.singleShot(1000, self.refresh_processes)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to kill process: {str(e)}")
    
    def kill_all_wine(self):
        """Kill all Wine processes"""
        reply = QMessageBox.question(
            self, "Confirm Kill All",
            "Are you sure you want to kill all Wine processes?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            import subprocess
            try:
                subprocess.run(['killall', 'wine', 'wineserver', 'wine64'], check=False)
                QMessageBox.information(self, "Success", "Terminated all Wine processes")
                QTimer.singleShot(1000, self.refresh_processes)
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to kill processes: {str(e)}")
