#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdio>

inline void LogError() {
	SDL_Log(SDL_GetError());
}

void breakHere() {}

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


struct Platform {
	WorldRect rect = {};
	bool abovePlatform = false;
	bool underPlatform = false;
	bool wasAbovePlatform = false;
	bool onPlatform = false;
};

struct Ladder {
	WorldRect rect = {};
	bool isOccupied = false;
};

enum PlayerState {
	inAir,
	onTransientGround,
	onSolidGround,
	onLadder
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
// input.stick.end{x/Y} get persisted by void changeFrame(Input &input);
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

char *newInputTextTemplate = "NewInput: {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}";
char *oldInputTextTemplate = "Delta   : {Up: %d} {Down: %d} {Left: %d} {Right: %d} {Jump: %d}";
char *TextTemplateMousePos = "WorldMouse: {%f,%f}  ScreenMouse: {%d,%d}";
const int textLen = 100;
char Text[textLen];

SDL_Surface *renderTextBlended(TTF_Font *font, const char *text, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color color = {r, g, b, a};
	return TTF_RenderText_Blended(font, text, color);
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

	TTF_Font *font = TTF_OpenFont("..\\font.ttf", 16);
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
	float xVel = 0.0f;
	float yVel = 0.0f;

	PlayerState state = PlayerState::inAir;


	bool dropDown = false;

	const int numSoloPlatforms = 4;
	const int numLadders = 4;
	const int numPlatforms = numSoloPlatforms + numLadders;

	Platform platform[numPlatforms];
	for(int i = 0; i < numSoloPlatforms; i++) {
		platform[i].rect.x = 4.0f + i * 1.3f; // meters
		platform[i].rect.y = 2.0f; // meters
		platform[i].rect.w = 1.0f; // meters
		platform[i].rect.h = 0.5f; // meters
	}

	Ladder ladder[numLadders];
	for(int i = 0; i < numLadders; i++) {
		ladder[i].rect = {2.0f + i * 2.0f, 0.0f, 0.5f, 6.0f};
		platform[i + numSoloPlatforms].rect.x = ladder[i].rect.x - 0.2f;
		platform[i + numSoloPlatforms].rect.y = ladder[i].rect.h - 0.5f;
		platform[i + numSoloPlatforms].rect.w = ladder[i].rect.w + 0.4f;
		platform[i + numSoloPlatforms].rect.h = 0.5f;
	}

	SDL_Texture *thisInputTexture = NULL;
	SDL_Texture *lastInputTexture = NULL;

	Input input = {};

	bool drawDebug = true;
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

		// process input
		if(state != PlayerState::onLadder) {
			if(input.stick.endY != 0) {
				for(int i = 0; i < numLadders; i++) {
					if(isBelowTop(player, ladder[i].rect) && xOverlap(player, ladder[i].rect)) {
						// attach to ladder
						state = PlayerState::onLadder;
						player.x = ladder[i].rect.x + (ladder[i].rect.w - player.w) / 2;
						ladder[i].isOccupied = true;
						break;
					} else {
						ladder[i].isOccupied = false;
					}
				}
			}
		}

		if(state == PlayerState::onSolidGround || state == PlayerState::onTransientGround) {
			xVel = input.stick.endX * moveSpeed;
			if(input.jump.isDown && !input.jump.wasDown) {
				// jump
				state = PlayerState::inAir;
				yVel += jumpSpeed;
			}
		}

		if(state == PlayerState::onTransientGround) {
			if(input.stick.endY < 0 && input.stick.startY >= 0) {
				// drop down
				dropDown = true;
				state = inAir;
				yVel = -jumpSpeed / 4;
			}
		}

		// ladder movement
		if(state == PlayerState::onLadder) {
			xVel = 0.0f;
			yVel = input.stick.endY * moveSpeed;

			if(input.jump.isDown && !input.jump.wasDown) {
				if(input.stick.endX != 0) {
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

			// if player is now in the air
			if(state == PlayerState::inAir) {

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
			for(int i = 0; i < numLadders; i++) {
				if(isAbove(player, ladder[i].rect)) {
					state = PlayerState::inAir;
					player.y = ladder[i].rect.y + ladder[i].rect.h;
					yVel = 0;
					break;
				}
			}
		}

		for(int i = 0; i < numPlatforms; i++) {
			if(dropDown) {
				platform[i].onPlatform = false;
				platform[i].abovePlatform = false;

			} else {
				platform[i].abovePlatform = isAbove(player, platform[i].rect) && 
											xOverlap(player, platform[i].rect);
				platform[i].underPlatform = isBelowTop(player, platform[i].rect) &&
											xOverlap(player, platform[i].rect);

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

		//draw ladder
		SDL_SetRenderDrawColor(renderer, 128, 64, 64, 255);
		for(int i = 0; i < numLadders; i++) {
			worldRectToRenderRect(ladder[i].rect, screenDest, screenProps);
			SDL_RenderFillRect(renderer, &screenDest);
		}

		//draw platform
		for(int i = 0; i < numPlatforms; i++) {
			worldRectToRenderRect(platform[i].rect, screenDest, screenProps);
			SDL_SetRenderDrawColor(renderer, (i + 1 % 2) * 128, 0, (i % 2) * 128, 255);
			SDL_RenderFillRect(renderer, &screenDest);
		}

		//draw player
		worldRectToRenderRect(player, screenDest, screenProps);
		if(state == PlayerState::inAir) {
			SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
		}
		SDL_RenderFillRect(renderer, &screenDest);

		if(drawDebug) {
			//render new input
			{
				sprintf_s(Text, textLen, newInputTextTemplate,
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
				sprintf_s(Text, textLen, oldInputTextTemplate,
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
				sprintf_s(Text, textLen, TextTemplateMousePos,
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

		dropDown = false;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	return 0;
}