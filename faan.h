#include <vector>
using namespace std;

#ifndef _BOTZONE_ONLINE
#include "core.h"
#include "lib/fan_calculator.h"
#endif

#pragma once

#define TILE(s, n) ((s) * 9 + (n))
#define SUIT(t) (((t)-1) / 9)
#define NUMBER(t) (((t)-1) % 9 + 1)
#define IS_SUIT(t) ((t) <= 27)
#define IS_WIND(t) ((t) > 27 && (t) <= 31)
#define IS_DRAGON(t) ((t) > 31)

const mahjong::tile_t _TILE2TILE_T[NUM_SYMBOL] = {
	0x0,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
};

#define TILE_T(t) (_TILE2TILE_T[t])

namespace faan {
	const int HU = 0x1;
	const int QI_DUI = 0x2;
	const int BU_KAO = 0x4;
	const int ORPHAN = 0x8;

	namespace probability {
		extern float decay_per_tile;
		typedef	float(*MappingFunction)(int remain);
		extern MappingFunction mapping[NUM_SYMBOL];
	}

	namespace statics {
		extern float self_draw;
	}

	vector<Wait> SearchWaits(const Game &game, int max_tiles, int target = HU | QI_DUI | BU_KAO | ORPHAN );
	vector<float> GetProbabilities(const Game &game, const vector<Wait> &waits);
	vector<float> GetWeightedProbabilities(const Game &game, const vector<Wait> &waits,
		int min_score = 8);
	vector<int> GetScores(const Game &game, const vector<Wait> &waits);

	vector<Wait> SearchHuWaits(const Game &game, int max_tiles);
	vector<Wait> SearchQiDuiWaits(const Game &game, int max_tiles);
	vector<Wait> SearchBuKaoWaits(const Game &game, int max_tiles);
	vector<Wait> SearchOrphanWaits(const Game &game, int max_tiles);

	typedef vector<Wait>(*SearchFunction)(const Game &game, int max_tiles);
	const vector<pair<int, SearchFunction> > FAANS = {
		{ HU, SearchHuWaits },
		{ QI_DUI, SearchQiDuiWaits },
		{ BU_KAO, SearchBuKaoWaits },
		{ ORPHAN, SearchOrphanWaits },
	};
}