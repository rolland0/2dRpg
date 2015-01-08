#include <iostream>
#include <SDL.h>

inline void LogError() {
	SDL_Log(SDL_GetError());
}

int main(int argc, char *argv[]) {

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		LogError();
		SDL_Quit();
		return 1;
	}

	int ScreenHeight = 720;
	int ScreenWidth = 1280;

	SDL_Window *window = SDL_CreateWindow("2D RPG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, 0);
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

	Uint32 msLastFrame = SDL_GetTicks(); // ms
	Uint32 msThisFrame = msLastFrame; // ms
	float dt = 0.0f; // seconds

	float pixPerVerticalMeter = ScreenHeight / 10.0f; // pixels per meter
	float pixPerHorizontalMeter = ScreenWidth / 10.0f; // pixels per meter

	float playerX = 0; // meters
	float playerY = 5; // meters
	float playerW = 0.4f; // meters
	float playerH = 1.75f; // meters

	int xVel = 0; // move direction
	float moveSpeed = 2.68224f; // meters per second
	float yVel = 0;
	float gravity = -9.8f; // meters per second per second
	float jumpSpeed = 4.0f; // meters per second

	bool isRunning = true;
	while(isRunning) {

		// timing
		msThisFrame = SDL_GetTicks();
		dt = (msThisFrame - msLastFrame) / 1000.0f;

		// input processing
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch(e.type) {
			case SDL_EventType::SDL_QUIT:
				isRunning = false;
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
					xVel++;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					xVel--;
					break;
				case SDL_Scancode::SDL_SCANCODE_SPACE:
					yVel += jumpSpeed;
					break;
				}
				break;

			case SDL_EventType::SDL_KEYUP:
				if(e.key.repeat) {
					break;
				}

				switch(e.key.keysym.scancode) {
				case SDL_Scancode::SDL_SCANCODE_D:
					xVel--;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					xVel++;
					break;
				}
				break;
			}
		}

		//update game logic
		playerX += xVel * moveSpeed * dt;
		playerY += yVel * dt;
		if(playerY > 0) {
			yVel += gravity * dt;
		} else {
			playerY = 0;
			yVel = 0;
		}

		// clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw rect
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_Rect rect = {};
		rect.w = (int)(playerW * pixPerHorizontalMeter);
		rect.h = (int)(playerH * pixPerVerticalMeter);
		rect.x = (int)(playerX * pixPerHorizontalMeter);
		rect.y = ScreenHeight - ((int)(playerY * pixPerVerticalMeter) + rect.h);
		SDL_RenderFillRect(renderer, &rect);

		// display screen
		SDL_RenderPresent(renderer);

		// sleep
		SDL_Delay(15);

		msLastFrame = msThisFrame;
	}

	//SDL_Surface *surface = SDL_CreateRGBSurface(0, 64, 64, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	//if(surface == NULL) {
	//	LogError();
	//	SDL_DestroyRenderer(renderer);
	//	SDL_DestroyWindow(window);
	//	SDL_Quit();
	//	return 1;
	//}

	//SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	//if(texture == NULL) {
	//	LogError();
	//	SDL_FreeSurface(surface);
	//	SDL_DestroyRenderer(renderer);
	//	SDL_DestroyWindow(window);
	//	SDL_Quit();
	//	return 1;
	//}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}