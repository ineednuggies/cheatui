#pragma once
#include "Color.hpp"
#include <string>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace cui {

using ConfigValue = std::variant<bool, int, float, std::string, Color>;

// A flat key -> value store, keyed by strings like "combat.auto_block" or
// "visuals.accent_color". Widgets bind to a key; Menu can then dump every
// bound widget's state into one of these and write/read it as a profile.
class ConfigStore {
public:
    void SetBool(const std::string& key, bool v)        { data_[key] = v; }
    void SetInt(const std::string& key, int v)           { data_[key] = v; }
    void SetFloat(const std::string& key, float v)       { data_[key] = v; }
    void SetString(const std::string& key, const std::string& v) { data_[key] = v; }
    void SetColor(const std::string& key, Color v)       { data_[key] = v; }

    bool GetBool(const std::string& key, bool def = false) const {
        auto it = data_.find(key);
        if (it == data_.end() || !std::holds_alternative<bool>(it->second)) return def;
        return std::get<bool>(it->second);
    }
    int GetInt(const std::string& key, int def = 0) const {
        auto it = data_.find(key);
        if (it == data_.end() || !std::holds_alternative<int>(it->second)) return def;
        return std::get<int>(it->second);
    }
    float GetFloat(const std::string& key, float def = 0.0f) const {
        auto it = data_.find(key);
        if (it == data_.end() || !std::holds_alternative<float>(it->second)) return def;
        return std::get<float>(it->second);
    }
    std::string GetString(const std::string& key, const std::string& def = "") const {
        auto it = data_.find(key);
        if (it == data_.end() || !std::holds_alternative<std::string>(it->second)) return def;
        return std::get<std::string>(it->second);
    }
    Color GetColor(const std::string& key, Color def = Color()) const {
        auto it = data_.find(key);
        if (it == data_.end() || !std::holds_alternative<Color>(it->second)) return def;
        return std::get<Color>(it->second);
    }

    bool Has(const std::string& key) const { return data_.find(key) != data_.end(); }

    // Serializes to a simple "key=type:value" text format - human-readable
    // and diffable, deliberately not binary so users can hand-edit configs.
    bool SaveToFile(const std::string& path) const {
        std::ofstream f(path, std::ios::trunc);
        if (!f) return false;
        for (const auto& [key, value] : data_) {
            f << key << "=";
            std::visit([&f](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, bool>) {
                    f << "bool:" << (v ? "1" : "0");
                } else if constexpr (std::is_same_v<T, int>) {
                    f << "int:" << v;
                } else if constexpr (std::is_same_v<T, float>) {
                    f << "float:" << v;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    f << "str:" << v;
                } else if constexpr (std::is_same_v<T, Color>) {
                    f << "color:" << static_cast<int>(v.r) << "," << static_cast<int>(v.g)
                      << "," << static_cast<int>(v.b) << "," << static_cast<int>(v.a);
                }
            }, value);
            f << "\n";
        }
        return true;
    }

    bool LoadFromFile(const std::string& path) {
        std::ifstream f(path);
        if (!f) return false;
        data_.clear();
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string rest = line.substr(eq + 1);
            auto colon = rest.find(':');
            if (colon == std::string::npos) continue;
            std::string type = rest.substr(0, colon);
            std::string val = rest.substr(colon + 1);
            if (type == "bool") data_[key] = (val == "1");
            else if (type == "int") data_[key] = std::stoi(val);
            else if (type == "float") data_[key] = std::stof(val);
            else if (type == "str") data_[key] = val;
            else if (type == "color") {
                std::stringstream ss(val);
                std::string tok;
                int parts[4] = {255, 255, 255, 255};
                int i = 0;
                while (std::getline(ss, tok, ',') && i < 4) parts[i++] = std::stoi(tok);
                data_[key] = Color(static_cast<uint8_t>(parts[0]), static_cast<uint8_t>(parts[1]),
                                    static_cast<uint8_t>(parts[2]), static_cast<uint8_t>(parts[3]));
            }
        }
        return true;
    }

    // Lists "<name>.cfg" files in a directory - backs a "Config" tab's
    // load/save/delete profile list.
    static std::vector<std::string> ListProfiles(const std::string& dir) {
        std::vector<std::string> names;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return names;
        for (auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.path().extension() == ".cfg") names.push_back(entry.path().stem().string());
        }
        return names;
    }

private:
    std::unordered_map<std::string, ConfigValue> data_;
};

} // namespace cui
