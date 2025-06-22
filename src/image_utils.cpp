#include "image_utils.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <sstream>
#include <vector>
#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"


static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}


// Callback for stbi_write_png_to_func
static void png_write_callback(void* context, void* data, int size) {
    std::vector<unsigned char>* buffer = static_cast<std::vector<unsigned char>*>(context);
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    buffer->insert(buffer->end(), bytes, bytes + size);
}

std::string encode_texture_to_base64_png(vita2d_texture* texture) {
    if (!texture) {
        return "";
    }

    int width = vita2d_texture_get_width(texture);
    int height = vita2d_texture_get_height(texture);
    const void* texture_data = vita2d_texture_get_datap(texture);

    if (!texture_data) {
        return "";
    }

    std::vector<unsigned char> png_buffer;
    int result = stbi_write_png_to_func(
        png_write_callback,
        &png_buffer,
        width,
        height,
        4, // 4 components: RGBA
        texture_data,
        width * 4 // Stride in bytes
    );

    if (result == 0) {
        // Failed to write PNG
        return "";
    }

    return base64_encode(png_buffer.data(), png_buffer.size());
}


bool save_texture_to_file(vita2d_texture* texture, const std::string& path) {
    if (!texture) return false;
    
    // Get texture dimensions and data
    int width = vita2d_texture_get_width(texture);
    int height = vita2d_texture_get_height(texture);
    void* texture_data = vita2d_texture_get_datap(texture);
    
    if (!texture_data) return false;

    // The stride is the number of bytes from one row of pixels to the next.
    int stride_in_bytes = width * 4;
    int result = stbi_write_png(path.c_str(), width, height, 4, texture_data, stride_in_bytes);

    return result != 0;
}


vita2d_texture* load_texture_from_file(const std::string& path) {
    int width, height, channels;
    
    // Use stbi_load to load the PNG file from the given path
    unsigned char* image_data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!image_data) {
        return nullptr; // Failed to load image
    }

    // Create an empty vita2d texture
    vita2d_texture* texture = vita2d_create_empty_texture(width, height);
    if (!texture) {
        stbi_image_free(image_data);
        return nullptr;
    }

    // Get a pointer to the texture's data buffer
    void* texture_data = vita2d_texture_get_datap(texture);
    if (!texture_data) {
        vita2d_free_texture(texture);
        stbi_image_free(image_data);
        return nullptr;
    }

    // Copy the loaded image data into the texture's buffer
    memcpy(texture_data, image_data, width * height * 4); // 4 for RGBA

    // Free the memory allocated by stbi_load
    stbi_image_free(image_data);

    return texture;
}

// Generate a unique filename for an image in a session
std::string generate_image_filename(int session_id, int message_id) {
    std::stringstream ss;
    ss << "ux0:data/vela/images/session_" << session_id << "_msg_" << message_id << ".png";
    return ss.str();
} 