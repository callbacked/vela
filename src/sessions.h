#ifndef SESSIONS_H
#define SESSIONS_H

#include <vector>
#include "types.h"
#include <vita2d.h>

void draw_sessions_ui(vita2d_pgf* pgf, const std::vector<ChatSession>& sessions, int scroll_offset, int selected_session_index, bool show_delete_confirmation = false, bool delete_confirmation_selection = false);

#endif 