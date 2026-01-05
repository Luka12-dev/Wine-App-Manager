#include "wine_wrapper.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

namespace WineWrapper {

WineExecutor::WineExecutor(Logger& log, ProcessMonitor& mon, WinePrefixManager& pm)
    : logger(log), monitor(mon), prefix_manager(pm), 
      execution_active(false), current_process_pid(-1) {
    stdout_pipe[0] = stdout_pipe[1] = -1;
    stderr_pipe[0] = stderr_pipe[1] = -1;
    logger.info("WineExecutor initialized");
}

WineExecutor::~WineExecutor() {
    close_pipes();
    logger.info("WineExecutor shutting down");
}

void WineExecutor::set_configuration(const WineConfiguration& cfg) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    config = cfg;
    config.validate();
    logger.info("Wine configuration updated");
}

WineConfiguration WineExecutor::get_configuration() const {
    return config;
}

bool WineExecutor::setup_environment() {
    setenv("WINEPREFIX", config.wine_prefix.c_str(), 1);
    
    if (config.architecture == WineArchitecture::WIN32) {
        setenv("WINEARCH", "win32", 1);
    } else if (config.architecture == WineArchitecture::WIN64) {
        setenv("WINEARCH", "win64", 1);
    }
    
    if (config.enable_virtual_desktop && !config.virtual_desktop_resolution.empty()) {
        setenv("WINE_VD_RESOLUTION", config.virtual_desktop_resolution.c_str(), 1);
    }
    
    if (config.enable_csmt) {
        setenv("CSMT", "enabled", 1);
    }
    
    if (config.enable_esync) {
        setenv("WINEESYNC", "1", 1);
    }
    
    if (config.enable_fsync) {
        setenv("WINEFSYNC", "1", 1);
    }
    
    if (!config.audio_driver.empty()) {
        setenv("WINEDLLOVERRIDES", "winemapi.dll=n,b", 1);
    }
    
    for (const auto& pair : custom_environment) {
        setenv(pair.first.c_str(), pair.second.c_str(), 1);
    }
    
    for (const auto& pair : config.environment_variables) {
        setenv(pair.first.c_str(), pair.second.c_str(), 1);
    }
    
    logger.debug("Environment setup completed");
    return true;
}

bool WineExecutor::setup_pipes() {
    if (config.capture_stdout) {
        if (pipe(stdout_pipe) == -1) {
            logger.error("Failed to create stdout pipe");
            return false;
        }
        fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
    }
    
    if (config.capture_stderr) {
        if (pipe(stderr_pipe) == -1) {
            logger.error("Failed to create stderr pipe");
            close_pipes();
            return false;
        }
        fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);
    }
    
    return true;
}

void WineExecutor::close_pipes() {
    if (stdout_pipe[0] != -1) {
        close(stdout_pipe[0]);
        stdout_pipe[0] = -1;
    }
    if (stdout_pipe[1] != -1) {
        close(stdout_pipe[1]);
        stdout_pipe[1] = -1;
    }
    if (stderr_pipe[0] != -1) {
        close(stderr_pipe[0]);
        stderr_pipe[0] = -1;
    }
    if (stderr_pipe[1] != -1) {
        close(stderr_pipe[1]);
        stderr_pipe[1] = -1;
    }
}

std::vector<std::string> WineExecutor::build_wine_command(const std::string& exe_path, 
                                                          const std::vector<std::string>& args) {
    std::vector<std::string> command;
    
    command.push_back(config.wine_binary);
    command.push_back(exe_path);
    
    for (const auto& arg : args) {
        command.push_back(arg);
    }
    
    return command;
}

bool WineExecutor::execute_pre_launch_commands() {
    for (const auto& cmd : pre_launch_commands) {
        logger.debug("Executing pre-launch command: " + cmd);
        std::string output = Utils::execute_command(cmd);
        logger.debug("Pre-launch command output: " + output);
    }
    return true;
}

bool WineExecutor::execute_post_launch_commands() {
    for (const auto& cmd : post_launch_commands) {
        logger.debug("Executing post-launch command: " + cmd);
        std::string output = Utils::execute_command(cmd);
        logger.debug("Post-launch command output: " + output);
    }
    return true;
}

void WineExecutor::setup_dll_overrides() {
    if (!config.dll_overrides.empty()) {
        std::string overrides;
        for (size_t i = 0; i < config.dll_overrides.size(); ++i) {
            if (i > 0) overrides += ";";
            overrides += config.dll_overrides[i];
        }
        setenv("WINEDLLOVERRIDES", overrides.c_str(), 1);
        logger.debug("DLL overrides set: " + overrides);
    }
}

void WineExecutor::setup_registry_settings() {
    logger.debug("Setting up registry settings");
}

bool WineExecutor::validate_executable(const std::string& exe_path) {
    if (!Utils::file_exists(exe_path)) {
        logger.error("Executable not found: " + exe_path);
        return false;
    }
    
    std::string ext = Utils::get_extension(exe_path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext != ".exe" && ext != ".msi" && ext != ".bat") {
        logger.warning("Unexpected file extension: " + ext);
    }
    
    return true;
}

std::string WineExecutor::resolve_path(const std::string& path) {
    if (path.empty()) return path;
    
    if (path[0] == '~') {
        return Utils::get_home_directory() + path.substr(1);
    }
    
    if (path[0] != '/') {
        return Utils::get_current_directory() + "/" + path;
    }
    
    return path;
}

void WineExecutor::setup_graphics_environment() {
    if (config.graphics_driver == "x11") {
        setenv("DISPLAY", ":0", 0);
    } else if (config.graphics_driver == "wayland") {
        setenv("WAYLAND_DISPLAY", "wayland-0", 0);
    }
    
    if (config.enable_dxvk) {
        setenv("DXVK_HUD", "devinfo,fps", 0);
    }
}

void WineExecutor::setup_audio_environment() {
    if (config.audio_driver == "alsa") {
        setenv("WINE_AUDIO_DRIVER", "alsa", 1);
    } else if (config.audio_driver == "pulse") {
        setenv("WINE_AUDIO_DRIVER", "pulse", 1);
    } else if (config.audio_driver == "oss") {
        setenv("WINE_AUDIO_DRIVER", "oss", 1);
    }
}

char** WineExecutor::build_environment_array() {
    std::vector<std::string> env_strings;
    
    char** env_ptr = ::environ;
    
    while (env_ptr && *env_ptr) {
        env_strings.push_back(*env_ptr);
        env_ptr++;
    }
    
    char** env_array = new char*[env_strings.size() + 1];
    for (size_t i = 0; i < env_strings.size(); ++i) {
        env_array[i] = strdup(env_strings[i].c_str());
    }
    env_array[env_strings.size()] = nullptr;
    
    return env_array;
}

void WineExecutor::free_environment_array(char** env) {
    if (env == nullptr) return;
    
    for (char** ptr = env; *ptr != nullptr; ptr++) {
        free(*ptr);
    }
    delete[] env;
}

void WineExecutor::handle_child_process(const std::vector<std::string>& command) {
    if (config.capture_stdout && stdout_pipe[1] != -1) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
    }
    
    if (config.capture_stderr && stderr_pipe[1] != -1) {
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
    }
    
    if (config.nice_level != 0) {
        nice(config.nice_level);
    }
    
    std::vector<char*> argv;
    for (const auto& arg : command) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    
    char** env = build_environment_array();
    
    execvpe(command[0].c_str(), argv.data(), env);
    
    perror("execvpe failed");
    exit(1);
}

int WineExecutor::wait_for_process(pid_t pid) {
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return -WTERMSIG(status);
    }
    
    return -1;
}

pid_t WineExecutor::execute(const std::string& exe_path, 
                           const std::vector<std::string>& arguments) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    
    std::string resolved_path = resolve_path(exe_path);
    
    if (!validate_executable(resolved_path)) {
        return -1;
    }
    
    logger.info("Executing: " + resolved_path);
    
    if (!execute_pre_launch_commands()) {
        logger.error("Pre-launch commands failed");
        return -1;
    }
    
    setup_environment();
    setup_dll_overrides();
    setup_graphics_environment();
    setup_audio_environment();
    setup_registry_settings();
    
    if (!setup_pipes()) {
        return -1;
    }
    
    std::vector<std::string> command = build_wine_command(resolved_path, arguments);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        logger.error("Failed to fork process");
        close_pipes();
        return -1;
    } else if (pid == 0) {
        handle_child_process(command);
    } else {
        if (stdout_pipe[1] != -1) close(stdout_pipe[1]);
        if (stderr_pipe[1] != -1) close(stderr_pipe[1]);
        
        current_process_pid = pid;
        execution_active = true;
        
        ProcessInfo info;
        info.pid = pid;
        info.state = ProcessState::STARTING;
        info.executable_path = resolved_path;
        info.arguments = arguments;
        info.start_time = std::chrono::system_clock::now();
        info.exit_code = 0;
        info.wine_prefix = config.wine_prefix;
        info.architecture = config.architecture;
        
        monitor.add_process(pid, info);
        
        logger.info("Started process with PID: " + std::to_string(pid));
        
        return pid;
    }
    
    return -1;
}

bool WineExecutor::execute_async(const std::string& exe_path, 
                                const std::vector<std::string>& arguments) {
    pid_t pid = execute(exe_path, arguments);
    return pid > 0;
}

int WineExecutor::execute_sync(const std::string& exe_path, 
                              const std::vector<std::string>& arguments) {
    pid_t pid = execute(exe_path, arguments);
    
    if (pid <= 0) {
        return -1;
    }
    
    int exit_code = wait_for_process(pid);
    
    execute_post_launch_commands();
    
    execution_active = false;
    close_pipes();
    
    logger.info("Process " + std::to_string(pid) + " exited with code: " + std::to_string(exit_code));
    
    return exit_code;
}

void WineExecutor::add_environment_variable(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    custom_environment[key] = value;
    logger.debug("Added environment variable: " + key + "=" + value);
}

void WineExecutor::remove_environment_variable(const std::string& key) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    custom_environment.erase(key);
    logger.debug("Removed environment variable: " + key);
}

void WineExecutor::clear_environment_variables() {
    std::lock_guard<std::mutex> lock(execution_mutex);
    custom_environment.clear();
    logger.debug("Cleared custom environment variables");
}

void WineExecutor::add_pre_launch_command(const std::string& command) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    pre_launch_commands.push_back(command);
}

void WineExecutor::add_post_launch_command(const std::string& command) {
    std::lock_guard<std::mutex> lock(execution_mutex);
    post_launch_commands.push_back(command);
}

void WineExecutor::clear_pre_launch_commands() {
    std::lock_guard<std::mutex> lock(execution_mutex);
    pre_launch_commands.clear();
}

void WineExecutor::clear_post_launch_commands() {
    std::lock_guard<std::mutex> lock(execution_mutex);
    post_launch_commands.clear();
}

bool WineExecutor::is_executing() const {
    return execution_active;
}

pid_t WineExecutor::get_current_pid() const {
    return current_process_pid;
}

void WineExecutor::terminate_current_process() {
    if (current_process_pid > 0) {
        logger.info("Terminating current process: " + std::to_string(current_process_pid));
        kill(current_process_pid, SIGTERM);
    }
}

std::string WineExecutor::get_wine_version() {
    std::string cmd = config.wine_binary + " --version 2>&1";
    return Utils::execute_command(cmd);
}

std::vector<std::string> WineExecutor::get_installed_dlls() {
    std::vector<std::string> dlls;
    
    std::string system32_path = Utils::join_paths(config.wine_prefix, "drive_c/windows/system32");
    
    if (Utils::directory_exists(system32_path)) {
        auto files = Utils::list_directory(system32_path);
        for (const auto& file : files) {
            if (Utils::get_extension(file) == ".dll") {
                dlls.push_back(file);
            }
        }
    }
    
    return dlls;
}

bool WineExecutor::install_component(const std::string& component) {
    logger.info("Installing component: " + component);
    
    std::string cmd = "WINEPREFIX=" + config.wine_prefix + " winetricks -q " + component + " 2>&1";
    std::string output = Utils::execute_command(cmd);
    
    logger.debug("Install output: " + output);
    
    return true;
}

std::map<std::string, std::string> WineExecutor::get_wine_info() {
    std::map<std::string, std::string> info;
    
    info["version"] = get_wine_version();
    info["prefix"] = config.wine_prefix;
    info["binary"] = config.wine_binary;
    info["architecture"] = (config.architecture == WineArchitecture::WIN32) ? "Win32" :
                          (config.architecture == WineArchitecture::WIN64) ? "Win64" : "Auto";
    
    return info;
}

RegistryManager::RegistryManager(const std::string& prefix, Logger& log)
    : prefix_path(prefix), logger(log) {
    logger.info("RegistryManager initialized for prefix: " + prefix);
}

RegistryManager::~RegistryManager() {
    logger.info("RegistryManager shutting down");
}

std::string RegistryManager::get_registry_file_path(const std::string& hive) {
    std::string filename;
    
    if (hive == "HKEY_LOCAL_MACHINE" || hive == "HKLM") {
        filename = "system.reg";
    } else if (hive == "HKEY_CURRENT_USER" || hive == "HKCU") {
        filename = "user.reg";
    } else if (hive == "HKEY_USERS" || hive == "HKU") {
        filename = "userdef.reg";
    } else {
        return "";
    }
    
    return Utils::join_paths(prefix_path, filename);
}

bool RegistryManager::parse_registry_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        logger.error("Failed to open registry file: " + file_path);
        return false;
    }
    
    std::string line;
    std::string current_key;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_key = line.substr(1, end - 1);
            }
        } else if (!current_key.empty() && line.find('=') != std::string::npos) {
            size_t eq_pos = line.find('=');
            std::string name = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            name.erase(0, name.find_first_not_of(" \t\""));
            name.erase(name.find_last_not_of(" \t\"") + 1);
            value.erase(0, value.find_first_not_of(" \t\""));
            value.erase(value.find_last_not_of(" \t\"") + 1);
            
            registry_cache[current_key][name] = value;
        }
    }
    
    return true;
}

bool RegistryManager::write_registry_file(const std::string& file_path) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        logger.error("Failed to open registry file for writing: " + file_path);
        return false;
    }
    
    file << "REGEDIT4\n\n";
    
    for (const auto& key_pair : registry_cache) {
        file << "[" << key_pair.first << "]\n";
        for (const auto& value_pair : key_pair.second) {
            file << "\"" << value_pair.first << "\"=\"" << value_pair.second << "\"\n";
        }
        file << "\n";
    }
    
    return true;
}

std::string RegistryManager::escape_registry_value(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == '\\' || c == '"') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}

std::string RegistryManager::unescape_registry_value(const std::string& value) {
    std::string unescaped;
    bool escape_next = false;
    
    for (char c : value) {
        if (escape_next) {
            unescaped += c;
            escape_next = false;
        } else if (c == '\\') {
            escape_next = true;
        } else {
            unescaped += c;
        }
    }
    
    return unescaped;
}

bool RegistryManager::execute_regedit_command(const std::string& command) {
    std::string temp_file = "/tmp/wine_regedit_" + std::to_string(getpid()) + ".reg";
    Utils::write_file(temp_file, command);
    
    std::string cmd = "WINEPREFIX=" + prefix_path + " wine regedit " + temp_file + " 2>&1";
    std::string output = Utils::execute_command(cmd);
    
    Utils::delete_file(temp_file);
    
    return true;
}

bool RegistryManager::set_value(const std::string& key, const std::string& name, 
                               const std::string& value) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    registry_cache[key][name] = value;
    
    logger.debug("Set registry value: " + key + "\\" + name + " = " + value);
    
    std::string reg_command = "REGEDIT4\n\n[" + key + "]\n\"" + name + "\"=\"" + 
                             escape_registry_value(value) + "\"\n";
    
    return execute_regedit_command(reg_command);
}

std::string RegistryManager::get_value(const std::string& key, const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto key_it = registry_cache.find(key);
    if (key_it != registry_cache.end()) {
        auto value_it = key_it->second.find(name);
        if (value_it != key_it->second.end()) {
            return value_it->second;
        }
    }
    
    return "";
}

bool RegistryManager::delete_value(const std::string& key, const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto key_it = registry_cache.find(key);
    if (key_it != registry_cache.end()) {
        key_it->second.erase(name);
        logger.debug("Deleted registry value: " + key + "\\" + name);
        return true;
    }
    
    return false;
}

bool RegistryManager::create_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    if (registry_cache.find(key) == registry_cache.end()) {
        registry_cache[key] = std::map<std::string, std::string>();
        logger.debug("Created registry key: " + key);
        
        std::string reg_command = "REGEDIT4\n\n[" + key + "]\n";
        return execute_regedit_command(reg_command);
    }
    
    return true;
}

bool RegistryManager::delete_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    registry_cache.erase(key);
    logger.debug("Deleted registry key: " + key);
    
    std::string reg_command = "REGEDIT4\n\n[-" + key + "]\n";
    return execute_regedit_command(reg_command);
}

bool RegistryManager::key_exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    return registry_cache.find(key) != registry_cache.end();
}

std::vector<std::string> RegistryManager::list_keys(const std::string& parent_key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    std::vector<std::string> keys;
    
    for (const auto& pair : registry_cache) {
        if (pair.first.find(parent_key) == 0) {
            keys.push_back(pair.first);
        }
    }
    
    return keys;
}

std::vector<std::string> RegistryManager::list_values(const std::string& key) {
    std::lock_guard<std::mutex> lock(registry_mutex);
    std::vector<std::string> values;
    
    auto it = registry_cache.find(key);
    if (it != registry_cache.end()) {
        for (const auto& pair : it->second) {
            values.push_back(pair.first);
        }
    }
    
    return values;
}

bool RegistryManager::import_registry_file(const std::string& reg_file) {
    if (!Utils::file_exists(reg_file)) {
        logger.error("Registry file not found: " + reg_file);
        return false;
    }
    
    logger.info("Importing registry file: " + reg_file);
    
    std::string cmd = "WINEPREFIX=" + prefix_path + " wine regedit " + reg_file + " 2>&1";
    std::string output = Utils::execute_command(cmd);
    
    logger.debug("Import output: " + output);
    
    return parse_registry_file(reg_file);
}

bool RegistryManager::export_registry_file(const std::string& reg_file, const std::string& key) {
    logger.info("Exporting registry to file: " + reg_file);
    
    std::string cmd = "WINEPREFIX=" + prefix_path + " wine regedit /E " + reg_file;
    if (!key.empty()) {
        cmd += " \"" + key + "\"";
    }
    cmd += " 2>&1";
    
    std::string output = Utils::execute_command(cmd);
    logger.debug("Export output: " + output);
    
    return Utils::file_exists(reg_file);
}

void RegistryManager::clear_cache() {
    std::lock_guard<std::mutex> lock(registry_mutex);
    registry_cache.clear();
    logger.debug("Cleared registry cache");
}

void RegistryManager::refresh_cache() {
    std::lock_guard<std::mutex> lock(registry_mutex);
    registry_cache.clear();
    
    std::vector<std::string> reg_files = {"system.reg", "user.reg", "userdef.reg"};
    for (const auto& file : reg_files) {
        std::string file_path = Utils::join_paths(prefix_path, file);
        if (Utils::file_exists(file_path)) {
            parse_registry_file(file_path);
        }
    }
    
    logger.debug("Refreshed registry cache");
}

}
