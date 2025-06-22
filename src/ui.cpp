#include "ui.h"
#include <sstream>
#include <math.h>
#include <algorithm>


#define MONO_BLACK RGBA8(0, 0, 0, 255)           
#define MONO_DARK_GRAY RGBA8(48, 48, 48, 255)    
#define MONO_GRAY RGBA8(128, 128, 128, 255)      
#define MONO_LIGHT_GRAY RGBA8(160, 160, 160, 255) 
#define MONO_WHITE RGBA8(255, 255, 255, 255)     


std::vector<std::string> available_models; 


vita2d_texture* gear_icon = NULL;
vita2d_texture* camera_icon = NULL;
vita2d_texture* history_icon = NULL;

void draw_quarter_circle(float cx, float cy, float radius, int quadrant, unsigned int color) {
    for (int y = 0; y <= radius; ++y) {
        int x_width = round(sqrt(radius * radius - y * y));
        if (quadrant == 1) { // Top-right
            vita2d_draw_rectangle(cx, cy - y, x_width, 1, color);
        } else if (quadrant == 2) { // Top-left
            vita2d_draw_rectangle(cx - x_width, cy - y, x_width, 1, color);
        } else if (quadrant == 3) { // Bottom-left
            vita2d_draw_rectangle(cx - x_width, cy + y, x_width, 1, color);
        } else if (quadrant == 4) { // Bottom-right
            vita2d_draw_rectangle(cx, cy + y, x_width, 1, color);
        }
    }
}


void draw_circle(float cx, float cy, float radius, unsigned int color) {
    draw_quarter_circle(cx, cy, radius, 1, color); // Top-right
    draw_quarter_circle(cx, cy, radius, 2, color); // Top-left
    draw_quarter_circle(cx, cy, radius, 3, color); // Bottom-left
    draw_quarter_circle(cx, cy, radius, 4, color); // Bottom-right
}

void draw_rounded_rect(float x, float y, float w, float h, float radius, unsigned int color) {
    if (h <= 0 || w <= 0) return;

    float r = radius;
    if (r > w / 2.0f) r = w / 2.0f;
    if (r > h / 2.0f) r = h / 2.0f;
    if (r < 0) r = 0;


    vita2d_draw_rectangle(x + r, y, w - 2 * r, h, color);
    vita2d_draw_rectangle(x, y + r, r, h - 2 * r, color);
    vita2d_draw_rectangle(x + w - r, y + r, r, h - 2 * r, color);


    draw_quarter_circle(x + r, y + r, r, 2, color); // Top-left
    draw_quarter_circle(x + w - r, y + r, r, 1, color); // Top-right
    draw_quarter_circle(x + r, y + h - r, r, 3, color); // Bottom-left
    draw_quarter_circle(x + w - r, y + h - r, r, 4, color); // Bottom-right
}

void draw_ui(
    vita2d_pgf *pgf,
    const std::vector<ChatMessage>& chat_history,
    const std::string& user_question,
    int scroll_offset,
    int& total_history_height,
    UISelection current_selection,
    const std::vector<std::string>& models,
    int selected_model_index,
    bool model_selection_open,
    bool is_fetching_models,
    bool can_interact,
    unsigned int ui_alpha,
    unsigned int model_pill_alpha,
    bool camera_mode,
    vita2d_texture* camera_texture,
    bool photo_taken,
    vita2d_texture* staged_photo,
    unsigned int camera_overlay_alpha,
    float model_dropup_h,
    int hovered_message_index,
    float start_button_hold_duration)
{
    available_models = models;
    
    if (gear_icon == NULL) {
        gear_icon = vita2d_load_PNG_file("app0:img/gear.png");
    }
    if (camera_icon == NULL) {
        camera_icon = vita2d_load_PNG_file("app0:img/camera.png");
    }
    if (history_icon == NULL) {
        history_icon = vita2d_load_PNG_file("app0:img/history.png");
    }

    vita2d_start_drawing();
    vita2d_clear_screen();

    if (start_button_hold_duration > 0.0f) {
        float bar_width = (start_button_hold_duration / 2.0f) * SCREEN_WIDTH;
        vita2d_draw_rectangle(0, 0, bar_width, 5, MONO_WHITE);
    }

    if ((ui_alpha == 0 || is_fetching_models) && models.empty()) {
        const char* loading_text = "Loading Models...";
        float text_width = vita2d_pgf_text_width(pgf, 1.0f, loading_text);
        vita2d_pgf_draw_text(pgf, (SCREEN_WIDTH - text_width) / 2, SCREEN_HEIGHT / 2, 
                             RGBA8(255, 255, 255, 255), 1.0f, loading_text);
    }

    auto apply_alpha = [ui_alpha](unsigned int color) -> unsigned int {
        unsigned int existing_alpha = color & 0xFF;
        unsigned int new_alpha = (existing_alpha * ui_alpha) / 255;
        return (color & 0xFFFFFF00) | new_alpha;
    };

    auto apply_model_pill_alpha = [model_pill_alpha](unsigned int color) -> unsigned int {
        unsigned int existing_alpha = color & 0xFF;
        unsigned int new_alpha = (existing_alpha * model_pill_alpha) / 255;
        return (color & 0xFFFFFF00) | new_alpha;
    };

    if (ui_alpha > 0) {
        int current_y = 50 - scroll_offset;
        total_history_height = 0;
        
        int msg_index = 0;
        
        for (const auto& msg : chat_history) {
            bool has_image = (msg.image != NULL);
            float image_h = has_image ? 150.0f : 0.0f; // Height of the image thumbnail
            float image_w = has_image ? (150.0f * vita2d_texture_get_width(msg.image) / vita2d_texture_get_height(msg.image)) : 0.0f;

            int message_height = 0;
            if (&msg != &chat_history[0]) {
                 total_history_height += 10; // Margin
            }

            unsigned int message_alpha = (msg.alpha * ui_alpha) / 255;
            
            bool is_hovered = (msg_index == hovered_message_index);
            
            if (is_hovered) {
                float highlight_x = msg.sender == ChatMessage::USER ? SCREEN_WIDTH - 480 : 10;
                float highlight_w = 480;
                float highlight_h = 0;
                
                if (msg.sender == ChatMessage::USER) {
                    highlight_h = msg.wrapped_text.size() * 20 + 20 + (has_image ? image_h + 20 : 0) + 10;
                } else {
                    highlight_h = msg.wrapped_text.size() * 20 + 20 + (has_image ? image_h + 10 : 0) + 10;
                    if (msg.show_reasoning && !msg.wrapped_reasoning.empty()) {
                        highlight_h += msg.wrapped_reasoning.size() * 20 + 30;
                    }
                }
                
                vita2d_draw_rectangle(SCREEN_WIDTH - 10, current_y, 5, highlight_h, RGBA8(160, 160, 160, message_alpha));
            }

            if (msg.sender == ChatMessage::USER) {
                float max_text_width = 0.0f;
                for (const auto& line : msg.wrapped_text) {
                    float line_width = vita2d_pgf_text_width(pgf, 1.0f, line.c_str());
                    if (line_width > max_text_width) max_text_width = line_width;
                }
                float content_width = std::max(image_w, max_text_width);
                float bubble_width = content_width + 30;
                if (bubble_width > 480.0f) bubble_width = 480.0f;

                message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? image_h + 20 : 0);
                
                float bubble_x = SCREEN_WIDTH - bubble_width - 20;

                
                int text_y = current_y + 25;

                if (has_image) {
                    vita2d_draw_texture_scale(msg.image, bubble_x + 15, text_y, image_w / vita2d_texture_get_width(msg.image), image_h / vita2d_texture_get_height(msg.image));
                    text_y += image_h + 20;
                }

                for (const auto& line : msg.wrapped_text) {
                    vita2d_pgf_draw_text(pgf, bubble_x + 15, text_y, RGBA8(255, 255, 255, message_alpha), 1.0f, line.c_str());
                    text_y += 20;
                }
            } else {
                message_height = msg.wrapped_text.size() * 20 + 20 + (has_image ? image_h + 10 : 0);
                
                if (msg.show_reasoning && !msg.wrapped_reasoning.empty()) {
                    message_height += msg.wrapped_reasoning.size() * 20 + 30; // Extra space for reasoning
                }
                
                if (!msg.reasoning.empty() && is_hovered) {
                    const char* thinking_prompt = msg.show_reasoning ? "△ Hide Thinking" : "△ Show Thinking";
                    vita2d_pgf_draw_text(pgf, 35, current_y - 10, RGBA8(128, 128, 128, message_alpha), 0.9f, thinking_prompt);
                }
                
                unsigned int bubble_color = is_hovered ? RGBA8(50, 50, 50, message_alpha) : RGBA8(40, 40, 40, message_alpha);
                draw_rounded_rect(20, current_y, 480, message_height, 10, bubble_color);
                int text_y = current_y + 25;

                if (has_image) {
                    vita2d_draw_texture_scale(msg.image, 30, text_y, image_w / vita2d_texture_get_width(msg.image), image_h / vita2d_texture_get_height(msg.image));
                    text_y += image_h + 10;
                }
                
                if (msg.show_reasoning && !msg.wrapped_reasoning.empty()) {
                    for (const auto& line : msg.wrapped_reasoning) {
                        vita2d_pgf_draw_text(pgf, 45, text_y, RGBA8(180, 180, 180, message_alpha), 0.9f, line.c_str());
                        text_y += 20;
                    }
                    
                    text_y += 10;
                }

                for (const auto& line : msg.wrapped_text) {
                    vita2d_pgf_draw_text(pgf, 35, text_y, RGBA8(255, 255, 255, message_alpha), 1.0f, line.c_str());
                    text_y += 20;
                }
            }
            total_history_height += message_height;
            current_y += message_height + 10;
            msg_index++;
        }

        // Clamp scroll offset
        int max_scroll = total_history_height - (SCREEN_HEIGHT - 170); 
        if (max_scroll < 0) max_scroll = 0;
        scroll_offset = std::max(0, std::min(scroll_offset, max_scroll));

        if (scroll_offset > 0) {
            vita2d_pgf_draw_text(pgf, SCREEN_WIDTH - 30, 40, RGBA8(255, 255, 255, ui_alpha), 1.0f, "^");
        }
        if (scroll_offset < max_scroll) {
            vita2d_pgf_draw_text(pgf, SCREEN_WIDTH - 30, SCREEN_HEIGHT - 100, RGBA8(255, 255, 255, ui_alpha), 1.0f, "v");
        }
    }

    float model_pill_w = 300;
    float model_pill_h = 42;
    float model_pill_x = (SCREEN_WIDTH - model_pill_w) / 2;
    float model_pill_y = SCREEN_HEIGHT - 150;

    unsigned int model_pill_color = (current_selection == UISelection::MODEL_PILL) ? 
        RGBA8(160, 160, 160, model_pill_alpha) : RGBA8(48, 48, 48, model_pill_alpha);
    draw_rounded_rect(model_pill_x, model_pill_y, model_pill_w, model_pill_h, model_pill_h / 2, model_pill_color);
    
    std::string model_display_text;
    if (is_fetching_models) {
        model_display_text = "Fetching models...";
    } else if (available_models.empty()) {
        model_display_text = "No models available";
    } else if (selected_model_index >= 0 && selected_model_index < available_models.size()) {
        model_display_text = available_models[selected_model_index];
    } else {
        model_display_text = "No model selected";
    }
    
    float model_text_width = vita2d_pgf_text_width(pgf, 1.0f, model_display_text.c_str());
    vita2d_pgf_draw_text(pgf, model_pill_x + (model_pill_w - model_text_width) / 2, model_pill_y + 30, 
                         RGBA8(255, 255, 255, model_pill_alpha), 1.0f, model_display_text.c_str());
    
    if (model_dropup_h > 1.0f && !is_fetching_models && !available_models.empty()) {
        float dropup_y = model_pill_y - model_dropup_h - 5;
        draw_rounded_rect(model_pill_x, dropup_y, model_pill_w, model_dropup_h, 10, RGBA8(48, 48, 48, model_pill_alpha));

        for (int i = 0; i < available_models.size(); ++i) {
            float item_y = dropup_y + 10 + (i * 35);
            if (item_y > dropup_y && (item_y + 20) < (dropup_y + model_dropup_h)) {
                unsigned int item_color = (i == selected_model_index) ? 
                    RGBA8(160, 160, 160, model_pill_alpha) : RGBA8(255, 255, 255, model_pill_alpha);
                
                vita2d_pgf_draw_text(pgf, model_pill_x + 20, item_y + 15, item_color, 1.0f, available_models[i].c_str());
                if (i == selected_model_index) {
                     vita2d_draw_rectangle(model_pill_x + 5, item_y, 5, 20, RGBA8(160, 160, 160, model_pill_alpha));
                }
            }
        }
    }

    if (ui_alpha > 0) {
        float pill_h = 60;  
        float pill_w = 420; 
        float pill_y = SCREEN_HEIGHT - 90; 
        float outer_circle_radius = pill_h / 2;
        
        float pill_x = (SCREEN_WIDTH - pill_w) / 2;

        float circle1_cx = pill_x - 20 - outer_circle_radius;

        unsigned int circle1_color = (current_selection == UISelection::ACTION_BUTTON_1) ? 
            RGBA8(160, 160, 160, ui_alpha) : RGBA8(48, 48, 48, ui_alpha);
        draw_circle(circle1_cx, pill_y + outer_circle_radius, outer_circle_radius, circle1_color);
        draw_circle(circle1_cx, pill_y + outer_circle_radius, outer_circle_radius - 2, RGBA8(24, 24, 24, ui_alpha));
        
        if (gear_icon != NULL) {
            float icon_size = outer_circle_radius * 1.4; 
            float scale = icon_size / 44.0f; 
            
            float icon_x = circle1_cx - (44 * scale) / 2;
            float icon_y = pill_y + outer_circle_radius - (44 * scale) / 2;
            
            vita2d_draw_texture_scale(gear_icon, icon_x, icon_y, scale, scale);
        }

        unsigned int pill_base_color = can_interact ? MONO_DARK_GRAY : RGBA8(24, 24, 24, 255);
        unsigned int pill_color = (current_selection == UISelection::INPUT_PILL && can_interact) ? 
            MONO_LIGHT_GRAY : pill_base_color;
        draw_rounded_rect(pill_x, pill_y, pill_w, pill_h, pill_h / 2, pill_color);
        
        float circle_history_cx = circle1_cx - (outer_circle_radius * 2) - 10;
        unsigned int circle_history_color = (current_selection == UISelection::ACTION_BUTTON_3) ?
            RGBA8(160, 160, 160, ui_alpha) : RGBA8(48, 48, 48, ui_alpha);
        draw_circle(circle_history_cx, pill_y + outer_circle_radius, outer_circle_radius, circle_history_color);
        draw_circle(circle_history_cx, pill_y + outer_circle_radius, outer_circle_radius - 2, RGBA8(24, 24, 24, ui_alpha));
        
        if (history_icon != NULL) {
            float icon_size = outer_circle_radius * 1.4; 
            float scale = icon_size / 44.0f; 
            
            float icon_x = circle_history_cx - (44 * scale) / 2;
            float icon_y = pill_y + outer_circle_radius - (44 * scale) / 2;
            
            vita2d_draw_texture_scale(history_icon, icon_x, icon_y, scale, scale);
        }
        
        if (staged_photo) {
            float thumb_size = 36.0f; 
            vita2d_draw_texture_scale(staged_photo, pill_x + 12, pill_y + 12, 
                                     thumb_size / vita2d_texture_get_width(staged_photo), 
                                     thumb_size / vita2d_texture_get_height(staged_photo));
            vita2d_pgf_draw_text(pgf, pill_x + 60, pill_y + 38, RGBA8(255, 255, 255, ui_alpha), 1.0f, "Image ready...");
        } else {
            std::string display_text;
            if (!user_question.empty()) {
                display_text = user_question;
            } else if (current_selection == UISelection::INPUT_PILL && can_interact) {
                display_text = "[X] Type...";
            } else {
                display_text = "";
            }
            vita2d_pgf_draw_text(pgf, pill_x + 30, pill_y + 38, RGBA8(255, 255, 255, ui_alpha), 1.0f, display_text.c_str());
        }

        float inner_circle_radius = pill_h / 2 - 8;
        float circle2_cx = pill_x + pill_w - inner_circle_radius - 10;
        unsigned int circle2_base_color = can_interact ? MONO_DARK_GRAY : RGBA8(24, 24, 24, 255);
        unsigned int circle2_color = (current_selection == UISelection::ACTION_BUTTON_2 && can_interact) ? 
            MONO_LIGHT_GRAY : circle2_base_color;
        
        if (camera_mode) {
            circle2_color = MONO_LIGHT_GRAY; 
        }
        
        draw_circle(circle2_cx, pill_y + outer_circle_radius, inner_circle_radius, circle2_color);
        draw_circle(circle2_cx, pill_y + outer_circle_radius, inner_circle_radius - 2, pill_color); // Inner color matches pill
        
        if (camera_icon != NULL) {
            float icon_size = inner_circle_radius * 1.4; 
            float scale = icon_size / 44.0f; 
            
            float icon_x = circle2_cx - (44 * scale) / 2;
            float icon_y = pill_y + outer_circle_radius - (44 * scale) / 2;
            
            vita2d_draw_texture_scale(camera_icon, icon_x, icon_y, scale, scale);
        }
    }

    if (camera_overlay_alpha > 0) {
        vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA8(0, 0, 0, camera_overlay_alpha));
    
        if (camera_texture != NULL) {
            unsigned int full_alpha = (camera_overlay_alpha * 255) / 180;
            if (full_alpha > 255) full_alpha = 255;

            int tex_width = vita2d_texture_get_width(camera_texture);
            int tex_height = vita2d_texture_get_height(camera_texture);
            
            float x = (SCREEN_WIDTH - tex_width) / 2.0f;
            float y = (SCREEN_HEIGHT - tex_height) / 2.0f;
            
            vita2d_draw_texture_tint(camera_texture, x, y, RGBA8(255, 255, 255, full_alpha));
            
            unsigned int hud_bg_alpha = (120 * camera_overlay_alpha) / 180;
            unsigned int hud_text_alpha = full_alpha;

            auto draw_hud_text = [&](float x, float y, const char* text) {
                vita2d_pgf_draw_text(pgf, x + 1, y + 1, RGBA8(0, 0, 0, hud_text_alpha), 1.0f, text);
                vita2d_pgf_draw_text(pgf, x, y, RGBA8(255, 255, 255, hud_text_alpha), 1.0f, text);
            };

            float top_banner_y = 48; 

            float bottom_banner_y = SCREEN_HEIGHT - 20;

            if (photo_taken) {
                const char* confirm_text = "Photo Taken!";
                float text_w = vita2d_pgf_text_width(pgf, 1.0f, confirm_text);
                draw_hud_text((SCREEN_WIDTH - text_w) / 2, top_banner_y, confirm_text);

                const char* redo_prompt = "L Redo";
                const char* confirm_prompt = "O Confirm & Exit";
                draw_hud_text(40, bottom_banner_y, redo_prompt);
                
                float confirm_prompt_w = vita2d_pgf_text_width(pgf, 1.0f, confirm_prompt);
                draw_hud_text(SCREEN_WIDTH - 40 - confirm_prompt_w, bottom_banner_y, confirm_prompt);

            } else {
                draw_circle(45, top_banner_y - 8, 8, RGBA8(192, 192, 192, hud_text_alpha));
                draw_hud_text(65, top_banner_y, "Live View");

                const char* capture_prompt = "R Capture";
                float capture_prompt_w = vita2d_pgf_text_width(pgf, 1.0f, capture_prompt);
                draw_hud_text(SCREEN_WIDTH - 40 - capture_prompt_w, top_banner_y, capture_prompt);
                
                const char* switch_prompt = "△ Switch";
                const char* exit_prompt = "O Exit";
                draw_hud_text(40, bottom_banner_y, switch_prompt);
                
                float exit_prompt_w = vita2d_pgf_text_width(pgf, 1.0f, exit_prompt);
                draw_hud_text(SCREEN_WIDTH - 40 - exit_prompt_w, bottom_banner_y, exit_prompt);
            }
        }
    }

    vita2d_end_drawing();
}

std::vector<std::string> wrap_text(vita2d_pgf *pgf, const std::string& text, int max_line_width_pixels) {
    std::vector<std::string> lines;
    std::string current_line;
    std::string word;

    for (char c : text) {
        if (c == ' ' || c == '\n') {
            if (!word.empty()) {
                std::string temp_line = current_line + (current_line.empty() ? "" : " ") + word;
                if (vita2d_pgf_text_width(pgf, 1.0f, temp_line.c_str()) > max_line_width_pixels) {
                    lines.push_back(current_line);
                    current_line = word;
                } else {
                    current_line = temp_line;
                }
                word.clear();
            }
            if (c == '\n') {
                lines.push_back(current_line);
                current_line.clear();
            }
        } else {
            word += c;
        }
    }

    if (!word.empty()) {
        std::string temp_line = current_line + (current_line.empty() ? "" : " ") + word;
        if (vita2d_pgf_text_width(pgf, 1.0f, temp_line.c_str()) > max_line_width_pixels) {
            lines.push_back(current_line);
            lines.push_back(word);
        } else {
            lines.push_back(temp_line);
        }
    } else if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    return lines;
}

void draw_scene(vita2d_pgf* pgf, const std::string& text) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    std::vector<std::string> lines = wrap_text(pgf, text, 50);
    int y = 30;
    for (const auto& line : lines) {
        vita2d_pgf_draw_text(pgf, 20, y, RGBA8(255, 255, 255, 255), 1.0f, line.c_str());
        y += 20;
    }
    
    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void cleanup_ui_textures() {
    if (gear_icon != NULL) {
        vita2d_free_texture(gear_icon);
        gear_icon = NULL;
    }
    if (camera_icon != NULL) {
        vita2d_free_texture(camera_icon);
        camera_icon = NULL;
    }
    if (history_icon != NULL) {
        vita2d_free_texture(history_icon);
        history_icon = NULL;
    }
}

void draw_settings_ui(
    vita2d_pgf *pgf,
    const Settings& settings,
    SettingsSelection selection,
    unsigned int ui_alpha,
    unsigned int model_pill_alpha,
    const std::vector<std::string>& models,
    int selected_model_index,
    bool show_connecting_popup,
    bool is_fetching,
    bool connection_failed,
    bool model_selection_open
) {
    available_models = models;
    
    vita2d_start_drawing();
    vita2d_clear_screen();
    
    if (ui_alpha > 0) {
        const char* title = "Settings";
        float title_width = vita2d_pgf_text_width(pgf, 1.5f, title);
        float title_x = (SCREEN_WIDTH - title_width) / 2;
        vita2d_pgf_draw_text(pgf, title_x, 100, RGBA8(255, 255, 255, ui_alpha), 1.5f, title);
        
        std::string endpoint_text = "Endpoint: " + settings.endpoint;
        std::string apikey_text = std::string("API Key: ") + (settings.apiKey.empty() ? "[Not set]" : "********");
        std::string default_model_text = "Default Model: ";
        if (settings.default_models.count(settings.endpoint) > 0) {
            default_model_text += settings.default_models.at(settings.endpoint);
        } else {
            default_model_text += "[Not set]";
        }

        float endpoint_text_width = vita2d_pgf_text_width(pgf, 1.0f, endpoint_text.c_str());
        float apikey_text_width = vita2d_pgf_text_width(pgf, 1.0f, apikey_text.c_str());
        float default_model_text_width = vita2d_pgf_text_width(pgf, 1.0f, default_model_text.c_str());
        
        float max_text_width = std::max({endpoint_text_width, apikey_text_width, default_model_text_width});
        float text_x = (SCREEN_WIDTH - max_text_width) / 2;
        
        float endpoint_y = 170;
        float apikey_y = 220;
        float default_model_y = 270;

        vita2d_pgf_draw_text(pgf, text_x, endpoint_y, RGBA8(255, 255, 255, ui_alpha), 1.0f, endpoint_text.c_str());
        vita2d_pgf_draw_text(pgf, text_x, apikey_y, RGBA8(255, 255, 255, ui_alpha), 1.0f, apikey_text.c_str());
        vita2d_pgf_draw_text(pgf, text_x, default_model_y, RGBA8(255, 255, 255, ui_alpha), 1.0f, default_model_text.c_str());

        int selection_y_center = 0;
        if (selection == SettingsSelection::ENDPOINT) {
            selection_y_center = endpoint_y - 8; 
        } else if (selection == SettingsSelection::API_KEY_SETTING) {
            selection_y_center = apikey_y - 8; 
        } else if (selection == SettingsSelection::DEFAULT_MODEL) {
            selection_y_center = default_model_y - 8;
        }
        
        float highlight_padding = 40.0f; 
        float highlight_width = max_text_width + (highlight_padding * 2);
        float highlight_x = (SCREEN_WIDTH - highlight_width) / 2;
        float highlight_y = selection_y_center - 10;
        float highlight_height = 25;
        
        unsigned int base_alpha = ui_alpha / 5;
        unsigned int highlight_color = RGBA8(160, 160, 160, base_alpha);
        
        float feather_width = 30.0f; 
        float main_width = highlight_width - (feather_width * 2);
        vita2d_draw_rectangle(highlight_x + feather_width, highlight_y, main_width, highlight_height, highlight_color);
        
        int feather_steps = 15; 
        for (int i = 0; i < feather_steps; i++) {
            float step_width = feather_width / feather_steps;
            
            float right_opacity = (float)(feather_steps - i) / feather_steps; 
            unsigned int right_alpha = (unsigned int)(base_alpha * right_opacity);
            unsigned int right_color = RGBA8(160, 160, 160, right_alpha);
            float right_x = highlight_x + feather_width + main_width + (i * step_width);
            vita2d_draw_rectangle(right_x, highlight_y, step_width, highlight_height, right_color);
            
            float left_opacity = (float)(feather_steps - i) / feather_steps; 
            unsigned int left_alpha = (unsigned int)(base_alpha * left_opacity);
            unsigned int left_color = RGBA8(160, 160, 160, left_alpha);
            float left_x = highlight_x + feather_width - ((i + 1) * step_width);
            vita2d_draw_rectangle(left_x, highlight_y, step_width, highlight_height, left_color);
        }

        if (model_selection_open && !available_models.empty()) {
            float dropdown_h = available_models.size() * 35 + 15;
            float dropdown_w = 400;
            float dropdown_x = (SCREEN_WIDTH - dropdown_w) / 2;
            float dropdown_y = default_model_y + 20;
            
            draw_rounded_rect(dropdown_x, dropdown_y, dropdown_w, dropdown_h, 10, RGBA8(48, 48, 48, 240));

            for (size_t i = 0; i < available_models.size(); ++i) {
                unsigned int text_color = (i == (size_t)selected_model_index) ? MONO_WHITE : MONO_LIGHT_GRAY;
                vita2d_pgf_draw_text(pgf, dropdown_x + 20, dropdown_y + 30 + (i * 35), text_color, 1.0f, available_models[i].c_str());
            }
        }

        const char* instructions = "X Edit, O Return";
        float instructions_width = vita2d_pgf_text_width(pgf, 1.0f, instructions);
        float instructions_x = (SCREEN_WIDTH - instructions_width) / 2;
        vita2d_pgf_draw_text(pgf, instructions_x, SCREEN_HEIGHT - 50, RGBA8(255, 255, 255, ui_alpha), 1.0f, instructions);
        
        if (show_connecting_popup) {
            float popup_width = 400;
            float popup_height = 100;
            float popup_x = (SCREEN_WIDTH - popup_width) / 2;
            float popup_y = (SCREEN_HEIGHT - popup_height) / 2 + 100;
            
            draw_rounded_rect(popup_x, popup_y, popup_width, popup_height, 10, RGBA8(30, 30, 30, 230));
            
            const char* popup_title;
            unsigned int title_color;
            
            if (connection_failed) {
                popup_title = "Connection Failed";
                title_color = RGBA8(255, 100, 100, 255); 
            } else {
                popup_title = "Connecting...";
                title_color = RGBA8(255, 255, 255, 255); 
            }
            
            float popup_title_width = vita2d_pgf_text_width(pgf, 1.2f, popup_title);
            float popup_title_x = (SCREEN_WIDTH - popup_title_width) / 2;
            vita2d_pgf_draw_text(pgf, popup_title_x, popup_y + 40, title_color, 1.2f, popup_title);
            
            const char* popup_msg;
            if (connection_failed) {
                popup_msg = "Check endpoint URL and API key";
            } else if (is_fetching) {
                popup_msg = "Fetching available models";
            } else {
                popup_msg = "Testing connection";
            }
            
            float popup_msg_width = vita2d_pgf_text_width(pgf, 1.0f, popup_msg);
            float popup_msg_x = (SCREEN_WIDTH - popup_msg_width) / 2;
            vita2d_pgf_draw_text(pgf, popup_msg_x, popup_y + 70, RGBA8(200, 200, 200, 255), 1.0f, popup_msg);
        }
    }
    
    vita2d_end_drawing();
} 