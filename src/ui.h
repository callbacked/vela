#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include <vita2d.h>
#include "types.h"


void draw_quarter_circle(float cx, float cy, float radius, int quadrant, unsigned int color);
void draw_circle(float cx, float cy, float radius, unsigned int color);
void draw_rounded_rect(float x, float y, float w, float h, float radius, unsigned int color);


void draw_ui(
    vita2d_pgf *pgf,
    const std::vector<ChatMessage>& chat_history,
    const std::string& user_question,
    int scroll_offset,
    int& total_history_height,
    UISelection current_selection,
    const std::vector<std::string>& available_models,
    int selected_model_index,
    bool model_selection_open,
    bool is_fetching_models,
    bool can_interact,
    unsigned int ui_alpha,
    unsigned int model_pill_alpha,
    bool camera_mode,
    vita2d_texture* camera_texture,
    bool photo_taken,
    vita2d_texture* staged_photo = NULL,
    unsigned int camera_overlay_alpha = 0,
    float model_dropup_h = 0.0f,
    int hovered_message_index = -1,
    float start_button_hold_duration = 0.0f
);

// Settings UI drawing function
void draw_settings_ui(
    vita2d_pgf *pgf,
    const Settings& settings,
    SettingsSelection selection,
    unsigned int ui_alpha,
    unsigned int model_pill_alpha,
    const std::vector<std::string>& available_models,
    int selected_model_index,
    bool show_connecting_popup = false,
    bool is_fetching = false,
    bool connection_failed = false,
    bool model_selection_open = false
);


std::vector<std::string> wrap_text(vita2d_pgf *pgf, const std::string& text, int max_line_width_pixels);


void draw_scene(vita2d_pgf* pgf, const std::string& text);


extern vita2d_texture* gear_icon;
extern vita2d_texture* camera_icon;

void cleanup_ui_textures();

#endif 
