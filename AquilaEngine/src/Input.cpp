#include <PrecompiledHeader.h>
#include <Input.h>
void InputInfo(InputMap & info)
{
	
		//ImGui::initi
		ImGui::Begin("Input Info");

		ImGui::Text("Scroll         : %f", info.Mousewheel);
		ImGui::Text("Mouse X        : %i", info.MouseX);
		ImGui::Text("Mouse Y        : %i", info.MouseY);

		ImGui::End();
	
}

InputMap HandleInputEvent(InputMap & Input, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
		Input.Mousewheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		break;
		//io.MouseWheel 
	case WM_MOUSEMOVE:
		Input.MouseX = (signed short)(lParam);
		Input.MouseY = (signed short)(lParam >> 16);
		break;
	}

	return Input;
}

