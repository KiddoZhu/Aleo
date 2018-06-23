#include <assert.h>
#include <memory.h>
#include <algorithm>
using namespace std;

#ifndef _BOTZONE_ONLINE
#include "faan.h"
#include "parser.h"
#include "utils.h"
#endif

#define MY(field) (game.field[game.me])

// surrogate class for DFS & global variables
class DFSEngine {
public:
	const tile *hand;
	bool keep[NUM_HAND];
	int hand_count, max_tiles;
	int *remain;
	vector<Wait> waits;
	DFSEngine(const tile hand[], int hand_count, int remain[], int max_tiles) :
		hand(hand), hand_count(hand_count), remain(remain), max_tiles(max_tiles) {
		memset(keep, false, sizeof(keep));
	}
	virtual void DFS(int num_melds = 0, int start = 0) = 0;
protected:
	vector<tile> needs;
	vector<Obtain> obtains;
};

class HuEngine : public DFSEngine {
public:
	HuEngine(const tile hand[], int hand_count, int remain[], int max_tiles) :
		DFSEngine(hand, hand_count, remain, max_tiles) {}
	void DFS(int num_melds = 0, int start = 0);
};

class QiDuiEngine : public DFSEngine {
public:
	QiDuiEngine(const tile hand[], int hand_count, int remain[], int max_tiles) :
		DFSEngine(hand, hand_count, remain, max_tiles) {}
	void DFS(int num_melds = 0, int start = 0);
};

class BuKaoEngine : public DFSEngine {
public:
	bool used[3];
	BuKaoEngine(const tile hand[], int hand_count, int remain[], int max_tiles) :
		DFSEngine(hand, hand_count, remain, max_tiles) {
		memset(used, false, sizeof(used));
	}
	void DFS(int num_melds = 0, int start = 0);
};

class OrphanEngine : public DFSEngine {
public:
	static const int orphan[13];
	OrphanEngine(const tile hand[], int hand_count, int remain[], int max_tiles) :
		DFSEngine(hand, hand_count, remain, max_tiles) {}
	void DFS(int num_melds = 0, int start = 0);
};
const int OrphanEngine::orphan[] = { CARD("W1"), CARD("W9"), CARD("B1"), CARD("B9"), CARD("T1"), CARD("T9"),
CARD("F1"), CARD("F2"), CARD("F3"), CARD("F4"), CARD("J1"), CARD("J2"), CARD("J3") };

namespace faan {
	namespace probability {
		float decay_per_tile = 0.2;
		MappingFunction mapping[NUM_SYMBOL] = { nullptr };
	}

	namespace statics { // from Lian Zhong Mahjong-GB
		float self_drawn = 0.254;
	}

	vector<Wait> SearchWaits(const Game &game, int max_tiles, int target)
	{
		vector<Wait> waits, results;
		for (int i = 0; i < FAANS.size(); i++) {
			int faan = FAANS[i].first;
			SearchFunction search_func = FAANS[i].second;
			if (target & faan) {
				results = search_func(game, max_tiles);
				waits.insert(waits.end(), results.begin(), results.end());
			}
		}
		return waits;
	}

	vector<float> GetProbabilities(const Game &game, const vector<Wait> &waits)
	{
		using namespace probability;

		vector<float> probs;
		int prev_player = (game.me + NUM_PLAYER - 1) % NUM_PLAYER;
		int num_invisible = 0;
		float other_prob[NUM_PLAYER];
		float total_other_prob = 0.0;
		memcpy(other_prob, game.hand_count, sizeof(other_prob));
		for (int i = 1; i < NUM_SYMBOL; i++)
			num_invisible += game.remain[i];
		for (int i = 0; i < NUM_PLAYER; i++) {
			if (i != game.me) {
				for (int j = 0; j < game.steal_count[i]; j++) {
					if (game.steal[i][j][0] == 0 && game.steal[i][j][1] == 0) // An Gang
						other_prob[i] += 4;
				}
				other_prob[i] /= num_invisible;
				total_other_prob += other_prob[i];
			}
		}
		for (int i = 0; i < waits.size(); i++) {
			float total_prob = 0.0;
			int norm = 0;
			for (int j = 0; j < waits[i].needs.size(); j++) { // last need
				float prob = 1.0;
				for (int k = 0; k < waits[i].needs.size(); k++) {
					tile t = waits[i].needs[k];
					Obtain obtain = waits[i].obtains[k];
					float tile_prob;
					if (mapping[t])
						tile_prob = mapping[t](game.remain[t]);
					else
						tile_prob = game.remain[t];
					if (k == j || obtain == PENG) {
						tile_prob = (tile_prob + tile_prob * (NUM_PLAYER - 1) *
							(1 - total_other_prob)) / NUM_PLAYER;
					}
					else if (obtain == CHI) {
						tile_prob = (tile_prob + tile_prob *
							(1 - other_prob[prev_player])) / NUM_PLAYER;
					}
					else // self draw
						tile_prob = tile_prob / NUM_PLAYER;
					prob *= tile_prob / 4.0 * decay_per_tile;
				}
				// TODO: better design of norm
				total_prob += prob * game.remain[waits[i].needs[j]];
				norm += game.remain[waits[i].needs[j]];
			}
			probs.push_back(total_prob / norm);
		}
		return probs;
	}

	vector<float> GetWeightedProbabilities(const Game &game, const vector<Wait> &waits,
		int min_score)
	{
		using namespace statics;

		vector<float> probs = GetProbabilities(game, waits);
		vector<int> scores = GetScores(game, waits);
		for (int i = 0; i < waits.size(); i++) {
			if (scores[i] >= min_score) {
				probs[i] *= scores[i] * (1 + (NUM_PLAYER - 2) * self_drawn) +
					8 * (NUM_PLAYER - 1);
			}
			else
				probs[i] = 0;
		}
		return probs;
	}

	vector<int> GetScores(const Game &game, const vector<Wait> &waits)
	{
		int hand_count = MY(hand_count);
		vector<int> scores;
		mahjong::calculate_param_t param;

		param.flower_count = MY(flower_count);
		param.prevalent_wind = mahjong::wind_t(game.dealer);
		param.seat_wind = mahjong::wind_t(game.me);

		param.hand_tiles.pack_count = MY(steal_count);
		for (int i = 0; i < MY(steal_count); i++) {
			const tile *steal = MY(steal)[i];
			// TODO: the offering player of a steal
			if (steal[0] == steal[1]) // Peng
				param.hand_tiles.fixed_packs[i] =
				mahjong::make_pack(1, PACK_TYPE_PUNG, TILE_T(steal[0]));
			else if (steal[0] + 1 == steal[1]) // Chi
				param.hand_tiles.fixed_packs[i] =
				mahjong::make_pack(1, PACK_TYPE_CHOW, TILE_T(steal[0]));
			else if (steal[1] == 0) // Gang, assume Ming Gang
				param.hand_tiles.fixed_packs[i] =
				mahjong::make_pack(1, PACK_TYPE_KONG, TILE_T(steal[0]));
		}

		param.hand_tiles.tile_count = hand_count - hand_count % 3 + 1;
		for (int i = 0; i < waits.size(); i++) {
			if (waits[i].needs.size() == 0) { // real Hu
				param.win_flag = 0;
				if (game.last_turn.action == ACT_DRAW)
					param.win_flag |= WIN_FLAG_SELF_DRAWN;
				if (game.remain[game.last_card] == 0)
					param.win_flag |= WIN_FLAG_4TH_TILE;
				if (game.wall_count == 0)
					param.win_flag |= WIN_FLAG_WALL_LAST;
				if (game.last_turn.action == ACT_GANG || game.last_turn.action == ACT_BU_GANG)
					param.win_flag |= WIN_FLAG_ABOUT_KONG;
				int removed = 0;
				for (int j = 0; j < hand_count; j++) {
					if (!removed && game.hand[j] == game.last_card)
						removed = 1;
					else
						param.hand_tiles.standing_tiles[j - removed] = TILE_T(game.hand[j]);
				}
				param.win_tile = TILE_T(game.last_card);
			}
			else { // estimation
				param.win_flag = 0;
				int removed = 0;
				for (int j = 0; j < hand_count; j++) {
					if (removed < waits[i].discards.size() && game.hand[j] == waits[i].discards[removed])
						removed++;
					else
						param.hand_tiles.standing_tiles[j - removed] = TILE_T(game.hand[j]);
				}
				for (int j = 0; j < waits[i].needs.size() - 1; j++)
					param.hand_tiles.standing_tiles[hand_count - removed + j] = TILE_T(waits[i].needs[j]);
				// crude assumption: win_tile is the last tile in needs
				param.win_tile = TILE_T(waits[i].needs[waits[i].needs.size() - 1]);
			}
			int score = mahjong::calculate_fan(&param, nullptr);
			assert(score > 0);
			scores.push_back(score);
		}
		return scores;
	}

	vector<Wait> SearchHuWaits(const Game &game, int max_tiles)
	{
		// Computation ~ C(hand_count, 4) * 4^4 * 13 < 2e6
		// pruned by max_tiles and remain
		vector<Wait> waits;

		HuEngine engine(game.hand, MY(hand_count), const_cast<int*>(game.remain), max_tiles);
		engine.DFS();
		waits = engine.waits;
		// a crude solution to suppressing multiple recall
		sort(waits.begin(), waits.end());
		auto new_end = unique(waits.begin(), waits.end());
		if (waits.end() - new_end > 0)
			WARN_ONCE("Multiple recall happens in Hu.\n");
		waits.erase(new_end, waits.end());
		return waits;
	}

	vector<Wait> SearchQiDuiWaits(const Game &game, int max_tiles)
	{
		// Computation ~ C(13, 7) = 2e3
		// pruned by max_tiles and remain
		vector<Wait> waits;

		QiDuiEngine engine(game.hand, MY(hand_count), const_cast<int*>(game.remain), max_tiles);
		engine.DFS();
		waits = engine.waits;
		// a crude solution to suppressing multiple recall
		sort(waits.begin(), waits.end());
		auto new_end = unique(waits.begin(), waits.end());
		if (waits.end() - new_end > 0)
			WARN_ONCE("Multiple recall happens in Qi Dui.\n");
		waits.erase(new_end, waits.end());
		return waits;
	}

	vector<Wait> SearchBuKaoWaits(const Game &game, int max_tiles)
	{
		// Computation ~ 3^3 * C(16, 14) = 3e3
		// pruned by max_tiles and remain
		vector<Wait> waits;

		BuKaoEngine engine(game.hand, MY(hand_count), const_cast<int*>(game.remain), max_tiles);
		engine.DFS();
		waits = engine.waits;
		// a crude solution to suppressing multiple recall
		sort(waits.begin(), waits.end());
		auto new_end = unique(waits.begin(), waits.end());
		if (waits.end() - new_end > 0)
			WARN_ONCE("Multiple recall happens in Bu Kao.\n");
		waits.erase(new_end, waits.end());
		return waits;
	}

	vector<Wait> SearchOrphanWaits(const Game &game, int max_tiles)
	{
		// Computation ~ 13 * 13 = 2e2
		// pruned by max_tiles and remain
		vector<Wait> waits;

		OrphanEngine engine(game.hand, MY(hand_count), const_cast<int*>(game.remain), max_tiles);
		engine.DFS();
		waits = engine.waits;
		// a crude solution to suppressing multiple recall
		sort(waits.begin(), waits.end());
		auto new_end = unique(waits.begin(), waits.end());
		if (waits.end() - new_end > 0)
			WARN_ONCE("Multiple recall happens in Orphan.\n");
		waits.erase(new_end, waits.end());
		return waits;
	}
}

void Wait::print(int level) const
{
	indent_printf(level * 4, "needs =");
	for (int i = 0; i < this->needs.size(); i++)
		printf(" %s%c", TILE_REPR(this->needs[i]), ",\n"[i == this->needs.size() - 1]);
	indent_printf(level * 4, "discards =");
	for (int i = 0; i < this->discards.size(); i++)
		printf(" %s%c", TILE_REPR(this->discards[i]), ",\n"[i == this->discards.size() - 1]);
}

void HuEngine::DFS(int num_melds, int start)
{
	if (needs.size() > max_tiles)
		return;
	if (num_melds == hand_count / 3 + 1) {
		Wait wait;
		wait.needs = needs;
		wait.obtains = obtains;
		assert(wait.needs.size() == wait.obtains.size());
		for (int i = 0; i < hand_count; i++) {
			if (!keep[i]) {
				bool found = false;
				for (int j = 0; j < wait.needs.size(); j++) {
					if (wait.needs[j] == hand[i]) {
						WARN_ONCE("Tile happens in both needs and discards.\n");
						wait.needs.erase(wait.needs.begin() + j);
						wait.obtains.erase(wait.obtains.begin() + j);
						found = true;
						break;
					}
				}
				if (!found)
					wait.discards.push_back(hand[i]);
			}
		}
		waits.push_back(wait);
		return;
	}

	if (num_melds < hand_count / 3) { // decide a meld
		for (int i = start; i <= hand_count - hand_count / 3 + num_melds; i++) {
			// only the first occurred tile in hand should be considered
			// otherwise would cause multiple recall
			if (!keep[i] && (i - 1 < 0 || hand[i - 1] != hand[i] || keep[i - 1])) {
				keep[i] = true;
				// Peng
				int num_need = 2;
				for (int j = i + 1; j < hand_count && hand[j] == hand[i] && num_need > 0; j++) {
					if (!keep[j]) {
						keep[j] = true;
						num_need--;
					}
				}
				for (int j = 0; j < num_need; j++) {
					needs.push_back(hand[i]);
					obtains.push_back(j ? NONE : PENG);
				}
				remain[hand[i]] -= num_need;
				if (remain[hand[i]] >= 0)
					DFS(num_melds + 1, i + 1);
				remain[hand[i]] += num_need;
				for (int j = 0; j < num_need; j++) {
					needs.pop_back();
					obtains.pop_back();
				}
				for (int j = i + 1; j < hand_count && hand[j] == hand[i] && num_need < 2; j++) {
					if (keep[j]) {
						keep[j] = false;
						num_need++;
					}
				}
				// Chi {X, X+1, X+2}
				if (NUMBER(hand[i]) <= 7 && IS_SUIT(hand[i])) {
					int succ = 0, succ2 = 0;
					for (int j = i + 1; j < hand_count && hand[j] <= hand[i] + 2; j++) {
						if (!keep[j]) {
							if (!succ && hand[j] == hand[i] + 1) {
								keep[j] = true;
								succ = j;
							}
							else if (hand[j] == hand[i] + 2) {
								keep[j] = true;
								succ2 = j;
								break;
							}
						}
					}
					if (!succ) {
						needs.push_back(hand[i] + 1);
						remain[hand[i] + 1]--;
					}
					if (!succ2) {
						needs.push_back(hand[i] + 2);
						remain[hand[i] + 2]--;
					}
					if (remain[hand[i] + 1] >= 0 && remain[hand[i] + 2] >= 0) {
						if (!succ) {
							obtains.push_back(CHI);
							if (!succ2)
								obtains.push_back(NONE);
							DFS(num_melds + 1, i + 1);
							if (!succ2)
								obtains.pop_back();
							obtains.pop_back();
						}
						if (!succ2) {
							if (!succ)
								obtains.push_back(NONE);
							obtains.push_back(CHI);
							DFS(num_melds + 1, i + 1);
							obtains.pop_back();
							if (!succ)
								obtains.pop_back();
						}
						if (succ && succ2)
							DFS(num_melds + 1, i + 1);
					}
					if (!succ2) {
						needs.pop_back();
						remain[hand[i] + 2]++;
					}
					else
						keep[succ2] = false;
					if (!succ) {
						needs.pop_back();
						remain[hand[i] + 1]++;
					}
					else
						keep[succ] = false;
				}
				// Chi {X-1, X, X+1}, where X-1 should not be exist
				if (NUMBER(hand[i]) >= 2 && NUMBER(hand[i]) <= 8 && IS_SUIT(hand[i])) {
					bool valid = true;
					for (int j = i - 1; j > 0 && hand[j] >= hand[i] - 1; j--) {
						if (!keep[j] && hand[j] != hand[i]) {
							valid = false;
							break;
						}
					}
					if (valid) {
						int succ = 0;
						for (int j = i + 1; j < hand_count && hand[j] <= hand[i] + 1; j++) {
							if (!keep[j] && hand[j] == hand[i] + 1) {
								keep[j] = true;
								succ = j;
								break;
							}
						}
						needs.push_back(hand[i] - 1);
						remain[hand[i] - 1]--;
						if (!succ) {
							needs.push_back(hand[i] + 1);
							remain[hand[i] + 1]--;
						}
						if (remain[hand[i] - 1] >= 0 && remain[hand[i] + 1] >= 0) {
							obtains.push_back(CHI);
							if (!succ)
								obtains.push_back(NONE);
							DFS(num_melds + 1, i + 1);
							if (!succ)
								obtains.pop_back();
							obtains.pop_back();
							if (!succ) {
								obtains.push_back(NONE);
								obtains.push_back(CHI);
								DFS(num_melds + 1, i + 1);
								obtains.pop_back();
								obtains.pop_back();
							}
						}
						if (!succ) {
							needs.pop_back();
							remain[hand[i] + 1]++;
						}
						else
							keep[succ] = false;
						needs.pop_back();
						remain[hand[i] - 1]++;
					}
				}
				// Chi {X-2, X-1, X}, where X-2 and X-1 should not be exist
				if (NUMBER(hand[i]) >= 3 && IS_SUIT(hand[i])) {
					bool valid = true;
					for (int j = i - 1; j > 0 && hand[j] >= hand[i] - 2; j--) {
						if (!keep[j] && hand[j] != hand[i]) {
							valid = false;
							break;
						}
					}
					needs.push_back(hand[i] - 2);
					remain[hand[i] - 2]--;
					needs.push_back(hand[i] - 1);
					remain[hand[i] - 1]--;
					if (remain[hand[i] - 2] >= 0 && remain[hand[i] - 1] >= 0) {
						obtains.push_back(CHI);
						obtains.push_back(NONE);
						DFS(num_melds + 1, i + 1);
						obtains.pop_back();
						obtains.pop_back();
						obtains.push_back(NONE);
						obtains.push_back(CHI);
						DFS(num_melds + 1, i + 1);
						obtains.pop_back();
						obtains.pop_back();
					}
					needs.pop_back();
					needs.pop_back();
					remain[hand[i] - 1]++;
					remain[hand[i] - 2]++;
				}
				keep[i] = false;
			}
		}
	}
	else { // decide an eye
		for (int i = 0; i < hand_count; i++) {
			// where only the first occurred tile in hand should be considered
			// otherwise would cause multiple recall
			if (!keep[i] && (i - 1 < 0 || hand[i - 1] != hand[i] || keep[i - 1])) {
				keep[i] = true;
				bool has_another = false;
				for (int j = i + 1; j < hand_count && hand[j] == hand[i]; j--) {
					if (!keep[j]) {
						keep[j] = true;
						has_another = true;
						break;
					}
				}
				if (!has_another) {
					needs.push_back(hand[i]);
					obtains.push_back(NONE);
					remain[hand[i]]--;
				}
				if (remain[hand[i]] >= 0)
					DFS(num_melds + 1, hand_count);
				if (!has_another) {
					needs.pop_back();
					obtains.pop_back();
					remain[hand[i]]++;
				}
				for (int j = i + 1; j < hand_count && hand[j] == hand[i]; j++) {
					if (has_another && keep[j]) {
						keep[j] = false;
						has_another = false;
						break;
					}
				}
				keep[i] = false;
			}
		}
	}
}

void QiDuiEngine::DFS(int num_melds, int start)
{
	if (hand_count < 13 || needs.size() > max_tiles)
		return;
	if (num_melds == 7) {
		Wait wait;
		wait.needs = needs;
		wait.obtains = obtains;
		assert(wait.needs.size() == wait.obtains.size());
		for (int i = 0; i < hand_count; i++) {
			if (!keep[i]) {
				bool found = false;
				for (int j = 0; j < wait.needs.size(); j++) {
					if (wait.needs[j] == hand[i]) {
						WARN_ONCE("Tile happens in both needs and discards.\n");
						wait.needs.erase(wait.needs.begin() + j);
						wait.obtains.erase(wait.obtains.begin() + j);
						found = true;
						break;
					}
				}
				if (!found)
					wait.discards.push_back(hand[i]);
			}
		}
		waits.push_back(wait);
		return;
	}
	// decide an eye
	for (int i = start; i <= hand_count - 7 + num_melds; i++) {
		// where only the first occurred tile in hand should be considered
		// otherwise would cause multiple recall
		if (!keep[i] && (i - 1 < 0 || hand[i - 1] != hand[i] || keep[i - 1])) {
			keep[i] = true;
			bool has_another = false;
			for (int j = i + 1; j < hand_count && hand[j] == hand[i]; j--) {
				if (!keep[j]) {
					keep[j] = true;
					has_another = true;
					break;
				}
			}
			if (!has_another) {
				needs.push_back(hand[i]);
				obtains.push_back(NONE);
				remain[hand[i]]--;
			}
			if (remain[hand[i]] >= 0)
				DFS(num_melds + 1, start + 1);
			if (!has_another) {
				needs.pop_back();
				obtains.pop_back();
				remain[hand[i]]++;
			}
			for (int j = i + 1; j < hand_count && hand[j] == hand[i]; j++) {
				if (has_another && keep[j]) {
					keep[j] = false;
					has_another = false;
					break;
				}
			}
			keep[i] = false;
		}
	}
}

void BuKaoEngine::DFS(int num_melds, int start)
{
	if (hand_count < 13 || needs.size() > max_tiles + 2)
		return;
	if (num_melds == 10) {
		Wait wait;
		assert(needs.size() == obtains.size());
		for (int i = 0; i < hand_count; i++) {
			if (!keep[i])
				wait.discards.push_back(hand[i]);
		}
		for (int i = 0; i < needs.size(); i++) {
			for (int j = i + 1; j < needs.size(); j++) {
				wait.needs.clear();
				wait.obtains.clear();
				for (int k = 0; k < needs.size(); k++) {
					if (k != i && k != j) {
						wait.needs.push_back(needs[k]);
						wait.obtains.push_back(obtains[k]);
					}
				}
				waits.push_back(wait);
			}
		}
		return;
	}
	if (num_melds < 3) {
		for (int i = 0; i < 3; i++) {
			if (!used[i]) {
				used[i] = true;
				int j;
				int orphans[3] = { -1, -1, -1 };
				for (j = start; j < hand_count && SUIT(hand[j]) <= num_melds; j++) {
					if (NUMBER(hand[j]) % 3 == i) {
						int idx = (NUMBER(hand[j]) - 1) / 3;
						if (orphans[idx] == -1) {
							keep[j] = true;
							orphans[idx] = j;
						}
					}
				}
				bool valid = true;
				for (int k = 0; k < 3; k++) {
					if (orphans[k] == -1) {
						tile t = TILE(num_melds, (k + (i == 0)) * 3 + i);
						needs.push_back(t);
						obtains.push_back(NONE);
						valid &= --remain[t] >= 0;
					}
				}
				if (valid)
					DFS(num_melds + 1, j);
				for (int k = 2; k >= 0; k--) {
					if (orphans[k] != -1)
						keep[orphans[k]] = false;
					else {
						tile t = TILE(num_melds, (k + (i == 0)) * 3 + i);
						needs.pop_back();
						obtains.pop_back();
						remain[t]++;
					}
				}
				used[i] = false;
			}
		}
	}
	else {
		tile t = TILE(3, num_melds - 2);
		int i;
		for (i = start; i < hand_count && hand[i] < t; i++);
		if (i < hand_count && hand[i] == t)
			keep[i] = true;
		else {
			needs.push_back(t);
			obtains.push_back(NONE);
			remain[t]--;
		}
		if (remain[t] >= 0)
			DFS(num_melds + 1, i);
		if (i < hand_count && keep[i])
			keep[i] = false;
		else {
			needs.pop_back();
			obtains.pop_back();
			remain[t]++;
		}
	}
}

void OrphanEngine::DFS(int num_melds, int start)
{
	if (hand_count < 13 || needs.size() > max_tiles)
		return;
	if (num_melds == 13) {
		Wait wait;
		assert(needs.size() == obtains.size());
		for (int i = 0; i < 13; i++) {
			wait.needs = needs;
			wait.discards.clear();
			wait.obtains.clear();
			bool found = false; // whether the eye exist
			for (int j = 0; j < hand_count; j++) {
				if (!keep[j]) {
					if (!found && hand[j] == orphan[i])
						found = true;
					else
						wait.discards.push_back(hand[j]);
				}
			}
			if (!found) {
				wait.needs.push_back(orphan[i]);
				wait.obtains.push_back(Obtain(NONE));
				sort(wait.needs.begin(), wait.needs.end());
			}
			if (wait.needs.size() <= max_tiles)
				waits.push_back(wait);
		}
		return;
	}
	tile t = orphan[num_melds];
	int i;
	for (i = start; i < hand_count && hand[i] < t; i++);
	if (i < hand_count && hand[i] == t)
		keep[i] = true;
	else {
		needs.push_back(t);
		obtains.push_back(NONE);
		remain[t]--;
	}
	if (remain[t] >= 0)
		DFS(num_melds + 1, i);
	if (i < hand_count && keep[i])
		keep[i] = false;
	else {
		needs.pop_back();
		obtains.pop_back();
		remain[t]++;
	}
}