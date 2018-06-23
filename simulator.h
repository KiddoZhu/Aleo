#include "core.h"
#include "bot.h"

#define NUM_TURN (NUM_TILE * 2)
#define IS_FLOWER(t) (t >= NUM_SYMBOL)

class Simulator
{
public:
	static const int priority[12];

	Game game[NUM_PLAYER];
	Bot *player[NUM_PLAYER];
	tile wall[NUM_TILE];
	int wall_index, current_player;
	tile last_card;
	Message history[NUM_TURN][NUM_PLAYER];
	Message response[NUM_TURN][NUM_PLAYER];
	int turn;

	Simulator() {}

	Simulator(Bot *player) {
		for (int i = 0; i < NUM_PLAYER; i++)
			this->player[i] = player;
	}

	Simulator(Bot *player[]) {
		for (int i = 0; i < NUM_PLAYER; i++)
			this->player[i] = player[i];
	}

	void run();
	void seek(int turn);
	void save(FILE *fout) const;
	void load(FILE *fin);
	void check() const;
private:
	void broadcast(const Message &msg, bool is_an_gang = false);
};