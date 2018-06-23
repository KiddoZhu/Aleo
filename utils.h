#include <utility>
#include <chrono>
using namespace std;

#include <stdio.h>

#pragma once

#ifdef _BOTZONE_ONLINE
#define WARN_ONCE
#else
#define WARN_ONCE(msg, ...) do { \
	static bool warn = false; \
	if (!warn) { \
		printf("(" __FILE__ ": %d) " msg, __LINE__, ##__VA_ARGS__); \
		warn = true; \
	} \
} while (0)
#endif

#define indent_printf(indent, msg, ...) printf("%*s" msg, indent, "", ##__VA_ARGS__)

template <typename Duration=chrono::milliseconds, typename Func, typename ... Args>
typename Duration::rep profile(Func &&func, Args &&...args)
{
	using namespace std::chrono;

	time_point<system_clock> start = high_resolution_clock::now();
	forward<Func>(func)(forward<Args>(args)...);
	time_point<system_clock> stop = high_resolution_clock::now();
	return duration_cast<Duration>(stop - start).count();
}