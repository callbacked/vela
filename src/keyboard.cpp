#include "keyboard.h"
#include <psp2/kernel/clib.h>
#include <psp2/sysmodule.h>
#include <string.h>
#include <string>

static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
    int i;
    for (i = 0; src[i]; i++) {
        if ((src[i] & 0xFF80) == 0) {
            *(dst++) = src[i] & 0xFF;
        } else if ((src[i] & 0xF800) == 0) {
            *(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
            *(dst++) = (src[i] & 0x3F) | 0x80;
        } else if ((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
            *(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
            *(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
            *(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
            *(dst++) = (src[i + 1] & 0x3F) | 0x80;
            i += 1;
        } else {
            *(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
            *(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
            *(dst++) = (src[i] & 0x3F) | 0x80;
        }
    }

    *dst = '\0';
}

static void utf8_to_utf16(const uint8_t *src, uint16_t *dst) {
    int i;
    for (i = 0; src[i];) {
        if ((src[i] & 0xE0) == 0xE0) {
            *(dst++) = ((src[i] & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F);
            i += 3;
        } else if ((src[i] & 0xC0) == 0xC0) {
            *(dst++) = ((src[i] & 0x1F) << 6) | (src[i + 1] & 0x3F);
            i += 2;
        } else {
            *(dst++) = src[i];
            i += 1;
        }
    }

    *dst = '\0';
}

// Buffer for the IME dialog
static uint16_t ime_title_utf16[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t ime_initial_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t ime_input_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
static uint8_t ime_text_utf8[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];

static KeyboardState current_state = KEYBOARD_STATE_NONE;

void keyboard_init() {
    int ret = sceSysmoduleLoadModule(SCE_SYSMODULE_IME);
    sceClibPrintf("IME module load: %x\n", ret);
}

bool keyboard_start(const std::string& initial_text, const std::string& title) {
    if (current_state != KEYBOARD_STATE_NONE) {
        sceClibPrintf("Keyboard already active\n");
        return false; 
    }

    memset(ime_title_utf16, 0, sizeof(ime_title_utf16));
    memset(ime_initial_text_utf16, 0, sizeof(ime_initial_text_utf16));
    memset(ime_input_text_utf16, 0, sizeof(ime_input_text_utf16));
    
    utf8_to_utf16((const uint8_t*)title.c_str(), ime_title_utf16);
    utf8_to_utf16((const uint8_t*)initial_text.c_str(), ime_initial_text_utf16);

    SceImeDialogParam param;
    sceImeDialogParamInit(&param);
    
    param.type = SCE_IME_TYPE_DEFAULT;
    param.title = ime_title_utf16;
    param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
    param.initialText = ime_initial_text_utf16;
    param.inputTextBuffer = ime_input_text_utf16;
    
    int res = sceImeDialogInit(&param);
    sceClibPrintf("IME dialog init: %x\n", res);
    
    if (res < 0) {
        sceClibPrintf("Failed to initialize IME dialog: %x\n", res);
        return false;
    }
    
    current_state = KEYBOARD_STATE_RUNNING;
    return true;
}

KeyboardState keyboard_update() {
    if (current_state == KEYBOARD_STATE_RUNNING) {
        SceCommonDialogStatus status = sceImeDialogGetStatus();
        sceClibPrintf("IME dialog status: %d\n", status);
        
        if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            sceClibPrintf("IME dialog finished.\n");
            SceImeDialogResult result;
            memset(&result, 0, sizeof(SceImeDialogResult));
            sceImeDialogGetResult(&result);
            sceClibPrintf("IME dialog result button: %d\n", result.button);
            
            sceImeDialogTerm(); 

            if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                utf16_to_utf8(ime_input_text_utf16, ime_text_utf8);
                current_state = KEYBOARD_STATE_NONE;
                return KEYBOARD_STATE_FINISHED;
            } else { 
                current_state = KEYBOARD_STATE_NONE;
                return KEYBOARD_STATE_NONE;
            }
        }
    }
    
    return current_state;
}

std::string keyboard_get_text() {

    return std::string((const char*)ime_text_utf8);
}

void keyboard_terminate() {
    if (current_state != KEYBOARD_STATE_NONE) {
        sceImeDialogTerm();
        current_state = KEYBOARD_STATE_NONE;
    }
} 