#include <PrecompiledHeader.h>
#include "SimpleProfiler.h"



SimpleProfiler * g_SimpleProfiler = nullptr;

void DrawSystemPerformanceUnits(std::vector<SystemPerformanceUnit>& units)
{
	ImGui::Begin("Performance Info");
	
	std::sort(units.begin(), units.end(), [](SystemPerformanceUnit&a, SystemPerformanceUnit&b) {
		return a.miliseconds > b.miliseconds;
	});
	for (auto &p : units)
	{

		float ms = p.miliseconds / (float)p.iterations;
		ImGui::Text("%s : t=%f ms", p.EventName.c_str(), ms);
	}
	
	//units.clear();
	//ImGui::Text("Delta Time         : %f", info.deltaTime);
	//ImGui::Text("Averaged Delta Time: %f", info.averagedDeltaTime);
	//ImGui::Text("Drawcalls          : %i", info.Drawcalls);
	//ImGui::Text("Simulation Time    : %f", info.SimTime);
	//ImGui::Text("Rendering Time     : %f", info.RenderTime);
	
	ImGui::End();
}
