#include "wine_wrapper.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <getopt.h>

using namespace WineWrapper;

class WineApplicationCLI {
private:
    WineApplicationManager manager;
    bool verbose;
    bool quiet;
    
    void print_usage() {
        std::cout << "Wine Application Manager - Command Line Interface\n";
        std::cout << "Usage: wine-cli [OPTIONS] COMMAND [ARGS...]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -h, --help              Show this help message\n";
        std::cout << "  -v, --verbose           Enable verbose output\n";
        std::cout << "  -q, --quiet             Suppress output\n";
        std::cout << "  -c, --config DIR        Set configuration directory\n";
        std::cout << "  -p, --prefix PATH       Set Wine prefix path\n";
        std::cout << "  -a, --arch ARCH         Set architecture (win32/win64/auto)\n";
        std::cout << "\nCommands:\n";
        std::cout << "  run EXE [ARGS...]       Run an executable\n";
        std::cout << "  exec EXE [ARGS...]      Execute and wait for completion\n";
        std::cout << "  kill PID                Kill a process by PID\n";
        std::cout << "  killall                 Kill all Wine processes\n";
        std::cout << "  list-processes          List running Wine processes\n";
        std::cout << "  prefix-create NAME      Create a new Wine prefix\n";
        std::cout << "  prefix-delete NAME      Delete a Wine prefix\n";
        std::cout << "  prefix-list             List all Wine prefixes\n";
        std::cout << "  prefix-switch NAME      Switch to a Wine prefix\n";
        std::cout << "  prefix-info NAME        Show prefix information\n";
        std::cout << "  install COMPONENT       Install a winetricks component\n";
        std::cout << "  list-components         List available winetricks components\n";
        std::cout << "  shortcut-add NAME PATH  Add application shortcut\n";
        std::cout << "  shortcut-remove NAME    Remove application shortcut\n";
        std::cout << "  shortcut-list           List application shortcuts\n";
        std::cout << "  shortcut-run NAME       Run application from shortcut\n";
        std::cout << "  config-get KEY          Get configuration value\n";
        std::cout << "  config-set KEY VALUE    Set configuration value\n";
        std::cout << "  config-show             Show current configuration\n";
        std::cout << "  version                 Show version information\n";
        std::cout << "  info                    Show system information\n";
        std::cout << "  logs [COUNT]            Show recent log entries\n";
        std::cout << "\nExamples:\n";
        std::cout << "  wine-cli run /path/to/program.exe\n";
        std::cout << "  wine-cli exec /path/to/installer.exe /S\n";
        std::cout << "  wine-cli -p ~/.wine32 run notepad.exe\n";
        std::cout << "  wine-cli prefix-create gaming\n";
        std::cout << "  wine-cli install d3dx9\n";
    }
    
    void print_error(const std::string& message) {
        if (!quiet) {
            std::cerr << "Error: " << message << std::endl;
        }
    }
    
    void print_info(const std::string& message) {
        if (!quiet) {
            std::cout << message << std::endl;
        }
    }
    
    void print_verbose(const std::string& message) {
        if (verbose && !quiet) {
            std::cout << "[VERBOSE] " << message << std::endl;
        }
    }
    
    void print_process_info(const ProcessInfo& info) {
        std::cout << "PID: " << info.pid << "\n";
        std::cout << "  State: ";
        switch (info.state) {
            case ProcessState::IDLE: std::cout << "Idle"; break;
            case ProcessState::STARTING: std::cout << "Starting"; break;
            case ProcessState::RUNNING: std::cout << "Running"; break;
            case ProcessState::PAUSED: std::cout << "Paused"; break;
            case ProcessState::STOPPING: std::cout << "Stopping"; break;
            case ProcessState::STOPPED: std::cout << "Stopped"; break;
            case ProcessState::ERROR: std::cout << "Error"; break;
            case ProcessState::KILLED: std::cout << "Killed"; break;
        }
        std::cout << "\n";
        std::cout << "  Executable: " << info.executable_path << "\n";
        std::cout << "  Memory: " << (info.memory_usage / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  CPU: " << std::fixed << std::setprecision(2) << info.cpu_usage << "%\n";
    }
    
    int cmd_run(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing executable path");
            return 1;
        }
        
        std::string exe_path = argv[0];
        std::vector<std::string> args;
        
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
        
        print_verbose("Executing: " + exe_path);
        
        pid_t pid = manager.run_executable(exe_path, args);
        
        if (pid > 0) {
            print_info("Started process with PID: " + std::to_string(pid));
            return 0;
        } else {
            print_error("Failed to start process");
            return 1;
        }
    }
    
    int cmd_exec(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing executable path");
            return 1;
        }
        
        std::string exe_path = argv[0];
        std::vector<std::string> args;
        
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
        
        print_verbose("Executing synchronously: " + exe_path);
        
        int exit_code = manager.run_executable_sync(exe_path, args);
        
        print_info("Process exited with code: " + std::to_string(exit_code));
        
        return exit_code;
    }
    
    int cmd_kill(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing PID");
            return 1;
        }
        
        pid_t pid = std::stoi(argv[0]);
        
        print_verbose("Killing process: " + std::to_string(pid));
        
        manager.terminate_process(pid);
        print_info("Sent termination signal to process " + std::to_string(pid));
        
        return 0;
    }
    
    int cmd_killall(int argc, char** argv) {
        print_verbose("Killing all Wine processes");
        
        manager.kill_all_processes();
        print_info("Terminated all Wine processes");
        
        return 0;
    }
    
    int cmd_list_processes(int argc, char** argv) {
        auto processes = manager.get_all_running_processes();
        
        if (processes.empty()) {
            print_info("No running processes");
            return 0;
        }
        
        std::cout << "Running Wine Processes (" << processes.size() << "):\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& info : processes) {
            print_process_info(info);
            std::cout << std::string(80, '-') << "\n";
        }
        
        return 0;
    }
    
    int cmd_prefix_create(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing prefix name");
            return 1;
        }
        
        std::string name = argv[0];
        
        print_verbose("Creating Wine prefix: " + name);
        
        if (manager.create_wine_prefix(name)) {
            print_info("Successfully created prefix: " + name);
            return 0;
        } else {
            print_error("Failed to create prefix: " + name);
            return 1;
        }
    }
    
    int cmd_prefix_delete(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing prefix name");
            return 1;
        }
        
        std::string name = argv[0];
        
        print_verbose("Deleting Wine prefix: " + name);
        
        if (manager.delete_wine_prefix(name)) {
            print_info("Successfully deleted prefix: " + name);
            return 0;
        } else {
            print_error("Failed to delete prefix: " + name);
            return 1;
        }
    }
    
    int cmd_prefix_list(int argc, char** argv) {
        auto prefixes = manager.list_wine_prefixes();
        
        if (prefixes.empty()) {
            print_info("No Wine prefixes found");
            return 0;
        }
        
        std::cout << "Available Wine Prefixes (" << prefixes.size() << "):\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& name : prefixes) {
            std::cout << "  " << name << "\n";
        }
        
        return 0;
    }
    
    int cmd_prefix_switch(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing prefix name");
            return 1;
        }
        
        std::string name = argv[0];
        
        print_verbose("Switching to Wine prefix: " + name);
        
        if (manager.switch_wine_prefix(name)) {
            print_info("Switched to prefix: " + name);
            return 0;
        } else {
            print_error("Failed to switch to prefix: " + name);
            return 1;
        }
    }
    
    int cmd_prefix_info(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing prefix name");
            return 1;
        }
        
        std::string name = argv[0];
        
        auto& prefix_mgr = manager.get_prefix_manager();
        auto info = prefix_mgr.get_prefix_info(name);
        
        if (info.empty()) {
            print_error("Prefix not found: " + name);
            return 1;
        }
        
        std::cout << "Wine Prefix Information: " << name << "\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& pair : info) {
            std::cout << "  " << std::setw(20) << std::left << pair.first << ": " << pair.second << "\n";
        }
        
        return 0;
    }
    
    int cmd_install(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing component name");
            return 1;
        }
        
        std::string component = argv[0];
        
        print_verbose("Installing winetricks component: " + component);
        print_info("This may take several minutes...");
        
        if (manager.install_winetricks_component(component)) {
            print_info("Successfully installed: " + component);
            return 0;
        } else {
            print_error("Failed to install: " + component);
            return 1;
        }
    }
    
    int cmd_list_components(int argc, char** argv) {
        auto components = manager.list_available_components();
        
        if (components.empty()) {
            print_info("No components available (winetricks may not be installed)");
            return 0;
        }
        
        std::cout << "Available Winetricks Components (" << components.size() << "):\n";
        std::cout << std::string(80, '=') << "\n";
        
        int count = 0;
        for (const auto& component : components) {
            std::cout << std::setw(25) << std::left << component;
            if (++count % 3 == 0) {
                std::cout << "\n";
            }
        }
        if (count % 3 != 0) {
            std::cout << "\n";
        }
        
        return 0;
    }
    
    int cmd_shortcut_add(int argc, char** argv) {
        if (argc < 2) {
            print_error("Missing shortcut name or executable path");
            return 1;
        }
        
        std::string name = argv[0];
        std::string path = argv[1];
        
        print_verbose("Adding shortcut: " + name + " -> " + path);
        
        if (manager.add_application_shortcut(name, path)) {
            print_info("Added shortcut: " + name);
            return 0;
        } else {
            print_error("Failed to add shortcut");
            return 1;
        }
    }
    
    int cmd_shortcut_remove(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing shortcut name");
            return 1;
        }
        
        std::string name = argv[0];
        
        print_verbose("Removing shortcut: " + name);
        
        if (manager.remove_application_shortcut(name)) {
            print_info("Removed shortcut: " + name);
            return 0;
        } else {
            print_error("Failed to remove shortcut");
            return 1;
        }
    }
    
    int cmd_shortcut_list(int argc, char** argv) {
        auto shortcuts = manager.list_application_shortcuts();
        
        if (shortcuts.empty()) {
            print_info("No application shortcuts");
            return 0;
        }
        
        std::cout << "Application Shortcuts (" << shortcuts.size() << "):\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& name : shortcuts) {
            std::string path = manager.get_application_path(name);
            std::cout << "  " << std::setw(20) << std::left << name << " -> " << path << "\n";
        }
        
        return 0;
    }
    
    int cmd_shortcut_run(int argc, char** argv) {
        if (argc < 1) {
            print_error("Missing shortcut name");
            return 1;
        }
        
        std::string name = argv[0];
        std::string path = manager.get_application_path(name);
        
        if (path.empty()) {
            print_error("Shortcut not found: " + name);
            return 1;
        }
        
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
        
        print_verbose("Running shortcut: " + name);
        
        pid_t pid = manager.run_executable(path, args);
        
        if (pid > 0) {
            print_info("Started process with PID: " + std::to_string(pid));
            return 0;
        } else {
            print_error("Failed to start process");
            return 1;
        }
    }
    
    int cmd_config_show(int argc, char** argv) {
        auto config = manager.get_wine_configuration();
        
        std::cout << "Wine Configuration:\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << config.to_string();
        
        return 0;
    }
    
    int cmd_version(int argc, char** argv) {
        std::cout << manager.get_version() << "\n";
        std::cout << "Wine Version: " << manager.get_executor().get_wine_version();
        return 0;
    }
    
    int cmd_info(int argc, char** argv) {
        auto info = manager.get_system_info();
        
        std::cout << "System Information:\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& pair : info) {
            std::cout << "  " << std::setw(25) << std::left << pair.first << ": " << pair.second << "\n";
        }
        
        return 0;
    }
    
    int cmd_logs(int argc, char** argv) {
        size_t count = 50;
        
        if (argc >= 1) {
            count = std::stoi(argv[0]);
        }
        
        auto logs = manager.get_recent_logs(count);
        
        std::cout << "Recent Log Entries (" << logs.size() << "):\n";
        std::cout << std::string(80, '=') << "\n";
        
        for (const auto& log : logs) {
            std::cout << log << "\n";
        }
        
        return 0;
    }
    
public:
    WineApplicationCLI() : verbose(false), quiet(false) {}
    
    int run(int argc, char** argv) {
        std::string config_dir;
        std::string prefix_path;
        std::string architecture;
        
        static struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"verbose", no_argument,       0, 'v'},
            {"quiet",   no_argument,       0, 'q'},
            {"config",  required_argument, 0, 'c'},
            {"prefix",  required_argument, 0, 'p'},
            {"arch",    required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        while ((c = getopt_long(argc, argv, "hvqc:p:a:", long_options, &option_index)) != -1) {
            switch (c) {
                case 'h':
                    print_usage();
                    return 0;
                case 'v':
                    verbose = true;
                    break;
                case 'q':
                    quiet = true;
                    break;
                case 'c':
                    config_dir = optarg;
                    break;
                case 'p':
                    prefix_path = optarg;
                    break;
                case 'a':
                    architecture = optarg;
                    break;
                case '?':
                    return 1;
                default:
                    abort();
            }
        }
        
        if (!manager.initialize(config_dir)) {
            print_error("Failed to initialize Wine Application Manager");
            return 1;
        }
        
        if (verbose) {
            manager.set_log_level(LogLevel::DEBUG);
        }
        
        if (!prefix_path.empty() || !architecture.empty()) {
            auto config = manager.get_wine_configuration();
            
            if (!prefix_path.empty()) {
                config.wine_prefix = prefix_path;
            }
            
            if (!architecture.empty()) {
                if (architecture == "win32") {
                    config.architecture = WineArchitecture::WIN32;
                } else if (architecture == "win64") {
                    config.architecture = WineArchitecture::WIN64;
                } else {
                    config.architecture = WineArchitecture::AUTO_DETECT;
                }
            }
            
            manager.set_wine_configuration(config);
        }
        
        if (optind >= argc) {
            print_error("No command specified");
            print_usage();
            return 1;
        }
        
        std::string command = argv[optind];
        int cmd_argc = argc - optind - 1;
        char** cmd_argv = argv + optind + 1;
        
        int result = 0;
        
        if (command == "run") {
            result = cmd_run(cmd_argc, cmd_argv);
        } else if (command == "exec") {
            result = cmd_exec(cmd_argc, cmd_argv);
        } else if (command == "kill") {
            result = cmd_kill(cmd_argc, cmd_argv);
        } else if (command == "killall") {
            result = cmd_killall(cmd_argc, cmd_argv);
        } else if (command == "list-processes") {
            result = cmd_list_processes(cmd_argc, cmd_argv);
        } else if (command == "prefix-create") {
            result = cmd_prefix_create(cmd_argc, cmd_argv);
        } else if (command == "prefix-delete") {
            result = cmd_prefix_delete(cmd_argc, cmd_argv);
        } else if (command == "prefix-list") {
            result = cmd_prefix_list(cmd_argc, cmd_argv);
        } else if (command == "prefix-switch") {
            result = cmd_prefix_switch(cmd_argc, cmd_argv);
        } else if (command == "prefix-info") {
            result = cmd_prefix_info(cmd_argc, cmd_argv);
        } else if (command == "install") {
            result = cmd_install(cmd_argc, cmd_argv);
        } else if (command == "list-components") {
            result = cmd_list_components(cmd_argc, cmd_argv);
        } else if (command == "shortcut-add") {
            result = cmd_shortcut_add(cmd_argc, cmd_argv);
        } else if (command == "shortcut-remove") {
            result = cmd_shortcut_remove(cmd_argc, cmd_argv);
        } else if (command == "shortcut-list") {
            result = cmd_shortcut_list(cmd_argc, cmd_argv);
        } else if (command == "shortcut-run") {
            result = cmd_shortcut_run(cmd_argc, cmd_argv);
        } else if (command == "config-show") {
            result = cmd_config_show(cmd_argc, cmd_argv);
        } else if (command == "version") {
            result = cmd_version(cmd_argc, cmd_argv);
        } else if (command == "info") {
            result = cmd_info(cmd_argc, cmd_argv);
        } else if (command == "logs") {
            result = cmd_logs(cmd_argc, cmd_argv);
        } else {
            print_error("Unknown command: " + command);
            print_usage();
            result = 1;
        }
        
        manager.shutdown();
        
        return result;
    }
};

int main(int argc, char** argv) {
    WineApplicationCLI cli;
    return cli.run(argc, argv);
}
