#include <stdlib.h>
#include <assert.h>
#include "utils.h"
#include "simulator.h"
#include "bot.h"

//#undef assert
//#define assert(expr) do { \
//		if (!(expr)) { \
//			printf("assert(%s) failed: %s %d\n", #expr, __FILE__, __LINE__); \
//			exit(1); \
//		} \
//	} while(0)

int main(int argc, char *argv[])
{
	srand(time(NULL));
	int n = 1000;
	if (argc >= 2)
		n = atoi(argv[1]);
	char filename[256] = R"(data\MaxProbabilityBot[37]_train_1000.match)";
	if (argc >= 3)
		strcpy(filename, argv[2]);

	/*Simulator simulator;
	FILE *fin = fopen(filename, "rb");
	for (int i = 0; i < 100; i++) {
		simulator.load(fin);
		for (int i = 0; i < NUM_PLAYER; i++) {
			assert(simulator.response[0][i].action == ACT_PASS);
			assert(simulator.response[1][i].action == ACT_PASS);
		}
	}
	fclose(fin);*/

	FILE *fout = fopen(filename, "wb");
	MaxProbabilityBot bot(5);
	Simulator simulator(dynamic_cast<Bot*>(&bot));
	for (int i = 0; i < n; i++) {
		printf("simulating match %d\n", i);
		float seconds = profile([&]() -> void { simulator.run(); }) / 1000.0;
		printf("time elapsed: %g s\n", seconds);
		simulator.save(fout);
	}
	fclose(fout);
}