#include "app.h"
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>
#include <psp2/libssl.h>
#include <psp2/paf.h>
#include <psp2/sysmodule.h>
#include <psp2/common_dialog.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/system_param.h>
#include <psp2/ime_dialog.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <algorithm>
#include <memory>
#include <thread>
#include <chrono>
#include <jsoncpp/json/json.h>
#include <math.h>

#include "config.h"
#include "net.h"
#include "ui.h"
#include "keyboard.h"
#include "types.h"
#include "settings.h"
#include "persistence.h"
#include "camera.h"
#include "image_utils.h"
#include "sessions.h"
#include "input.h"

// color palette
#define MONO_BLACK RGBA8(0, 0, 0, 255)           
#define MONO_DARK_GRAY RGBA8(48, 48, 48, 255)    
#define MONO_GRAY RGBA8(128, 128, 128, 255)      
#define MONO_LIGHT_GRAY RGBA8(160, 160, 160, 255) 
#define MONO_WHITE RGBA8(255, 255, 255, 255)    


const unsigned int FADE_SPEED = 8;
const unsigned int CAMERA_FADE_SPEED = 15;

void initialize_app(AppContext& ctx) {
    // Load system modules
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);
    sceSysmoduleLoadModule(SCE_SYSMODULE_IME);


    // Init network
    SceNetInitParam netInitParam;
    ctx.memid = sceKernelAllocMemBlock("SceNetMemory", 0x0C20D060, 1 * 1024 * 1024, NULL);
    sceKernelGetMemBlockBase(ctx.memid, &netInitParam.memory);
    netInitParam.size = 1 * 1024 * 1024;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);

    sceNetCtlInit();
    sceHttpInit(1 * 1024 * 1024);
    sceSslInit(1 * 1024 * 1024);

    // Init UI and input
    keyboard_init();
    vita2d_init();
    vita2d_set_clear_color(MONO_BLACK);
    ctx.pgf = vita2d_load_default_pgf();

    // Init camera
    ctx.camera_initialized = camera_init();
    ctx.camera_mode_active = false;
    ctx.photo_taken = false;
    ctx.staged_photo = NULL;
    ctx.photo_to_free = NULL;
    ctx.camera_fade_alpha = 0;

    // Load settings
    ctx.settings = load_settings();
    ctx.app_state = AppState::CHAT;
    ctx.settings_selection = SettingsSelection::ENDPOINT;
    ctx.settings_model_selection_open = false;
    ctx.settings_model_selection_index = 0;
    
    // Init sessions
    ctx.sessions = load_sessions();
    ctx.sessions.push_back(ChatSession()); // Add a new blank session
    ctx.current_session_index = ctx.sessions.size() - 1; // Set the current session to the new blank one
    ctx.session_selection_index = 0;
    ctx.session_scroll_offset = 0;
    ctx.show_delete_confirmation = false;
    ctx.delete_confirmation_selection = false;

    // text wrapping for loaded messages
    const int BUBBLE_CONTENT_WIDTH = 400 - 30; // Same as when creating new messages
    for (auto& session : ctx.sessions) {
        for (auto& msg : session) {
            msg.wrapped_text = wrap_text(ctx.pgf, msg.text, BUBBLE_CONTENT_WIDTH);
            if (!msg.reasoning.empty()) {
                msg.wrapped_reasoning = wrap_text(ctx.pgf, msg.reasoning, BUBBLE_CONTENT_WIDTH - 10);
            }
        }
    }

    // Init UI state
    ctx.user_question = "";
    ctx.keyboard_active = false;
    ctx.settings_keyboard_active = false;
    ctx.scroll_offset = 0;
    ctx.total_history_height = 0;
    ctx.current_selection = UISelection::INPUT_PILL;
    ctx.hovered_message_index = -1; // -1 means no message is hovered
    
    // model selection state
    ctx.available_models.clear();
    ctx.selected_model_index = -1;
    ctx.hovered_model_index = -1;
    ctx.model_selection_open = false;
    ctx.is_fetching_models = false;
    ctx.startup_counter = 0;
    ctx.models_loaded = false;
    ctx.fetch_scheduled = false;
    ctx.connection_failed = false;

    // Initialize animation state
    ctx.model_dropup_h = 0.0f;
    ctx.ui_alpha = 0;  // Start fully transparent
    ctx.model_pill_alpha = 0;  // Model pill also starts fully transparent
    ctx.start_button_hold_duration = 0.0f;
}

void run_app(AppContext& ctx) {
    SceCtrlData pad, old_pad;
    memset(&old_pad, 0, sizeof(old_pad));
    
    bool should_exit = false;
    while (!should_exit) {
        
        if (ctx.photo_to_free) {
            vita2d_free_texture(ctx.photo_to_free);
            ctx.photo_to_free = NULL;
        }

        // Handle UI fade-in
        if (ctx.ui_alpha < 255 && ctx.models_loaded) {
            ctx.ui_alpha += FADE_SPEED;
            if (ctx.ui_alpha > 255) ctx.ui_alpha = 255;
            
            // Fade in model pill at the same rate as the main UI
            ctx.model_pill_alpha += FADE_SPEED;
            if (ctx.model_pill_alpha > 255) ctx.model_pill_alpha = 255;
        }

        // Handle camera fade-in/out
        if (ctx.camera_mode_active) {
            if (ctx.camera_fade_alpha < 180) { // Target alpha for overlay
                ctx.camera_fade_alpha += CAMERA_FADE_SPEED;
                if (ctx.camera_fade_alpha > 180) ctx.camera_fade_alpha = 180;
            }
        } else {
            if (ctx.camera_fade_alpha > 0) {
                ctx.camera_fade_alpha = (ctx.camera_fade_alpha > CAMERA_FADE_SPEED) ? 
                                       ctx.camera_fade_alpha - CAMERA_FADE_SPEED : 0;
            }
        }

        // Handle model selection slide animation
        float model_dropup_target_h = ctx.model_selection_open ? 
                                     (ctx.available_models.size() * 35 + 15) : 0.0f;
        ctx.model_dropup_h += (model_dropup_target_h - ctx.model_dropup_h) * 0.25f; // Easing factor

        // Handle message fade-in animation
        for (auto& session : ctx.sessions) {
            for (auto& msg : session) {
                if (msg.alpha < 255) {
                    msg.alpha += 15; // Animation speed
                    if (msg.alpha > 255) msg.alpha = 255;
                }
            }
        }

        // Trigger model fetching after the first frame
        if (ctx.startup_counter == 1 && !ctx.is_fetching_models && ctx.available_models.empty()) {
            ctx.is_fetching_models = true;
        }
        
        // Perform the actual fetch on the next frame after setting the flag
        if (ctx.is_fetching_models && !ctx.fetch_scheduled && ctx.startup_counter <= 2) {
            ctx.fetch_scheduled = true;
        } else if (ctx.is_fetching_models && ctx.fetch_scheduled && ctx.startup_counter <= 2) {
            ctx.available_models = fetch_models(ctx.settings.endpoint, ctx.settings.apiKey);

            if (ctx.available_models.empty()) {
                ctx.selected_model_index = -1;
            } else {
                // Check if there's a default model for the current endpoint
                if (ctx.settings.default_models.count(ctx.settings.endpoint) > 0) {
                    const std::string& default_model = ctx.settings.default_models.at(ctx.settings.endpoint);
                    auto it = std::find(ctx.available_models.begin(), ctx.available_models.end(), default_model);
                    if (it != ctx.available_models.end()) {
                        // Default model found, set it as selected
                        ctx.selected_model_index = std::distance(ctx.available_models.begin(), it);
                    } else {
                        // Default model not found in the available list, fallback to first model
                        ctx.selected_model_index = 0;
                    }
                } else {
                    // No default model, select the first one
                    ctx.selected_model_index = 0;
                }
            }
            
            ctx.hovered_model_index = ctx.selected_model_index;
            ctx.is_fetching_models = false;
            ctx.fetch_scheduled = false;
            ctx.models_loaded = true;  // Mark models as loaded to trigger the rest of the UI to fade in
        }

        if (ctx.startup_counter < 2) {
            ctx.startup_counter++;
        }

        sceCtrlPeekBufferPositive(0, &pad, 1);

        // Hold START to exit
        if (pad.buttons & SCE_CTRL_START) {
            ctx.start_button_hold_duration += 1.0f / 60.0f; // Assuming 60 FPS
            if (ctx.start_button_hold_duration >= 2.0f) {
                should_exit = true;
            }
        } else {
            ctx.start_button_hold_duration = 0.0f;
        }

        if (should_exit) {
            continue;
        }

        // Handle input based on the current app state
        if (ctx.app_state == AppState::CHAT) {
            // Handle keyboard input for chat screen
            if (ctx.keyboard_active) {
                KeyboardState state = keyboard_update();
                if (state == KEYBOARD_STATE_FINISHED) {
                    ctx.user_question = keyboard_get_text();
                    ctx.keyboard_active = false;

                    const int BUBBLE_CONTENT_WIDTH = 400 - 30; // 400 bubble width, 15px padding each side

                    ChatMessage user_msg;
                    user_msg.sender = ChatMessage::USER;
                    user_msg.text = ctx.user_question;
                    user_msg.wrapped_text = wrap_text(ctx.pgf, ctx.user_question, BUBBLE_CONTENT_WIDTH);
                    user_msg.alpha = 0;
                    
                    // Temporarily hold the photo to be sent. We'll save it later.
                    vita2d_texture* photo_to_send = NULL;
                    if (ctx.staged_photo) {
                        photo_to_send = ctx.staged_photo;
                        user_msg.image = photo_to_send; // Attach for immediate UI display
                        ctx.staged_photo = NULL;      // Clear staged photo
                    }

                    ctx.sessions[ctx.current_session_index].push_back(user_msg);
                    
                    std::string submitted_question = ctx.user_question;
                    ctx.user_question.clear();

                    // --- Animation loop for user message ---
                    bool user_msg_faded_in = false;
                    while (!user_msg_faded_in && !should_exit) {
                        user_msg_faded_in = true;
                        for (auto& msg : ctx.sessions[ctx.current_session_index]) {
                            if (msg.alpha < 255) {
                                msg.alpha += 15;
                                if (msg.alpha > 255) msg.alpha = 255;
                                user_msg_faded_in = false; // Still animating
                            }
                        }

                        sceCtrlPeekBufferPositive(0, &pad, 1);
                        // No START check here anymore, handled globally
                        
                        // Get camera texture if camera is active
                        vita2d_texture* camera_tex = NULL;
                        if (ctx.camera_mode_active && camera_is_active() && !ctx.photo_taken) {
                            camera_tex = camera_get_frame_texture();
                        }

                        draw_ui(ctx.pgf, ctx.sessions[ctx.current_session_index], ctx.user_question, 
                               ctx.scroll_offset, ctx.total_history_height, ctx.current_selection, 
                               ctx.available_models, ctx.model_selection_open ? ctx.hovered_model_index : ctx.selected_model_index, 
                               ctx.model_selection_open, ctx.is_fetching_models, !ctx.available_models.empty(), 
                               ctx.ui_alpha, ctx.model_pill_alpha, ctx.camera_mode_active, 
                               ctx.photo_taken ? ctx.staged_photo : camera_tex, ctx.photo_taken, 
                               ctx.staged_photo, ctx.camera_fade_alpha, ctx.model_dropup_h, ctx.hovered_message_index,
                               ctx.start_button_hold_duration);
                        vita2d_common_dialog_update();
                        vita2d_swap_buffers();
                    }
                    if(should_exit) continue;

                    // --- Now, save the image and make the network request ---

                    // If there was a photo, save it to a file now and update the message path
                    if (photo_to_send) {
                        std::string image_filename = generate_image_filename(ctx.current_session_index, ctx.sessions[ctx.current_session_index].size() - 1);
                        if (save_texture_to_file(photo_to_send, image_filename)) {
                            // Find the message we just added and update its image_path
                            ctx.sessions[ctx.current_session_index].back().image_path = image_filename;
                        }
                    }

                    // Save sessions right after potentially adding an image path
                    save_sessions(ctx.sessions);

                    std::string model_name = (ctx.selected_model_index >= 0 && ctx.selected_model_index < ctx.available_models.size()) ? 
                                           ctx.available_models[ctx.selected_model_index] : MODEL;
                    std::string response_text;

                    if (photo_to_send != NULL) {
                        // We have an image to send
                        std::string base64_image = encode_texture_to_base64_png(photo_to_send);
                        if (!base64_image.empty()) {
                            response_text = nativePostRequestWithImage(ctx.settings.endpoint, submitted_question, 
                                                                     base64_image, model_name, ctx.settings.apiKey);
                        } else {
                            // Handle encoding error
                            response_text = "Error: Could not encode image.";
                        }
                    } else {
                        // Build the json obj w conversation history
                        Json::Value root;
                        root["model"] = model_name;
                        
                        Json::Value messages(Json::arrayValue);
                        for (const auto& msg : ctx.sessions[ctx.current_session_index]) {
                            Json::Value message;
                            if (msg.sender == ChatMessage::USER) {
                                message["role"] = "user";
                            } else {
                                message["role"] = "assistant";
                            }
                            message["content"] = msg.text;
                            messages.append(message);
                        }
                        root["messages"] = messages;
                        
                        Json::StreamWriterBuilder writer;
                        std::string json_payload = Json::writeString(writer, root);
                        
                        response_text = nativePostRequest(ctx.settings.endpoint, json_payload, ctx.settings.apiKey);
                    }

                    // --- Parse JSON response ---
                    Json::Value root;
                    Json::CharReaderBuilder reader_builder;
                    std::unique_ptr<Json::CharReader> const reader(reader_builder.newCharReader());
                    JSONCPP_STRING errs;
                    std::string parsed_content = response_text; // fallback to raw response
                    std::string reasoning_text = ""; 

                    if (reader->parse(response_text.c_str(), response_text.c_str() + response_text.length(), &root, &errs)) {
                        if (root.isObject() && root.isMember("choices") && root["choices"].isArray() && root["choices"].size() > 0) {
                            const Json::Value& first_choice = root["choices"][0];
                            if (first_choice.isObject() && first_choice.isMember("message") && first_choice["message"].isObject() && first_choice["message"].isMember("content")) {
                                std::string full_content = first_choice["message"]["content"].asString();
                                
                                // parse think tags for formatting and presentation
                                size_t thought_start = full_content.find("<think>");
                                size_t thought_end = full_content.find("</think>");
                                
                                if (thought_start != std::string::npos && thought_end != std::string::npos && thought_end > thought_start) {
                                    
                                    reasoning_text = full_content.substr(thought_start + 7, thought_end - thought_start - 7);
                                    
                                    parsed_content = full_content.substr(0, thought_start);
                                    if (thought_end + 8 < full_content.length()) {
                                        parsed_content += full_content.substr(thought_end + 8);
                                    }
                                } else {
                                    parsed_content = full_content;
                                }
                            }
                        }
                    }
                    // --- End of JSON parsing ---

                    // Add the new message from the LLM to the chat history
                    ChatMessage llm_msg;
                    llm_msg.sender = ChatMessage::LLM;
                    llm_msg.text = parsed_content;
                    llm_msg.wrapped_text = wrap_text(ctx.pgf, parsed_content, BUBBLE_CONTENT_WIDTH);
                    llm_msg.alpha = 0;
                    
                    // Store reasoning if available
                    if (!reasoning_text.empty()) {
                        llm_msg.reasoning = reasoning_text;
                        llm_msg.wrapped_reasoning = wrap_text(ctx.pgf, reasoning_text, BUBBLE_CONTENT_WIDTH - 10);
                    }
                    
                    ctx.sessions[ctx.current_session_index].push_back(llm_msg);

                    // Save sessions after adding an LLM response
                    save_sessions(ctx.sessions);

                    // Save sessions whenever a message is sent
                    if ((pad.buttons & SCE_CTRL_CROSS) && !(old_pad.buttons & SCE_CTRL_CROSS) && 
                        ctx.current_selection == UISelection::INPUT_PILL && !ctx.user_question.empty()) {
                        save_sessions(ctx.sessions);
                    }

                } else if (state == KEYBOARD_STATE_NONE) {
                    ctx.keyboard_active = false;
                }
            } else {
                // Handle regular chat input when keyboard is not active
                handle_chat_input(
                    pad, old_pad, ctx.current_selection, ctx.hovered_message_index,
                    ctx.keyboard_active, ctx.camera_mode_active, ctx.photo_taken,
                    ctx.model_selection_open, ctx.hovered_model_index, ctx.selected_model_index,
                    ctx.scroll_offset, ctx.total_history_height, ctx.staged_photo, ctx.photo_to_free,
                    ctx.sessions[ctx.current_session_index], ctx.available_models, ctx.camera_initialized,
                    ctx.app_state
                );
            }
        } else if (ctx.app_state == AppState::SETTINGS) {
            // Handle settings input
            if (ctx.settings_keyboard_active) {
                // Handle settings keyboard input
                KeyboardState state;
                std::string old_endpoint = ctx.settings.endpoint;
                std::string old_apiKey = ctx.settings.apiKey;
                handle_settings_keyboard(ctx.settings, ctx.settings_selection, ctx.settings_keyboard_active, state);
                
                if (state == KEYBOARD_STATE_FINISHED) {
                    bool settings_changed = false;
                    
                    // Check if settings actually changed
                    if ((ctx.settings_selection == SettingsSelection::ENDPOINT && ctx.settings.endpoint != old_endpoint) ||
                        (ctx.settings_selection == SettingsSelection::API_KEY_SETTING && ctx.settings.apiKey != old_apiKey)) {
                        settings_changed = true;
                    }
                    
                    if (settings_changed) {
                        save_settings(ctx.settings);
                        
                        // Connection state tracking
                        bool is_connecting = true;  // Initially just connecting
                        bool is_fetching = false;   // Not fetching models yet
                        ctx.connection_failed = false; // Track if connection failed
                        ctx.fetch_scheduled = false;
                        
                        // Draw the connecting popup for at least a few frames
                        int min_popup_frames = 30; // Show popup for at least this many frames
                        
                        for (int i = 0; i < min_popup_frames || is_connecting || is_fetching; i++) {
                            // After a few frames, test the connection
                            if (i >= 5 && is_connecting && !is_fetching && !ctx.connection_failed) {
                                // If endpoint is empty, mark as failed
                                if (ctx.settings.endpoint.empty()) {
                                    ctx.connection_failed = true;
                                    is_connecting = false;
                                } else {
                                    // Move to fetching state
                                    is_fetching = true;
                                }
                            }
                            
                            // Then handle the actual fetch
                            if (is_fetching && i >= 10 && !ctx.fetch_scheduled) {
                                ctx.fetch_scheduled = true;
                                ctx.available_models = fetch_models(ctx.settings.endpoint, ctx.settings.apiKey);
                                
                                // Check if models were fetched successfully
                                if (ctx.available_models.empty()) {
                                    ctx.connection_failed = true;
                                    ctx.selected_model_index = -1;
                                } else {
                                    // Check if there's a default model for the new endpoint
                                    if (ctx.settings.default_models.count(ctx.settings.endpoint) > 0) {
                                        const std::string& default_model = ctx.settings.default_models.at(ctx.settings.endpoint);
                                        auto it = std::find(ctx.available_models.begin(), ctx.available_models.end(), default_model);
                                        if (it != ctx.available_models.end()) {
                                            ctx.selected_model_index = std::distance(ctx.available_models.begin(), it);
                                        } else {
                                            ctx.selected_model_index = 0; // Fallback
                                        }
                                    } else {
                                        ctx.selected_model_index = 0; // Fallback
                                    }
                                }
                                
                                ctx.hovered_model_index = ctx.selected_model_index;
                                is_fetching = false;
                                is_connecting = false;
                                ctx.models_loaded = true;
                            }
                            
                            // Determine the popup state for drawing
                            bool show_popup = true;
                            bool show_fetching = is_fetching;
                            bool show_failed = ctx.connection_failed;
                            
                            // Draw the settings UI with the connecting popup
                            draw_settings_ui(ctx.pgf, ctx.settings, ctx.settings_selection, ctx.ui_alpha, 
                                           ctx.model_pill_alpha, ctx.available_models, ctx.settings_model_selection_index, 
                                           show_popup, show_fetching, show_failed, ctx.settings_model_selection_open);
                            vita2d_common_dialog_update();
                            vita2d_swap_buffers();
                            
                            // Check for START button to exit
                            sceCtrlPeekBufferPositive(0, &pad, 1);
                            // No START check here anymore, handled globally
                        }
                    }
                }
            } else {
                // Handle regular settings input
                handle_settings_input(pad, old_pad, ctx.settings, ctx.settings_selection, 
                                     ctx.settings_keyboard_active, ctx.app_state,
                                     ctx.settings_model_selection_open,
                                     ctx.settings_model_selection_index);
            }
        } else if (ctx.app_state == AppState::SESSIONS) {
            // Handle sessions input
            handle_sessions_input(
                pad, old_pad, ctx.sessions, ctx.session_selection_index, ctx.current_session_index,
                ctx.session_scroll_offset, ctx.show_delete_confirmation, ctx.delete_confirmation_selection,
                ctx.app_state, ctx.scroll_offset
            );
        }

        // --- Drawing ---
        if (ctx.app_state == AppState::CHAT) {
            // Get camera texture if camera is active
            vita2d_texture* camera_tex = NULL;
            if (ctx.camera_mode_active && camera_is_active() && !ctx.photo_taken) {
                camera_tex = camera_get_frame_texture();
            }
            
            draw_ui(ctx.pgf, ctx.sessions[ctx.current_session_index], ctx.user_question, 
                   ctx.scroll_offset, ctx.total_history_height, ctx.current_selection, 
                   ctx.available_models, ctx.model_selection_open ? ctx.hovered_model_index : ctx.selected_model_index, 
                   ctx.model_selection_open, ctx.is_fetching_models, !ctx.available_models.empty(), 
                   ctx.ui_alpha, ctx.model_pill_alpha, ctx.camera_mode_active, 
                   ctx.photo_taken ? ctx.staged_photo : camera_tex, ctx.photo_taken, 
                   ctx.staged_photo, ctx.camera_fade_alpha, ctx.model_dropup_h, ctx.hovered_message_index,
                   ctx.start_button_hold_duration);
        } else if (ctx.app_state == AppState::SETTINGS) {
            draw_settings_ui(ctx.pgf, ctx.settings, ctx.settings_selection, ctx.ui_alpha, 
                           ctx.model_pill_alpha, ctx.available_models, ctx.settings_model_selection_index, 
                           ctx.is_fetching_models, ctx.is_fetching_models, ctx.connection_failed, ctx.settings_model_selection_open);
        } else if (ctx.app_state == AppState::SESSIONS) {
            draw_sessions_ui(ctx.pgf, ctx.sessions, ctx.session_scroll_offset, ctx.session_selection_index, 
                           ctx.show_delete_confirmation, ctx.delete_confirmation_selection);
        }
        
        vita2d_common_dialog_update();
        vita2d_swap_buffers();

        old_pad = pad;
    }

    // Save sessions before exiting
    save_sessions(ctx.sessions);
}

// unload everything
void cleanup_app(AppContext& ctx) {
    // Clean up textures in chat history
    for (auto& session : ctx.sessions) {
        for (auto& msg : session) {
            if (msg.image) {
                vita2d_free_texture(msg.image);
            }
        }
    }

    // Clean up camera resources
    if (ctx.camera_initialized) {
        if (ctx.camera_mode_active) {
            camera_stop();
            camera_close();
        }
        if (ctx.staged_photo) {
            vita2d_free_texture(ctx.staged_photo);
        }
        camera_terminate();
    }

    // Clean up UI textures
    cleanup_ui_textures();

    // Clean up other resources
    keyboard_terminate();
    vita2d_fini();
    vita2d_free_pgf(ctx.pgf);
    
    // Clean up network resources
    sceSslTerm();
    sceHttpTerm();
    sceNetCtlTerm();
    sceNetTerm();
    sceKernelFreeMemBlock(ctx.memid);
    
    // Unload modules
    sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
    sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

} 