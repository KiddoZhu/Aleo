#include <memory.h>
#include <vector>
using namespace std;

#pragma once

typedef unsigned char tile;

#define NUM_PLAYER 4
#define NUM_HAND 14
#define NUM_STEAL 4
#define NUM_FLOWER 8
#define NUM_TILE 144
// 34 symbols in total excluding flowers, while 0 is invalid
#define NUM_SYMBOL 35

enum Action {
	ACT_QUAN,
	ACT_RECEIVE,
	ACT_DRAW,
	ACT_BU_HUA,
	ACT_OTHER_DRAW,
	ACT_PLAY,
	ACT_PENG,
	ACT_CHI,
	ACT_GANG,
	ACT_BU_GANG,
	ACT_PASS,
	ACT_HU
};

enum Obtain {
	PENG,
	CHI,
	NONE
};

const char _ACTION2NAME[12][11] = {
	"Quan", "Receive", "Draw", "Bu Hua", "Other Draw",
	"Play", "Peng", "Chi", "Gang", "Bu Gang", "Pass", "Hu"
};

const char _TILE2NAME[NUM_SYMBOL + 8][8] = {
	"Invalid",
	"W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", "W9",
	"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9",
	"T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9",
	"F1", "F2", "F3", "F4", "J1", "J2", "J3",
	"H1", "H2", "H3", "H4", "H5", "H6", "H7", "H8"
};

#define ACTION_REPR(a) (_ACTION2NAME[a])
#define TILE_REPR(t) (_TILE2NAME[t])

class MaxProbabilityBot;

class Message {
public:
	Action action;
	int player, dealer;
	int flower_count[NUM_PLAYER];
	tile card[NUM_HAND];

	Message() {}
	Message(Action action) : action(action) {}
	Message(Action action, tile card) : action(action)
	{ this->card[0] = card; }
	Message(Action action, tile card0, tile card1) : action(action)
	{ this->card[0] = card0; this->card[1] = card1; }

	void print(int level = 0) const;
};

class Wait {
public:
	vector<tile> needs, discards;
	vector<Obtain> obtains;

	bool operator <(const Wait &other) const {
		if (this->needs.size() != other.needs.size())
			return this->needs.size() < other.needs.size();
		return this->needs < other.needs ||
			(this->needs == other.needs && (this->discards < other.discards ||
			(this->discards == other.discards && this->obtains < other.obtains)));
	}

	bool operator ==(const Wait &other) const {
		if (this->needs.size() != other.needs.size())
			return false;
		return this->needs == other.needs && this->discards == other.discards &&
			this->obtains == other.obtains;
	}

	void print(int level = 0) const;
};

namespace chi {
	const int LEFT_CHI = 0x1; // {X, X+1, X+2}, X is the last discard
	const int MID_CHI = 0x2; // {X-1, X, X+1}
	const int RIGHT_CHI = 0x4; // {X-2, X-1, X}
	const int NEED_DELTA[5][2] = { { 0, 0 }, { 1, 2 }, { -1, 1 }, { 0, 0 }, { -2, -1 } };
	const int OUTPUT_DELTA[5] = { 0, 1, 0, 0, -1 };
}

class Game {
public:
	// hand should always be sorted
	tile hand[NUM_HAND];
	/*
	 * Eye: {1, 1} -> {0, 1}
	 * Chi: {1, 2, 3} -> {2, 3}
	 * Peng: {1, 1, 1} -> {1, 1}
	 * An Gang: {-, -, -, -} -> {0, 0}
	 * Ming Gang: {1, 1, 1, 1} -> {1, 0}
	 */
	tile steal[NUM_PLAYER][NUM_STEAL][2];
	int me, dealer;
	int hand_count[NUM_PLAYER], steal_count[NUM_PLAYER], flower_count[NUM_PLAYER];
	int wall_count, discard_count;
	// history: discarded tiles
	// history does not include tiles that is Peng, Chi or Gang by others.
	tile history[NUM_TILE];
	// remain: max number of tiles in the wall
	int remain[NUM_SYMBOL];
	Message last_turn;
	tile last_card;

	Game() { this->reset(); }
	// do not allow implicit copy since it is expensive
	explicit Game(const Game &game) = default;
	void reset();
	void play(Message msg);
	void replay(vector<Message> msgs);
	bool can_hu() const;
	bool can_peng() const;
	int can_chi() const;
	bool can_ming_gang() const;
	vector<tile> can_an_gang() const;
	bool can_bu_gang() const;
	void check() const;

	void print_hand(int level = 0) const;
	void print_win_hand(const Wait &wait, int level = 0) const;
private:
	friend class MaxProbabilityBot;
	void add_to_hand(tile t);
	void remove_from_hand(tile t);
};