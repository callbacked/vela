#include "persistence.h"
#include <jsoncpp/json/json.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include "ui.h" 
#include "image_utils.h" 

static void ensure_directory_exists(const char* path) {
    sceIoMkdir(path, 0755);
}

bool save_sessions(const std::vector<ChatSession>& sessions) {
    std::string dir_path = "ux0:data/vela";
    ensure_directory_exists(dir_path.c_str());
    
    std::string images_dir = dir_path + "/images";
    ensure_directory_exists(images_dir.c_str());
    
    Json::Value root(Json::arrayValue);
    
    for (size_t session_idx = 0; session_idx < sessions.size(); session_idx++) {
        const auto& session = sessions[session_idx];
        Json::Value sessionJson(Json::arrayValue);
        
        for (size_t msg_idx = 0; msg_idx < session.size(); msg_idx++) {
            const auto& message = session[msg_idx];
            Json::Value messageJson;
            messageJson["sender"] = message.sender == ChatMessage::USER ? "user" : "llm";
            messageJson["text"] = message.text;
            
            if (message.sender == ChatMessage::LLM) {
                messageJson["reasoning"] = message.reasoning;
                messageJson["show_reasoning"] = message.show_reasoning;
            }
            
            if (!message.image_path.empty()) {
                messageJson["image_path"] = message.image_path;
            }
            
            sessionJson.append(messageJson);
        }
        
        root.append(sessionJson);
    }
    
    Json::StreamWriterBuilder writer_builder;
    std::string content = Json::writeString(writer_builder, root);
    
    std::string sessions_path = "ux0:data/vela/sessions.json";
    std::ofstream file(sessions_path);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    file.close();
    return true;
}

std::vector<ChatSession> load_sessions() {
    std::vector<ChatSession> sessions;
    std::string sessions_path = "ux0:data/vela/sessions.json";
    
    std::ifstream file(sessions_path);
    if (!file.is_open()) {
        sessions.push_back(ChatSession());
        return sessions;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    Json::Value root;
    Json::CharReaderBuilder reader_builder;
    std::unique_ptr<Json::CharReader> const reader(reader_builder.newCharReader());
    JSONCPP_STRING errs;
    
    if (reader->parse(content.c_str(), content.c_str() + content.length(), &root, &errs)) {
        if (root.isArray()) {
            for (const auto& sessionJson : root) {
                ChatSession session;
                
                if (sessionJson.isArray()) {
                    for (const auto& messageJson : sessionJson) {
                        ChatMessage message;
                        
                        if (messageJson.isMember("sender")) {
                            std::string sender = messageJson["sender"].asString();
                            message.sender = (sender == "user") ? ChatMessage::USER : ChatMessage::LLM;
                        }
                        
                        if (messageJson.isMember("text")) {
                            message.text = messageJson["text"].asString();
                        }
                        
                        if (message.sender == ChatMessage::LLM) {
                            if (messageJson.isMember("reasoning")) {
                                message.reasoning = messageJson["reasoning"].asString();
                            }
                            
                            if (messageJson.isMember("show_reasoning")) {
                                message.show_reasoning = messageJson["show_reasoning"].asBool();
                            }
                        }
                        
                        if (messageJson.isMember("image_path")) {
                            message.image_path = messageJson["image_path"].asString();
                            message.image = load_texture_from_file(message.image_path);
                        }
                        session.push_back(message);
                    }
                }
                
                sessions.push_back(session);
            }
        }
    }
    
    // If no sessions were loaded, add a default empty session
    if (sessions.empty()) {
        sessions.push_back(ChatSession());
    }
    
    return sessions;
} 