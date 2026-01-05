#ifndef WINE_WRAPPER_HPP
#define WINE_WRAPPER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <regex>

namespace WineWrapper {

enum class ProcessState {
    IDLE,
    STARTING,
    RUNNING,
    PAUSED,
    STOPPING,
    STOPPED,
    ERROR,
    KILLED
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR_LOG,
    CRITICAL
};

enum class WineArchitecture {
    WIN32,
    WIN64,
    AUTO_DETECT
};

struct ProcessInfo {
    pid_t pid;
    ProcessState state;
    std::string executable_path;
    std::vector<std::string> arguments;
    std::map<std::string, std::string> environment;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    int exit_code;
    std::string stdout_data;
    std::string stderr_data;
    size_t memory_usage;
    double cpu_usage;
    std::string wine_prefix;
    WineArchitecture architecture;
};

struct WineConfiguration {
    std::string wine_prefix;
    std::string wine_binary;
    WineArchitecture architecture;
    std::map<std::string, std::string> environment_variables;
    std::map<std::string, std::string> registry_overrides;
    std::vector<std::string> dll_overrides;
    std::string virtual_desktop_resolution;
    bool enable_virtual_desktop;
    bool enable_csmt;
    bool enable_dxvk;
    bool enable_esync;
    bool enable_fsync;
    std::string audio_driver;
    std::string graphics_driver;
    int nice_level;
    std::vector<std::string> winetricks_components;
    bool debug_output;
    std::string log_file;
    int max_log_size_mb;
    bool capture_stdout;
    bool capture_stderr;
    
    WineConfiguration();
    void load_from_file(const std::string& config_file);
    void save_to_file(const std::string& config_file) const;
    std::string to_string() const;
    void validate();
    void apply_defaults();
    bool is_valid() const;
};

class Logger {
private:
    std::ofstream log_file;
    LogLevel min_level;
    std::mutex log_mutex;
    std::string log_file_path;
    size_t max_file_size;
    bool console_output;
    std::queue<std::string> log_buffer;
    size_t max_buffer_size;
    std::atomic<bool> async_logging;
    std::thread logging_thread;
    std::condition_variable log_cv;
    std::atomic<bool> stop_logging;
    
    void rotate_log_file();
    void async_log_worker();
    std::string format_log_message(LogLevel level, const std::string& message);
    std::string get_timestamp();
    std::string level_to_string(LogLevel level);
    
public:
    Logger();
    Logger(const std::string& file_path, LogLevel level = LogLevel::INFO);
    ~Logger();
    
    void set_log_file(const std::string& file_path);
    void set_min_level(LogLevel level);
    void set_console_output(bool enabled);
    void set_max_file_size(size_t size_mb);
    void enable_async_logging(bool enabled);
    
    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void flush();
    std::vector<std::string> get_recent_logs(size_t count);
    void clear_logs();
};

class WinePrefixManager {
private:
    std::string base_prefix_directory;
    std::map<std::string, WineConfiguration> prefix_configs;
    std::mutex prefix_mutex;
    Logger& logger;
    
    bool create_directory_structure(const std::string& prefix_path);
    bool initialize_registry(const std::string& prefix_path, WineArchitecture arch);
    bool install_components(const std::string& prefix_path, const std::vector<std::string>& components);
    std::string get_wine_version(const std::string& wine_binary);
    bool verify_prefix_integrity(const std::string& prefix_path);
    void backup_prefix(const std::string& prefix_path);
    void restore_prefix(const std::string& prefix_path, const std::string& backup_path);
    
public:
    WinePrefixManager(Logger& log);
    ~WinePrefixManager();
    
    bool create_prefix(const std::string& prefix_name, const WineConfiguration& config);
    bool delete_prefix(const std::string& prefix_name);
    bool update_prefix(const std::string& prefix_name, const WineConfiguration& config);
    std::vector<std::string> list_prefixes();
    WineConfiguration get_prefix_config(const std::string& prefix_name);
    bool prefix_exists(const std::string& prefix_name);
    std::string get_prefix_path(const std::string& prefix_name);
    void set_base_directory(const std::string& directory);
    bool validate_prefix(const std::string& prefix_name);
    size_t get_prefix_size(const std::string& prefix_name);
    void cleanup_prefix(const std::string& prefix_name);
    bool clone_prefix(const std::string& source, const std::string& destination);
    std::map<std::string, std::string> get_prefix_info(const std::string& prefix_name);
};

class ProcessMonitor {
private:
    std::map<pid_t, ProcessInfo> monitored_processes;
    std::mutex monitor_mutex;
    std::atomic<bool> monitoring_active;
    std::thread monitor_thread;
    Logger& logger;
    std::vector<std::function<void(const ProcessInfo&)>> state_change_callbacks;
    std::chrono::milliseconds update_interval;
    
    void monitor_loop();
    void update_process_stats(ProcessInfo& info);
    double calculate_cpu_usage(pid_t pid);
    size_t get_memory_usage(pid_t pid);
    ProcessState get_process_state(pid_t pid);
    std::string read_process_stdout(pid_t pid);
    std::string read_process_stderr(pid_t pid);
    void notify_state_change(const ProcessInfo& info);
    bool is_process_alive(pid_t pid);
    
public:
    ProcessMonitor(Logger& log);
    ~ProcessMonitor();
    
    void start_monitoring();
    void stop_monitoring();
    void add_process(pid_t pid, const ProcessInfo& info);
    void remove_process(pid_t pid);
    ProcessInfo get_process_info(pid_t pid);
    std::vector<ProcessInfo> get_all_processes();
    void register_callback(std::function<void(const ProcessInfo&)> callback);
    void clear_callbacks();
    void set_update_interval(std::chrono::milliseconds interval);
    bool is_process_monitored(pid_t pid);
    void pause_process(pid_t pid);
    void resume_process(pid_t pid);
    void kill_process(pid_t pid, int signal = SIGTERM);
    std::map<std::string, double> get_system_stats();
};

class WineExecutor {
private:
    WineConfiguration config;
    Logger& logger;
    ProcessMonitor& monitor;
    WinePrefixManager& prefix_manager;
    std::atomic<bool> execution_active;
    std::map<std::string, std::string> custom_environment;
    std::vector<std::string> pre_launch_commands;
    std::vector<std::string> post_launch_commands;
    int stdout_pipe[2];
    int stderr_pipe[2];
    pid_t current_process_pid;
    std::mutex execution_mutex;
    
    bool setup_environment();
    bool setup_pipes();
    void close_pipes();
    std::vector<std::string> build_wine_command(const std::string& exe_path, const std::vector<std::string>& args);
    bool execute_pre_launch_commands();
    bool execute_post_launch_commands();
    void setup_dll_overrides();
    void setup_registry_settings();
    bool validate_executable(const std::string& exe_path);
    std::string resolve_path(const std::string& path);
    void setup_graphics_environment();
    void setup_audio_environment();
    char** build_environment_array();
    void free_environment_array(char** env);
    void handle_child_process(const std::vector<std::string>& command);
    int wait_for_process(pid_t pid);
    
public:
    WineExecutor(Logger& log, ProcessMonitor& mon, WinePrefixManager& pm);
    ~WineExecutor();
    
    void set_configuration(const WineConfiguration& cfg);
    WineConfiguration get_configuration() const;
    pid_t execute(const std::string& exe_path, const std::vector<std::string>& arguments = {});
    bool execute_async(const std::string& exe_path, const std::vector<std::string>& arguments = {});
    int execute_sync(const std::string& exe_path, const std::vector<std::string>& arguments = {});
    void add_environment_variable(const std::string& key, const std::string& value);
    void remove_environment_variable(const std::string& key);
    void clear_environment_variables();
    void add_pre_launch_command(const std::string& command);
    void add_post_launch_command(const std::string& command);
    void clear_pre_launch_commands();
    void clear_post_launch_commands();
    bool is_executing() const;
    pid_t get_current_pid() const;
    void terminate_current_process();
    std::string get_wine_version();
    std::vector<std::string> get_installed_dlls();
    bool install_component(const std::string& component);
    std::map<std::string, std::string> get_wine_info();
};

class RegistryManager {
private:
    std::string prefix_path;
    Logger& logger;
    std::map<std::string, std::map<std::string, std::string>> registry_cache;
    std::mutex registry_mutex;
    
    std::string get_registry_file_path(const std::string& hive);
    bool parse_registry_file(const std::string& file_path);
    bool write_registry_file(const std::string& file_path);
    std::string escape_registry_value(const std::string& value);
    std::string unescape_registry_value(const std::string& value);
    bool execute_regedit_command(const std::string& command);
    
public:
    RegistryManager(const std::string& prefix, Logger& log);
    ~RegistryManager();
    
    bool set_value(const std::string& key, const std::string& name, const std::string& value);
    std::string get_value(const std::string& key, const std::string& name);
    bool delete_value(const std::string& key, const std::string& name);
    bool create_key(const std::string& key);
    bool delete_key(const std::string& key);
    bool key_exists(const std::string& key);
    std::vector<std::string> list_keys(const std::string& parent_key);
    std::vector<std::string> list_values(const std::string& key);
    bool import_registry_file(const std::string& reg_file);
    bool export_registry_file(const std::string& reg_file, const std::string& key = "");
    void clear_cache();
    void refresh_cache();
};

class WinetricksManager {
private:
    std::string winetricks_path;
    Logger& logger;
    std::vector<std::string> available_verbs;
    std::map<std::string, std::vector<std::string>> verb_categories;
    std::mutex winetricks_mutex;
    
    bool find_winetricks_executable();
    bool update_verb_list();
    std::string execute_winetricks_command(const std::string& command);
    bool parse_verb_output(const std::string& output);
    
public:
    WinetricksManager(Logger& log);
    ~WinetricksManager();
    
    bool install_verb(const std::string& verb, const std::string& prefix);
    bool uninstall_verb(const std::string& verb, const std::string& prefix);
    std::vector<std::string> list_installed_verbs(const std::string& prefix);
    std::vector<std::string> list_available_verbs();
    std::vector<std::string> list_verbs_by_category(const std::string& category);
    std::vector<std::string> list_categories();
    bool is_verb_installed(const std::string& verb, const std::string& prefix);
    std::string get_verb_description(const std::string& verb);
    bool update_winetricks();
    std::string get_winetricks_version();
};

class WineApplicationManager {
private:
    Logger logger;
    ProcessMonitor monitor;
    WinePrefixManager prefix_manager;
    WineExecutor executor;
    RegistryManager* registry_manager;
    WinetricksManager winetricks_manager;
    WineConfiguration current_config;
    std::string config_directory;
    std::map<std::string, std::string> application_shortcuts;
    std::mutex manager_mutex;
    
    bool initialize_directories();
    bool load_application_shortcuts();
    bool save_application_shortcuts();
    
public:
    WineApplicationManager();
    ~WineApplicationManager();
    
    bool initialize(const std::string& config_dir = "");
    void shutdown();
    
    pid_t run_executable(const std::string& exe_path, const std::vector<std::string>& args = {});
    int run_executable_sync(const std::string& exe_path, const std::vector<std::string>& args = {});
    
    void set_wine_configuration(const WineConfiguration& config);
    WineConfiguration get_wine_configuration() const;
    
    bool create_wine_prefix(const std::string& name);
    bool delete_wine_prefix(const std::string& name);
    bool switch_wine_prefix(const std::string& name);
    std::vector<std::string> list_wine_prefixes();
    
    void set_log_level(LogLevel level);
    std::vector<std::string> get_recent_logs(size_t count);
    
    ProcessInfo get_process_info(pid_t pid);
    std::vector<ProcessInfo> get_all_running_processes();
    void terminate_process(pid_t pid);
    void kill_all_processes();
    
    bool add_application_shortcut(const std::string& name, const std::string& exe_path);
    bool remove_application_shortcut(const std::string& name);
    std::string get_application_path(const std::string& name);
    std::vector<std::string> list_application_shortcuts();
    
    bool install_winetricks_component(const std::string& component);
    std::vector<std::string> list_available_components();
    
    std::map<std::string, std::string> get_system_info();
    std::string get_version();
    
    Logger& get_logger() { return logger; }
    ProcessMonitor& get_monitor() { return monitor; }
    WinePrefixManager& get_prefix_manager() { return prefix_manager; }
    WineExecutor& get_executor() { return executor; }
    WinetricksManager& get_winetricks_manager() { return winetricks_manager; }
};

class ConfigurationParser {
private:
    std::map<std::string, std::string> config_data;
    std::string config_file_path;
    
    void trim(std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    bool parse_line(const std::string& line);
    
public:
    ConfigurationParser();
    ConfigurationParser(const std::string& file_path);
    
    bool load_from_file(const std::string& file_path);
    bool save_to_file(const std::string& file_path);
    
    void set_value(const std::string& key, const std::string& value);
    std::string get_value(const std::string& key, const std::string& default_value = "");
    bool has_key(const std::string& key);
    void remove_key(const std::string& key);
    void clear();
    
    std::map<std::string, std::string> get_all_values();
    std::vector<std::string> get_keys();
};

class PathResolver {
private:
    std::string wine_prefix;
    std::map<std::string, std::string> path_mappings;
    
public:
    PathResolver(const std::string& prefix);
    
    std::string windows_to_unix(const std::string& windows_path);
    std::string unix_to_windows(const std::string& unix_path);
    std::string resolve_drive_letter(char drive);
    bool create_drive_mapping(char drive, const std::string& unix_path);
    std::vector<std::pair<char, std::string>> get_drive_mappings();
    bool is_absolute_path(const std::string& path);
    std::string normalize_path(const std::string& path);
    bool path_exists(const std::string& path);
    std::string get_dosdevices_path();
};

namespace Utils {
    std::string execute_command(const std::string& command);
    bool file_exists(const std::string& path);
    bool directory_exists(const std::string& path);
    bool create_directory(const std::string& path);
    bool remove_directory(const std::string& path);
    bool copy_file(const std::string& source, const std::string& destination);
    bool move_file(const std::string& source, const std::string& destination);
    bool delete_file(const std::string& path);
    std::vector<std::string> list_directory(const std::string& path);
    size_t get_file_size(const std::string& path);
    size_t get_directory_size(const std::string& path);
    std::string get_home_directory();
    std::string get_current_directory();
    bool set_file_permissions(const std::string& path, mode_t mode);
    std::string read_file(const std::string& path);
    bool write_file(const std::string& path, const std::string& content);
    std::string get_extension(const std::string& path);
    std::string get_filename(const std::string& path);
    std::string get_directory(const std::string& path);
    std::string join_paths(const std::string& path1, const std::string& path2);
    bool is_executable(const std::string& path);
    std::vector<std::string> find_files(const std::string& directory, const std::string& pattern);
    std::string get_timestamp_string();
    long long get_timestamp_ms();
    void sleep_ms(int milliseconds);
    std::string encode_base64(const std::string& input);
    std::string decode_base64(const std::string& input);
    std::string calculate_md5(const std::string& input);
    std::string calculate_sha256(const std::string& input);
    bool is_process_running(pid_t pid);
    std::vector<pid_t> get_child_processes(pid_t parent_pid);
    void kill_process_tree(pid_t pid);
}

}

#endif
