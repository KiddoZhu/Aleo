#ifndef _BOTZONE_ONLINE
#include "core.h"
#include "faan.h"
#endif

#pragma once

class Bot {
public:
	Bot() {}
	explicit Bot(const Bot &bot) = default;
	virtual Message play(Game &game) = 0;
};

class MaxProbabilityBot : public Bot {
public:
	int max_wait_tiles;

	MaxProbabilityBot(int max_wait_tiles) :
		max_wait_tiles(max_wait_tiles) {}
	Message play(Game &game) override;
private:
	float get_probability(const Game &game, int delta_tiles = 0);
	pair<float, int> get_best_discard(const Game &game);
};