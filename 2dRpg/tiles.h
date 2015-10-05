#pragma once

enum TileType {
	TileNone = 0,
	TilePlatform = 1,
	TileLadder = 2,
	TileNumElements
};

enum TileSolidity {
	TsNonSolid,
	TsTransientSolid,
	TsSolid,
	TsNumElements
};

struct TileImpl {
	float hitboxOffsetX;
	float hitboxOffsetY;
	float hitboxWidth;
	float hitboxHeight;
	TileSolidity solidity;
};

TileImpl TileCommon[TileType::TileNumElements];

struct Tile {
	TileType type = TileType::TileNone;
	float xPos = 0.0f;
	float yPos = 0.0f;
	TileImpl *common = 0;
	//float hitboxWidth;
	//float hitboxHeight;
	//TileSolidity solidity;
	bool isOccupied = false;
};

struct TileMap {
	static const int Width = 10;
	static const int Height = 10;
	float tileWidth = 0.0f;
	float tileHeight = 0.0f;

	Tile tiles[Height][Width];
};

inline
void readTileMapText(TileMap &map, const char* filename, float worldWidth, float worldHeight)
{
	map.tileWidth = worldWidth / map.Width;
	map.tileHeight = worldHeight / map.Height;

	TileCommon[TileType::TileNone] = TileImpl {0.0f, 0.0f, map.tileWidth, map.tileHeight, TileSolidity::TsNonSolid};
	TileCommon[TileType::TileLadder] = TileImpl {0.0f, 0.0f, map.tileWidth, map.tileHeight, TileSolidity::TsTransientSolid};
	TileCommon[TileType::TilePlatform] = TileImpl {0.0f, 0.0f, map.tileWidth, map.tileHeight, TileSolidity::TsTransientSolid};


	std::ifstream file;
	std::string line;
	file.open(filename);
	if(file.is_open())
	{
		int y = 0;
		while(getline(file, line))
		{
			for(unsigned int x = 0; x < line.length() && y < TileMap::Height; x++)
			{
				TileType type = (TileType)(line.at(x) - '0');
				Tile *tile = &(map.tiles[((map.Height - 1) - y)][x]);
				switch(type)
				{
				case TileType::TileNone:
					tile->type = TileType::TileNone;
					tile->xPos = 0.0f;
					tile->yPos = 0.0f;
					tile->common = &TileCommon[TileType::TileNone];
					break;

				case TileType::TilePlatform:
					tile->type = TileType::TilePlatform;
					tile->xPos = x*map.tileWidth;
					tile->yPos = ((map.Height - 1) - y)*map.tileHeight;
					tile->common = &TileCommon[TileType::TilePlatform];
					break;

				case TileType::TileLadder:
					tile->type = TileType::TileLadder;
					tile->xPos = x*map.tileWidth;
					tile->yPos = ((map.Height - 1) - y)*map.tileHeight;
					tile->common = &TileCommon[TileType::TileLadder];
					break;
				}
			}
			y++;
		}
		file.close();
	}
}

struct OccupiedTiles {
	static const int MaxNumOccupiedTiles = 20;
	Tile *playerOccupiedTiles[MaxNumOccupiedTiles] = {};
	int numOccupiedTiles = 0;
	int iter = 0;

	bool hasNext()
	{
		if(iter < numOccupiedTiles)
		{
			return true;
		} else
		{
			iter = 0;
			return false;
		}
	}

	Tile *next()
	{
		return playerOccupiedTiles[iter++];
	}

	void removeCurrent()
	{
		iter--;
		iter = clamp(iter, 0, numOccupiedTiles - 1);
		playerOccupiedTiles[iter] = playerOccupiedTiles[numOccupiedTiles - 1];
		playerOccupiedTiles[numOccupiedTiles - 1] = NULL;
		numOccupiedTiles--;
	}

	void add(Tile* tile)
	{
		bool alreadyExists = false;
		for(int i = 0; i < numOccupiedTiles && !alreadyExists; i++)
		{
			alreadyExists = (tile == playerOccupiedTiles[i]);
		}
		if(!alreadyExists)
		{
			playerOccupiedTiles[numOccupiedTiles++] = tile;
		}
	}
};
