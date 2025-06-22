#pragma once

#include <string>
#include <vector>
#include <vita2d.h>
#include <map>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544

enum class AppState {
    CHAT,
    SETTINGS,
    SESSIONS
};

struct Settings {
    std::string endpoint;
    std::string apiKey;
    std::map<std::string, std::string> default_models;
};

enum class UISelection {
    MODEL_PILL,
    ACTION_BUTTON_1,
    INPUT_PILL,
    ACTION_BUTTON_2,
    ACTION_BUTTON_3
};

enum class SettingsSelection {
    ENDPOINT,
    API_KEY_SETTING,
    DEFAULT_MODEL
};

struct ChatMessage {
    enum Sender { USER, LLM };
    Sender sender;
    std::string text;
    std::vector<std::string> wrapped_text;
    int alpha = 255;
    vita2d_texture* image = NULL; 
    std::string image_path;       
    std::string reasoning;        
    std::vector<std::string> wrapped_reasoning; 
    bool show_reasoning = false; 
};

using ChatSession = std::vector<ChatMessage>; 