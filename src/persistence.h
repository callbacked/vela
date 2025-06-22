#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "types.h"
#include <vector>

bool save_sessions(const std::vector<ChatSession>& sessions);
std::vector<ChatSession> load_sessions();

#endif  