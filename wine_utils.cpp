#include "wine_wrapper.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <pwd.h>

namespace WineWrapper {

WinetricksManager::WinetricksManager(Logger& log) : logger(log) {
    find_winetricks_executable();
    update_verb_list();
    logger.info("WinetricksManager initialized");
}

WinetricksManager::~WinetricksManager() {
    logger.info("WinetricksManager shutting down");
}

bool WinetricksManager::find_winetricks_executable() {
    std::vector<std::string> possible_paths = {
        "/usr/bin/winetricks",
        "/usr/local/bin/winetricks",
        Utils::get_home_directory() + "/.local/bin/winetricks"
    };
    
    for (const auto& path : possible_paths) {
        if (Utils::file_exists(path) && Utils::is_executable(path)) {
            winetricks_path = path;
            logger.info("Found winetricks at: " + path);
            return true;
        }
    }
    
    std::string which_output = Utils::execute_command("which winetricks 2>&1");
    if (!which_output.empty() && which_output.find("not found") == std::string::npos) {
        winetricks_path = which_output;
        winetricks_path.erase(winetricks_path.find_last_not_of(" \n\r\t") + 1);
        logger.info("Found winetricks via which: " + winetricks_path);
        return true;
    }
    
    logger.warning("Winetricks not found");
    return false;
}

bool WinetricksManager::update_verb_list() {
    if (winetricks_path.empty()) {
        return false;
    }
    
    std::string cmd = winetricks_path + " list-all 2>&1";
    std::string output = Utils::execute_command(cmd);
    
    return parse_verb_output(output);
}

std::string WinetricksManager::execute_winetricks_command(const std::string& command) {
    if (winetricks_path.empty()) {
        logger.error("Winetricks executable not found");
        return "";
    }
    
    std::string cmd = winetricks_path + " " + command + " 2>&1";
    return Utils::execute_command(cmd);
}

bool WinetricksManager::parse_verb_output(const std::string& output) {
    std::istringstream stream(output);
    std::string line;
    
    available_verbs.clear();
    
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t space_pos = line.find(' ');
        if (space_pos != std::string::npos) {
            std::string verb = line.substr(0, space_pos);
            available_verbs.push_back(verb);
        }
    }
    
    logger.info("Loaded " + std::to_string(available_verbs.size()) + " winetricks verbs");
    return true;
}

bool WinetricksManager::install_verb(const std::string& verb, const std::string& prefix) {
    std::lock_guard<std::mutex> lock(winetricks_mutex);
    
    logger.info("Installing winetricks verb: " + verb + " in prefix: " + prefix);
    
    std::string cmd = "WINEPREFIX=" + prefix + " " + verb + " -q";
    std::string output = execute_winetricks_command(cmd);
    
    logger.debug("Winetricks output: " + output);
    
    return output.find("error") == std::string::npos;
}

bool WinetricksManager::uninstall_verb(const std::string& verb, const std::string& prefix) {
    std::lock_guard<std::mutex> lock(winetricks_mutex);
    
    logger.info("Uninstalling winetricks verb: " + verb + " from prefix: " + prefix);
    
    std::string cmd = "WINEPREFIX=" + prefix + " " + verb + " -q --uninstall";
    std::string output = execute_winetricks_command(cmd);
    
    logger.debug("Winetricks output: " + output);
    
    return true;
}

std::vector<std::string> WinetricksManager::list_installed_verbs(const std::string& prefix) {
    std::vector<std::string> installed;
    
    std::string log_file = prefix + "/winetricks.log";
    if (Utils::file_exists(log_file)) {
        std::ifstream file(log_file);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                installed.push_back(line);
            }
        }
    }
    
    return installed;
}

std::vector<std::string> WinetricksManager::list_available_verbs() {
    return available_verbs;
}

std::vector<std::string> WinetricksManager::list_verbs_by_category(const std::string& category) {
    std::vector<std::string> verbs;
    
    auto it = verb_categories.find(category);
    if (it != verb_categories.end()) {
        return it->second;
    }
    
    return verbs;
}

std::vector<std::string> WinetricksManager::list_categories() {
    std::vector<std::string> categories;
    
    for (const auto& pair : verb_categories) {
        categories.push_back(pair.first);
    }
    
    return categories;
}

bool WinetricksManager::is_verb_installed(const std::string& verb, const std::string& prefix) {
    auto installed = list_installed_verbs(prefix);
    return std::find(installed.begin(), installed.end(), verb) != installed.end();
}

std::string WinetricksManager::get_verb_description(const std::string& verb) {
    std::string cmd = verb + " --help";
    return execute_winetricks_command(cmd);
}

bool WinetricksManager::update_winetricks() {
    logger.info("Updating winetricks");
    
    std::string cmd = "--self-update";
    std::string output = execute_winetricks_command(cmd);
    
    logger.debug("Update output: " + output);
    
    return update_verb_list();
}

std::string WinetricksManager::get_winetricks_version() {
    std::string cmd = "--version";
    return execute_winetricks_command(cmd);
}

ConfigurationParser::ConfigurationParser() {}

ConfigurationParser::ConfigurationParser(const std::string& file_path) {
    load_from_file(file_path);
}

void ConfigurationParser::trim(std::string& str) {
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
}

std::vector<std::string> ConfigurationParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool ConfigurationParser::parse_line(const std::string& line) {
    if (line.empty() || line[0] == '#' || line[0] == ';') {
        return true;
    }
    
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
        return false;
    }
    
    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);
    
    trim(key);
    trim(value);
    
    config_data[key] = value;
    
    return true;
}

bool ConfigurationParser::load_from_file(const std::string& file_path) {
    config_file_path = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    config_data.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        parse_line(line);
    }
    
    return true;
}

bool ConfigurationParser::save_to_file(const std::string& file_path) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : config_data) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    return true;
}

void ConfigurationParser::set_value(const std::string& key, const std::string& value) {
    config_data[key] = value;
}

std::string ConfigurationParser::get_value(const std::string& key, const std::string& default_value) {
    auto it = config_data.find(key);
    if (it != config_data.end()) {
        return it->second;
    }
    return default_value;
}

bool ConfigurationParser::has_key(const std::string& key) {
    return config_data.find(key) != config_data.end();
}

void ConfigurationParser::remove_key(const std::string& key) {
    config_data.erase(key);
}

void ConfigurationParser::clear() {
    config_data.clear();
}

std::map<std::string, std::string> ConfigurationParser::get_all_values() {
    return config_data;
}

std::vector<std::string> ConfigurationParser::get_keys() {
    std::vector<std::string> keys;
    for (const auto& pair : config_data) {
        keys.push_back(pair.first);
    }
    return keys;
}

PathResolver::PathResolver(const std::string& prefix) : wine_prefix(prefix) {
    std::string dosdevices = Utils::join_paths(prefix, "dosdevices");
    
    if (Utils::directory_exists(dosdevices)) {
        auto entries = Utils::list_directory(dosdevices);
        for (const auto& entry : entries) {
            if (entry.length() == 2 && entry[1] == ':') {
                char drive = entry[0];
                std::string link_path = Utils::join_paths(dosdevices, entry);
                
                char resolved_path[PATH_MAX];
                ssize_t len = readlink(link_path.c_str(), resolved_path, sizeof(resolved_path) - 1);
                if (len != -1) {
                    resolved_path[len] = '\0';
                    path_mappings[std::string(1, drive)] = resolved_path;
                }
            }
        }
    }
}

std::string PathResolver::windows_to_unix(const std::string& windows_path) {
    if (windows_path.length() < 3 || windows_path[1] != ':') {
        return windows_path;
    }
    
    char drive = std::toupper(windows_path[0]);
    std::string unix_path = resolve_drive_letter(drive);
    
    if (unix_path.empty()) {
        return windows_path;
    }
    
    std::string path_part = windows_path.substr(2);
    std::replace(path_part.begin(), path_part.end(), '\\', '/');
    
    return unix_path + path_part;
}

std::string PathResolver::unix_to_windows(const std::string& unix_path) {
    for (const auto& pair : path_mappings) {
        if (unix_path.find(pair.second) == 0) {
            std::string windows_path = pair.first + ":" + unix_path.substr(pair.second.length());
            std::replace(windows_path.begin(), windows_path.end(), '/', '\\');
            return windows_path;
        }
    }
    
    return "Z:" + unix_path;
}

std::string PathResolver::resolve_drive_letter(char drive) {
    std::string key(1, std::toupper(drive));
    auto it = path_mappings.find(key);
    if (it != path_mappings.end()) {
        return it->second;
    }
    return "";
}

bool PathResolver::create_drive_mapping(char drive, const std::string& unix_path) {
    std::string dosdevices = Utils::join_paths(wine_prefix, "dosdevices");
    std::string drive_letter(1, std::tolower(drive));
    std::string link_path = Utils::join_paths(dosdevices, drive_letter + ":");
    
    if (symlink(unix_path.c_str(), link_path.c_str()) == 0) {
        path_mappings[std::string(1, std::toupper(drive))] = unix_path;
        return true;
    }
    
    return false;
}

std::vector<std::pair<char, std::string>> PathResolver::get_drive_mappings() {
    std::vector<std::pair<char, std::string>> mappings;
    for (const auto& pair : path_mappings) {
        mappings.push_back({pair.first[0], pair.second});
    }
    return mappings;
}

bool PathResolver::is_absolute_path(const std::string& path) {
    if (path.empty()) return false;
    
    if (path[0] == '/') return true;
    
    if (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    
    return false;
}

std::string PathResolver::normalize_path(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    while (normalized.find("//") != std::string::npos) {
        normalized.replace(normalized.find("//"), 2, "/");
    }
    
    return normalized;
}

bool PathResolver::path_exists(const std::string& path) {
    std::string unix_path = windows_to_unix(path);
    return Utils::file_exists(unix_path) || Utils::directory_exists(unix_path);
}

std::string PathResolver::get_dosdevices_path() {
    return Utils::join_paths(wine_prefix, "dosdevices");
}

namespace Utils {

std::string execute_command(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[128];
    std::string result;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

bool directory_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool create_directory(const std::string& path) {
    if (directory_exists(path)) {
        return true;
    }
    
    std::string current_path;
    std::istringstream path_stream(path);
    std::string part;
    
    while (std::getline(path_stream, part, '/')) {
        if (part.empty()) {
            current_path = "/";
            continue;
        }
        
        current_path += part + "/";
        
        if (!directory_exists(current_path)) {
            if (mkdir(current_path.c_str(), 0755) != 0) {
                if (errno != EEXIST) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool remove_directory(const std::string& path) {
    std::string cmd = "rm -rf \"" + path + "\" 2>&1";
    execute_command(cmd);
    return !directory_exists(path);
}

bool copy_file(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(destination, std::ios::binary);
    
    if (!src.is_open() || !dst.is_open()) {
        return false;
    }
    
    dst << src.rdbuf();
    
    return true;
}

bool move_file(const std::string& source, const std::string& destination) {
    return rename(source.c_str(), destination.c_str()) == 0;
}

bool delete_file(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> entries;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return entries;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            entries.push_back(name);
        }
    }
    
    closedir(dir);
    return entries;
}

size_t get_file_size(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

size_t get_directory_size(const std::string& path) {
    size_t total_size = 0;
    
    auto entries = list_directory(path);
    for (const auto& entry : entries) {
        std::string full_path = join_paths(path, entry);
        
        if (directory_exists(full_path)) {
            total_size += get_directory_size(full_path);
        } else {
            total_size += get_file_size(full_path);
        }
    }
    
    return total_size;
}

std::string get_home_directory() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home);
    }
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        return std::string(pw->pw_dir);
    }
    
    return "/tmp";
}

std::string get_current_directory() {
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        return std::string(buffer);
    }
    return "";
}

bool set_file_permissions(const std::string& path, mode_t mode) {
    return chmod(path.c_str(), mode) == 0;
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return true;
}

std::string get_extension(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos && dot_pos > path.find_last_of('/')) {
        return path.substr(dot_pos);
    }
    return "";
}

std::string get_filename(const std::string& path) {
    size_t slash_pos = path.find_last_of('/');
    if (slash_pos != std::string::npos) {
        return path.substr(slash_pos + 1);
    }
    return path;
}

std::string get_directory(const std::string& path) {
    size_t slash_pos = path.find_last_of('/');
    if (slash_pos != std::string::npos) {
        return path.substr(0, slash_pos);
    }
    return ".";
}

std::string join_paths(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    if (path1.back() == '/') {
        return path1 + path2;
    } else {
        return path1 + "/" + path2;
    }
}

bool is_executable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> find_files(const std::string& directory, const std::string& pattern) {
    std::vector<std::string> matches;
    
    auto entries = list_directory(directory);
    for (const auto& entry : entries) {
        if (entry.find(pattern) != std::string::npos) {
            matches.push_back(entry);
        }
    }
    
    return matches;
}

std::string get_timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S");
    return ss.str();
}

long long get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::string encode_base64(const std::string& input) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string output;
    int val = 0;
    int valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (output.size() % 4) {
        output.push_back('=');
    }
    
    return output;
}

std::string decode_base64(const std::string& input) {
    static const unsigned char base64_decode_table[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };
    
    std::string output;
    int val = 0;
    int valb = -8;
    
    for (unsigned char c : input) {
        if (base64_decode_table[c] == 64) break;
        val = (val << 6) + base64_decode_table[c];
        valb += 6;
        if (valb >= 0) {
            output.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return output;
}

std::string calculate_md5(const std::string& input) {
    return "";
}

std::string calculate_sha256(const std::string& input) {
    return "";
}

bool is_process_running(pid_t pid) {
    return kill(pid, 0) == 0;
}

std::vector<pid_t> get_child_processes(pid_t parent_pid) {
    std::vector<pid_t> children;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return children;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            pid_t pid = atoi(entry->d_name);
            if (pid > 0) {
                std::string stat_file = "/proc/" + std::string(entry->d_name) + "/stat";
                std::ifstream file(stat_file);
                if (file.is_open()) {
                    std::string line;
                    std::getline(file, line);
                    
                    size_t first_paren = line.find('(');
                    size_t last_paren = line.rfind(')');
                    if (first_paren != std::string::npos && last_paren != std::string::npos) {
                        std::string after_comm = line.substr(last_paren + 2);
                        std::istringstream iss(after_comm);
                        char state;
                        pid_t ppid;
                        iss >> state >> ppid;
                        
                        if (ppid == parent_pid) {
                            children.push_back(pid);
                        }
                    }
                }
            }
        }
    }
    
    closedir(proc_dir);
    return children;
}

void kill_process_tree(pid_t pid) {
    auto children = get_child_processes(pid);
    for (pid_t child : children) {
        kill_process_tree(child);
    }
    kill(pid, SIGTERM);
}

}

}
