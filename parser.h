#include <stdio.h>
#include <vector>
using namespace std;

#ifndef _BOTZONE_ONLINE
#include "core.h"
#endif

#pragma once

const tile _ALPHA2TILE[26] = {
	0, 9, 0, 0, 0, 27, 0, 0, 0, 31,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 18,
	0, 0, 0, 0, 0, 0
};

const char _ACTION2OUTPUT[12][11] = {
	"INVALID", "INVALID", "INVALID", "INVALID", "INVALID",
	"PLAY", "PENG", "CHI", "GANG", "BUGANG", "PASS", "HU"
};

#define DIGIT(c) ((c) - '0')
#define CARD(s) (tile(_ALPHA2TILE[*(s) - 'A'] + DIGIT(*((s)+1))))
#define ACTION_OUTPUT(a) (_ACTION2OUTPUT[a])
#define TILE_OUTPUT TILE_REPR

vector<Message> ParseInput();
void Output(const Message &msg);