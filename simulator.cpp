#include <assert.h>
#include <algorithm>
using namespace std;

#include "simulator.h"

//#undef assert
//#define assert(expr) do { \
//		if (!(expr)) { \
//			printf("assert(%s) failed: %s %d\n", #expr, __FILE__, __LINE__); \
//			exit(1); \
//		} \
//	} while(0)

#define NEXT_CARD (this->wall[this->wall_index++])

const int Simulator::priority[12] = { 0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 0, 3 };

void Simulator::run()
{
	for (int i = 0; i < NUM_PLAYER; i++)
		this->game[i].reset();
	for (tile t = 1; t < NUM_SYMBOL; t++) {
		for (int i = 0; i < 4; i++)
			this->wall[(t - 1) * 4 + i] = t;
	}
	for (tile t = 0; t < 8; t++)
		this->wall[(NUM_SYMBOL - 1) * 4 + t] = NUM_SYMBOL + t;
	random_shuffle(this->wall, this->wall + NUM_TILE);

	this->turn = 0;
	Message msg(ACT_QUAN);
	msg.dealer = rand() % NUM_PLAYER;
	this->broadcast(msg);
	for (int i = 0; i < NUM_PLAYER; i++)
		assert(this->response[this->turn][i].action == ACT_PASS);
	this->turn++;

	int flower_count[NUM_PLAYER] = { 0 };
	tile card[NUM_PLAYER][13];
	this->wall_index = 0;
	for (int i = 0; i < NUM_PLAYER; i++) {
		for (int j = 0; j < 13; j++) {
			tile t;
			for (t = NEXT_CARD; IS_FLOWER(t); t = NEXT_CARD)
				flower_count[i]++;
			card[i][j] = t;
			// we do not care which flower card is received
		}
	}
	msg.action = ACT_RECEIVE;
	memcpy(msg.flower_count, flower_count, sizeof(flower_count));
	for (int i = 0; i < NUM_PLAYER; i++) {
		msg.player = i;
		memcpy(msg.card, card[i], sizeof(card[i]));
		this->game[i].play(msg);
		this->history[this->turn][i] = msg;
		this->response[this->turn][i] = this->player[i]->play(this->game[i]);
		assert(this->response[this->turn][i].action == ACT_PASS);
	}
	this->turn++;
	
	this->current_player = 0; // start from east, same as botzone implementation
	while (this->wall_index < NUM_TILE) {
		msg.player = this->current_player;
		msg.card[0] = NEXT_CARD;
		if (IS_FLOWER(msg.card[0]))
			msg.action = ACT_BU_HUA;
		else
			msg.action = ACT_DRAW;
		this->broadcast(msg);
		if (IS_FLOWER(msg.card[0])) {
			this->turn++;
			continue;
		}
		// Play
		for (int i = 0; i < NUM_PLAYER; i++) {
			if (i == this->current_player) {
				assert(this->response[this->turn][i].action == ACT_PLAY ||
					this->response[this->turn][i].action == ACT_GANG ||
					this->response[this->turn][i].action == ACT_BU_GANG ||
					this->response[this->turn][i].action == ACT_HU);
			}
			else
				assert(this->response[this->turn][i].action == ACT_PASS);
		}
		if (this->response[this->turn][this->current_player].action == ACT_HU) {
			this->turn++;
			return;
		}
		msg = this->response[this->turn++][this->current_player];
		msg.player = this->current_player;
		this->broadcast(msg, true);
		int highest;
		do { // check response
			highest = 0;
			for (int i = 0; i < NUM_PLAYER; i++) {
				assert(this->response[this->turn][i].action == ACT_PASS ||
					priority[this->response[this->turn][i].action]);
				if (priority[this->response[this->turn][highest].action] <
					priority[this->response[this->turn][i].action])
					highest = i;
			}
			if (this->response[this->turn][highest].action == ACT_PASS) {
				this->turn++;
				break;
			}
			if (this->response[this->turn][highest].action == ACT_HU) {
				this->turn++;
				return;
			}
			msg = this->response[this->turn++][highest];
			msg.player = this->current_player = highest;
			this->broadcast(msg);
		} while (true);
		this->current_player = (this->current_player + 1) % 4;
	}
}

void Simulator::seek(int turn)
{
	assert(turn <= this->turn);
	for (int i = 0; i < NUM_PLAYER; i++) {
		this->game[i].reset();
		for (int j = 0; j < turn; j++)
			this->game[i].play(this->history[j][i]);
	}
}

void Simulator::save(FILE *fout) const
{
	fwrite(&this->turn, sizeof(int), 1, fout);
	fwrite(this->history, sizeof(Message), this->turn * NUM_PLAYER, fout);
	fwrite(this->response, sizeof(Message), this->turn * NUM_PLAYER, fout);
}

void Simulator::load(FILE *fin)
{
	fread(&this->turn, sizeof(int), 1, fin);
	fread(this->history, sizeof(Message), this->turn * NUM_PLAYER, fin);
	fread(this->response, sizeof(Message), this->turn * NUM_PLAYER, fin);
}

void Simulator::broadcast(const Message &msg, bool is_an_gang)
{
	static int last_turn;
	assert(msg.action == ACT_QUAN || last_turn == 0 ||
		this->turn == last_turn + 1);
	last_turn = this->turn;

	Message player_msg = msg;
	switch (msg.action) {
	case ACT_QUAN:
		for (int i = 0; i < NUM_PLAYER; i++) {
			player_msg.player = i;
			this->game[i].play(player_msg);
			this->history[this->turn][i] = player_msg;
		}
		break;
	case ACT_DRAW:
		for (int i = 0; i < NUM_PLAYER; i++) {
			if (i == msg.player)
				player_msg.action = ACT_DRAW;
			else
				player_msg.action = ACT_OTHER_DRAW;
			this->game[i].play(player_msg);
			this->history[this->turn][i] = player_msg;
		}
		break;
	case ACT_GANG:
		if (is_an_gang) { // An Gang
			for (int i = 0; i < NUM_PLAYER; i++) {
				if (i == msg.player)
					player_msg.card[0] = msg.card[0];
				else
					player_msg.card[0] = 0;
				this->game[i].play(player_msg);
				this->history[this->turn][i] = player_msg;
			}
			break;
		}
		// Ming Gang fall through
	default:
		for (int i = 0; i < NUM_PLAYER; i++) {
			this->game[i].play(msg);
			this->history[this->turn][i] = msg;
		}
		break;
	}
	tile last_card;
	if (msg.action == ACT_PENG || msg.action == ACT_CHI)
		last_card = msg.card[1];
	else
		last_card = msg.card[0];
	for (int i = 0; i < NUM_PLAYER; i++) {
		Message response = this->player[i]->play(this->game[i]);
		switch (response.action) {
		case ACT_PENG:
			response.card[1] = response.card[0];
			response.card[0] = last_card;
			break;
		case ACT_GANG:
			if (!response.card[0]) // Ming Gang
				response.card[0] = last_card;
		}
		this->response[this->turn][i] = response;
	}
}