#pragma once

struct InputMap {
	float Mousewheel{ 0.0f };
	signed short MouseX{ 0 };
	signed short MouseY{ 0 };
};
static InputMap g_InputMap;


void InputInfo(InputMap & info);

InputMap HandleInputEvent(InputMap & Input, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);