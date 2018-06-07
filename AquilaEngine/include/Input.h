#pragma once

struct InputMap {
	float Mousewheel{ 0.0f };
	signed short MouseX{ 0 };
	signed short MouseY{ 0 };
	signed short MouseDeltaX{ 0 };
	signed short MouseDeltaY{ 0 };
	float MoveForward{ 0.0f };
	float MoveRight{ 0.0f };
	float MoveUp{ 0.0f };
};
static InputMap g_InputMap;


void InputInfo(InputMap & info);

InputMap HandleInputEvent(InputMap & Input, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);