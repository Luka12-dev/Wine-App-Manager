#include "wine_wrapper.hpp"
#include <fstream>
#include <sstream>

namespace WineWrapper {

WineApplicationManager::WineApplicationManager()
    : logger(), monitor(logger), prefix_manager(logger), 
      executor(logger, monitor, prefix_manager), winetricks_manager(logger),
      registry_manager(nullptr) {
}

WineApplicationManager::~WineApplicationManager() {
    shutdown();
    if (registry_manager) {
        delete registry_manager;
        registry_manager = nullptr;
    }
}

bool WineApplicationManager::initialize_directories() {
    if (config_directory.empty()) {
        config_directory = Utils::get_home_directory() + "/.config/wineapp";
    }
    
    if (!Utils::create_directory(config_directory)) {
        logger.error("Failed to create config directory: " + config_directory);
        return false;
    }
    
    std::string log_directory = Utils::join_paths(config_directory, "logs");
    if (!Utils::create_directory(log_directory)) {
        logger.error("Failed to create log directory: " + log_directory);
        return false;
    }
    
    std::string shortcuts_directory = Utils::join_paths(config_directory, "shortcuts");
    if (!Utils::create_directory(shortcuts_directory)) {
        logger.error("Failed to create shortcuts directory: " + shortcuts_directory);
        return false;
    }
    
    return true;
}

bool WineApplicationManager::load_application_shortcuts() {
    std::string shortcuts_file = Utils::join_paths(config_directory, "shortcuts.conf");
    
    if (!Utils::file_exists(shortcuts_file)) {
        return true;
    }
    
    ConfigurationParser parser(shortcuts_file);
    application_shortcuts = parser.get_all_values();
    
    logger.info("Loaded " + std::to_string(application_shortcuts.size()) + " application shortcuts");
    
    return true;
}

bool WineApplicationManager::save_application_shortcuts() {
    std::string shortcuts_file = Utils::join_paths(config_directory, "shortcuts.conf");
    
    ConfigurationParser parser;
    for (const auto& pair : application_shortcuts) {
        parser.set_value(pair.first, pair.second);
    }
    
    return parser.save_to_file(shortcuts_file);
}

bool WineApplicationManager::initialize(const std::string& config_dir) {
    if (!config_dir.empty()) {
        config_directory = config_dir;
    }
    
    logger.info("Initializing Wine Application Manager");
    
    if (!initialize_directories()) {
        return false;
    }
    
    std::string log_file = Utils::join_paths(config_directory, "logs/wineapp.log");
    logger.set_log_file(log_file);
    logger.set_min_level(LogLevel::INFO);
    logger.set_console_output(true);
    
    current_config = WineConfiguration();
    current_config.apply_defaults();
    current_config.validate();
    
    std::string default_config = Utils::join_paths(config_directory, "wine.conf");
    if (Utils::file_exists(default_config)) {
        current_config.load_from_file(default_config);
        logger.info("Loaded configuration from: " + default_config);
    }
    
    executor.set_configuration(current_config);
    
    monitor.start_monitoring();
    
    load_application_shortcuts();
    
    if (!current_config.wine_prefix.empty()) {
        registry_manager = new RegistryManager(current_config.wine_prefix, logger);
    }
    
    logger.info("Wine Application Manager initialized successfully");
    
    return true;
}

void WineApplicationManager::shutdown() {
    logger.info("Shutting down Wine Application Manager");
    
    save_application_shortcuts();
    
    monitor.stop_monitoring();
    
    std::string default_config = Utils::join_paths(config_directory, "wine.conf");
    current_config.save_to_file(default_config);
    
    logger.info("Wine Application Manager shutdown complete");
}

pid_t WineApplicationManager::run_executable(const std::string& exe_path, 
                                            const std::vector<std::string>& args) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    
    logger.info("Running executable: " + exe_path);
    
    pid_t pid = executor.execute(exe_path, args);
    
    if (pid > 0) {
        logger.info("Successfully started executable with PID: " + std::to_string(pid));
    } else {
        logger.error("Failed to start executable: " + exe_path);
    }
    
    return pid;
}

int WineApplicationManager::run_executable_sync(const std::string& exe_path, 
                                               const std::vector<std::string>& args) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    
    logger.info("Running executable synchronously: " + exe_path);
    
    int exit_code = executor.execute_sync(exe_path, args);
    
    logger.info("Executable exited with code: " + std::to_string(exit_code));
    
    return exit_code;
}

void WineApplicationManager::set_wine_configuration(const WineConfiguration& config) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    
    current_config = config;
    current_config.validate();
    executor.set_configuration(current_config);
    
    if (registry_manager) {
        delete registry_manager;
    }
    registry_manager = new RegistryManager(current_config.wine_prefix, logger);
    
    logger.info("Updated Wine configuration");
}

WineConfiguration WineApplicationManager::get_wine_configuration() const {
    return current_config;
}

bool WineApplicationManager::create_wine_prefix(const std::string& name) {
    logger.info("Creating Wine prefix: " + name);
    
    bool success = prefix_manager.create_prefix(name, current_config);
    
    if (success) {
        logger.info("Successfully created Wine prefix: " + name);
    } else {
        logger.error("Failed to create Wine prefix: " + name);
    }
    
    return success;
}

bool WineApplicationManager::delete_wine_prefix(const std::string& name) {
    logger.info("Deleting Wine prefix: " + name);
    
    bool success = prefix_manager.delete_prefix(name);
    
    if (success) {
        logger.info("Successfully deleted Wine prefix: " + name);
    } else {
        logger.error("Failed to delete Wine prefix: " + name);
    }
    
    return success;
}

bool WineApplicationManager::switch_wine_prefix(const std::string& name) {
    logger.info("Switching to Wine prefix: " + name);
    
    if (!prefix_manager.prefix_exists(name)) {
        logger.error("Wine prefix does not exist: " + name);
        return false;
    }
    
    WineConfiguration config = prefix_manager.get_prefix_config(name);
    set_wine_configuration(config);
    
    logger.info("Switched to Wine prefix: " + name);
    
    return true;
}

std::vector<std::string> WineApplicationManager::list_wine_prefixes() {
    return prefix_manager.list_prefixes();
}

void WineApplicationManager::set_log_level(LogLevel level) {
    logger.set_min_level(level);
}

std::vector<std::string> WineApplicationManager::get_recent_logs(size_t count) {
    return logger.get_recent_logs(count);
}

ProcessInfo WineApplicationManager::get_process_info(pid_t pid) {
    return monitor.get_process_info(pid);
}

std::vector<ProcessInfo> WineApplicationManager::get_all_running_processes() {
    return monitor.get_all_processes();
}

void WineApplicationManager::terminate_process(pid_t pid) {
    logger.info("Terminating process: " + std::to_string(pid));
    monitor.kill_process(pid, SIGTERM);
}

void WineApplicationManager::kill_all_processes() {
    logger.warning("Killing all Wine processes");
    
    auto processes = monitor.get_all_processes();
    for (const auto& info : processes) {
        monitor.kill_process(info.pid, SIGKILL);
    }
}

bool WineApplicationManager::add_application_shortcut(const std::string& name, 
                                                      const std::string& exe_path) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    
    application_shortcuts[name] = exe_path;
    
    logger.info("Added application shortcut: " + name + " -> " + exe_path);
    
    return save_application_shortcuts();
}

bool WineApplicationManager::remove_application_shortcut(const std::string& name) {
    std::lock_guard<std::mutex> lock(manager_mutex);
    
    auto it = application_shortcuts.find(name);
    if (it == application_shortcuts.end()) {
        logger.warning("Application shortcut not found: " + name);
        return false;
    }
    
    application_shortcuts.erase(it);
    
    logger.info("Removed application shortcut: " + name);
    
    return save_application_shortcuts();
}

std::string WineApplicationManager::get_application_path(const std::string& name) {
    auto it = application_shortcuts.find(name);
    if (it != application_shortcuts.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> WineApplicationManager::list_application_shortcuts() {
    std::vector<std::string> names;
    for (const auto& pair : application_shortcuts) {
        names.push_back(pair.first);
    }
    return names;
}

bool WineApplicationManager::install_winetricks_component(const std::string& component) {
    logger.info("Installing winetricks component: " + component);
    
    bool success = winetricks_manager.install_verb(component, current_config.wine_prefix);
    
    if (success) {
        logger.info("Successfully installed component: " + component);
    } else {
        logger.error("Failed to install component: " + component);
    }
    
    return success;
}

std::vector<std::string> WineApplicationManager::list_available_components() {
    return winetricks_manager.list_available_verbs();
}

std::map<std::string, std::string> WineApplicationManager::get_system_info() {
    std::map<std::string, std::string> info;
    
    info["wine_version"] = executor.get_wine_version();
    info["wine_prefix"] = current_config.wine_prefix;
    info["architecture"] = (current_config.architecture == WineArchitecture::WIN32) ? "Win32" :
                          (current_config.architecture == WineArchitecture::WIN64) ? "Win64" : "Auto";
    
    auto system_stats = monitor.get_system_stats();
    for (const auto& pair : system_stats) {
        info[pair.first] = std::to_string(pair.second);
    }
    
    info["config_directory"] = config_directory;
    info["log_file"] = Utils::join_paths(config_directory, "logs/wineapp.log");
    
    auto prefixes = prefix_manager.list_prefixes();
    info["prefix_count"] = std::to_string(prefixes.size());
    
    auto processes = monitor.get_all_processes();
    info["running_processes"] = std::to_string(processes.size());
    
    return info;
}

std::string WineApplicationManager::get_version() {
    return "WineApp 1.0.0";
}

}
