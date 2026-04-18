#ifndef CHESS_ENV_CONFIG_H
#define CHESS_ENV_CONFIG_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace utils {
    class EnvConfig {
    public:
        static void load(const std::string& filename = ".env") {
            std::ifstream file(filename);
            if (!file.is_open()) return;

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                size_t delimiterPos = line.find('=');
                if (delimiterPos == std::string::npos) continue;

                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                
                key = trim(key);
                value = trim(value);

#ifdef _WIN32
                _putenv_s(key.c_str(), value.c_str());
#else
                setenv(key.c_str(), value.c_str(), 1);
#endif
            }
        }

        static std::string get(const std::string& key, const std::string& defaultValue = "") {
            const char* val = std::getenv(key.c_str());
            return val ? std::string(val) : defaultValue;
        }

        static unsigned short getPort(const std::string& key, unsigned short defaultValue = 0) {
            const char* val = std::getenv(key.c_str());
            return val ? static_cast<unsigned short>(std::stoi(val)) : defaultValue;
        }

    private:
        static std::string trim(const std::string& s) {
            size_t first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return "";
            size_t last = s.find_last_not_of(" \t\r\n");
            return s.substr(first, last - first + 1);
        }
    };
}

#endif
