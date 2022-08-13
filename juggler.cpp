#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
#include <mpir.h>
#include <map>
#include <chrono>
#include <utility>
#include <omp.h>
#include <mutex>
#include <set>

typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()

//uint64_t
typedef unsigned long long ull;

//32MB of int = 32 * 1024 * 1024 / 4
//const int CACHE_SIZE = 8388608;
//int CACHE[CACHE_SIZE];
std::mutex MUTEX;

void juggle_one(mpz_t n) {
	if (mpz_even_p(n) != 0) {
		mpz_sqrt(n, n);
	}
	else {
		mpz_pow_ui(n, n, 3);
		mpz_sqrt(n, n);
	}
}

/*ull juggle_one(ull n) {
	if (n%2 == 0) {
		return sqrt(n);
	}
	else {
		return sqrt(n * n * n);
	}
}*/

int juggle(mpz_t n) {

	mpz_t hold;
	mpz_init(hold);
	mpz_set(hold, n);

	int jumps = 0;
	//std::cout << hold << "\t" << mpz_cmp_ui(hold, 1) << "\n";
	while (mpz_cmp_ui(hold, 1) > 0) {
		juggle_one(hold);
		jumps += 1;
	}

	mpz_clear(hold);

	return jumps;
}

int juggle_thrash(mpz_t n) {

	int jumps = 0;
	while (mpz_cmp_ui(n, 1) > 0) {
		juggle_one(n);
		jumps += 1;
	}

	return jumps;
}

std::map<ull, int> juggle_sweep_1(ull cap) {

	mpz_t op;
	mpz_init(op);

	std::map<ull, int> m;
	int max_steps = -1;
	for (ull n = 1; n < cap; n += 2) {
		mpz_set_ui(op, n);
		int steps = juggle(op);
		if (steps > max_steps) {
			m[n] = steps;
			//std::cout << n << "\t" << steps << "\n";
			max_steps = steps;
		}
	}

	mpz_clear(op);
	
	return m;
}
/*
std::map<ull, int> juggle_sweep_2(ull cap) {

	mpz_t hold;
	mpz_init(hold);
	std::map<ull, int> m;
	int max_steps = -1;
	for (ull n = 1; n < cap; n += 2) {
		mpz_set_ui(hold, n);

		int steps = 0;
		//std::cout << hold << "\t" << mpz_cmp_ui(hold, 1) << "\n";
		while (mpz_cmp_ui(hold, 1) > 0) {
			juggle_one(hold);

			if (mpz_fits_ulong_p(hold) != 0) {
				ull hold_ull = mpz_get_ui(hold);
				if (hold_ull % 2 == 1) {
					if (hold_ull < CACHE_SIZE < 0 && hold_ull < n) {
						steps += 1 + CACHE[hold_ull / 2];
						break;
					}
				}
			}
			steps += 1;
		}

		if (steps > max_steps) {
			m[n] = steps;
			std::cout << n << "\t" << steps << "\n";
			max_steps = steps;
		}

		if (n < CACHE_SIZE) {
			CACHE[n/2] = steps;
		}
	}

	mpz_clear(hold);

	return m;

}*/


std::map<ull, int> juggle_sweep_3(long long cap) {

	std::map<ull, int> m;
	int max_steps = -1;

#pragma omp parallel for ordered schedule(dynamic) num_threads(16)
	for (long long n = 1; n < cap; n += 2) {

		ull un = (ull)n;
		mpz_t hold;
		mpz_init(hold);
		mpz_set_ui(hold, un);
		int steps = juggle_thrash(hold);
		mpz_clear(hold);
		if (steps > max_steps) {
			MUTEX.lock();
			bool small_best = true;
			for (auto const& [num, rec_steps] : m) {
				if (num <= un && rec_steps >= steps) {
					small_best = false;
				}
			}
			if (small_best) {
				m[un] = steps;
				//std::cout << un << "\t" << steps << "\n";
				max_steps = steps;
			}
			MUTEX.unlock();
		}
	}

	std::set<ull> kills;
	for (auto const& [num1, steps1] : m) {
		for (auto const& [num2, steps2] : m) {
			if (num1 < num2 && steps1 >= steps2) {
				kills.insert(num2);
			}
		}
	}
	for (ull kill : kills) {
		m.erase(kill);
	}

	return m;
}

double secDelt(TimeVar stop, TimeVar start) {
	return ((double)duration(stop - start)) / 1'000'000'000;
}

std::map<ull, int> juggle_sweep_4(ull cap) {
	TimeVar start = timeNow();
	TimeVar lastTick = start;
	std::map<ull, int> m;
	int max_steps = -1;

#pragma omp parallel for ordered schedule(dynamic) num_threads(16)
	for (int blockInd = 0; blockInd < cap / 1000; blockInd++) {
		mpz_t hold;
		mpz_init(hold);
		for (int threadInd = 1; threadInd < 1000; threadInd += 2) {

			ull un = blockInd;
			un = un * 1000 + threadInd;

			if (un % 10'000'000 == 1) {
				TimeVar now = timeNow();
				std::cout << "Progress: " << un / 1'000'000 << " mil\t(" << secDelt(now, start) << " tot s,\t" << secDelt(now,lastTick) << "delt s)\n";
				lastTick = now;
			}

			mpz_set_ui(hold, un);
			int steps = juggle_thrash(hold);
			
			if (steps > max_steps) {
				MUTEX.lock();
				bool small_best = true;
				for (auto const& [num, rec_steps] : m) {
					if (num <= un && rec_steps >= steps) {
						small_best = false;
					}
				}
				if (small_best) {
					m[un] = steps;
					std::cout << un << "\t" << steps << "\n";
					max_steps = steps;
				}
				MUTEX.unlock();
			}
		}
		mpz_clear(hold);
	}


	std::set<ull> kills;
	for (auto const& [num1, steps1] : m) {
		for (auto const& [num2, steps2] : m) {
			if (num1 < num2 && steps1 >= steps2) {
				kills.insert(num2);
			}
		}
	}
	for (ull kill : kills) {
		m.erase(kill);
	}

	return m;
}

template<typename F, typename... Args>
double funcTime(F func, Args&&... args) {
	TimeVar t1 = timeNow();
	func(std::forward<Args>(args)...);
	return ((double)duration(timeNow() - t1))/1'000'000'000;
}

template<typename k, typename v>
void printMap(std::map<k, v> m) {
	for (auto const& [key, val] : m) {
		std::cout << key << "\t" << val << "\n";
	}
}

int main() {

	std::cout << std::boolalpha;
	std::cout << "max threads: " << omp_get_max_threads() << std::endl;
	ull small = 10'000;
	ull med = 1'000'000;
	ull infin = 10'000'000'000;

	//juggle_sweep_1(small);
	//juggle_sweep_2(small);
	juggle_sweep_4(infin);

	//printMap(juggle_sweep_1(med));
	//std::cout << "\n";
	//std::cout << "juggle_sweep_1:\t" << funcTime(juggle_sweep_1, med) << "\n";
	//printMap(juggle_sweep_3(med));
	//std::cout << "\n";
	//std::cout << "juggle_sweep_3:\t" << funcTime(juggle_sweep_3, med) << "\n";

	//std::cout << "\n";
	//std::cout << "juggle_sweep_4:\t" << funcTime(juggle_sweep_4, med) << "\n";
}