#pragma once
#include "types.h"
#include <string>

struct Settings;

Settings load_settings();
void save_settings(const Settings& settings); 