#include <iostream>
#include <SDL.h>

inline void LogError() {
	SDL_Log(SDL_GetError());
}

struct WorldRect {
	float x;
	float y;
	float w;
	float h;
};

void worldRectToRenderRect(WorldRect &wRect, SDL_Rect &rRect, float pixPerHorizontalMeter, float pixPerVerticalMeter, int ScreenHeight) {
	rRect.w = (int)(wRect.w * pixPerHorizontalMeter);
	rRect.h = (int)(wRect.h * pixPerVerticalMeter);
	rRect.x = (int)(wRect.x * pixPerHorizontalMeter);
	rRect.y = ScreenHeight - ((int)(wRect.y * pixPerVerticalMeter) + rRect.h);
}

int main(int argc, char *argv[]) {

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		LogError();
		SDL_Quit();
		return 1;
	}

	int ScreenHeight = 720;
	int ScreenWidth = 1280;

	SDL_Window *window = SDL_CreateWindow("2D RPG", 
										  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
										  ScreenWidth, ScreenHeight, 
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

	Uint32 msLastFrame = SDL_GetTicks(); // ms
	Uint32 msThisFrame = msLastFrame;    // ms
	float dt = 0.0f; // seconds

	float pixPerVerticalMeter = ScreenHeight / 10.0f; // pixels per meter
	float pixPerHorizontalMeter = ScreenWidth / 10.0f; // pixels per meter
	float moveSpeed = 2.68224f; // meters per second
	float gravity = -9.8f; // meters per second per second
	float jumpSpeed = 9.0f; // meters per second

	WorldRect player = {};
	player.x = 0.00f; // meters
	player.y = 5.00f; // meters
	player.w = 0.40f; // meters
	player.h = 1.75f; // meters
	float xVel = 0.0f;
	float yVel = 0.0f;

	bool abovePlatform = false;
	bool inPlatform = false;

	WorldRect platform = {};
	platform.x = 4.0f; // meters
	platform.y = 2.0f; // meters
	platform.w = 2.0f; // meters
	platform.h = 1.0f; // meters

	int xDir = 0; // move direction
	int yDir = 0; // move direction

	bool lastAbovePlatform = false;
	bool onSolidGround = false;
	

	bool isRunning = true;
	while(isRunning) {

		// timing
		msThisFrame = SDL_GetTicks();
		dt = (msThisFrame - msLastFrame) / 1000.0f;
		lastAbovePlatform = abovePlatform;

		// input processing
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch(e.type) {
			case SDL_EventType::SDL_QUIT:
				isRunning = false;
				break;

			case SDL_EventType::SDL_WINDOWEVENT:
				if(e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					SDL_Log("Window resized to (%d,%d)", e.window.data1, e.window.data2);
					ScreenWidth = e.window.data1;
					ScreenHeight = e.window.data2;
					pixPerHorizontalMeter = ScreenWidth / 10.0f;
					pixPerVerticalMeter = ScreenHeight / 10.0f;
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
					xDir++;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					xDir--;
					break;
				case SDL_Scancode::SDL_SCANCODE_W:
					yDir++;
					break;
				case SDL_Scancode::SDL_SCANCODE_S:
					onSolidGround = false;
					yVel = -jumpSpeed/4;
					yDir--;
					break;
				case SDL_Scancode::SDL_SCANCODE_SPACE:
					onSolidGround = false;
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
					xDir--;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					xDir++;
					break;
				case SDL_Scancode::SDL_SCANCODE_W:
					yDir--;
					break;
				case SDL_Scancode::SDL_SCANCODE_S:
					yDir++;
					break;
				}
				break;
			}
		}

		//update game logic
		player.x += xDir * moveSpeed * dt;
		if(yDir) {
			//yVel = yDir * moveSpeed;
		}

		// if player walked off platform
		if(onSolidGround && !(player.x + player.w > platform.x &&
			player.x < platform.x + platform.w)) {
			onSolidGround = false;
		}

		// if player is not on solid ground
		if(!onSolidGround) {
			yVel += gravity * dt;
			player.y += yVel * dt;
		}

		if(player.y <= 0) {
			player.y = 0;
			yVel = 0;
			onSolidGround = true;
		} else {
			abovePlatform = player.y > platform.y + platform.h &&
				player.x + player.w > platform.x &&
				player.x < platform.x + platform.w;
			inPlatform = player.y > platform.y && 
				player.y < platform.y + platform.h && 
				player.x + player.w > platform.x && 
				player.x < platform.x + platform.w;
			if(lastAbovePlatform && inPlatform) {
				player.y = platform.y + platform.h;
				yVel = 0;
				onSolidGround = true;
			}
		}

		// clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw rect
		SDL_Rect screenDest;

		//draw platform
		worldRectToRenderRect(platform, screenDest, pixPerHorizontalMeter, pixPerVerticalMeter, ScreenHeight);
		SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
		SDL_RenderFillRect(renderer, &screenDest);

		//draw player
		worldRectToRenderRect(player, screenDest, pixPerHorizontalMeter, pixPerVerticalMeter, ScreenHeight);
		if(inPlatform && !onSolidGround) {
			SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		}
		SDL_RenderFillRect(renderer, &screenDest);

		// display screen
		SDL_RenderPresent(renderer);

		// sleep
		SDL_Delay(15);

		msLastFrame = msThisFrame;
		lastAbovePlatform = abovePlatform;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}