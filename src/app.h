#pragma once

#include <psp2/kernel/sysmem.h>
#include <vita2d.h>
#include <vector>
#include <string>
#include "types.h"
#include "settings.h"

struct AppContext {
    vita2d_pgf* pgf;
    int memid;
    
    Settings settings;
    SettingsSelection settings_selection;
    bool settings_keyboard_active;
    bool settings_model_selection_open;
    int settings_model_selection_index;
    
    UISelection current_selection;
    int hovered_message_index;
    std::string user_question;
    bool keyboard_active;
    int scroll_offset;
    int total_history_height;
    
    std::vector<ChatSession> sessions;
    int current_session_index;
    int session_selection_index;
    int session_scroll_offset;
    bool show_delete_confirmation;
    bool delete_confirmation_selection;
    
    std::vector<std::string> available_models;
    int selected_model_index;
    int hovered_model_index;
    bool model_selection_open;
    bool is_fetching_models;
    int startup_counter;
    bool models_loaded;
    bool fetch_scheduled;
    bool connection_failed;
    
    bool camera_initialized;
    bool camera_mode_active;
    bool photo_taken;
    vita2d_texture* staged_photo;
    vita2d_texture* photo_to_free;
    unsigned int camera_fade_alpha;
    
    float model_dropup_h;
    unsigned int ui_alpha;
    unsigned int model_pill_alpha;
    float start_button_hold_duration; 
    
    AppState app_state;
};

void initialize_app(AppContext& ctx);
void run_app(AppContext& ctx);
void cleanup_app(AppContext& ctx); 