#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
using glm::vec2;

#include "mathUtil.h"
#include "input.h"
#include "worldRect.h"
#include "display.h"
#include "tiles.h"

inline void LogError() {
	SDL_Log(SDL_GetError());
}

void breakHere() {}

const int textLen = 100;
char Text[textLen];

SDL_Surface *renderTextBlended(TTF_Font *font, const char *text, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color color = {r, g, b, a};
	return TTF_RenderText_Blended(font, text, color);
}

void printText(SDL_Texture **texture, SDL_Renderer *renderer, TTF_Font *font, int lineNum, char *fmt, ...)
{
	va_list argList;
	va_start(argList, fmt);
	vsprintf_s(Text, textLen, fmt, argList);
	va_end(argList);

	if(*texture != NULL) {
		SDL_DestroyTexture(*texture);
		*texture = NULL;
	}
	SDL_Surface *textSurface = TTF_RenderText_Blended(font, Text, SDL_Color {128, 128, 128, 0});
	SDL_Rect screenDest;
	screenDest.x = 0;
	screenDest.y = textSurface->h * lineNum;
	screenDest.w = textSurface->w;
	screenDest.h = textSurface->h;
	*texture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);
	SDL_RenderCopy(renderer, *texture, NULL, &screenDest);
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

	if(IMG_Init(IMG_InitFlags::IMG_INIT_PNG) == 0) {
		LogError();
		SDL_Quit();
		return 1;
	}

	ScreenProperties screenProps = {};
	screenProps.screenHeight = 720;
	screenProps.screenWidth = 1280;
	float aspectRatio = (float)screenProps.screenWidth / (float)screenProps.screenHeight;
	float pxPerTile = 16.0f;

	float WorldHeight = 15; 
	float WorldWidth = WorldHeight * aspectRatio;


	screenProps.pixPerVerticalMeter = screenProps.screenHeight / WorldHeight; // pixels per meter
	screenProps.pixPerHorizontalMeter = screenProps.screenWidth / WorldWidth; // pixels per meter

	SDL_Window *window = SDL_CreateWindow("2D RPG",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		screenProps.screenWidth, screenProps.screenHeight,
		SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WindowFlags::SDL_WINDOW_OPENGL );
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

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

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
	float jumpSpeed = 6.0f; // meters per second

	WorldRect player = {};
	player.x = 0.00f; // meters
	player.y = 5.00f; // meters
	player.w = 0.40f; // meters
	player.h = 1.75f; // meters

	//vec2 vel(0.0f, 0.0f);

	float xVel = 0.0f;
	float yVel = 0.0f;

	TileMap map = {};
	readTileMapText(map, "..\\res\\TileMap.txt", WorldWidth, WorldHeight);

	SDL_Texture *thisInputTexture = NULL;
	SDL_Texture *lastInputTexture = NULL;

	Input input = {};

	OccupiedTiles occupiedTiles;

	SDL_Surface *tiles = IMG_Load("..\\res\\grotto_escape_pack\\graphics\\tiles.png");
	if(tiles == 0) {
		SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Failed to load tiles.");
	}

	SDL_Texture *tilesTexture = SDL_CreateTextureFromSurface(renderer, tiles);

	SDL_Surface *mapSurface = SDL_CreateRGBSurface(
		0, (int)(map.Width*16), (int)(map.Height*16),
		32, 0xFF000000, 0x00FF0000, 0X0000FF00, 0X000000FF);

	int tilesPerRow = 8;
	int tilesPerColumn = 5;

	SDL_Rect source;
	source.w = source.h = 16;
	source.x = source.y = 0;

	SDL_Rect dest;
	dest.w = (int)screenProps.pixPerHorizontalMeter;
	dest.h = (int)screenProps.pixPerVerticalMeter;
	dest.x = (int)dest.y = 0;

	for(int y = 0; y < map.Height; y++) {
		for(int x = 0; x < map.Width; x++) {
			source.x = (((int)map.tiles[y][x].type) % tilesPerRow) * 16;
			source.y = (((int)map.tiles[y][x].type) / tilesPerRow) * 16;
			dest.x = x * dest.w;
			dest.y = y * dest.h;
			SDL_BlitSurface(tiles, &source, mapSurface, &dest);
		}
	}

	SDL_Texture *mapTexture = SDL_CreateTextureFromSurface(renderer, mapSurface);

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

		// move player
		WorldRect playerPosCopy = player;

		xVel = (float)(input.arrowRight.isDown - input.arrowLeft.isDown) * moveSpeed;
		yVel = (float)(input.arrowUp.isDown - input.arrowDown.isDown) * moveSpeed;

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
		if(player.y < 0)
		{
			player.y = 0;
			yVel = 0;
		} else if(player.y + player.h > WorldHeight)
		{
			player.y = WorldHeight - player.h;
			yVel = 0;
		}


		// check current tile collisions
		int numCollidedTiles = 0;
		int minTileX = (int)(player.x / map.tileWidth);// -1;
		int maxTileX = (int)((player.x + player.w) / map.tileWidth) + 1;
		int minTileY = (int)(player.y / map.tileHeight);// -1;
		int maxTileY = (int)((player.y + player.h) / map.tileHeight) + 1;

		maxTileX = min(maxTileX, map.Width);
		maxTileY = min(maxTileY, map.Height);
		minTileX = max(minTileX, 0);
		minTileY = max(minTileY, 0);

		for(int tileY = minTileY; tileY < maxTileY; tileY++)
		{
			for(int tileX = minTileX; tileX < maxTileX; tileX++)
			{
				Tile *tile = &map.tiles[tileY][tileX];
				WorldRect wRect = {tile->xPos, tile->yPos, tile->common->hitboxWidth, tile->common->hitboxHeight};
			}
		}
			WorldRect collideRect = {(float)minTileX, (float)minTileY, (float)maxTileX - minTileX, (float)maxTileY - minTileY};
			collideRect.x *= map.tileWidth;
			collideRect.y *= map.tileHeight;
			collideRect.w *= map.tileWidth;
			collideRect.h *= map.tileHeight;

// Rendering

		// clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw rect
		SDL_Rect screenDest;

		//tile map
		SDL_RenderCopy(renderer, mapTexture, NULL, NULL);
		WorldRect worldDest = {};
		//for(int y = 0; y < TileMap::Height; y++) {
		//	for(int x = 0; x < TileMap::Width; x++) {
		//		worldDest = {
		//			map.tiles[y][x].xPos,
		//			map.tiles[y][x].yPos,
		//			map.tiles[y][x].common->hitboxWidth,
		//			map.tiles[y][x].common->hitboxHeight
		//		};
		//		Uint8 value = map.tiles[y][x].type;
		//		if(value > 0) {
		//			SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
		//			if(value == 1) {
		//				SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		//			} else if(value == 2) {
		//				SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
		//			} else if(value == 3) {
		//				SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
		//			}
		//			worldRectToRenderRect(worldDest, screenDest, screenProps);
		//			SDL_RenderFillRect(renderer, &screenDest);
		//		}
		//	}
		//}

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

		// draw collide rect
		worldRectToRenderRect(collideRect, screenDest, screenProps);
		SDL_SetRenderDrawColor(renderer, 128, 64, 0, 64);
		SDL_RenderFillRect(renderer, &screenDest);

			//render new input
			printText(&thisInputTexture, renderer, font, 0, 
				"NewInput: {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}",
				input.arrowUp.isDown,
				input.arrowDown.isDown,
				input.arrowLeft.isDown,
				input.arrowRight.isDown,
				input.jump.isDown);


			//render delta input
			printText(&thisInputTexture, renderer, font, 1,
				"Delta   : {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}",
				input.arrowUp.isDown != input.arrowUp.wasDown,
				input.arrowDown.isDown != input.arrowDown.wasDown,
				input.arrowLeft.isDown != input.arrowLeft.wasDown,
				input.arrowRight.isDown != input.arrowRight.wasDown,
				input.jump.isDown != input.jump.wasDown);

			//render mouse position text
			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);
			printText(&thisInputTexture, renderer, font, 2,
				"WorldMouse: {%f,%f}  ScreenMouse: {%d,%d}",
				mouseX / screenProps.pixPerHorizontalMeter,
				mouseY / screenProps.pixPerVerticalMeter,
				mouseX, mouseY);

			//render player pos
			printText(&thisInputTexture, renderer, font, 3,
				"PlayerPos: {%f, %f} PlayerVel: {%f, %f}", 
				player.x, player.y, xVel, yVel);

			//render player state
			char *target = "PlayerState: Unknown State";
			
			printText(&thisInputTexture, renderer, font, 4, target);

		} // if(drawDebug)

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
	IMG_Quit();
	SDL_Quit();
	return 0;
}
