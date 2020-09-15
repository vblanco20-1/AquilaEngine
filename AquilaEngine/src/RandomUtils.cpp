#include <PrecompiledHeader.h>
import "RandomUtils.h";
#include <random>

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(1, 10);
std::uniform_int_distribution<int> randompos(-30, 30);
std::uniform_real_distribution<float> randomfloat(-1, 1);


float rng::RandomFloat() {
	return randomfloat(generator);
}
int rng::RandomPosition() {
	return randompos(generator);
}