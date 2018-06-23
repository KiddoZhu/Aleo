#include <assert.h>
#include <algorithm>
using namespace std;

#ifndef _BOTZONE_ONLINE
#include "core.h"
#include "faan.h"
#include "utils.h"
#endif

#define MY(field) (this->field[this->me])
#define IS_PLAY(action) \
	(action == ACT_PLAY || action == ACT_PENG || action == ACT_CHI)

void Message::print(int level) const
{
	indent_printf(level * 4, "action = %s\n", ACTION_REPR(this->action));
	indent_printf(level * 4, "player = %d\n", this->player);
	switch (this->action) {
	case ACT_RECEIVE:
		indent_printf(level * 4, "flower_count =");
		for (int i = 0; i < NUM_PLAYER; i++)
			printf(" %d%c", this->flower_count[i], ",\n"[i == NUM_PLAYER - 1]);
		indent_printf(level * 4, "card =");
		for (int i = 0; i < 13; i++)
			printf(" %s%c", TILE_REPR(this->card[i]), ",\n"[i == 12]);
		break;
	case ACT_DRAW: case ACT_PLAY: case ACT_GANG: case ACT_BU_GANG:
		indent_printf(level * 4, "card = %s\n", TILE_REPR(this->card[0]));
		break;
	case ACT_PENG: case ACT_CHI:
		indent_printf(level * 4, "card = %s, %s\n", TILE_REPR(this->card[0]),
			TILE_REPR(this->card[1]));
		break;
	}
}

void Game::reset()
{
	this->wall_count = NUM_TILE;
	this->discard_count = 0;
	memset(this->hand_count, 0, sizeof(this->hand_count));
	memset(this->steal_count, 0, sizeof(this->steal_count));
	memset(this->flower_count, 0, sizeof(this->flower_count));
	for (tile t = 1; t < NUM_SYMBOL; t++)
		this->remain[t] = 4;
}

void Game::play(Message msg)
{
	//printf("Game of player %d\n", this->me);
	//if (this->wall_count <= NUM_TILE - 52)
	//	this->print_hand();
	//msg.print();
	switch (msg.action) {
	case ACT_QUAN:
		this->me = msg.player;
		this->dealer = msg.dealer;
		break;
	case ACT_RECEIVE:
		memcpy(this->flower_count, msg.flower_count, sizeof(this->flower_count));
		memcpy(this->hand, msg.card, sizeof(this->hand));
		sort(this->hand, this->hand + 13);
		for (int j = 0; j < 13; j++)
			this->remain[this->hand[j]]--;
		for (int j = 0; j < NUM_PLAYER; j++) {
			this->wall_count -= this->flower_count[j];
			this->hand_count[j] = 13;
		}
		this->wall_count -= 13 * NUM_PLAYER;
		break;
	case ACT_DRAW:
		this->add_to_hand(msg.card[0]);
		this->remain[msg.card[0]]--;
		this->wall_count--;
		break;
	case ACT_BU_HUA:
		this->flower_count[msg.player]++;
		this->wall_count--;
		break;
	case ACT_OTHER_DRAW:
		this->hand_count[msg.player]++;
		this->wall_count--;
		break;
	case ACT_PLAY:
		if (msg.player == this->me)
			this->remove_from_hand(msg.card[0]);
		else {
			this->hand_count[msg.player]--;
			this->remain[msg.card[0]]--;
		}
		this->history[this->discard_count++] = msg.card[0];
		break;
	case ACT_PENG:
		if (msg.player == this->me) {
			this->remove_from_hand(msg.card[0]);
			this->remove_from_hand(msg.card[0]);
			this->remove_from_hand(msg.card[1]);
		}
		else {
			this->hand_count[msg.player] -= 3;
			this->remain[msg.card[0]] -= 2;
			this->remain[msg.card[1]]--;
		}
		this->steal[msg.player][this->steal_count[msg.player]][0] = msg.card[0];
		this->steal[msg.player][this->steal_count[msg.player]++][1] = msg.card[0];
		this->history[this->discard_count - 1] = msg.card[1];
		break;
	case ACT_CHI:
		tile target;
		target = this->history[this->discard_count - 1];
		if (msg.player == this->me) {
			for (int j = -1; j <= 1; j++) {
				if (msg.card[0] + j != target)
					this->remove_from_hand(msg.card[0] + j);
			}
			this->remove_from_hand(msg.card[1]);
		}
		else {
			this->hand_count[msg.player] -= 3;
			for (int j = -1; j <= 1; j++) {
				if (msg.card[0] + j != target)
					this->remain[msg.card[0] + j]--;
			}
			this->remain[msg.card[1]]--;
		}
		this->steal[msg.player][this->steal_count[msg.player]][0] = msg.card[0];
		this->steal[msg.player][this->steal_count[msg.player]++][1] = msg.card[0] + 1;
		this->history[this->discard_count - 1] = msg.card[1];
		break;
	case ACT_GANG:
		if (msg.player == this->me) { // my
			this->remove_from_hand(msg.card[0]);
			this->remove_from_hand(msg.card[0]);
			this->remove_from_hand(msg.card[0]);
			if (MY(hand_count) % 3 == 2) // An Gang
				this->remove_from_hand(msg.card[0]);
			else // Ming Gang
				this->discard_count--;
		}
		else { // other's
			this->hand_count[msg.player] -= 3;
			if (!msg.card[0]) // An Gang
				this->hand_count[msg.player]--;
			else { // Ming Gang
				this->remain[msg.card[0]] -= 3;
				this->discard_count--;
			}
		}
		this->steal[msg.player][this->steal_count[msg.player]][0] = msg.card[0];
		this->steal[msg.player][this->steal_count[msg.player]++][1] = 0;
		break;
	case ACT_BU_GANG:
		if (msg.player == this->me)
			this->remove_from_hand(msg.card[0]);
		else {
			this->hand_count[msg.player]--;
			this->remain[msg.card[0]]--;
		}
		for (int j = 0; j < this->steal_count[msg.player]; j++) {
			if (this->steal[msg.player][j][0] == msg.card[0]) {
				this->steal[msg.player][j][1] = 0;
				break;
			}
		}
		break;
	}
	this->last_turn = msg;
	if (msg.action == ACT_PENG || msg.action == ACT_CHI)
		this->last_card = msg.card[1];
	else
		this->last_card = msg.card[0];
}

void Game::replay(vector<Message> msgs)
{
	for (int i = 0; i < msgs.size(); i++) {
		this->play(msgs[i]);
		if (i > 0)
			this->check();
	}
}

bool Game::can_hu() const
{
	if ((!IS_PLAY(this->last_turn.action) || this->last_turn.player == me) &&
		this->last_turn.action != ACT_DRAW)
		return false;

	Game* var = const_cast<Game*>(this);
	if (IS_PLAY(this->last_turn.action))
		var->add_to_hand(this->last_card);
	vector<Wait> waits = faan::SearchWaits(*var, 0);
	vector<int> scores = faan::GetScores(*var, waits);
	bool result = !scores.empty() && scores[0] >= 8;
	if (IS_PLAY(this->last_turn.action))
		var->remove_from_hand(this->last_card);
	this->check();
	return result;
}

bool Game::can_peng() const
{
	if (!IS_PLAY(this->last_turn.action) || this->last_turn.player == this->me)
		return false;

	int found = 0;
	for (int i = 0; i < MY(hand_count); i++) {
		if (this->hand[i] == this->last_card) {
			if (++found >= 2)
				return true;
		}
	}
	return false;
}

int Game::can_chi() const
{
	using namespace chi;

	if (!IS_PLAY(this->last_turn.action) ||
		this->last_turn.player != (this->me + NUM_PLAYER - 1) % NUM_PLAYER ||
		!IS_SUIT(this->last_card))
		return false;

	int result = 0;
	bool exist[5] = { false };
	for (int i = 0; i < MY(hand_count); i++) {
		if (this->hand[i] - this->last_card >= -2 && this->hand[i] - this->last_card <= 2)
			exist[this->hand[i] - this->last_card + 2] = true;
	}
	if (exist[3] && exist[4] && NUMBER(this->last_card) <= 7)
		result |= LEFT_CHI;
	if (exist[1] && exist[3] && NUMBER(this->last_card) <= 8 && NUMBER(this->last_card) >= 2)
		result |= MID_CHI;
	if (exist[0] && exist[1] && NUMBER(this->last_card) >= 3)
		result |= RIGHT_CHI;
	return result;
}

bool Game::can_ming_gang() const
{
	if (!IS_PLAY(this->last_turn.action) || this->last_turn.player == this->me)
		return false;

	int found = 0;
	for (int i = 0; i < MY(hand_count); i++) {
		if (this->hand[i] == this->last_card) {
			if (++found >= 3)
				return true;
		}
	}
	return false;
}

vector<tile> Game::can_an_gang() const
{
	vector<tile> results;
	if (this->last_turn.action != ACT_DRAW)
		return results;

	int found = 1;
	for (int i = 1; i < MY(hand_count); i++) {
		if (this->hand[i] == this->hand[i - 1]) {
			if (++found >= 4)
				results.push_back(hand[i]);
		}
		else
			found = 1;
	}
	return results;
}

bool Game::can_bu_gang() const
{
	if (this->last_turn.action != ACT_DRAW)
		return false;

	for (int i = 0; i < MY(steal_count); i++) {
		if (MY(steal)[i][0] == last_card && MY(steal)[i][0] == MY(steal)[i][1])
			return true;
	}
	return false;
}


void Game::check() const
{
	int num_in_hand = 0, num_visible = MY(hand_count), num_invisible = 0;
	for (int i = 0; i < MY(hand_count) - 1; i++)
		assert(this->hand[i] <= this->hand[i + 1]);
	for (int i = 0; i < NUM_PLAYER; i++) {
		assert(this->hand_count[i] % 3 != 0);
		assert(this->hand_count[i] / 3 + this->steal_count[i] == 4);
		num_in_hand += this->hand_count[i];
		for (int j = 0; j < this->steal_count[i]; j++) {
			if (this->steal[i][j][1] != 0) { // Peng, Chi
				num_in_hand += 3;
				num_visible += 3;
			}
			else { // Gang
				num_in_hand += 4;
				if (this->steal[i][j][0] != 0) // Ming Gang	
					num_visible += 4;
			}
		}
		num_in_hand += this->flower_count[i];
	}
	for (int i = 1; i < NUM_SYMBOL; i++) {
		assert(this->remain[i] >= 0 && this->remain[i] <= 4);
		num_invisible += this->remain[i];
	}
	assert(num_in_hand + this->discard_count + this->wall_count == NUM_TILE);
	assert(num_visible + num_invisible + this->discard_count == NUM_TILE - NUM_FLOWER);
}

void Game::print_hand(int level) const
{
	indent_printf(level * 4, "hand =");
	for (int i = 0; i < MY(hand_count); i++)
		printf(" %s%c", TILE_REPR(this->hand[i]), ",\n"[i == MY(hand_count) - 1]);
}

void Game::print_win_hand(const Wait &wait, int level) const
{
	int hand_count = MY(hand_count);
	tile win_hand[NUM_HAND];
	for (int i = 0, j = 0; i < hand_count; i++) {
		if (j < wait.discards.size() && this->hand[i] == wait.discards[j])
			win_hand[i] = wait.needs[j++];
		else
			win_hand[i] = this->hand[i];
	}
	if (wait.needs.size() > wait.discards.size())
		win_hand[hand_count++] = this->hand[wait.needs.size() - 1];
	indent_printf(level * 4, "win_hand =");
	for (int i = 0; i < hand_count; i++)
		printf(" %s%c", TILE_REPR(win_hand[i]), ",\n"[i == hand_count - 1]);
}

void Game::add_to_hand(tile t)
{
	for (int i = 0; i <= MY(hand_count); i++) {
		if (this->hand[i] > t || i == MY(hand_count)) {
			for (int j = MY(hand_count); j > i; j--)
				this->hand[j] = this->hand[j - 1];
			this->hand[i] = t;
			break;
		}
	}
	MY(hand_count)++;
}

void Game::remove_from_hand(tile t)
{
	for (int i = 0; i < MY(hand_count); i++) {
		if (this->hand[i] == t) {
			for (int j = i; j < MY(hand_count) - 1; j++)
				this->hand[j] = this->hand[j + 1];
			this->hand[MY(hand_count) - 1] = 0;
			break;
		}
	}
	MY(hand_count)--;
}