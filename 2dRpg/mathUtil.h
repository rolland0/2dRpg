#pragma once

inline int min(int a, int b)
{
	return (a < b) ? a : b;
}

inline int max(int a, int b)
{
	return (a > b) ? a : b;
}

inline float clamp(float val, float min, float max)
{
	if(val < min) return min;
	if(val > max) return max;
	return val;
}

inline int clamp(int val, int min, int max)
{
	if(val < min) return min;
	if(val > max) return max;
	return val;
}
