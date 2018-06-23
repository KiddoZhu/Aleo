#include <assert.h>

#ifndef _BOTZONE_ONLINE
#include "bot.h"
#include "faan.h"
#endif

#define MY(field) (game.field[game.me])
#define IS_PLAY(action) \
	(action == ACT_PLAY || action == ACT_PENG || action == ACT_CHI)

Message MaxProbabilityBot::play(Game &game)
{
	using namespace chi;
	using namespace faan::probability;

	if (!IS_PLAY(game.last_turn.action) && game.last_turn.action != ACT_DRAW)
		return Message(ACT_PASS);
	// Hu
	if (game.can_hu())
		return Message(ACT_HU);

	Message msg;
	pair<float, int> best;
	// Play
	if (MY(hand_count) % 3 == 2) {
		best = this->get_best_discard(game);
		msg = Message(ACT_PLAY, game.hand[best.second]);
	}
	// Pass
	else {
		best = make_pair(this->get_probability(game), 0);
		msg = Message(ACT_PASS);
	}
	// Peng
	if (game.can_peng()) {
		game.remove_from_hand(game.last_card);
		game.remove_from_hand(game.last_card);
		MY(steal)[MY(steal_count)][0] = game.last_card;
		MY(steal)[MY(steal_count)++][1] = game.last_card;
		pair<float, int> result = this->get_best_discard(game);
		if (result > best) {
			best = result;
			msg = Message(ACT_PENG, game.hand[best.second]);
		}
		MY(steal_count)--;
		game.add_to_hand(game.last_card);
		game.add_to_hand(game.last_card);
	}
	// Chi
	int can_chi = game.can_chi();
	int chi_types[3] = { LEFT_CHI, MID_CHI, RIGHT_CHI };
	for (int i = 0; i < 3; i++) {
		int chi_type = chi_types[i];
		if (can_chi & chi_type) {
			game.remove_from_hand(game.last_card + NEED_DELTA[chi_type][0]);
			game.remove_from_hand(game.last_card + NEED_DELTA[chi_type][1]);
			MY(steal)[MY(steal_count)][0] = game.last_card + OUTPUT_DELTA[chi_type];
			MY(steal)[MY(steal_count)++][1] = game.last_card + OUTPUT_DELTA[chi_type] + 1;
			pair<float, int> result = this->get_best_discard(game);
			if (result > best) {
				best = result;
				msg = Message(ACT_CHI, game.last_card + OUTPUT_DELTA[chi_type], game.hand[best.second]);
			}
			MY(steal_count)--;
			game.add_to_hand(game.last_card + NEED_DELTA[chi_type][1]);
			game.add_to_hand(game.last_card + NEED_DELTA[chi_type][0]);
		}
	}
	// Ming Gang
	if (game.can_ming_gang()) {
		game.remove_from_hand(game.last_card);
		game.remove_from_hand(game.last_card);
		game.remove_from_hand(game.last_card);
		MY(steal)[MY(steal_count)][0] = game.last_card;
		MY(steal)[MY(steal_count)++][1] = 0;
		float result = this->get_probability(game);
		if (result > best.first) {
			best.first = result;
			msg = Message(ACT_GANG, 0);
		}
		MY(steal_count)--;
		game.add_to_hand(game.last_card);
		game.add_to_hand(game.last_card);
		game.add_to_hand(game.last_card);
	}
	// An Gang
	vector<tile> results = game.can_an_gang();
	if (!results.empty()) {
		for (int i = 0; i < results.size(); i++) {
			game.remove_from_hand(results[i]);
			game.remove_from_hand(results[i]);
			game.remove_from_hand(results[i]);
			game.remove_from_hand(results[i]);
			MY(steal)[MY(steal_count)][0] = results[i];
			MY(steal)[MY(steal_count)++][1] = 0;
			// consider one draw
			float result = 0;
			int norm = 0;
			if (game.wall_count--) {
				for (tile t = 1; t < NUM_SYMBOL; t++) {
					if (game.remain[t]) {
						game.remain[t]--;
						game.add_to_hand(t);
						tile last_card_backup = game.last_card;
						game.last_card = t;
						// consider one less wait to save time
						float prob = this->get_probability(game, -1);
						game.last_card = last_card_backup;
						game.remove_from_hand(t);
						game.remain[t]++;
						result += prob * game.remain[t];
						norm += game.remain[t];
					}
				}
				result /= norm;
			}
			else
				result = 0;
			game.wall_count++;
			if (result > best.first) {
				best.first = result;
				msg = Message(ACT_GANG, results[i]);
			}
			MY(steal_count)--;
			game.add_to_hand(results[i]);
			game.add_to_hand(results[i]);
			game.add_to_hand(results[i]);
			game.add_to_hand(results[i]);
		}
	}
	// Bu Gang
	if (game.can_bu_gang()) {
		game.remove_from_hand(game.last_card);
		int j;
		for (j = 0; j < MY(steal_count); j++) {
			if (MY(steal)[j][0] == game.last_card) {
				MY(steal)[j][1] = 0;
				break;
			}
		}
		// consider one draw
		float result = 0;
		int norm = 0;
		if (game.wall_count--) {
			for (tile t = 1; t < NUM_SYMBOL; t++) {
				if (game.remain[t]) {
					game.remain[t]--;
					game.add_to_hand(t);
					tile last_card_backup = game.last_card;
					game.last_card = t;
					// consider one less wait to save time
					float prob = this->get_probability(game, -1);
					game.last_card = last_card_backup;
					game.remove_from_hand(t);
					game.remain[t]++;
					result += prob * game.remain[t];
					norm += game.remain[t];
				}
			}
			result /= norm;
		}
		else
			result = 0;
		game.wall_count++;
		if (result > best.first) {
			best.first = result;
			msg = Message(ACT_BU_GANG);
		}
		MY(steal)[j][1] = MY(steal)[j][0];
		game.add_to_hand(game.last_card);
	}
	game.check();
	return msg;
}

float MaxProbabilityBot::get_probability(const Game &game, int delta_tiles)
{
	vector<Wait> waits = faan::SearchWaits(game, this->max_wait_tiles + delta_tiles);
	vector<float> probs = faan::GetWeightedProbabilities(game, waits);
	float prob = 0;
	for (int i = 0; i < probs.size(); i++)
		prob += probs[i];
	return prob;
}

pair<float, int> MaxProbabilityBot::get_best_discard(const Game &game)
{
	assert(MY(hand_count) % 3 == 2);
	vector<Wait> _waits = faan::SearchWaits(game, this->max_wait_tiles);
	vector<Wait> waits;
	for (int i = 0; i < _waits.size(); i++) {
		if (_waits[i].needs.size() > 0)
			waits.push_back(_waits[i]);
	}
	vector<float> probs = faan::GetWeightedProbabilities(game, waits);
	float prob_if_discard[NUM_SYMBOL] = { 0 };
	for (int i = 0; i < waits.size(); i++) {
		for (int j = 0; j < waits[i].discards.size(); j++) {
			// do not consider repeated tiles
			if (j == 0 || waits[i].discards[j - 1] != waits[i].discards[j])
				prob_if_discard[waits[i].discards[j]] += probs[i];
		}
	}
	int best = 0;
	for (int i = 1; i < MY(hand_count); i++) {
		if (prob_if_discard[game.hand[i]] > prob_if_discard[game.hand[best]])
			best = i;
	}
	return make_pair(prob_if_discard[game.hand[best]], best);
}