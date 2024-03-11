#pragma once

#define VERSION_STR             "v0.5"
#define MAX_CIVILIZATIONS       4
#define CITIES_PER_CIVILIZATION 8
#define MAX_CITIES              MAX_CIVILIZATIONS*CITIES_PER_CIVILIZATION
#define UNITS_PER_CITY          5
#define MAX_UNITS               MAX_CIVILIZATIONS*CITIES_PER_CIVILIZATION*UNITS_PER_CITY
#define PLAYER_TIMEOUT          60*60*2     /* timeout in 2 minutes */
#define TAX_RATE                0.07f
