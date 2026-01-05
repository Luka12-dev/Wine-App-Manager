#include "wine_wrapper.hpp"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace WineWrapper {

WineConfiguration::WineConfiguration() {
    wine_prefix = Utils::get_home_directory() + "/.wine";
    wine_binary = "wine";
    architecture = WineArchitecture::AUTO_DETECT;
    enable_virtual_desktop = false;
    enable_csmt = true;
    enable_dxvk = false;
    enable_esync = true;
    enable_fsync = false;
    audio_driver = "alsa";
    graphics_driver = "x11";
    nice_level = 0;
    debug_output = false;
    max_log_size_mb = 100;
    capture_stdout = true;
    capture_stderr = true;
}

void WineConfiguration::load_from_file(const std::string& config_file) {
    ConfigurationParser parser(config_file);
    wine_prefix = parser.get_value("wine_prefix", wine_prefix);
    wine_binary = parser.get_value("wine_binary", wine_binary);
    
    std::string arch = parser.get_value("architecture", "auto");
    if (arch == "win32") architecture = WineArchitecture::WIN32;
    else if (arch == "win64") architecture = WineArchitecture::WIN64;
    else architecture = WineArchitecture::AUTO_DETECT;
    
    enable_virtual_desktop = parser.get_value("enable_virtual_desktop", "false") == "true";
    virtual_desktop_resolution = parser.get_value("virtual_desktop_resolution", "1024x768");
    enable_csmt = parser.get_value("enable_csmt", "true") == "true";
    enable_dxvk = parser.get_value("enable_dxvk", "false") == "true";
    enable_esync = parser.get_value("enable_esync", "true") == "true";
    enable_fsync = parser.get_value("enable_fsync", "false") == "true";
    audio_driver = parser.get_value("audio_driver", "alsa");
    graphics_driver = parser.get_value("graphics_driver", "x11");
    nice_level = std::stoi(parser.get_value("nice_level", "0"));
    debug_output = parser.get_value("debug_output", "false") == "true";
    log_file = parser.get_value("log_file", "");
    max_log_size_mb = std::stoi(parser.get_value("max_log_size_mb", "100"));
    capture_stdout = parser.get_value("capture_stdout", "true") == "true";
    capture_stderr = parser.get_value("capture_stderr", "true") == "true";
}

void WineConfiguration::save_to_file(const std::string& config_file) const {
    ConfigurationParser parser;
    parser.set_value("wine_prefix", wine_prefix);
    parser.set_value("wine_binary", wine_binary);
    
    std::string arch = "auto";
    if (architecture == WineArchitecture::WIN32) arch = "win32";
    else if (architecture == WineArchitecture::WIN64) arch = "win64";
    parser.set_value("architecture", arch);
    
    parser.set_value("enable_virtual_desktop", enable_virtual_desktop ? "true" : "false");
    parser.set_value("virtual_desktop_resolution", virtual_desktop_resolution);
    parser.set_value("enable_csmt", enable_csmt ? "true" : "false");
    parser.set_value("enable_dxvk", enable_dxvk ? "true" : "false");
    parser.set_value("enable_esync", enable_esync ? "true" : "false");
    parser.set_value("enable_fsync", enable_fsync ? "true" : "false");
    parser.set_value("audio_driver", audio_driver);
    parser.set_value("graphics_driver", graphics_driver);
    parser.set_value("nice_level", std::to_string(nice_level));
    parser.set_value("debug_output", debug_output ? "true" : "false");
    parser.set_value("log_file", log_file);
    parser.set_value("max_log_size_mb", std::to_string(max_log_size_mb));
    parser.set_value("capture_stdout", capture_stdout ? "true" : "false");
    parser.set_value("capture_stderr", capture_stderr ? "true" : "false");
    
    parser.save_to_file(config_file);
}

std::string WineConfiguration::to_string() const {
    std::stringstream ss;
    ss << "Wine Configuration:\n";
    ss << "  Prefix: " << wine_prefix << "\n";
    ss << "  Binary: " << wine_binary << "\n";
    ss << "  Architecture: ";
    if (architecture == WineArchitecture::WIN32) ss << "Win32\n";
    else if (architecture == WineArchitecture::WIN64) ss << "Win64\n";
    else ss << "Auto-detect\n";
    ss << "  Virtual Desktop: " << (enable_virtual_desktop ? "Enabled" : "Disabled");
    if (enable_virtual_desktop) ss << " (" << virtual_desktop_resolution << ")";
    ss << "\n";
    ss << "  CSMT: " << (enable_csmt ? "Enabled" : "Disabled") << "\n";
    ss << "  DXVK: " << (enable_dxvk ? "Enabled" : "Disabled") << "\n";
    ss << "  ESYNC: " << (enable_esync ? "Enabled" : "Disabled") << "\n";
    ss << "  FSYNC: " << (enable_fsync ? "Enabled" : "Disabled") << "\n";
    ss << "  Audio Driver: " << audio_driver << "\n";
    ss << "  Graphics Driver: " << graphics_driver << "\n";
    ss << "  Nice Level: " << nice_level << "\n";
    return ss.str();
}

void WineConfiguration::validate() {
    if (!Utils::directory_exists(wine_prefix)) {
        Utils::create_directory(wine_prefix);
    }
    
    if (nice_level < -20) nice_level = -20;
    if (nice_level > 19) nice_level = 19;
    
    if (max_log_size_mb < 1) max_log_size_mb = 1;
    if (max_log_size_mb > 10000) max_log_size_mb = 10000;
}

void WineConfiguration::apply_defaults() {
    if (wine_prefix.empty()) {
        wine_prefix = Utils::get_home_directory() + "/.wine";
    }
    if (wine_binary.empty()) {
        wine_binary = "wine";
    }
    if (audio_driver.empty()) {
        audio_driver = "alsa";
    }
    if (graphics_driver.empty()) {
        graphics_driver = "x11";
    }
}

bool WineConfiguration::is_valid() const {
    if (wine_binary.empty()) return false;
    if (wine_prefix.empty()) return false;
    return true;
}

Logger::Logger() : min_level(LogLevel::INFO), max_file_size(100 * 1024 * 1024),
                   console_output(true), max_buffer_size(10000),
                   async_logging(false), stop_logging(false) {
}

Logger::Logger(const std::string& file_path, LogLevel level)
    : min_level(level), log_file_path(file_path), max_file_size(100 * 1024 * 1024),
      console_output(true), max_buffer_size(10000),
      async_logging(false), stop_logging(false) {
    log_file.open(file_path, std::ios::app);
}

Logger::~Logger() {
    stop_logging = true;
    if (async_logging && logging_thread.joinable()) {
        log_cv.notify_all();
        logging_thread.join();
    }
    flush();
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::set_log_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.close();
    }
    log_file_path = file_path;
    log_file.open(file_path, std::ios::app);
}

void Logger::set_min_level(LogLevel level) {
    min_level = level;
}

void Logger::set_console_output(bool enabled) {
    console_output = enabled;
}

void Logger::set_max_file_size(size_t size_mb) {
    max_file_size = size_mb * 1024 * 1024;
}

void Logger::enable_async_logging(bool enabled) {
    if (enabled && !async_logging) {
        async_logging = true;
        stop_logging = false;
        logging_thread = std::thread(&Logger::async_log_worker, this);
    } else if (!enabled && async_logging) {
        stop_logging = true;
        log_cv.notify_all();
        if (logging_thread.joinable()) {
            logging_thread.join();
        }
        async_logging = false;
    }
}

void Logger::rotate_log_file() {
    if (!log_file_path.empty() && Utils::file_exists(log_file_path)) {
        size_t current_size = Utils::get_file_size(log_file_path);
        if (current_size > max_file_size) {
            log_file.close();
            std::string backup_path = log_file_path + ".old";
            Utils::move_file(log_file_path, backup_path);
            log_file.open(log_file_path, std::ios::app);
        }
    }
}

void Logger::async_log_worker() {
    while (!stop_logging) {
        std::unique_lock<std::mutex> lock(log_mutex);
        log_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !log_buffer.empty() || stop_logging;
        });
        
        while (!log_buffer.empty()) {
            std::string message = log_buffer.front();
            log_buffer.pop();
            if (log_file.is_open()) {
                log_file << message << std::endl;
            }
            if (console_output) {
                std::cout << message << std::endl;
            }
        }
        
        if (log_file.is_open()) {
            log_file.flush();
        }
    }
}

std::string Logger::format_log_message(LogLevel level, const std::string& message) {
    std::stringstream ss;
    ss << "[" << get_timestamp() << "] ";
    ss << "[" << level_to_string(level) << "] ";
    ss << message;
    return ss.str();
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR_LOG: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level) return;
    
    std::string formatted_message = format_log_message(level, message);
    
    if (async_logging) {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_buffer.size() < max_buffer_size) {
            log_buffer.push(formatted_message);
            log_cv.notify_one();
        }
    } else {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file << formatted_message << std::endl;
            rotate_log_file();
        }
        if (console_output) {
            std::cout << formatted_message << std::endl;
        }
    }
}

void Logger::debug(const std::string& message) { log(LogLevel::DEBUG, message); }
void Logger::info(const std::string& message) { log(LogLevel::INFO, message); }
void Logger::warning(const std::string& message) { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message) { log(LogLevel::ERROR_LOG, message); }
void Logger::critical(const std::string& message) { log(LogLevel::CRITICAL, message); }

void Logger::flush() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.flush();
    }
}

std::vector<std::string> Logger::get_recent_logs(size_t count) {
    std::vector<std::string> logs;
    if (log_file_path.empty() || !Utils::file_exists(log_file_path)) {
        return logs;
    }
    
    std::ifstream file(log_file_path);
    std::string line;
    std::deque<std::string> recent;
    
    while (std::getline(file, line)) {
        recent.push_back(line);
        if (recent.size() > count) {
            recent.pop_front();
        }
    }
    
    logs.assign(recent.begin(), recent.end());
    return logs;
}

void Logger::clear_logs() {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open()) {
        log_file.close();
    }
    if (!log_file_path.empty()) {
        Utils::delete_file(log_file_path);
        log_file.open(log_file_path, std::ios::app);
    }
}

}
