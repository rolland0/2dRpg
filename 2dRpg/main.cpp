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

struct Platform {
	WorldRect rect = {};
	bool abovePlatform = false;
	bool underPlatform = false;
	bool wasAbovePlatform = false;
	bool onPlatform = false;
};

enum PlayerState {
	inAir,
	onTransientGround,
	onSolidGround,
	onLadder
};

int main(int argc, char *argv[]) {

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
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

	Uint32 msLastFrame = SDL_GetTicks(); // ms
	Uint32 msThisFrame = msLastFrame;    // ms
	float dt = 0.0f; // seconds

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

	PlayerState state = PlayerState::inAir;

	WorldRect ladder = {};
	ladder.x = 2.0f;
	ladder.y = 0.0f;
	ladder.w = 0.5f;
	ladder.h = 6.0f;
	bool onThisLadder = false;

	bool dropDown = false;

	const int numPlatforms = 5;
	Platform platform[numPlatforms];
	for(int i = 0; i < 4; i++) {
		platform[i].rect.x = 4.0f + i * 1.3f; // meters
		platform[i].rect.y = 2.0f; // meters
		platform[i].rect.w = 1.0f; // meters
		platform[i].rect.h = 0.5f; // meters
	}
	platform[4].rect.x = ladder.x - 0.2f;
	platform[4].rect.y = ladder.h - 0.5f;
	platform[4].rect.w = ladder.w + 0.4f;
	platform[4].rect.h = 0.5f;

	bool newX = false;
	int xInput = 0; // move direction
	bool newY = false;
	int yInput = 0; // move direction
	bool spacePressed;

	bool isRunning = true;
	while(isRunning) {

		// timing
		msThisFrame = SDL_GetTicks();
		dt = (msThisFrame - msLastFrame) / 1000.0f;

		// input processing
		newX = false;
		newY = false;
		spacePressed = false;
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
					if(xInput <= 0) {
						newX = true;
					}
					xInput++;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					if(xInput >= 0) {
						newX = true;
					}
					xInput--;
					break;
				case SDL_Scancode::SDL_SCANCODE_W:
					if(yInput <= 0) {
						newY = true;
					}
					yInput++;
					break;
				case SDL_Scancode::SDL_SCANCODE_S:
					if(yInput >= 0) {
						newY = true;
					}
					yInput--;
					break;
				case SDL_Scancode::SDL_SCANCODE_SPACE:
					spacePressed = true;
					break;
				}
				break;

			case SDL_EventType::SDL_KEYUP:
				if(e.key.repeat) {
					break;
				}

				switch(e.key.keysym.scancode) {
				case SDL_Scancode::SDL_SCANCODE_D:
					xInput--;
					break;
				case SDL_Scancode::SDL_SCANCODE_A:
					xInput++;
					break;
				case SDL_Scancode::SDL_SCANCODE_W:
					yInput--;
					break;
				case SDL_Scancode::SDL_SCANCODE_S:
					yInput++;
					break;
				}
				break;
			}
		}

		// process input
		if(state != PlayerState::onLadder) {
			if(yInput > 0) {
				if(isBelowTop(player, ladder) && xOverlap(player, ladder)) {
					// attach to ladder
					state = PlayerState::onLadder;
					player.x = ladder.x + (ladder.w - player.w) / 2;
				}
			}

			if(yInput < 0) {
				if(isBelowTop(player, ladder) && xOverlap(player, ladder)) {
					// attach to ladder
					state = PlayerState::onLadder;
					player.x = ladder.x + (ladder.w - player.w) / 2;
				}
			}
		}

		if(state == PlayerState::onSolidGround || state == PlayerState::onTransientGround) {
			xVel = xInput * moveSpeed;
			if(spacePressed) {
				// jump
				state = PlayerState::inAir;
				yVel += jumpSpeed;
			}
		}

		if(state == PlayerState::onTransientGround) {
			if(yInput < 0 && newY) {
				// drop down
				dropDown = true;
				state = inAir;
				yVel = -jumpSpeed / 4;
			}
		}

		// ladder movement
		if(state == PlayerState::onLadder) {
			xVel = 0.0f;
			yVel = yInput * moveSpeed;

			if(spacePressed) {
				if(xInput != 0) {
					// jump
					state = PlayerState::inAir;
					yVel += jumpSpeed;
				} else {
					// drop down
					dropDown = true;
					state = inAir;
					yVel = -jumpSpeed / 4;
				}
			}
		}

		// if player is not on solid ground
		if(state == PlayerState::inAir) {
			xVel = (float)xInput / 2.0f * moveSpeed;
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
		if(state == PlayerState::onTransientGround) {
			state = PlayerState::inAir;
			for(int i = 0; i < numPlatforms; i++) {
				if(platform[i].onPlatform) {
					if(!xOverlap(player, platform[i].rect)) {
						platform[i].onPlatform = false;
					} else {
						state = PlayerState::onTransientGround;
					}
				}
			}
		}

		// move y
		player.y += yVel * dt;
		if(player.y <= 0) {
			player.y = 0;
			yVel = 0;
			state = PlayerState::onSolidGround;
		} else if(player.y + player.h > WorldHeight) {
			player.y = WorldHeight - player.h;
			yVel = 0;
			state = PlayerState::inAir;
		}

		if(state == PlayerState::onLadder) {
			if(isAbove(player, ladder)) {
				state = PlayerState::inAir;
				player.y = ladder.y + ladder.h;
				yVel = 0;
			}
		}

		for(int i = 0; i < numPlatforms; i++) {
			if(dropDown) {
				platform[i].onPlatform = false;
				platform[i].abovePlatform = false;
			} else {
				platform[i].abovePlatform = isAbove(player, platform[i].rect) && xOverlap(player, platform[i].rect);
				platform[i].underPlatform = isBelowTop(player, platform[i].rect) && xOverlap(player, platform[i].rect);
				if(platform[i].wasAbovePlatform && platform[i].underPlatform) {
					player.y = platform[i].rect.y + platform[i].rect.h;
					yVel = 0;
					platform[i].onPlatform = true;
					state = PlayerState::onTransientGround;
				}
			}
			platform[i].wasAbovePlatform = platform[i].abovePlatform;
		}

		// clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// draw rect
		SDL_Rect screenDest;

		//draw platform
		for(int i = 0; i < numPlatforms; i++) {
			worldRectToRenderRect(platform[i].rect, screenDest, screenProps);
			SDL_SetRenderDrawColor(renderer, (i + 1 % 2) * 128, 0, (i % 2) * 128, 255);
			SDL_RenderFillRect(renderer, &screenDest);
		}

		//draw ladder
		worldRectToRenderRect(ladder, screenDest, screenProps);
		SDL_SetRenderDrawColor(renderer, 128, 64, 64, 255);
		SDL_RenderFillRect(renderer, &screenDest);

		//draw player
		worldRectToRenderRect(player, screenDest, screenProps);
		if(state == PlayerState::inAir) {
			SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		}
		SDL_RenderFillRect(renderer, &screenDest);

		// display screen
		SDL_RenderPresent(renderer);

		// sleep
		SDL_Delay(15);

		// post-frame timing
		msLastFrame = msThisFrame;

		dropDown = false;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}