CXX=g++
generator: CXXFLAGS=-O2 -Wall -std=c++11 -DNDEBUG=1
fan_calculator: CXXFLAGS=-fPIC -O2 -Wall -std=c++11 -DNDEBUG=1

generator: lib/shanten.o lib/fan_calculator.o core.o faan.o simulator.o bot.o generator.o
	$(CXX) $(CXXFLAGS) -o generator \
		lib/shanten.o lib/fan_calculator.o core.o faan.o simulator.o bot.o generator.o 

fan_calculator: lib/shanten.o lib/fan_calculator.o
	$(CXX) $(CXXFLAGS) -nostartfiles -o lib/fan_calculator.so \
		lib/shanten.o lib/fan_calculator.o

clean:
	rm -f lib/*.o
	rm -f *.o