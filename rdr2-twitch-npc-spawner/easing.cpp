#include <cmath>
#include <numbers>
#include "easing.h"

float Easing::Sine(float x)
{
	return -(cos(x * std::numbers::pi_v<float>) - 1) / 2;
}

float Easing::SineIn(float x)
{
	return 1 - cos((x * std::numbers::pi_v<float>) / 2);
}

float Easing::SineOut(float x)
{
	return sin((x * std::numbers::pi_v<float>) / 2);
}

float Easing::Quad(float x)
{
	return x < 0.5f ? 2.0f * x * x : 1.0f - (-2.0f * x + 2.0f) * (-2.0f * x + 2.0f) / 2.0f;
}

float Easing::QuadIn(float x)
{
	return x * x;
}

float Easing::QuadOut(float x)
{
	return 1.0f - (1.0f - x) * (1.0f - x);
}
