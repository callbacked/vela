#include "input.h"
#include "keyboard.h"
#include "settings.h"
#include "persistence.h"
#include "camera.h"
#include <psp2/ctrl.h>

// a lot of potential to reuse code here but it works for now

const int STICK_CENTER = 128;

bool is_left_stick_up(const SceCtrlData& pad, int deadzone) {
    return (pad.ly < STICK_CENTER - deadzone);
}

bool is_left_stick_down(const SceCtrlData& pad, int deadzone) {
    return (pad.ly > STICK_CENTER + deadzone);
}

bool is_left_stick_left(const SceCtrlData& pad, int deadzone) {
    return (pad.lx < STICK_CENTER - deadzone);
}

bool is_left_stick_right(const SceCtrlData& pad, int deadzone) {
    return (pad.lx > STICK_CENTER + deadzone);
}

bool is_right_stick_up(const SceCtrlData& pad, int deadzone) {
    return (pad.ry < STICK_CENTER - deadzone);
}

bool is_right_stick_down(const SceCtrlData& pad, int deadzone) {
    return (pad.ry > STICK_CENTER + deadzone);
}

bool is_right_stick_left(const SceCtrlData& pad, int deadzone) {
    return (pad.rx < STICK_CENTER - deadzone);
}

bool is_right_stick_right(const SceCtrlData& pad, int deadzone) {
    return (pad.rx > STICK_CENTER + deadzone);
}

// input for the main chat screen
void handle_keyboard_input(std::string& input_text, bool& keyboard_active, KeyboardState& state) {
    keyboard_active = true;
    state = keyboard_update();
    
    if (state == KEYBOARD_STATE_FINISHED) {
        input_text = keyboard_get_text();
        keyboard_active = false;
    } else if (state == KEYBOARD_STATE_NONE) {
        keyboard_active = false;
    }
}

// input for the settings screen
void handle_settings_keyboard(Settings& settings, SettingsSelection selection, bool& keyboard_active, KeyboardState& state) {
    keyboard_active = true;
    state = keyboard_update();
    
    if (state == KEYBOARD_STATE_FINISHED) {
        std::string input = keyboard_get_text();
        
        if (selection == SettingsSelection::ENDPOINT) {
            settings.endpoint = input;
        } else if (selection == SettingsSelection::API_KEY_SETTING) {
            settings.apiKey = input;
        }
        
        keyboard_active = false;
    } else if (state == KEYBOARD_STATE_NONE) {
        keyboard_active = false;
    }
}

// input for the settings screen
void handle_settings_input(
    SceCtrlData& pad,
    SceCtrlData& old_pad,
    Settings& settings,
    SettingsSelection& settings_selection,
    bool& settings_keyboard_active,
    AppState& app_state,
    bool& settings_model_selection_open,
    int& settings_model_selection_index
) {
    if (!settings_keyboard_active) {
        const int STICK_DEADZONE = 50;
        static int analog_cooldown = 0; // Prevent too rapid movement
        
        if (analog_cooldown > 0) {
            analog_cooldown--;
        }
        
        bool left_stick_up = is_left_stick_up(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_up = is_right_stick_up(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_UP) && !(old_pad.buttons & SCE_CTRL_UP)) || left_stick_up || right_stick_up) {
            if (settings_model_selection_open) {
                // model selection
                settings_model_selection_index--;
                if (settings_model_selection_index < 0) {
                    settings_model_selection_index = 0; 
                }
                if (left_stick_up || right_stick_up) analog_cooldown = 15;
            } else {
                //  settings options
                if (settings_selection == SettingsSelection::API_KEY_SETTING) {
                    settings_selection = SettingsSelection::ENDPOINT;
                    if (left_stick_up || right_stick_up) analog_cooldown = 15;
                } else if (settings_selection == SettingsSelection::DEFAULT_MODEL) {
                    settings_selection = SettingsSelection::API_KEY_SETTING;
                    if (left_stick_up || right_stick_up) analog_cooldown = 15;
                }
            }
        }
        
        // Check both D-pad and both analog sticks for down movement
        bool left_stick_down = is_left_stick_down(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_down = is_right_stick_down(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_DOWN) && !(old_pad.buttons & SCE_CTRL_DOWN)) || left_stick_down || right_stick_down) {
            if (settings_model_selection_open) {
                // Navigate model selection
                settings_model_selection_index++;
                // We don't know available_models.size() here, but we can prevent out-of-bounds in the drawing code
                if (left_stick_down || right_stick_down) analog_cooldown = 15;
            } else {
                // Navigate settings options
                if (settings_selection == SettingsSelection::ENDPOINT) {
                    settings_selection = SettingsSelection::API_KEY_SETTING;
                    if (left_stick_down || right_stick_down) analog_cooldown = 15;
                } else if (settings_selection == SettingsSelection::API_KEY_SETTING) {
                    settings_selection = SettingsSelection::DEFAULT_MODEL;
                    if (left_stick_down || right_stick_down) analog_cooldown = 15;
                }
            }
        }

        if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
            if (settings_model_selection_open) {
                // Select the chosen model and close dropdown
                settings_model_selection_open = false;
                extern std::vector<std::string> available_models; 
                
                if (settings_model_selection_index >= 0 && settings_model_selection_index < (int)available_models.size()) {
                    // Save for this endpoint
                    settings.default_models[settings.endpoint] = available_models[settings_model_selection_index];
                    save_settings(settings);
                }
            } else {
                if (settings_selection == SettingsSelection::ENDPOINT) {
                    if (keyboard_start(settings.endpoint, "Enter Endpoint URL")) {
                        settings_keyboard_active = true;
                    }
                } else if (settings_selection == SettingsSelection::API_KEY_SETTING) {
                    if (keyboard_start(settings.apiKey, "Enter API Key")) {
                        settings_keyboard_active = true;
                    }
                } else if (settings_selection == SettingsSelection::DEFAULT_MODEL) {
                    // Show model selection dropdown
                    settings_model_selection_open = true;
                    
                    // Try to find the current default model and set selection
                    extern std::vector<std::string> available_models; // Declare external variable
                    
                    settings_model_selection_index = 0; // Default to first model
                    
                    // If a default model is set for this endpoint, try to find it
                    if (settings.default_models.count(settings.endpoint) > 0) {
                        std::string current_default = settings.default_models[settings.endpoint];
                        for (size_t i = 0; i < available_models.size(); i++) {
                            if (available_models[i] == current_default) {
                                settings_model_selection_index = i;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Close model selection dropdown with Circle
        if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
            if (settings_model_selection_open) {
                settings_model_selection_open = false;
            } else {
                app_state = AppState::CHAT;
            }
        }
    }
}

// Handle input for the sessions screen
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
) {
    if (!show_delete_confirmation) {
        // Normal session list navigation
        // Add analog stick support - similar to D-pad
        const int STICK_DEADZONE = 50;
        static int analog_cooldown = 0; // Prevent too rapid movement
        
        if (analog_cooldown > 0) {
            analog_cooldown--;
        }
        
        // Check both D-pad and both analog sticks for up movement
        bool left_stick_up = is_left_stick_up(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_up = is_right_stick_up(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_UP) && !(old_pad.buttons & SCE_CTRL_UP)) || left_stick_up || right_stick_up) {
            session_selection_index--;
            if (session_selection_index < -1) { // -1 is "New Chat"
                session_selection_index = sessions.size() - 1;
            }
            if (left_stick_up || right_stick_up) analog_cooldown = 15; // Set cooldown if analog was used
        }
        
        // Check both D-pad and both analog sticks for down movement
        bool left_stick_down = is_left_stick_down(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_down = is_right_stick_down(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_DOWN) && !(old_pad.buttons & SCE_CTRL_DOWN)) || left_stick_down || right_stick_down) {
            session_selection_index++;
            if (session_selection_index >= (int)sessions.size()) {
                session_selection_index = -1; // -1 is "New Chat"
            }
            if (left_stick_down || right_stick_down) analog_cooldown = 15; // Set cooldown if analog was used
        }
        
        if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
            if (session_selection_index == -1) {
                // Create a new session
                sessions.push_back(ChatSession());
                current_session_index = sessions.size() - 1;
                app_state = AppState::CHAT;
                scroll_offset = 0; // Reset scroll when switching session
                
                // Save sessions after creating a new session
                save_sessions(sessions);
            } else {
                // Switch to the selected session
                current_session_index = session_selection_index;
                app_state = AppState::CHAT;
                scroll_offset = 0; // Reset scroll when switching session
            }
        }
        if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
            app_state = AppState::CHAT;
        }
        
        // Handle Square button to initiate session deletion
        if ((pad.buttons & SCE_CTRL_SQUARE) && !(old_pad.buttons & SCE_CTRL_SQUARE)) {
            // Only allow deletion if a valid session is selected (not "New Chat")
            if (session_selection_index >= 0 && session_selection_index < sessions.size()) {
                show_delete_confirmation = true;
                delete_confirmation_selection = false; // Default to "No"
            }
        }
    } else {
        // Delete confirmation dialog navigation
        // Add analog stick support for left/right movement
        const int STICK_DEADZONE = 50;
        static int analog_cooldown = 0; // Prevent too rapid movement
        
        if (analog_cooldown > 0) {
            analog_cooldown--;
        }
        
        // Check both D-pad and both analog sticks for left movement
        bool left_stick_left = is_left_stick_left(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_left = is_right_stick_left(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_LEFT) && !(old_pad.buttons & SCE_CTRL_LEFT)) || left_stick_left || right_stick_left) {
            delete_confirmation_selection = true; // Select "Yes"
            if (left_stick_left || right_stick_left) analog_cooldown = 15; // Set cooldown if analog was used
        }
        
        // Check both D-pad and both analog sticks for right movement
        bool left_stick_right = is_left_stick_right(pad, STICK_DEADZONE) && analog_cooldown == 0;
        bool right_stick_right = is_right_stick_right(pad, STICK_DEADZONE) && analog_cooldown == 0;
        if (((pad.buttons & SCE_CTRL_RIGHT) && !(old_pad.buttons & SCE_CTRL_RIGHT)) || left_stick_right || right_stick_right) {
            delete_confirmation_selection = false; // Select "No"
            if (left_stick_right || right_stick_right) analog_cooldown = 15; // Set cooldown if analog was used
        }
        
        // Confirm selection
        if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
            if (delete_confirmation_selection) {
                // "Yes" was selected - delete the session
                // Clean up any textures in the session's messages
                for (auto& msg : sessions[session_selection_index]) {
                    if (msg.image) {
                        vita2d_free_texture(msg.image);
                        msg.image = NULL;
                    }
                }
                
                // Remove the session
                sessions.erase(sessions.begin() + session_selection_index);
                
                // Save sessions after deleting a session
                save_sessions(sessions);

                // If all sessions were deleted, create a new empty one
                if (sessions.empty()) {
                    sessions.push_back(ChatSession());
                    current_session_index = 0;
                    session_selection_index = 0;
                } else {
                    // If the current session is being deleted, switch to another session
                    if (current_session_index == session_selection_index) {
                        current_session_index = (session_selection_index > 0) ? session_selection_index - 1 : 0;
                    }
                    // If deleting a session with a lower index than current, adjust current_session_index
                    else if (session_selection_index < current_session_index) {
                        current_session_index--;
                    }
                    
                    // Adjust selection index if needed
                    if (session_selection_index >= sessions.size()) {
                        session_selection_index = sessions.size() - 1;
                    }
                }
            }
            // Exit delete confirmation mode
            show_delete_confirmation = false;
        }
        
        // Cancel deletion
        if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
            show_delete_confirmation = false;
        }
    }

    // Add session save logic after session modifications
    if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
        if (session_selection_index == -1 || delete_confirmation_selection) {
            // A new session was created or a session was deleted
            save_sessions(sessions);
        }
    }
}

// Handle input for the chat screen
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
) {
    // First, try to handle camera input if camera is active
    if (camera_mode_active) {
        if (handle_camera_input(pad, old_pad, camera_mode_active, photo_taken, staged_photo, photo_to_free)) {
            return; // Camera input was handled
        }
    }

    if (!keyboard_active) {
        if (model_selection_open) {
            // Handle input for model selection drop-up
            if ((pad.buttons & SCE_CTRL_UP) && !(old_pad.buttons & SCE_CTRL_UP)) {
                hovered_model_index = (hovered_model_index > 0) ? hovered_model_index - 1 : available_models.size() - 1;
            }
            if ((pad.buttons & SCE_CTRL_DOWN) && !(old_pad.buttons & SCE_CTRL_DOWN)) {
                hovered_model_index = (hovered_model_index < available_models.size() - 1) ? hovered_model_index + 1 : 0;
            }
            if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
                selected_model_index = hovered_model_index;
                model_selection_open = false;
            }
            if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
                model_selection_open = false;
            }
        } else {
            // D-pad navigation for UI elements only (not scrolling)
            if ((pad.buttons & SCE_CTRL_UP) && !(old_pad.buttons & SCE_CTRL_UP)) {
                bool models_are_available = !available_models.empty();
                if (current_selection != UISelection::MODEL_PILL && models_are_available) {
                    current_selection = UISelection::MODEL_PILL;
                }
            }
            if ((pad.buttons & SCE_CTRL_DOWN) && !(old_pad.buttons & SCE_CTRL_DOWN)) {
                if (current_selection == UISelection::MODEL_PILL) {
                    current_selection = UISelection::INPUT_PILL;
                }
            }
            if ((pad.buttons & SCE_CTRL_LEFT) && !(old_pad.buttons & SCE_CTRL_LEFT)) {
                if (current_selection == UISelection::INPUT_PILL) {
                    current_selection = UISelection::ACTION_BUTTON_1;
                } else if (current_selection == UISelection::ACTION_BUTTON_2) {
                    current_selection = UISelection::INPUT_PILL;
                } else if (current_selection == UISelection::ACTION_BUTTON_1) {
                    current_selection = UISelection::ACTION_BUTTON_3;
                }
            }
            if ((pad.buttons & SCE_CTRL_RIGHT) && !(old_pad.buttons & SCE_CTRL_RIGHT)) {
                if (current_selection == UISelection::ACTION_BUTTON_1) {
                    current_selection = UISelection::INPUT_PILL;
                } else if (current_selection == UISelection::INPUT_PILL) {
                    current_selection = UISelection::ACTION_BUTTON_2;
                } else if (current_selection == UISelection::ACTION_BUTTON_3) {
                    current_selection = UISelection::ACTION_BUTTON_1;
                }
            }

            // Action button logic
            if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS)) {
                // Only allow interaction with chat/model functions if models are available
                bool models_are_available = !available_models.empty();

                if (current_selection == UISelection::INPUT_PILL) {
                    if (models_are_available && keyboard_start("", "Enter your question")) {
                        keyboard_active = true;
                    }
                } else if (current_selection == UISelection::ACTION_BUTTON_1) {
                    app_state = AppState::SETTINGS;
                } else if (current_selection == UISelection::ACTION_BUTTON_2) {
                    // Toggle camera mode
                    if (models_are_available && camera_initialized) {
                        camera_mode_active = !camera_mode_active;
                        photo_taken = false; // Reset photo state when entering/exiting camera
                        if (staged_photo) {
                            photo_to_free = staged_photo; // Defer freeing to next frame
                            staged_photo = NULL;
                        }
                        
                        if (camera_mode_active) {
                            // Open and start camera
                            if (camera_open(camera_get_current_device(), CAMERA_RESOLUTION_640_480, CAMERA_FRAMERATE_30_FPS)) {
                                camera_start();
                            } else {
                                camera_mode_active = false;
                            }
                        } else {
                            // Stop and close camera
                            camera_stop();
                            camera_close();
                        }
                    }
                } else if (current_selection == UISelection::MODEL_PILL) {
                    if (models_are_available) {
                        model_selection_open = !model_selection_open;
                        hovered_model_index = selected_model_index;
                    }
                } else if (current_selection == UISelection::ACTION_BUTTON_3) {
                    app_state = AppState::SESSIONS;
                }
            }
            
            // Combined scrolling and message selection with a single analog stick
            bool message_selection_changed = false;
            bool is_hard_scrolling = false; // Flag to check for hard scrolling
            
            // Define stick thresholds and timing
            const int STICK_DEADZONE = 50;
            const int SCROLL_INITIATE_FRAMES = 8; // Frames to hold before scrolling starts

            static int stick_held_frames = 0;
            
            if (!chat_history.empty()) {
                int stick_y = pad.ly;
                bool is_stick_active = abs(stick_y - STICK_CENTER) > STICK_DEADZONE;

                int old_stick_y = old_pad.ly;
                bool was_stick_active = abs(old_stick_y - STICK_CENTER) > STICK_DEADZONE;
                
                if (is_stick_active) {
                    stick_held_frames++;
                }
                
                // On stick release, check if it was a flick
                if (was_stick_active && !is_stick_active) {
                    if (stick_held_frames > 0 && stick_held_frames < SCROLL_INITIATE_FRAMES) {
                        // It was a flick, so select a message
                        bool flick_up = (old_stick_y < STICK_CENTER - STICK_DEADZONE);
                        bool flick_down = (old_stick_y > STICK_CENTER + STICK_DEADZONE);
                        
                        if (flick_up) {
                            if (hovered_message_index == -1) {
                                hovered_message_index = chat_history.size() - 1;
                            } else if (hovered_message_index > 0) {
                                hovered_message_index--;
                            }
                            message_selection_changed = true;
                        } else if (flick_down) {
                            if (hovered_message_index == -1) {
                                hovered_message_index = 0;
                            } else if (hovered_message_index < chat_history.size() - 1) {
                                hovered_message_index++;
                            }
                            message_selection_changed = true;
                        }
                    }
                }
                
                // After checking for a flick, reset the timer if the stick is not active
                if (!is_stick_active) {
                    stick_held_frames = 0;
                }
                
                // If the stick is held long enough, start scrolling
                if (stick_held_frames >= SCROLL_INITIATE_FRAMES) {
                    is_hard_scrolling = true; // We are in hard scrolling mode

                    // Smooth scrolling logic
                    float push_amount = (stick_y - STICK_CENTER) / 128.0f;
                    int scroll_speed = (int)(push_amount * std::abs(push_amount) * 15.0f); // Use push_amount * abs(push_amount) for a signed quadratic curve
                    
                    scroll_offset += scroll_speed;
                    
                    // Bounds checking for scroll_offset
                    int max_scroll = total_history_height - (SCREEN_HEIGHT - 150);
                    if (max_scroll < 0) max_scroll = 0;
                    scroll_offset = std::max(0, std::min(scroll_offset, max_scroll));

                    // Update hovered_message_index to follow the scroll
                    if (push_amount < 0 && scroll_offset == 0) {
                        hovered_message_index = 0;
                    } else if (push_amount > 0 && scroll_offset >= max_scroll) {
                        hovered_message_index = chat_history.size() - 1;
                    } else {
                        int y_cursor = scroll_offset + (SCREEN_HEIGHT - 150) / 2;
                        int current_y = 0;
                        int new_hovered_index = -1;

                        for (int i = 0; i < chat_history.size(); i++) {
                            const auto& msg = chat_history[i];
                            bool has_image = (msg.image != NULL);
                            
                            // Calculate message height
                            int message_height = 0;
                            if (msg.sender == ChatMessage::USER) {
                                message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? 150.0f + 20 : 0);
                            } else {
                                message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? 150.0f + 10 : 0);
                                if (msg.show_reasoning && !msg.wrapped_reasoning.empty()) {
                                    message_height += msg.wrapped_reasoning.size() * 20 + 30;
                                }
                            }

                            if (y_cursor >= current_y && y_cursor < current_y + message_height) {
                                new_hovered_index = i;
                                break;
                            }
                            current_y += message_height + 10;
                        }
                        hovered_message_index = new_hovered_index;
                    }
                }
            }
            
            // If message selection changed, adjust scroll to make the selected message visible
            if (message_selection_changed && !is_hard_scrolling && hovered_message_index >= 0 && hovered_message_index < chat_history.size()) {
                // Calculate position of the selected message
                int message_position = 0;
                int message_height = 0;
                
                for (int i = 0; i < chat_history.size(); i++) {
                    const auto& msg = chat_history[i];
                    bool has_image = (msg.image != NULL);
                    
                    // Calculate message height (similar to the calculation in draw_ui)
                    if (msg.sender == ChatMessage::USER) {
                        message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? 150.0f + 20 : 0);
                    } else {
                        message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? 150.0f + 10 : 0);
                        if (msg.show_reasoning && !msg.wrapped_reasoning.empty()) {
                            message_height += msg.wrapped_reasoning.size() * 20 + 30;
                        }
                    }
                    
                    if (i < hovered_message_index) {
                        message_position += message_height + 10; // Add message height + margin
                    } else if (i == hovered_message_index) {
                        // We found our message, store its height for centering
                        break;
                    }
                }
                
                // Adjust scroll to center the selected message in the visible area
                int visible_area_height = SCREEN_HEIGHT - 150; // Approximate visible area height
                int target_scroll = message_position - (visible_area_height / 2) + (message_height / 2);
                
                // Smooth scrolling towards target - gradual transition
                int scroll_diff = target_scroll - scroll_offset;
                scroll_offset += scroll_diff / 3; // Move 1/3 of the way to target for smoother scrolling
                
                // Ensure scroll is within bounds
                int max_scroll = total_history_height - (SCREEN_HEIGHT - 150);
                if (max_scroll < 0) max_scroll = 0;
                scroll_offset = std::max(0, std::min(scroll_offset, max_scroll));
            }
            
            // Toggle reasoning display with Triangle button for the hovered message
            if ((pad.buttons & SCE_CTRL_TRIANGLE) && !(old_pad.buttons & SCE_CTRL_TRIANGLE)) {
                if (hovered_message_index >= 0 && hovered_message_index < chat_history.size()) {
                    ChatMessage& msg = chat_history[hovered_message_index];
                    if (msg.sender == ChatMessage::LLM && !msg.reasoning.empty()) {
                        msg.show_reasoning = !msg.show_reasoning;
                    }
                } else {
                    // If no message is hovered, fall back to the previous behavior
                    // Find the last LLM message and toggle its reasoning display
                    for (auto it = chat_history.rbegin(); it != chat_history.rend(); ++it) {
                        if (it->sender == ChatMessage::LLM && !it->reasoning.empty()) {
                            it->show_reasoning = !it->show_reasoning;
                            break; // Only toggle the most recent LLM message
                        }
                    }
                }
            }
            
            // Reset hover when switching screens or sessions
            if ((pad.buttons & SCE_CTRL_CIRCLE) && !(old_pad.buttons & SCE_CTRL_CIRCLE)) {
                hovered_message_index = -1;
            }
        }
    }
} 