#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <string>
#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>

enum KeyboardState {
    KEYBOARD_STATE_NONE,
    KEYBOARD_STATE_RUNNING,
    KEYBOARD_STATE_FINISHED
};

void keyboard_init();

bool keyboard_start(const std::string& initial_text, const std::string& title);

KeyboardState keyboard_update();

std::string keyboard_get_text();


void keyboard_terminate();

#endif 