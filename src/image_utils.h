#pragma once

#include <vita2d.h>
#include <string>


std::string encode_texture_to_base64_png(vita2d_texture* texture);

bool save_texture_to_file(vita2d_texture* texture, const std::string& path);

vita2d_texture* load_texture_from_file(const std::string& path);

std::string generate_image_filename(int session_id, int message_id); 