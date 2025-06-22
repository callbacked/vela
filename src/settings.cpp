#include "settings.h"
#include "config.h" 
#include <jsoncpp/json/json.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>


void ensure_directory_exists(const char* path) {
    sceIoMkdir(path, 0755);
}

Settings load_settings() {
    Settings settings;
    std::string settings_path = "ux0:data/vela/settings.json";

    std::ifstream file(settings_path);
    if (!file.is_open()) {

        settings.endpoint = API_ENDPOINT;
        settings.apiKey = ""; 
        return settings;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    Json::Value root;
    Json::CharReaderBuilder reader_builder;
    std::unique_ptr<Json::CharReader> const reader(reader_builder.newCharReader());
    JSONCPP_STRING errs;

    if (reader->parse(content.c_str(), content.c_str() + content.length(), &root, &errs)) {
        settings.endpoint = root.get("endpoint", API_ENDPOINT).asString();
        settings.apiKey = root.get("apiKey", "").asString();
        
        // Load default models map
        if (root.isMember("default_models") && root["default_models"].isObject()) {
            Json::Value default_models_json = root["default_models"];
            for (auto const& key : default_models_json.getMemberNames()) {
                settings.default_models[key] = default_models_json[key].asString();
            }
        }
    } else {
        // JSON is invalid, return defaults
        settings.endpoint = API_ENDPOINT;
        settings.apiKey = "";
    }

    return settings;
}

void save_settings(const Settings& settings) {
    std::string dir_path = "ux0:data/vela";
    ensure_directory_exists(dir_path.c_str());
    
    Json::Value root;
    root["endpoint"] = settings.endpoint;
    root["apiKey"] = settings.apiKey;

    Json::Value default_models_json(Json::objectValue);
    for (const auto& pair : settings.default_models) {
        default_models_json[pair.first] = pair.second;
    }
    root["default_models"] = default_models_json;

    Json::StreamWriterBuilder writer_builder;
    std::string content = Json::writeString(writer_builder, root);

    std::string settings_path = "ux0:data/vela/settings.json";
    std::ofstream file(settings_path);
    if (file.is_open()) {
        file << content;
        file.close();
    }
} 