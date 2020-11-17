
module;
#pragma once
#include <windows.h> 
export module input;


export struct InputMap {
	float Mousewheel{ 0.0f };
	signed short MouseX{ 0 };
	signed short MouseY{ 0 };
	signed short MouseDeltaX{ 0 };
	signed short MouseDeltaY{ 0 };
	float MoveForward{ 0.0f };
	float MoveRight{ 0.0f };
	float MoveUp{ 0.0f };
	bool bShiftDown{ false };
};

export extern InputMap g_InputMap;

InputMap g_InputMap = {};

export struct PlayerInputTag {
	InputMap Input;
	//XMMATRIX Matrix;
};

static bool bHasFocus{ true };

export void InputInfo(InputMap & info);

export InputMap HandleInputEvent(InputMap & Input, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);