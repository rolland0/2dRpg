#pragma once

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
inline void changeFrame(Input &input)
{
	input.stick.startX = input.stick.minX = input.stick.maxX = input.stick.endX;
	input.stick.startY = input.stick.minY = input.stick.maxY = input.stick.endY;
	for(int i = 0; i < NumButtons; i++)
	{
		input.buttons[i].wasDown = input.buttons[i].isDown;
	}
}

// fills in analog stick values based on movement key values
// input.stick.end{X/Y} get persisted by function "void changeFrame(Input &input)"
inline void buildAnalogInput(Input &input)
{
	if(!input.isAnalog)
	{
		input.stick.endX = 0;
		input.stick.endX += input.arrowRight.isDown;
		input.stick.endX -= input.arrowLeft.isDown;
		input.stick.endY = 0;
		input.stick.endY += input.arrowUp.isDown;
		input.stick.endY -= input.arrowDown.isDown;
	}
}