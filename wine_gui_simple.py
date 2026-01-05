#!/usr/bin/env python3
"""
Wine Application Manager - Simple GUI (Fallback)
Works without Qt platform plugins
"""

import sys
import os
import subprocess
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import configparser
from pathlib import Path


class WineAppGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Wine Application Manager")
        self.root.geometry("900x600")
        
        # Configuration
        self.config_dir = os.path.join(Path.home(), '.config', 'wineapp')
        os.makedirs(self.config_dir, exist_ok=True)
        
        self.config = {
            'wine_prefix': os.path.join(Path.home(), '.wine'),
            'wine_binary': 'wine',
            'architecture': 'auto',
        }
        
        self.load_config()
        
        # Create UI
        self.create_menu()
        self.create_widgets()
        
    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Run Executable...", command=self.browse_executable)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        tools_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Tools", menu=tools_menu)
        tools_menu.add_command(label="Wine Configuration", command=lambda: self.run_tool('winecfg'))
        tools_menu.add_command(label="Registry Editor", command=lambda: self.run_tool('regedit'))
        tools_menu.add_command(label="Task Manager", command=lambda: self.run_tool('taskmgr'))
        
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
    
    def create_widgets(self):
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(3, weight=1)
        
        # Title
        title = ttk.Label(main_frame, text="Wine Application Manager", 
                         font=('Arial', 16, 'bold'))
        title.grid(row=0, column=0, pady=10)
        
        # Run executable section
        run_frame = ttk.LabelFrame(main_frame, text="Run Windows Executable", padding="10")
        run_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=10)
        run_frame.columnconfigure(1, weight=1)
        
        ttk.Label(run_frame, text="Executable:").grid(row=0, column=0, sticky=tk.W)
        self.exe_entry = ttk.Entry(run_frame, width=50)
        self.exe_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5)
        
        ttk.Button(run_frame, text="Browse...", command=self.browse_executable).grid(row=0, column=2)
        ttk.Button(run_frame, text="Run", command=self.run_executable, 
                  style='Accent.TButton').grid(row=0, column=3, padx=5)
        
        ttk.Label(run_frame, text="Arguments:").grid(row=1, column=0, sticky=tk.W, pady=(5,0))
        self.args_entry = ttk.Entry(run_frame, width=50)
        self.args_entry.grid(row=1, column=1, columnspan=3, sticky=(tk.W, tk.E), padx=5, pady=(5,0))
        
        # Configuration section
        config_frame = ttk.LabelFrame(main_frame, text="Configuration", padding="10")
        config_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=10)
        config_frame.columnconfigure(1, weight=1)
        
        ttk.Label(config_frame, text="Wine Prefix:").grid(row=0, column=0, sticky=tk.W)
        self.prefix_entry = ttk.Entry(config_frame, width=50)
        self.prefix_entry.insert(0, self.config['wine_prefix'])
        self.prefix_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5)
        ttk.Button(config_frame, text="Browse...", 
                  command=self.browse_prefix).grid(row=0, column=2)
        
        ttk.Label(config_frame, text="Wine Binary:").grid(row=1, column=0, sticky=tk.W, pady=(5,0))
        self.binary_entry = ttk.Entry(config_frame, width=50)
        self.binary_entry.insert(0, self.config['wine_binary'])
        self.binary_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=5, pady=(5,0))
        
        ttk.Button(config_frame, text="Save Config", 
                  command=self.save_config).grid(row=1, column=2, pady=(5,0))
        
        # Log output
        log_frame = ttk.LabelFrame(main_frame, text="Output Log", padding="10")
        log_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, width=80, 
                                                   bg='#1e1e1e', fg='#d4d4d4')
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        log_btn_frame = ttk.Frame(log_frame)
        log_btn_frame.grid(row=1, column=0, sticky=tk.W, pady=(5,0))
        
        ttk.Button(log_btn_frame, text="Clear Log", 
                  command=self.clear_log).pack(side=tk.LEFT, padx=5)
        
        self.log("Wine Application Manager started")
        self.log(f"Wine Prefix: {self.config['wine_prefix']}")
        
    def browse_executable(self):
        filename = filedialog.askopenfilename(
            title="Select Windows Executable",
            filetypes=[("Windows Executables", "*.exe *.msi"), ("All Files", "*.*")]
        )
        if filename:
            self.exe_entry.delete(0, tk.END)
            self.exe_entry.insert(0, filename)
    
    def browse_prefix(self):
        dirname = filedialog.askdirectory(title="Select Wine Prefix Directory")
        if dirname:
            self.prefix_entry.delete(0, tk.END)
            self.prefix_entry.insert(0, dirname)
    
    def run_executable(self):
        exe_path = self.exe_entry.get()
        
        if not exe_path:
            messagebox.showwarning("Warning", "Please select an executable file")
            return
        
        if not os.path.exists(exe_path):
            messagebox.showerror("Error", f"File not found: {exe_path}")
            return
        
        args = self.args_entry.get().split() if self.args_entry.get() else []
        
        self.log(f"Running: {exe_path}")
        
        # Run in background thread
        thread = threading.Thread(target=self._execute_wine, args=(exe_path, args))
        thread.daemon = True
        thread.start()
    
    def _execute_wine(self, exe_path, args):
        try:
            # Get the directory where this script is located
            script_dir = os.path.dirname(os.path.abspath(__file__))
            cli_path = os.path.join(script_dir, 'bin', 'wine-cli')
            
            # Check if wine-cli exists
            if os.path.exists(cli_path):
                # Use our wine-cli for better integration
                cmd = [cli_path, '-p', self.prefix_entry.get(), 'run', exe_path] + args
                self.log(f"Using wine-cli: {exe_path}")
            else:
                # Fallback to direct wine execution
                env = os.environ.copy()
                env['WINEPREFIX'] = self.prefix_entry.get()
                cmd = [self.binary_entry.get(), exe_path] + args
                self.log(f"Using direct wine: {exe_path}")
            
            self.log(f"Command: {' '.join(cmd)}")
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )
            
            self.log(f"Process started with PID: {process.pid}")
            
            # Read output in real-time
            for line in process.stdout:
                if line.strip():
                    self.log(line.strip())
            
            process.wait()
            self.log(f"Process exited with code: {process.returncode}")
            
            if process.returncode == 0:
                self.log("✓ Execution completed successfully")
            else:
                self.log(f"✗ Execution failed with code {process.returncode}")
            
        except Exception as e:
            self.log(f"Error: {str(e)}")
            import traceback
            self.log(traceback.format_exc())
    
    def run_tool(self, tool):
        env = os.environ.copy()
        env['WINEPREFIX'] = self.prefix_entry.get()
        
        try:
            subprocess.Popen([self.binary_entry.get(), tool], env=env)
            self.log(f"Launched: {tool}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to run {tool}: {str(e)}")
    
    def log(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)
        self.log_text.update()
    
    def clear_log(self):
        self.log_text.delete(1.0, tk.END)
    
    def load_config(self):
        config_file = os.path.join(self.config_dir, 'wine.conf')
        if os.path.exists(config_file):
            parser = configparser.ConfigParser()
            parser.read(config_file)
            if 'Wine' in parser:
                for key in self.config:
                    if key in parser['Wine']:
                        self.config[key] = parser['Wine'][key]
    
    def save_config(self):
        self.config['wine_prefix'] = self.prefix_entry.get()
        self.config['wine_binary'] = self.binary_entry.get()
        
        config_file = os.path.join(self.config_dir, 'wine.conf')
        parser = configparser.ConfigParser()
        parser['Wine'] = self.config
        
        with open(config_file, 'w') as f:
            parser.write(f)
        
        messagebox.showinfo("Success", "Configuration saved")
        self.log("Configuration saved")
    
    def show_about(self):
        messagebox.showinfo(
            "About Wine Application Manager",
            "Wine Application Manager v1.0\n\n"
            "A GUI for managing Wine applications on Linux.\n\n"
            "Features:\n"
            "- Run Windows executables\n"
            "- Configure Wine settings\n"
            "- Access Wine tools\n\n"
            "Built with Python and Tkinter"
        )


def main():
    root = tk.Tk()
    
    # Set theme
    style = ttk.Style()
    try:
        style.theme_use('clam')
    except:
        pass
    
    app = WineAppGUI(root)
    root.mainloop()


if __name__ == '__main__':
    main()
