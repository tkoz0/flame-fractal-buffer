#pragma once

#include "utils/json_small.hpp"
#include "flame/types.h"

// parses details from JSON representation of flame parameters
void flame_from_json(const Json& input, flame_t *flame);
