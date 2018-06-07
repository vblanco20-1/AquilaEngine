#include <PrecompiledHeader.h>
#include <Input.h>
void InputInfo(InputMap & info)
{
	
		//ImGui::initi
		ImGui::Begin("Input Info");

		ImGui::Text("Scroll         : %f", info.Mousewheel);
		ImGui::Text("Mouse X        : %i", info.MouseX);
		ImGui::Text("Mouse Y        : %i", info.MouseY);
		ImGui::Text("Mouse Delta X  : %i", info.MouseDeltaX);
		ImGui::Text("Mouse Delta Y  : %i", info.MouseDeltaY);

		ImGui::End();
	
}

InputMap HandleInputEvent(InputMap & Input, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	signed short MouseX{ Input.MouseX };
	signed short MouseY{ Input.MouseY };

	switch (message)
	{
	case WM_MOUSEWHEEL:
		Input.Mousewheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		break;
		//io.MouseWheel 
	case WM_MOUSEMOVE:
		
		//MouseX = (signed short)(lParam);
		//MouseY = (signed short)(lParam >> 16);
		//
		//GetCursorPos
		//	Input.MouseDeltaX = MouseX - Input.MouseX;
		//	Input.MouseDeltaY = MouseY - Input.MouseY;
		//	SetCursorPos(500, 500);
		
		break;
	}

	Input.MouseX = MouseX;
	Input.MouseY = MouseY;

	

	return Input;
}

