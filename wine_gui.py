#!/usr/bin/env python3
"""
Wine Application Manager - PyQt6 GUI
A comprehensive GUI for managing Wine applications on Linux
"""

import sys
import os
import subprocess
import json
import configparser
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional, Any

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QLineEdit, QTextEdit, QListWidget, QTreeWidget,
    QTreeWidgetItem, QTabWidget, QGroupBox, QComboBox, QSpinBox,
    QCheckBox, QFileDialog, QMessageBox, QDialog, QDialogButtonBox,
    QTableWidget, QTableWidgetItem, QProgressBar, QSplitter, QFrame,
    QScrollArea, QGridLayout, QMenuBar, QMenu, QToolBar, QStatusBar,
    QSystemTrayIcon, QStyle, QSlider, QRadioButton, QButtonGroup
)
from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSize, QPoint, QRect,
    QProcess, QSettings, QByteArray, QObject
)
from PyQt6.QtGui import (
    QIcon, QAction, QFont, QColor, QPalette, QPixmap,
    QTextCursor, QKeySequence, QPainter, QBrush, QPen
)


class WineConfig:
    """Wine configuration management"""
    
    def __init__(self, config_dir: str = None):
        if config_dir is None:
            self.config_dir = os.path.join(Path.home(), '.config', 'wineapp')
        else:
            self.config_dir = config_dir
        
        self.config_file = os.path.join(self.config_dir, 'wine.conf')
        self.shortcuts_file = os.path.join(self.config_dir, 'shortcuts.conf')
        self.prefixes_dir = os.path.join(Path.home(), '.local', 'share', 'wineprefixes')
        
        os.makedirs(self.config_dir, exist_ok=True)
        os.makedirs(self.prefixes_dir, exist_ok=True)
        
        self.config = {
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
        
        self.shortcuts = {}
        self.load_config()
        self.load_shortcuts()
    
    def load_config(self):
        """Load configuration from file"""
        if os.path.exists(self.config_file):
            parser = configparser.ConfigParser()
            try:
                parser.read(self.config_file)
                
                if 'Wine' in parser:
                    for key, value in parser['Wine'].items():
                        if key in self.config:
                            if isinstance(self.config[key], bool):
                                self.config[key] = value.lower() in ('true', 'yes', '1')
                            elif isinstance(self.config[key], int):
                                self.config[key] = int(value)
                            else:
                                self.config[key] = value
            except configparser.MissingSectionHeaderError:
                # Config file is corrupted, recreate it
                print("Config file corrupted, recreating...")
                self.save_config()
    
    def save_config(self):
        """Save configuration to file"""
        parser = configparser.ConfigParser()
        parser['Wine'] = {}
        
        for key, value in self.config.items():
            parser['Wine'][key] = str(value)
        
        with open(self.config_file, 'w') as f:
            parser.write(f)
    
    def load_shortcuts(self):
        """Load application shortcuts"""
        if os.path.exists(self.shortcuts_file):
            parser = configparser.ConfigParser()
            try:
                parser.read(self.shortcuts_file)
                
                if 'Shortcuts' in parser:
                    self.shortcuts = dict(parser['Shortcuts'])
            except configparser.MissingSectionHeaderError:
                # Shortcuts file is corrupted, recreate it
                print("Shortcuts file corrupted, recreating...")
                self.save_shortcuts()
    
    def save_shortcuts(self):
        """Save application shortcuts"""
        parser = configparser.ConfigParser()
        parser['Shortcuts'] = self.shortcuts
        
        with open(self.shortcuts_file, 'w') as f:
            parser.write(f)
    
    def add_shortcut(self, name: str, path: str):
        """Add application shortcut"""
        self.shortcuts[name] = path
        self.save_shortcuts()
    
    def remove_shortcut(self, name: str):
        """Remove application shortcut"""
        if name in self.shortcuts:
            del self.shortcuts[name]
            self.save_shortcuts()
    
    def get_shortcut(self, name: str) -> Optional[str]:
        """Get shortcut path by name"""
        return self.shortcuts.get(name)
    
    def list_shortcuts(self) -> List[str]:
        """List all shortcut names"""
        return list(self.shortcuts.keys())


class ProcessMonitorThread(QThread):
    """Thread for monitoring Wine processes"""
    
    process_update = pyqtSignal(dict)
    process_finished = pyqtSignal(int, int)
    
    def __init__(self):
        super().__init__()
        self.running = True
        self.processes = {}
    
    def add_process(self, pid: int, name: str):
        """Add process to monitor"""
        self.processes[pid] = {
            'pid': pid,
            'name': name,
            'start_time': datetime.now(),
            'status': 'Running'
        }
    
    def remove_process(self, pid: int):
        """Remove process from monitoring"""
        if pid in self.processes:
            del self.processes[pid]
    
    def run(self):
        """Monitor processes"""
        while self.running:
            dead_pids = []
            
            for pid, info in list(self.processes.items()):
                try:
                    os.kill(pid, 0)
                    self.process_update.emit(info)
                except OSError:
                    dead_pids.append(pid)
                    self.process_finished.emit(pid, 0)
            
            for pid in dead_pids:
                self.remove_process(pid)
            
            self.msleep(1000)
    
    def stop(self):
        """Stop monitoring"""
        self.running = False


class WineExecutorThread(QThread):
    """Thread for executing Wine processes"""
    
    output_ready = pyqtSignal(str)
    error_ready = pyqtSignal(str)
    process_started = pyqtSignal(int)
    process_finished = pyqtSignal(int)
    
    def __init__(self, exe_path: str, args: List[str], config: Dict[str, Any]):
        super().__init__()
        self.exe_path = exe_path
        self.args = args
        self.config = config
        self.process = None
    
    def run(self):
        """Execute Wine process"""
        self.process = QProcess()
        
        env = QProcess.systemEnvironment()
        env.append(f"WINEPREFIX={self.config['wine_prefix']}")
        
        if self.config['architecture'] == 'win32':
            env.append("WINEARCH=win32")
        elif self.config['architecture'] == 'win64':
            env.append("WINEARCH=win64")
        
        if self.config['enable_csmt']:
            env.append("CSMT=enabled")
        
        if self.config['enable_esync']:
            env.append("WINEESYNC=1")
        
        if self.config['enable_fsync']:
            env.append("WINEFSYNC=1")
        
        self.process.setEnvironment(env)
        
        self.process.readyReadStandardOutput.connect(self._handle_stdout)
        self.process.readyReadStandardError.connect(self._handle_stderr)
        self.process.finished.connect(self._handle_finished)
        
        command = [self.config['wine_binary'], self.exe_path] + self.args
        
        self.process.start(command[0], command[1:])
        
        if self.process.waitForStarted():
            pid = self.process.processId()
            self.process_started.emit(pid)
        
        self.exec()
    
    def _handle_stdout(self):
        """Handle standard output"""
        data = self.process.readAllStandardOutput()
        text = bytes(data).decode('utf-8', errors='ignore')
        self.output_ready.emit(text)
    
    def _handle_stderr(self):
        """Handle standard error"""
        data = self.process.readAllStandardError()
        text = bytes(data).decode('utf-8', errors='ignore')
        self.error_ready.emit(text)
    
    def _handle_finished(self):
        """Handle process finished"""
        exit_code = self.process.exitCode()
        self.process_finished.emit(exit_code)
    
    def terminate_process(self):
        """Terminate the process"""
        if self.process and self.process.state() == QProcess.ProcessState.Running:
            self.process.terminate()
            if not self.process.waitForFinished(3000):
                self.process.kill()


class AddShortcutDialog(QDialog):
    """Dialog for adding application shortcuts"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Add Application Shortcut")
        self.setModal(True)
        self.resize(500, 150)
        
        layout = QVBoxLayout()
        
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Shortcut Name:"))
        self.name_edit = QLineEdit()
        name_layout.addWidget(self.name_edit)
        layout.addLayout(name_layout)
        
        path_layout = QHBoxLayout()
        path_layout.addWidget(QLabel("Executable Path:"))
        self.path_edit = QLineEdit()
        path_layout.addWidget(self.path_edit)
        browse_btn = QPushButton("Browse...")
        browse_btn.clicked.connect(self.browse_file)
        path_layout.addWidget(browse_btn)
        layout.addLayout(path_layout)
        
        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | 
            QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)
        
        self.setLayout(layout)
    
    def browse_file(self):
        """Browse for executable file"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Executable", "", "Executables (*.exe *.msi);;All Files (*)"
        )
        if file_path:
            self.path_edit.setText(file_path)
    
    def get_values(self):
        """Get dialog values"""
        return self.name_edit.text(), self.path_edit.text()


class PrefixManagerDialog(QDialog):
    """Dialog for managing Wine prefixes"""
    
    def __init__(self, config: WineConfig, parent=None):
        super().__init__(parent)
        self.config = config
        self.setWindowTitle("Wine Prefix Manager")
        self.setModal(True)
        self.resize(700, 500)
        
        layout = QVBoxLayout()
        
        info_label = QLabel("Manage Wine prefixes for different applications and configurations")
        info_label.setStyleSheet("QLabel { color: #666; margin-bottom: 10px; }")
        layout.addWidget(info_label)
        
        list_layout = QHBoxLayout()
        
        self.prefix_list = QListWidget()
        self.refresh_prefix_list()
        list_layout.addWidget(self.prefix_list)
        
        button_layout = QVBoxLayout()
        
        create_btn = QPushButton("Create New")
        create_btn.clicked.connect(self.create_prefix)
        button_layout.addWidget(create_btn)
        
        delete_btn = QPushButton("Delete")
        delete_btn.clicked.connect(self.delete_prefix)
        button_layout.addWidget(delete_btn)
        
        switch_btn = QPushButton("Switch To")
        switch_btn.clicked.connect(self.switch_prefix)
        button_layout.addWidget(switch_btn)
        
        info_btn = QPushButton("Show Info")
        info_btn.clicked.connect(self.show_prefix_info)
        button_layout.addWidget(info_btn)
        
        clone_btn = QPushButton("Clone")
        clone_btn.clicked.connect(self.clone_prefix)
        button_layout.addWidget(clone_btn)
        
        button_layout.addStretch()
        
        list_layout.addLayout(button_layout)
        layout.addLayout(list_layout)
        
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)
        layout.addWidget(close_btn)
        
        self.setLayout(layout)
    
    def refresh_prefix_list(self):
        """Refresh the prefix list"""
        self.prefix_list.clear()
        
        if os.path.exists(self.config.prefixes_dir):
            for entry in os.listdir(self.config.prefixes_dir):
                prefix_path = os.path.join(self.config.prefixes_dir, entry)
                if os.path.isdir(prefix_path):
                    self.prefix_list.addItem(entry)
    
    def create_prefix(self):
        """Create new Wine prefix"""
        from PyQt6.QtWidgets import QInputDialog
        
        name, ok = QInputDialog.getText(self, "Create Prefix", "Enter prefix name:")
        if ok and name:
            prefix_path = os.path.join(self.config.prefixes_dir, name)
            
            if os.path.exists(prefix_path):
                QMessageBox.warning(self, "Error", f"Prefix '{name}' already exists")
                return
            
            os.makedirs(prefix_path, exist_ok=True)
            
            env = os.environ.copy()
            env['WINEPREFIX'] = prefix_path
            
            try:
                subprocess.run(['wineboot', '-u'], env=env, check=True)
                QMessageBox.information(self, "Success", f"Created prefix '{name}'")
                self.refresh_prefix_list()
            except subprocess.CalledProcessError:
                QMessageBox.critical(self, "Error", f"Failed to create prefix '{name}'")
    
    def delete_prefix(self):
        """Delete selected Wine prefix"""
        current_item = self.prefix_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a prefix to delete")
            return
        
        name = current_item.text()
        
        reply = QMessageBox.question(
            self, "Confirm Delete",
            f"Are you sure you want to delete prefix '{name}'?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            prefix_path = os.path.join(self.config.prefixes_dir, name)
            try:
                import shutil
                shutil.rmtree(prefix_path)
                QMessageBox.information(self, "Success", f"Deleted prefix '{name}'")
                self.refresh_prefix_list()
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to delete prefix: {str(e)}")
    
    def switch_prefix(self):
        """Switch to selected Wine prefix"""
        current_item = self.prefix_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a prefix to switch to")
            return
        
        name = current_item.text()
        prefix_path = os.path.join(self.config.prefixes_dir, name)
        
        self.config.config['wine_prefix'] = prefix_path
        self.config.save_config()
        
        QMessageBox.information(self, "Success", f"Switched to prefix '{name}'")
    
    def show_prefix_info(self):
        """Show information about selected prefix"""
        current_item = self.prefix_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a prefix")
            return
        
        name = current_item.text()
        prefix_path = os.path.join(self.config.prefixes_dir, name)
        
        info = f"Prefix: {name}\n"
        info += f"Path: {prefix_path}\n"
        
        if os.path.exists(prefix_path):
            size = self.get_directory_size(prefix_path)
            info += f"Size: {size / (1024*1024):.2f} MB\n"
        
        QMessageBox.information(self, "Prefix Information", info)
    
    def clone_prefix(self):
        """Clone selected Wine prefix"""
        current_item = self.prefix_list.currentItem()
        if not current_item:
            QMessageBox.warning(self, "Warning", "Please select a prefix to clone")
            return
        
        source_name = current_item.text()
        
        from PyQt6.QtWidgets import QInputDialog
        new_name, ok = QInputDialog.getText(self, "Clone Prefix", "Enter new prefix name:")
        
        if ok and new_name:
            source_path = os.path.join(self.config.prefixes_dir, source_name)
            dest_path = os.path.join(self.config.prefixes_dir, new_name)
            
            if os.path.exists(dest_path):
                QMessageBox.warning(self, "Error", f"Prefix '{new_name}' already exists")
                return
            
            try:
                import shutil
                shutil.copytree(source_path, dest_path)
                QMessageBox.information(self, "Success", f"Cloned prefix to '{new_name}'")
                self.refresh_prefix_list()
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to clone prefix: {str(e)}")
    
    def get_directory_size(self, path):
        """Calculate directory size"""
        total = 0
        for dirpath, dirnames, filenames in os.walk(path):
            for filename in filenames:
                filepath = os.path.join(dirpath, filename)
                if os.path.exists(filepath):
                    total += os.path.getsize(filepath)
        return total
