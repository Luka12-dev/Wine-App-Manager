#include "wine_wrapper.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <cstdlib>

namespace WineWrapper {

WinePrefixManager::WinePrefixManager(Logger& log) : logger(log) {
    base_prefix_directory = Utils::get_home_directory() + "/.local/share/wineprefixes";
    Utils::create_directory(base_prefix_directory);
    
    auto prefixes = Utils::list_directory(base_prefix_directory);
    for (const auto& prefix_name : prefixes) {
        std::string prefix_path = Utils::join_paths(base_prefix_directory, prefix_name);
        if (Utils::directory_exists(prefix_path)) {
            WineConfiguration config;
            config.wine_prefix = prefix_path;
            std::string config_file = Utils::join_paths(prefix_path, "config.ini");
            if (Utils::file_exists(config_file)) {
                config.load_from_file(config_file);
            }
            prefix_configs[prefix_name] = config;
        }
    }
    
    logger.info("WinePrefixManager initialized with base directory: " + base_prefix_directory);
}

WinePrefixManager::~WinePrefixManager() {
    for (auto& pair : prefix_configs) {
        std::string config_file = Utils::join_paths(pair.second.wine_prefix, "config.ini");
        pair.second.save_to_file(config_file);
    }
    logger.info("WinePrefixManager shutting down");
}

bool WinePrefixManager::create_directory_structure(const std::string& prefix_path) {
    if (!Utils::create_directory(prefix_path)) {
        logger.error("Failed to create prefix directory: " + prefix_path);
        return false;
    }
    
    std::vector<std::string> subdirs = {
        "dosdevices", "drive_c", "drive_c/windows", "drive_c/windows/system32",
        "drive_c/Program Files", "drive_c/Program Files (x86)",
        "drive_c/users", "drive_c/users/Public"
    };
    
    for (const auto& subdir : subdirs) {
        std::string full_path = Utils::join_paths(prefix_path, subdir);
        if (!Utils::create_directory(full_path)) {
            logger.warning("Failed to create subdirectory: " + full_path);
        }
    }
    
    std::string dosdevices = Utils::join_paths(prefix_path, "dosdevices");
    std::string c_drive = Utils::join_paths(prefix_path, "drive_c");
    std::string c_link = Utils::join_paths(dosdevices, "c:");
    
    symlink(c_drive.c_str(), c_link.c_str());
    
    std::string z_link = Utils::join_paths(dosdevices, "z:");
    symlink("/", z_link.c_str());
    
    logger.info("Created directory structure for prefix: " + prefix_path);
    return true;
}

bool WinePrefixManager::initialize_registry(const std::string& prefix_path, WineArchitecture arch) {
    logger.info("Initializing registry for prefix: " + prefix_path);
    
    std::string wine_cmd = "WINEPREFIX=" + prefix_path + " ";
    if (arch == WineArchitecture::WIN32) {
        wine_cmd += "WINEARCH=win32 ";
    } else if (arch == WineArchitecture::WIN64) {
        wine_cmd += "WINEARCH=win64 ";
    }
    wine_cmd += "wineboot -u 2>&1";
    
    std::string output = Utils::execute_command(wine_cmd);
    logger.debug("Wineboot output: " + output);
    
    return Utils::directory_exists(Utils::join_paths(prefix_path, "system.reg"));
}

bool WinePrefixManager::install_components(const std::string& prefix_path, 
                                           const std::vector<std::string>& components) {
    if (components.empty()) return true;
    
    logger.info("Installing components for prefix: " + prefix_path);
    
    for (const auto& component : components) {
        std::string cmd = "WINEPREFIX=" + prefix_path + " winetricks -q " + component + " 2>&1";
        logger.debug("Running: " + cmd);
        std::string output = Utils::execute_command(cmd);
        logger.debug("Winetricks output: " + output);
    }
    
    return true;
}

std::string WinePrefixManager::get_wine_version(const std::string& wine_binary) {
    std::string cmd = wine_binary + " --version 2>&1";
    return Utils::execute_command(cmd);
}

bool WinePrefixManager::verify_prefix_integrity(const std::string& prefix_path) {
    std::vector<std::string> required_files = {
        "system.reg", "user.reg", "userdef.reg"
    };
    
    for (const auto& file : required_files) {
        std::string full_path = Utils::join_paths(prefix_path, file);
        if (!Utils::file_exists(full_path)) {
            logger.warning("Missing required file: " + full_path);
            return false;
        }
    }
    
    std::vector<std::string> required_dirs = {
        "dosdevices", "drive_c"
    };
    
    for (const auto& dir : required_dirs) {
        std::string full_path = Utils::join_paths(prefix_path, dir);
        if (!Utils::directory_exists(full_path)) {
            logger.warning("Missing required directory: " + full_path);
            return false;
        }
    }
    
    return true;
}

void WinePrefixManager::backup_prefix(const std::string& prefix_path) {
    std::string backup_path = prefix_path + ".backup." + Utils::get_timestamp_string();
    logger.info("Creating backup: " + backup_path);
    
    std::string cmd = "cp -r \"" + prefix_path + "\" \"" + backup_path + "\" 2>&1";
    Utils::execute_command(cmd);
}

void WinePrefixManager::restore_prefix(const std::string& prefix_path, 
                                      const std::string& backup_path) {
    logger.info("Restoring prefix from backup: " + backup_path);
    
    if (Utils::directory_exists(prefix_path)) {
        Utils::remove_directory(prefix_path);
    }
    
    std::string cmd = "cp -r \"" + backup_path + "\" \"" + prefix_path + "\" 2>&1";
    Utils::execute_command(cmd);
}

bool WinePrefixManager::create_prefix(const std::string& prefix_name, 
                                     const WineConfiguration& config) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    if (prefix_configs.find(prefix_name) != prefix_configs.end()) {
        logger.error("Prefix already exists: " + prefix_name);
        return false;
    }
    
    std::string prefix_path = Utils::join_paths(base_prefix_directory, prefix_name);
    
    logger.info("Creating Wine prefix: " + prefix_name + " at " + prefix_path);
    
    if (!create_directory_structure(prefix_path)) {
        return false;
    }
    
    WineConfiguration new_config = config;
    new_config.wine_prefix = prefix_path;
    
    if (!initialize_registry(prefix_path, new_config.architecture)) {
        logger.error("Failed to initialize registry");
        return false;
    }
    
    if (!new_config.winetricks_components.empty()) {
        install_components(prefix_path, new_config.winetricks_components);
    }
    
    std::string config_file = Utils::join_paths(prefix_path, "config.ini");
    new_config.save_to_file(config_file);
    
    prefix_configs[prefix_name] = new_config;
    
    logger.info("Successfully created prefix: " + prefix_name);
    return true;
}

bool WinePrefixManager::delete_prefix(const std::string& prefix_name) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        logger.error("Prefix not found: " + prefix_name);
        return false;
    }
    
    std::string prefix_path = it->second.wine_prefix;
    logger.info("Deleting Wine prefix: " + prefix_name);
    
    backup_prefix(prefix_path);
    
    if (Utils::remove_directory(prefix_path)) {
        prefix_configs.erase(it);
        logger.info("Successfully deleted prefix: " + prefix_name);
        return true;
    } else {
        logger.error("Failed to delete prefix directory: " + prefix_path);
        return false;
    }
}

bool WinePrefixManager::update_prefix(const std::string& prefix_name, 
                                     const WineConfiguration& config) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        logger.error("Prefix not found: " + prefix_name);
        return false;
    }
    
    logger.info("Updating prefix configuration: " + prefix_name);
    
    WineConfiguration new_config = config;
    new_config.wine_prefix = it->second.wine_prefix;
    
    std::string config_file = Utils::join_paths(new_config.wine_prefix, "config.ini");
    new_config.save_to_file(config_file);
    
    prefix_configs[prefix_name] = new_config;
    
    logger.info("Successfully updated prefix: " + prefix_name);
    return true;
}

std::vector<std::string> WinePrefixManager::list_prefixes() {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    std::vector<std::string> prefixes;
    
    for (const auto& pair : prefix_configs) {
        prefixes.push_back(pair.first);
    }
    
    return prefixes;
}

WineConfiguration WinePrefixManager::get_prefix_config(const std::string& prefix_name) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    auto it = prefix_configs.find(prefix_name);
    if (it != prefix_configs.end()) {
        return it->second;
    }
    
    return WineConfiguration();
}

bool WinePrefixManager::prefix_exists(const std::string& prefix_name) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    return prefix_configs.find(prefix_name) != prefix_configs.end();
}

std::string WinePrefixManager::get_prefix_path(const std::string& prefix_name) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    auto it = prefix_configs.find(prefix_name);
    if (it != prefix_configs.end()) {
        return it->second.wine_prefix;
    }
    
    return "";
}

void WinePrefixManager::set_base_directory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    base_prefix_directory = directory;
    Utils::create_directory(directory);
    logger.info("Set base prefix directory to: " + directory);
}

bool WinePrefixManager::validate_prefix(const std::string& prefix_name) {
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        return false;
    }
    
    return verify_prefix_integrity(it->second.wine_prefix);
}

size_t WinePrefixManager::get_prefix_size(const std::string& prefix_name) {
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        return 0;
    }
    
    return Utils::get_directory_size(it->second.wine_prefix);
}

void WinePrefixManager::cleanup_prefix(const std::string& prefix_name) {
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        logger.error("Prefix not found: " + prefix_name);
        return;
    }
    
    logger.info("Cleaning up prefix: " + prefix_name);
    
    std::vector<std::string> cleanup_dirs = {
        "drive_c/windows/temp",
        "drive_c/users/Public/Temp",
        "drive_c/windows/Installer"
    };
    
    for (const auto& dir : cleanup_dirs) {
        std::string full_path = Utils::join_paths(it->second.wine_prefix, dir);
        if (Utils::directory_exists(full_path)) {
            auto files = Utils::list_directory(full_path);
            for (const auto& file : files) {
                std::string file_path = Utils::join_paths(full_path, file);
                Utils::delete_file(file_path);
            }
        }
    }
}

bool WinePrefixManager::clone_prefix(const std::string& source, 
                                    const std::string& destination) {
    std::lock_guard<std::mutex> lock(prefix_mutex);
    
    auto it = prefix_configs.find(source);
    if (it == prefix_configs.end()) {
        logger.error("Source prefix not found: " + source);
        return false;
    }
    
    if (prefix_configs.find(destination) != prefix_configs.end()) {
        logger.error("Destination prefix already exists: " + destination);
        return false;
    }
    
    logger.info("Cloning prefix from " + source + " to " + destination);
    
    std::string source_path = it->second.wine_prefix;
    std::string dest_path = Utils::join_paths(base_prefix_directory, destination);
    
    std::string cmd = "cp -r \"" + source_path + "\" \"" + dest_path + "\" 2>&1";
    Utils::execute_command(cmd);
    
    if (Utils::directory_exists(dest_path)) {
        WineConfiguration dest_config = it->second;
        dest_config.wine_prefix = dest_path;
        prefix_configs[destination] = dest_config;
        
        std::string config_file = Utils::join_paths(dest_path, "config.ini");
        dest_config.save_to_file(config_file);
        
        logger.info("Successfully cloned prefix to: " + destination);
        return true;
    }
    
    return false;
}

std::map<std::string, std::string> WinePrefixManager::get_prefix_info(const std::string& prefix_name) {
    std::map<std::string, std::string> info;
    
    auto it = prefix_configs.find(prefix_name);
    if (it == prefix_configs.end()) {
        return info;
    }
    
    const WineConfiguration& config = it->second;
    
    info["name"] = prefix_name;
    info["path"] = config.wine_prefix;
    info["wine_binary"] = config.wine_binary;
    info["architecture"] = (config.architecture == WineArchitecture::WIN32) ? "Win32" :
                          (config.architecture == WineArchitecture::WIN64) ? "Win64" : "Auto";
    info["size"] = std::to_string(get_prefix_size(prefix_name));
    info["valid"] = verify_prefix_integrity(config.wine_prefix) ? "Yes" : "No";
    
    return info;
}

ProcessMonitor::ProcessMonitor(Logger& log) 
    : logger(log), monitoring_active(false), 
      update_interval(std::chrono::milliseconds(1000)) {
    logger.info("ProcessMonitor initialized");
}

ProcessMonitor::~ProcessMonitor() {
    stop_monitoring();
    logger.info("ProcessMonitor shutting down");
}

void ProcessMonitor::start_monitoring() {
    if (monitoring_active) {
        logger.warning("Process monitoring already active");
        return;
    }
    
    monitoring_active = true;
    monitor_thread = std::thread(&ProcessMonitor::monitor_loop, this);
    logger.info("Started process monitoring");
}

void ProcessMonitor::stop_monitoring() {
    if (!monitoring_active) {
        return;
    }
    
    monitoring_active = false;
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    logger.info("Stopped process monitoring");
}

void ProcessMonitor::monitor_loop() {
    while (monitoring_active) {
        {
            std::lock_guard<std::mutex> lock(monitor_mutex);
            
            std::vector<pid_t> dead_processes;
            
            for (auto& pair : monitored_processes) {
                if (!is_process_alive(pair.first)) {
                    pair.second.state = ProcessState::STOPPED;
                    pair.second.end_time = std::chrono::system_clock::now();
                    dead_processes.push_back(pair.first);
                    notify_state_change(pair.second);
                } else {
                    update_process_stats(pair.second);
                }
            }
            
            for (pid_t pid : dead_processes) {
                logger.info("Process " + std::to_string(pid) + " has terminated");
            }
        }
        
        std::this_thread::sleep_for(update_interval);
    }
}

void ProcessMonitor::update_process_stats(ProcessInfo& info) {
    info.cpu_usage = calculate_cpu_usage(info.pid);
    info.memory_usage = get_memory_usage(info.pid);
    info.state = get_process_state(info.pid);
}

double ProcessMonitor::calculate_cpu_usage(pid_t pid) {
    std::string stat_file = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(stat_file);
    
    if (!file.is_open()) {
        return 0.0;
    }
    
    std::string line;
    std::getline(file, line);
    
    return 0.0;
}

size_t ProcessMonitor::get_memory_usage(pid_t pid) {
    std::string status_file = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(status_file);
    
    if (!file.is_open()) {
        return 0;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line);
            std::string label;
            size_t value;
            iss >> label >> value;
            return value * 1024;
        }
    }
    
    return 0;
}

ProcessState ProcessMonitor::get_process_state(pid_t pid) {
    std::string stat_file = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(stat_file);
    
    if (!file.is_open()) {
        return ProcessState::STOPPED;
    }
    
    std::string line;
    std::getline(file, line);
    
    size_t first_paren = line.find('(');
    size_t last_paren = line.rfind(')');
    
    if (first_paren == std::string::npos || last_paren == std::string::npos) {
        return ProcessState::ERROR;
    }
    
    std::string after_comm = line.substr(last_paren + 2);
    char state = after_comm[0];
    
    switch (state) {
        case 'R': return ProcessState::RUNNING;
        case 'S': return ProcessState::RUNNING;
        case 'T': return ProcessState::PAUSED;
        case 'Z': return ProcessState::STOPPED;
        default: return ProcessState::RUNNING;
    }
}

std::string ProcessMonitor::read_process_stdout(pid_t pid) {
    return "";
}

std::string ProcessMonitor::read_process_stderr(pid_t pid) {
    return "";
}

void ProcessMonitor::notify_state_change(const ProcessInfo& info) {
    for (const auto& callback : state_change_callbacks) {
        callback(info);
    }
}

bool ProcessMonitor::is_process_alive(pid_t pid) {
    return kill(pid, 0) == 0;
}

void ProcessMonitor::add_process(pid_t pid, const ProcessInfo& info) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    monitored_processes[pid] = info;
    logger.info("Added process " + std::to_string(pid) + " to monitoring");
}

void ProcessMonitor::remove_process(pid_t pid) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    monitored_processes.erase(pid);
    logger.info("Removed process " + std::to_string(pid) + " from monitoring");
}

ProcessInfo ProcessMonitor::get_process_info(pid_t pid) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    
    auto it = monitored_processes.find(pid);
    if (it != monitored_processes.end()) {
        return it->second;
    }
    
    return ProcessInfo();
}

std::vector<ProcessInfo> ProcessMonitor::get_all_processes() {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    std::vector<ProcessInfo> processes;
    
    for (const auto& pair : monitored_processes) {
        processes.push_back(pair.second);
    }
    
    return processes;
}

void ProcessMonitor::register_callback(std::function<void(const ProcessInfo&)> callback) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    state_change_callbacks.push_back(callback);
}

void ProcessMonitor::clear_callbacks() {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    state_change_callbacks.clear();
}

void ProcessMonitor::set_update_interval(std::chrono::milliseconds interval) {
    update_interval = interval;
}

bool ProcessMonitor::is_process_monitored(pid_t pid) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    return monitored_processes.find(pid) != monitored_processes.end();
}

void ProcessMonitor::pause_process(pid_t pid) {
    if (kill(pid, SIGSTOP) == 0) {
        logger.info("Paused process " + std::to_string(pid));
    } else {
        logger.error("Failed to pause process " + std::to_string(pid));
    }
}

void ProcessMonitor::resume_process(pid_t pid) {
    if (kill(pid, SIGCONT) == 0) {
        logger.info("Resumed process " + std::to_string(pid));
    } else {
        logger.error("Failed to resume process " + std::to_string(pid));
    }
}

void ProcessMonitor::kill_process(pid_t pid, int signal) {
    if (kill(pid, signal) == 0) {
        logger.info("Sent signal " + std::to_string(signal) + " to process " + std::to_string(pid));
    } else {
        logger.error("Failed to send signal to process " + std::to_string(pid));
    }
}

std::map<std::string, double> ProcessMonitor::get_system_stats() {
    std::map<std::string, double> stats;
    
    std::ifstream loadavg("/proc/loadavg");
    if (loadavg.is_open()) {
        double load1, load5, load15;
        loadavg >> load1 >> load5 >> load15;
        stats["load_1min"] = load1;
        stats["load_5min"] = load5;
        stats["load_15min"] = load15;
    }
    
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line);
                std::string label;
                size_t value;
                iss >> label >> value;
                stats["memory_total"] = value;
            } else if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line);
                std::string label;
                size_t value;
                iss >> label >> value;
                stats["memory_available"] = value;
            }
        }
    }
    
    return stats;
}

}
