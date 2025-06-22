#include "sessions.h"
#include "ui.h" 

#define MONO_WHITE RGBA8(255, 255, 255, 255)
#define MONO_LIGHT_GRAY RGBA8(160, 160, 160, 255)
#define MONO_DARK_GRAY RGBA8(48, 48, 48, 255)

void draw_feathered_highlight(float x, float y, float w, float h) {
    unsigned int base_alpha = 255 / 5; 
    unsigned int highlight_color = RGBA8(160, 160, 160, base_alpha);

    float feather_width = 30.0f;
    if (feather_width * 2 > w) {
        feather_width = w / 2;
    }
    float main_width = w - (feather_width * 2);

    vita2d_draw_rectangle(x + feather_width, y, main_width, h, highlight_color);

    int feather_steps = 15;
    for (int i = 0; i < feather_steps; i++) {
        float step_width = feather_width / feather_steps;
        float opacity = (float)(feather_steps - i) / feather_steps;
        unsigned int alpha = (unsigned int)(base_alpha * opacity);
        unsigned int color = RGBA8(160, 160, 160, alpha);

        vita2d_draw_rectangle(x + feather_width + main_width + (i * step_width), y, step_width, h, color);
        vita2d_draw_rectangle(x + feather_width - ((i + 1) * step_width), y, step_width, h, color);
    }
}

void draw_sessions_ui(vita2d_pgf* pgf, const std::vector<ChatSession>& sessions, int scroll_offset, int selected_session_index, bool show_delete_confirmation, bool delete_confirmation_selection) {
    vita2d_start_drawing();
    vita2d_clear_screen();

    vita2d_pgf_draw_text(pgf, 20, 30, MONO_WHITE, 1.2f, "Chat Sessions");

    // --- Draw "New Chat" Button ---
    const char* new_chat_text = "New Chat";
    float new_chat_text_width = vita2d_pgf_text_width(pgf, 1.0f, new_chat_text);
    float new_chat_x = (SCREEN_WIDTH - new_chat_text_width) / 2;
    float new_chat_y = 80;

    if (selected_session_index == -1) {
        float highlight_w = new_chat_text_width + 80;
        float highlight_x = (SCREEN_WIDTH - highlight_w) / 2;
        draw_feathered_highlight(highlight_x, new_chat_y - 22, highlight_w, 35);
    }
    vita2d_pgf_draw_text(pgf, new_chat_x, new_chat_y, MONO_WHITE, 1.0f, new_chat_text);


    // --- Draw Session List ---
    int current_y = 140 - scroll_offset;
    float item_x = 80;

    for (int i = 0; i < sessions.size(); ++i) {
        const auto& session = sessions[i];
        
        std::string preview_text = "Session " + std::to_string(i + 1);
        if (!session.empty() && !session[0].text.empty()) {
             preview_text = session[0].text;
        } else {
             preview_text = "New session...";
        }
        
        if (preview_text.length() > 60) {
            preview_text = preview_text.substr(0, 57) + "...";
        }
        
        float text_width = vita2d_pgf_text_width(pgf, 1.0f, preview_text.c_str());
        float text_x = 60; 
        
        if (i == selected_session_index) {
            float highlight_w = text_width + 80;
            float highlight_x = text_x - 40;
            draw_feathered_highlight(highlight_x, current_y - 22, highlight_w, 35);
        }

        if (current_y > 50 && current_y < 500) {
            vita2d_pgf_draw_text(pgf, text_x, current_y, MONO_WHITE, 1.0f, preview_text.c_str());
        }
        
        current_y += 50;
    }
    
    // Instructions at the bottom
    if (!show_delete_confirmation) {
        vita2d_pgf_draw_text(pgf, 20, 520, MONO_WHITE, 1.0f, "X Select, O Return, □ Delete");
    }

    // Draw delete confirmation dialog if active
    if (show_delete_confirmation && selected_session_index >= 0 && selected_session_index < sessions.size()) {
        vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA8(0, 0, 0, 200));
        
        // Dialog box
        float dialog_w = 400;
        float dialog_h = 200;
        float dialog_x = (SCREEN_WIDTH - dialog_w) / 2;
        float dialog_y = (SCREEN_HEIGHT - dialog_h) / 2;
        
        // Draw dialog background
        draw_rounded_rect(dialog_x, dialog_y, dialog_w, dialog_h, 15, RGBA8(48, 48, 48, 255));
        
        // Dialog title
        const char* dialog_title = "Delete Session?";
        float title_width = vita2d_pgf_text_width(pgf, 1.2f, dialog_title);
        vita2d_pgf_draw_text(pgf, dialog_x + (dialog_w - title_width) / 2, dialog_y + 50, MONO_WHITE, 1.2f, dialog_title);
        
        // Dialog message
        const char* dialog_msg = "This action cannot be undone.";
        float msg_width = vita2d_pgf_text_width(pgf, 1.0f, dialog_msg);
        vita2d_pgf_draw_text(pgf, dialog_x + (dialog_w - msg_width) / 2, dialog_y + 90, MONO_WHITE, 1.0f, dialog_msg);
        
        // Yes/No buttons
        const char* yes_text = "Yes";
        const char* no_text = "No";
        float yes_width = vita2d_pgf_text_width(pgf, 1.0f, yes_text);
        float no_width = vita2d_pgf_text_width(pgf, 1.0f, no_text);
        
        float button_spacing = 80;
        float yes_x = dialog_x + (dialog_w / 2) - button_spacing - (yes_width / 2);
        float no_x = dialog_x + (dialog_w / 2) + button_spacing - (no_width / 2);
        float button_y = dialog_y + 140;
        
        if (delete_confirmation_selection) {
            // Yes is selected
            draw_feathered_highlight(yes_x - 30, button_y - 22, yes_width + 60, 35);
        } else {
            // No is selected
            draw_feathered_highlight(no_x - 30, button_y - 22, no_width + 60, 35);
        }
        
        // Draw button text
        vita2d_pgf_draw_text(pgf, yes_x, button_y, MONO_WHITE, 1.0f, yes_text);
        vita2d_pgf_draw_text(pgf, no_x, button_y, MONO_WHITE, 1.0f, no_text);
    }

    vita2d_end_drawing();
} 