#include <iostream>

#ifndef _BOTZONE_ONLINE
#include "core.h"
#include "parser.h"
#include "faan.h"
#include "bot.h"
#endif

void init(const Game &game)
{
	using namespace faan::probability;
	// 0.05 ~ 0.3 results in human understandable behavior
	// however, larger decay may bring suprising performance
	decay_per_tile = 0.1 + 0.3 * game.wall_count / (NUM_TILE - 13 * NUM_PLAYER);
	// 1 & 9
	for (tile t = CARD("W1"); t <= CARD("T1"); t += 9) {
		mapping[t] = [](int remain) -> float { return remain == 1 ? 2 : remain; };
		mapping[t+8] = [](int remain) -> float { return remain == 1 ? 2 : remain; };
	}
	// winds & dragons
	for (tile t = CARD("F1"); t <= CARD("J3"); t++)
		mapping[t] = [](int remain) -> float { return remain == 1 ? 4 : remain; };
}

int main(int argc, char* argv[])
{
	vector<Message> inputs;
	Game game;

	inputs = ParseInput();
	game.replay(inputs);
	init(game);

	MaxProbabilityBot bot(5);
	Output(bot.play(game));
}