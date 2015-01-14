#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdio>
#include <fstream>
#include <string>

inline void LogError() {
	SDL_Log(SDL_GetError());
}

void breakHere() {}

int min(int a, int b) {
	return (a < b) ? a : b;
}

int max(int a, int b) {
	return (a > b) ? a : b;
}

float clamp(float val, float min, float max) {
	if(val < min) return min;
	if(val > max) return max;
	return val;
}

struct WorldRect {
	float x;
	float y;
	float w;
	float h;
};

bool xOverlap(WorldRect &a, WorldRect &b) {
	return a.x + a.w > b.x && a.x < b.x + b.w;
}

bool isAbove(WorldRect &a, WorldRect &b) {
	return a.y >= b.y + b.h;
}

bool isBelowBottom(WorldRect &a, WorldRect &b) {
	return a.y < b.y;
}

bool isBelowTop(WorldRect &a, WorldRect &b) {
	return a.y < b.y + b.h;
}

struct ScreenProperties {
	int screenWidth;
	int screenHeight;
	float pixPerHorizontalMeter;
	float pixPerVerticalMeter;
};

// creates an SDL_Rect from WorldRect, based on screen properties
void worldRectToRenderRect(WorldRect &wRect, SDL_Rect &rRect, ScreenProperties &screenProps) {
	rRect.w = (int)(wRect.w * screenProps.pixPerHorizontalMeter);
	rRect.h = (int)(wRect.h * screenProps.pixPerVerticalMeter);
	rRect.x = (int)(wRect.x * screenProps.pixPerHorizontalMeter);
	rRect.y = screenProps.screenHeight - ((int)(wRect.y * screenProps.pixPerVerticalMeter) + rRect.h);
}

enum TileType {
	TileNone = 0,
	TilePlatform = 1,
	TileLadder = 2
};

enum TileSolidity {
	TsNonSolid,
	TsTransientSolid,
	TsSolid
};

//struct TileImpl {
//	float hitboxWidth;
//	float hitboxHeight;
//	TileSolidity solidity;
//};

struct Tile {
	TileType type = TileType::TileNone;
	float xPos = 0.0f;
	float yPos = 0.0f;
	//TileImpl *common = NULL;
	float hitboxWidth;
	float hitboxHeight;
	TileSolidity solidity;
	bool isOccupied = false;
};

enum PlayerState {
	PsInAir,
	PsOnTransientGround,
	PsPsOnSolidGround,
	PsOnLadder
};

struct Button {
	bool isDown;
	bool wasDown;
};

struct Stick {
	float startX;
	float minX;
	float maxX;
	float endX;

	float startY;
	float minY;
	float maxY;
	float endY;
};

const int NumButtons = 6;

struct Input {
	bool isAnalog;
	Stick stick;
	union {
		Button buttons[NumButtons];
		struct {
			Button arrowUp;
			Button arrowDown;
			Button arrowLeft;
			Button arrowRight;
			Button jump;
			Button attack;
		};
	};
};

// set all lastValue to currentValue (everything that was new is now old)
void changeFrame(Input &input) {
	input.stick.startX = input.stick.minX = input.stick.maxX = input.stick.endX;
	input.stick.startY = input.stick.minY = input.stick.maxY = input.stick.endY;
	for(int i = 0; i < NumButtons; i++) {
		input.buttons[i].wasDown = input.buttons[i].isDown;
	}
}

// fills in analog stick values based on movement key values
// input.stick.end{X/Y} get persisted by function "void changeFrame(Input &input)"
void buildAnalogInput(Input &input) {
	if(!input.isAnalog) {
		input.stick.endX = 0;
		input.stick.endX += input.arrowRight.isDown;
		input.stick.endX -= input.arrowLeft.isDown;
		input.stick.endY = 0;
		input.stick.endY += input.arrowUp.isDown;
		input.stick.endY -= input.arrowDown.isDown;
	}
}

const int textLen = 100;
char Text[textLen];

SDL_Surface *renderTextBlended(TTF_Font *font, const char *text, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color color = {r, g, b, a};
	return TTF_RenderText_Blended(font, text, color);
}

struct TileMap {
	static const int Width = 10;
	static const int Height = 10;
	float tileWidth = 0.0f;
	float tileHeight = 0.0f;

	Tile tiles[Height][Width];
};


void readTileMapText(TileMap &map, const char* filename, float worldWidth, float worldHeight) {
	map.tileWidth = worldWidth / map.Width;
	map.tileHeight = worldHeight / map.Height;

	std::ifstream file;
	std::string line;
	file.open(filename);
	if(file.is_open()) {
		int y = 0;
		while(getline(file, line)) {
			for(unsigned int x = 0; x < line.length() && y < TileMap::Height; x++) {
				TileType type = (TileType)(line.at(x) - '0');
				Tile *tile = &(map.tiles[((map.Height - 1) - y)][x]);
				switch(type) {
				case TileType::TileNone:
					tile->type = TileType::TileNone;
					tile->xPos = 0.0f;
					tile->yPos = 0.0f;
					tile->hitboxWidth = 1;
					tile->hitboxHeight = 1;
					tile->solidity = TsNonSolid;
					break;

				case TileType::TilePlatform:
					tile->type = TileType::TilePlatform;
					tile->xPos = x*map.tileWidth;
					tile->yPos = ((map.Height - 1) - y)*map.tileHeight;
					tile->hitboxWidth = map.tileWidth;
					tile->hitboxHeight = map.tileHeight / 2;
					tile->solidity = TsTransientSolid;
					break;

				case TileType::TileLadder:
					tile->type = TileType::TileLadder;
					tile->xPos = x*map.tileWidth;
					tile->yPos = ((map.Height - 1) - y)*map.tileHeight;
					tile->hitboxWidth = map.tileWidth / 2;
					tile->hitboxHeight = map.tileHeight;
					tile->solidity = TsTransientSolid;
					break;
				}
			}
			y++;
		}
		file.close();
	}
}

int main(int argc, char *argv[]) {

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		LogError();
		SDL_Quit();
		return 1;
	}

	if(TTF_Init() < 0) {
		LogError();
		SDL_Quit();
		return 1;
	}

	float WorldWidth = 10.0f;
	float WorldHeight = 10.0f;

	ScreenProperties screenProps = {};
	screenProps.screenHeight = 720;
	screenProps.screenWidth = 1280;
	screenProps.pixPerVerticalMeter = screenProps.screenHeight / WorldHeight; // pixels per meter
	screenProps.pixPerHorizontalMeter = screenProps.screenWidth / WorldWidth; // pixels per meter

	SDL_Window *window = SDL_CreateWindow("2D RPG",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		screenProps.screenWidth, screenProps.screenHeight,
		SDL_WindowFlags::SDL_WINDOW_RESIZABLE);
	if(window == NULL) {
		LogError();
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RendererFlags::SDL_RENDERER_ACCELERATED | SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL) {
		LogError();
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	TTF_Font *font = TTF_OpenFont("..\\res\\font.ttf", 16);
	if(font == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Font is NULL");
		LogError();
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	Uint32 msLastFrame = SDL_GetTicks(); // ms
	Uint32 msThisFrame = msLastFrame;    // ms
	float dt = 0.0f; // seconds

	float moveSpeed = 2.68224f; // meters per second
	float gravity = -9.8f; // meters per second per second
	float jumpSpeed = 12.0f; // meters per second

	WorldRect player = {};
	player.x = 0.00f; // meters
	player.y = 5.00f; // meters
	player.w = 0.40f; // meters
	player.h = 1.75f; // meters
	float xVel = 0.0f;
	float yVel = 0.0f;

	PlayerState state = PlayerState::PsInAir;
	bool dropDown = false;
	Tile *playerOccupiedPlatform = NULL;

	TileMap map = {};
	readTileMapText(map, "..\\res\\TileMap.txt", WorldWidth, WorldHeight);

	SDL_Texture *thisInputTexture = NULL;
	SDL_Texture *lastInputTexture = NULL;

	Input input = {};

	bool drawDebug = true;
	bool drawTileGrid = false;
	bool shouldBreak = false;
	bool isRunning = true;
	while(isRunning) {

		// timing
		msThisFrame = SDL_GetTicks();
		dt = (msThisFrame - msLastFrame) / 1000.0f;

		// input processing
		changeFrame(input);
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch(e.type) {
			case SDL_EventType::SDL_QUIT:
				isRunning = false;
				break;

			case SDL_EventType::SDL_WINDOWEVENT:
				if(e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					SDL_Log("Window resized to (%d,%d)", e.window.data1, e.window.data2);
					screenProps.screenWidth = e.window.data1;
					screenProps.screenHeight = e.window.data2;
					screenProps.pixPerHorizontalMeter = screenProps.screenWidth / WorldWidth;
					screenProps.pixPerVerticalMeter = screenProps.screenHeight / WorldHeight;
				}
				break;

			case SDL_EventType::SDL_KEYDOWN:
				if(e.key.repeat) {
					break;
				}

				switch(e.key.keysym.scancode) {
				case SDL_Scancode::SDL_SCANCODE_ESCAPE:
					isRunning = false;
					break;

				case SDL_Scancode::SDL_SCANCODE_D:
					input.arrowRight.isDown = true;
					break;

				case SDL_Scancode::SDL_SCANCODE_A:
					input.arrowLeft.isDown = true;
					break;

				case SDL_Scancode::SDL_SCANCODE_W:
					input.arrowUp.isDown = true;
					break;

				case SDL_Scancode::SDL_SCANCODE_S:
					input.arrowDown.isDown = true;
					break;

				case SDL_Scancode::SDL_SCANCODE_SPACE:
					input.jump.isDown = true;
					break;

				case SDL_Scancode::SDL_SCANCODE_F1:
					breakHere();
					break;

				case SDL_Scancode::SDL_SCANCODE_F2:
					drawTileGrid = !drawTileGrid;
					break;

				case SDL_Scancode::SDL_SCANCODE_F4:
					drawDebug = !drawDebug;
					break;
				}
				break;

			case SDL_EventType::SDL_KEYUP:
				if(e.key.repeat) {
					break;
				}

				switch(e.key.keysym.scancode) {
				case SDL_Scancode::SDL_SCANCODE_D:
					input.arrowRight.isDown = false;
					break;

				case SDL_Scancode::SDL_SCANCODE_A:
					input.arrowLeft.isDown = false;
					break;

				case SDL_Scancode::SDL_SCANCODE_W:
					input.arrowUp.isDown = false;
					break;

				case SDL_Scancode::SDL_SCANCODE_S:
					input.arrowDown.isDown = false;
					break;

				case SDL_Scancode::SDL_SCANCODE_SPACE:
					input.jump.isDown = false;
					break;
				}
				break;
			}
		}

		// emulate joystick values from arrow/WASD keys
		buildAnalogInput(input);

		// process input based on player state
		switch(state) {
		case PlayerState::PsInAir:
			xVel = input.stick.endX * moveSpeed;
			yVel += gravity * dt;
			break;

		case PlayerState::PsOnLadder:
			if(input.jump.isDown && !input.jump.wasDown) {
				state = PlayerState::PsInAir;
				dropDown = true;
				xVel = input.stick.endX * moveSpeed;
				yVel += jumpSpeed / 3;
			} else {
				xVel = 0;
				yVel = input.stick.endY * moveSpeed;
			}
			break;

		case PlayerState::PsOnTransientGround:
			xVel = input.stick.endX * moveSpeed;
			if(input.stick.endY < 0 && input.stick.startY != input.stick.endY) {
				dropDown = true;
				state = PlayerState::PsInAir;
				yVel -= moveSpeed;
			} else if(input.jump.isDown && !input.jump.wasDown) {
				// jump
				state = PlayerState::PsInAir;
				yVel += jumpSpeed;
			}
			break;

		case PlayerState::PsPsOnSolidGround:
			xVel = input.stick.endX * moveSpeed;
			if(input.jump.isDown && !input.jump.wasDown) {
				// jump
				state = PlayerState::PsInAir;
				yVel += jumpSpeed;
			}
			break;
		}

		// move player
		WorldRect playerPosCopy = player;

		// move x
		player.x += xVel * dt;
		if(player.x < 0) {
			player.x = 0;
			xVel = 0;
		} else if(player.x + player.w > WorldWidth) {
			player.x = WorldWidth - player.w;
			xVel = 0;
		}

		// move y
		player.y += yVel * dt;
		if(player.y <= 0) {
			player.y = 0;
			yVel = 0;
			state = PlayerState::PsPsOnSolidGround;
		} else if(player.y + player.h > WorldHeight) {
			player.y = WorldHeight - player.h;
			yVel = 0;
			state = PlayerState::PsInAir;
		}



		// check current tile collisions
		int numCollidedTiles = 0;
		int minTileX = (int)(player.x / map.tileWidth) - 1;
		int maxTileX = (int)((player.x + player.w) / map.tileWidth) + 1;
		int minTileY = (int)(player.y / map.tileHeight) - 1;
		int maxTileY = (int)((player.y + player.h) / map.tileHeight) + 1;

		maxTileX = min(maxTileX, map.Width);
		maxTileY = min(maxTileY, map.Height);
		minTileX = max(minTileX, 0);
		minTileY = max(minTileY, 0);

		bool isOnTransientGround = false;
		bool isOnLadder = false;

		//TODO: Standing on top of a ladder is buggy;
		// player continuously falls into the ladder

		for(int tileY = minTileY; tileY < maxTileY; tileY++) {
			for(int tileX = minTileX; tileX < maxTileX; tileX++) {
				Tile *tile = &map.tiles[tileY][tileX];
				WorldRect wRect = {tile->xPos, tile->yPos, tile->hitboxWidth, tile->hitboxHeight};

				switch(tile->type) {
				case TileType::TileNone:
					break;

				case TileType::TilePlatform:
					// if the player was in the air, and was above platform, and is now under top layer...
					// then player is on the platform
					if(state == PlayerState::PsInAir) {
						if(!dropDown &&
							isAbove(playerPosCopy, wRect) && xOverlap(playerPosCopy, wRect) &&
							isBelowTop(player, wRect) && xOverlap(player, wRect)) {
							// land on platform
							state = PlayerState::PsOnTransientGround;
							isOnTransientGround = true;
							yVel = 0;
							player.y = tile->yPos + tile->hitboxHeight;
						}

						// else if the player was on a platform, and the player is on this platform...
						// then the player is still on a platform
					} else if(state == PlayerState::PsOnTransientGround) {
						if(/*isAbove(player, wRect)*/ player.y == wRect.y && xOverlap(player, wRect)) {
							isOnTransientGround = true;
						}

						// if the player was going down a ladder and is now on this platform...
						// then the player is now on this platform
					} else if(state == PlayerState::PsOnLadder) {
						if(yVel < 0 && xOverlap(player, wRect) && isBelowTop(player, wRect)) {
							// land on platform
							state = PlayerState::PsOnTransientGround;
							isOnTransientGround = true;
							yVel = 0;
							player.y = tile->yPos + tile->hitboxHeight;
						}
					}
					break;

				case TileType::TileLadder:
					// if the player is on a ladder, and is vertically on this ladder...
					// then the player is still on this ladder
					if(state == PlayerState::PsOnLadder) {
						if(!dropDown && xOverlap(player, wRect) && 
							!isBelowBottom(player, wRect) && !isAbove(player, wRect)) {
							isOnLadder = true;
						}
					} else {

						// if the player was not on a ladder, and the player presses up or down,
						// and the player is overlapping this ladder...
						// then the player is now on this ladder
						if(/*!dropDown &&*/ input.stick.startY != input.stick.endY &&
							(input.stick.endY > 0 || input.stick.endY < 0)) {
							if(xOverlap(player, wRect) && !isBelowBottom(player, wRect) && !isAbove(player, wRect)) {
								isOnLadder = true;
								state = PlayerState::PsOnLadder;
								xVel = 0;
								yVel = 0;
								player.x = tile->xPos + (tile->hitboxWidth - player.w) / 2;
							}
						}

						// if the player was in the air, and was above platform, and is now under top layer,
						// and this ladder is not below another ladder...
						// then player is on the ladder top
						if(state == PlayerState::PsInAir) {
							bool ladderHasNoLadderAboveIt = true;
							if(tileY < (maxTileY - 1)) {
								if(map.tiles[tileY + 1][tileX].type == TileType::TileLadder) {
									ladderHasNoLadderAboveIt = false;
								}
							}
							if(!dropDown && ladderHasNoLadderAboveIt &&
								isAbove(playerPosCopy, wRect) && xOverlap(playerPosCopy, wRect) &&
								isBelowTop(player, wRect) && xOverlap(player, wRect)) {
								// land on platform
								state = PlayerState::PsOnTransientGround;
								isOnTransientGround = true;
								yVel = 0;
								player.y = tile->yPos + tile->hitboxHeight;
							}

							// else if the player was on a ladder top, and the player is on this ladder top...
							// then the player is still on this ladder top
						} else if(state == PlayerState::PsOnTransientGround) {
							if(isAbove(player, wRect) && xOverlap(player, wRect)) {
								isOnTransientGround = true;
							}
						}

					}
					break;
				}
			}
		}

		// process post tile collision
		if((state == PlayerState::PsOnLadder && !isOnLadder) ||
		   (state == PlayerState::PsOnTransientGround && !isOnTransientGround)) {
			state = PlayerState::PsInAir;
		}
		dropDown = false;


		WorldRect collideRect = {(float)minTileX, (float)minTileY, (float)maxTileX - minTileX, (float)maxTileY - minTileY};

		/*
		// process input
		if(state != PlayerState::PsOnLadder) {
			if(input.stick.endY != 0) {
				for(int i = 0; i < map.numLadders; i++) {
					if(isBelowTop(player, map.ladders[i].rect) && xOverlap(player, map.ladders[i].rect)) {
						// attach to ladder
						state = PlayerState::PsOnLadder;
						player.x = map.ladders[i].rect.x + (map.ladders[i].rect.w - player.w) / 2;
						map.ladders[i].isOccupied = true;
						break;
					} else {
						map.ladders[i].isOccupied = false;
					}
				}
			}
		}

		if(state == PlayerState::PsPsOnSolidGround || state == PlayerState::PsOnTransientGround) {
			xVel = input.stick.endX * moveSpeed;
			if(input.jump.isDown && !input.jump.wasDown) {
				// jump
				state = PlayerState::PsInAir;
				yVel += jumpSpeed;
			}
		}

		if(state == PlayerState::PsOnTransientGround) {
			if(input.stick.endY < 0 && input.stick.startY >= 0) {
				// drop down
				PsDropDown = true;
				state = PsInAir;
				yVel = -jumpSpeed / 4;
			}
		}

		// ladder movement
		if(state == PlayerState::PsOnLadder) {
			xVel = 0.0f;
			yVel = input.stick.endY * moveSpeed;

			if(input.jump.isDown && !input.jump.wasDown) {
				if(input.stick.endX != 0) {
					// jump
					state = PlayerState::PsInAir;
					yVel += jumpSpeed;
				} else {
					// drop down
					PsDropDown = true;
					state = PsInAir;
					yVel = -jumpSpeed / 4;
				}
			}
		}

		// if player is not on solid ground
		if(state == PlayerState::PsInAir) {
			//launchXvel = clamp((launchXvel + input.stick.endX * dt * 100), -1, 1);
			xVel = input.stick.endX * moveSpeed;
			yVel += gravity * dt;
		}

		// move x
		player.x += xVel * dt;
		if(player.x < 0) {
			player.x = 0;
			xVel = 0;
		} else if(player.x + player.w > WorldWidth) {
			player.x = WorldWidth - player.w;
			xVel = 0;
		}

		// if player walked off platform
		if(state == PlayerState::PsOnTransientGround) {
			state = PlayerState::PsInAir;
			for(int i = 0; i < map.numPlatforms; i++) {
				if(map.platforms[i].onPlatform) {
					if(!xOverlap(player, map.platforms[i].rect)) {
						map.platforms[i].onPlatform = false;
					} else {
						state = PlayerState::PsOnTransientGround;
					}
				}
			}

			// if player is now in the air
			if(state == PlayerState::PsInAir) {

			}
		}


		// move y
		player.y += yVel * dt;
		if(player.y <= 0) {
			player.y = 0;
			yVel = 0;
			state = PlayerState::PsPsOnSolidGround;
		} else if(player.y + player.h > WorldHeight) {
			player.y = WorldHeight - player.h;
			yVel = 0;
			state = PlayerState::PsInAir;
		}

		if(state == PlayerState::PsOnLadder) {
			for(int i = 0; i < map.numLadders; i++) {
				if(isAbove(player, map.ladders[i].rect)) {
					state = PlayerState::PsInAir;
					player.y = map.ladders[i].rect.y + map.ladders[i].rect.h;
					yVel = 0;
					break;
				}
			}
		}

		for(int i = 0; i < map.numPlatforms; i++) {
			if(PsDropDown) {
				map.platforms[i].onPlatform = false;
				map.platforms[i].abovePlatform = false;

			} else {
				map.platforms[i].abovePlatform = isAbove(player, map.platforms[i].rect) && 
											xOverlap(player, map.platforms[i].rect);
				map.platforms[i].underPlatform = isBelowTop(player, map.platforms[i].rect) &&
											xOverlap(player, map.platforms[i].rect);

				if(map.platforms[i].wasAbovePlatform && map.platforms[i].underPlatform) {
					player.y = map.platforms[i].rect.y + map.platforms[i].rect.h;
					yVel = 0;
					map.platforms[i].onPlatform = true;
					state = PlayerState::PsOnTransientGround;
				}
			}
			map.platforms[i].wasAbovePlatform = map.platforms[i].abovePlatform;
		}
		*/

// Rendering

		// clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw rect
		SDL_Rect screenDest;

		// draw collide rect
		worldRectToRenderRect(collideRect, screenDest, screenProps);
		SDL_SetRenderDrawColor(renderer, 128, 64, 0, 255);
		SDL_RenderFillRect(renderer, &screenDest);

		//tile map
		WorldRect worldDest = {};
		for(int y = 0; y < TileMap::Height; y++) {
			for(int x = 0; x < TileMap::Width; x++) {
				worldDest = {
					map.tiles[y][x].xPos,
					map.tiles[y][x].yPos,
					map.tiles[y][x].hitboxWidth,
					map.tiles[y][x].hitboxHeight
				};
				Uint8 value = map.tiles[y][x].type;
				if(value > 0) {
					SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
					if(value == 1) {
						SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
					} else if(value == 2) {
						SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
					} else if(value == 3) {
						SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
					}
					worldRectToRenderRect(worldDest, screenDest, screenProps);
					SDL_RenderFillRect(renderer, &screenDest);
				}
			}
		}

		//draw player
		worldRectToRenderRect(player, screenDest, screenProps);
		SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		SDL_RenderFillRect(renderer, &screenDest);

		//draw tile grid
		if(drawTileGrid) {
			SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);
			for(int y = 0; y < map.Height; y++) {
				for(int x = 0; x < map.Width; x++) {
					WorldRect rect = {x*map.tileWidth, y*map.tileHeight, map.tileWidth, map.tileHeight};
					worldRectToRenderRect(rect, screenDest, screenProps);
					SDL_RenderDrawRect(renderer, &screenDest);
				}
			}
		}

		if(drawDebug) {
			//render new input
			{
				sprintf_s(Text, textLen, "NewInput: {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}",
					input.arrowUp.isDown,
					input.arrowDown.isDown,
					input.arrowLeft.isDown,
					input.arrowRight.isDown,
					input.jump.isDown);

				if(thisInputTexture != NULL) {
					SDL_DestroyTexture(thisInputTexture);
					thisInputTexture = NULL;
				}
				SDL_Surface *textSurface = TTF_RenderText_Blended(font, Text, SDL_Color {128, 128, 128, 0});
				screenDest.x = 0;
				screenDest.y = textSurface->h * 0;
				screenDest.w = textSurface->w;
				screenDest.h = textSurface->h;
				thisInputTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				SDL_FreeSurface(textSurface);
				SDL_RenderCopy(renderer, thisInputTexture, NULL, &screenDest);
			}

				//render delta input
			{
				sprintf_s(Text, textLen, "Delta   : {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}",
					input.arrowUp.isDown != input.arrowUp.wasDown,
					input.arrowDown.isDown != input.arrowDown.wasDown,
					input.arrowLeft.isDown != input.arrowLeft.wasDown,
					input.arrowRight.isDown != input.arrowRight.wasDown,
					input.jump.isDown != input.jump.wasDown);

				if(thisInputTexture != NULL) {
					SDL_DestroyTexture(thisInputTexture);
					thisInputTexture = NULL;
				}

				SDL_Surface *textSurface = TTF_RenderText_Blended(font, Text, SDL_Color {128, 128, 128, 0});
				screenDest.x = 0;
				screenDest.y = textSurface->h * 1;
				screenDest.w = textSurface->w;
				screenDest.h = textSurface->h;
				thisInputTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				SDL_FreeSurface(textSurface);
				SDL_RenderCopy(renderer, thisInputTexture, NULL, &screenDest);
			}

			//render mouse position text
			{
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);
				sprintf_s(Text, textLen, "WorldMouse: {%f,%f}  ScreenMouse: {%d,%d}",
					mouseX / screenProps.pixPerHorizontalMeter,
					mouseY / screenProps.pixPerVerticalMeter,
					mouseX, mouseY);

				if(thisInputTexture != NULL) {
					SDL_DestroyTexture(thisInputTexture);
					thisInputTexture = NULL;
				}

				SDL_Surface *textSurface = TTF_RenderText_Blended(font, Text, SDL_Color {128, 128, 128, 0});
				screenDest.x = 0;
				screenDest.y = textSurface->h * 2;
				screenDest.w = textSurface->w;
				screenDest.h = textSurface->h;
				thisInputTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
				SDL_FreeSurface(textSurface);
				SDL_RenderCopy(renderer, thisInputTexture, NULL, &screenDest);
			}
		}

		// display screen
		SDL_RenderPresent(renderer);

		// sleep
		SDL_Delay(15);

		// post-frame timing
		msLastFrame = msThisFrame;
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
