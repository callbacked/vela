#ifndef INPUT_H
#define INPUT_H

#include <psp2/ctrl.h>
#include <string>
#include "types.h"
#include "keyboard.h"
#include "settings.h"

void handle_keyboard_input(std::string& input_text, bool& keyboard_active, KeyboardState& state);
void handle_settings_keyboard(Settings& settings, SettingsSelection selection, bool& keyboard_active, KeyboardState& state);

void handle_chat_input(
    SceCtrlData& pad, 
    SceCtrlData& old_pad,
    UISelection& current_selection, 
    int& hovered_message_index,
    bool& keyboard_active,
    bool& camera_mode_active,
    bool& photo_taken,
    bool& model_selection_open,
    int& hovered_model_index,
    int& selected_model_index,
    int& scroll_offset,
    int total_history_height,
    vita2d_texture*& staged_photo,
    vita2d_texture*& photo_to_free,
    std::vector<ChatMessage>& chat_history,
    const std::vector<std::string>& available_models,
    bool camera_initialized,
    AppState& app_state
);

void handle_settings_input(
    SceCtrlData& pad, 
    SceCtrlData& old_pad,
    Settings& settings,
    SettingsSelection& settings_selection,
    bool& settings_keyboard_active,
    AppState& app_state,
    bool& settings_model_selection_open,
    int& settings_model_selection_index
);

void handle_sessions_input(
    SceCtrlData& pad, 
    SceCtrlData& old_pad,
    std::vector<ChatSession>& sessions,
    int& session_selection_index,
    int& current_session_index,
    int& session_scroll_offset,
    bool& show_delete_confirmation,
    bool& delete_confirmation_selection,
    AppState& app_state,
    int& scroll_offset
);


bool is_left_stick_up(const SceCtrlData& pad, int deadzone = 50);
bool is_left_stick_down(const SceCtrlData& pad, int deadzone = 50);
bool is_left_stick_left(const SceCtrlData& pad, int deadzone = 50);
bool is_left_stick_right(const SceCtrlData& pad, int deadzone = 50);

bool is_right_stick_up(const SceCtrlData& pad, int deadzone = 50);
bool is_right_stick_down(const SceCtrlData& pad, int deadzone = 50);
bool is_right_stick_left(const SceCtrlData& pad, int deadzone = 50);
bool is_right_stick_right(const SceCtrlData& pad, int deadzone = 50);

#endif 