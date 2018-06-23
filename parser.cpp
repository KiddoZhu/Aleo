#ifndef _BOTZONE_ONLINE
#include "parser.h"
#endif

vector<Message> ParseInput()
{
	vector<Message> inputs;
	int me;
	tile last_card = 0;
	int n;
	char s[100000], *req;

	scanf("%d", &n);
	for (int i = 0; i < 2 * n - 1; i++) {
		Message msg;
		scanf("\n%[^\n]", s);
		// responses
		req = nullptr;
		switch (s[0]) {
		case '0':
			msg.action = ACT_QUAN;
			msg.player = me = DIGIT(s[2]);
			msg.dealer = DIGIT(s[4]);
			break;
		case '1':
			msg.action = ACT_RECEIVE;
			msg.player = me;
			char *p;
			p = s + 2;
			for (int i = 0; i < NUM_PLAYER; i++, p += 2)
				msg.flower_count[i] = DIGIT(*p);
			for (int i = 0; i < 13; i++, p += 3)
				msg.card[i] = CARD(p);
			// we do not parse flower cards here
			break;
		case '2':
			msg.action = ACT_DRAW;
			msg.player = me;
			msg.card[0] = CARD(s + 2);
			break;
		case '3':
			// my An Gang
			if (s[4] == 'G' && inputs[inputs.size() - 1].action == ACT_DRAW)
				continue;
			msg.player = DIGIT(s[2]);
			req = s + 4;
			break;
		default:
			// not my An Gang
			if (s[0] != 'G' || s[4] == 0)
				continue;
			msg.player = me;
			req = s;
		}
		// requests
		if (req)
			switch (req[0]) {
			case 'B':
				if (req[2] == 'H')
					// we do not parse flower cards here
					msg.action = ACT_BU_HUA;
				else {
					msg.action = ACT_BU_GANG;
					msg.card[0] = CARD(req + 7);
				}
				break;
			case 'D':
				msg.action = ACT_OTHER_DRAW;
				break;
			case 'P':
				if (req[1] == 'L') {
					msg.action = ACT_PLAY;
					msg.card[0] = last_card = CARD(req + 5);
				}
				else if (req[1] == 'E') {
					msg.action = ACT_PENG;
					msg.card[0] = last_card;
					msg.card[1] = last_card = CARD(req + 5);
				}
				else
					continue;
				break;
			case 'C':
				msg.action = ACT_CHI;
				msg.card[0] = CARD(req + 4);
				msg.card[1] = last_card = CARD(req + 7);
				break;
			case 'G':
				msg.action = ACT_GANG;
				if (inputs[inputs.size() - 1].action == ACT_DRAW) // My An Gang
					msg.card[0] = CARD(req + 5);
				else if (inputs[inputs.size() - 1].action == ACT_OTHER_DRAW) // other's An Gang
					msg.card[0] = 0;
				else
					msg.card[0] = last_card;
				break;
		}
		// pass & responses except An Gang have been ignored
		inputs.push_back(msg);
	}
	// data & global_data is not processed
	return inputs;
}

void Output(const Message &msg)
{
	printf("%s", ACTION_OUTPUT(msg.action));
	switch (msg.action) {
	case ACT_GANG:
		if (!msg.card[0]) // Ming Gang
			break;
		// An Gang fall through
	case ACT_PLAY: case ACT_PENG:
		printf(" %s", TILE_OUTPUT(msg.card[0]));
		break;
	case ACT_CHI:
		printf(" %s %s", TILE_OUTPUT(msg.card[0]), TILE_OUTPUT(msg.card[1]));
		break;
	}
	printf("\n");
}