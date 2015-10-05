#pragma once

struct WorldRect {
	float x;
	float y;
	float w;
	float h;
};

inline 
bool xOverlap(WorldRect &a, WorldRect &b)
{
	return a.x + a.w > b.x && a.x < b.x + b.w;
}

inline 
bool yOverlap(WorldRect &a, WorldRect &b)
{
	return a.y + a.h > b.y && a.y < b.y + b.h;
}

inline 
bool standingOn(WorldRect &a, WorldRect &b)
{
	return a.y == b.y + b.h;
}

inline 
bool isAbove(WorldRect &a, WorldRect &b)
{
	return a.y >= b.y + b.h;
}

inline 
bool isBelowBottom(WorldRect &a, WorldRect &b)
{
	return a.y < b.y;
}

inline
bool isBelowTop(WorldRect &a, WorldRect &b)
{
	return a.y < b.y + b.h;
}
