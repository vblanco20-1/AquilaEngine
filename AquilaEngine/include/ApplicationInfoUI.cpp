#include "ApplicationInfoUI.h"



void AppInfoUI(ApplicationInfo & info)
{
	ImGui::Begin("Application Info");

	ImGui::Text("Delta Time         : %f", info.deltaTime);
	ImGui::Text("Averaged Delta Time: %f", info.averagedDeltaTime);
	ImGui::Text("Drawcalls          : %i", info.Drawcalls);
	ImGui::Text("Total Entities     : %i", info.TotalEntities);
	ImGui::Text("Boids              : %i", info.BoidEntities);
	ImGui::Text("Simulation Time    : %f", info.SimTime);
	ImGui::Text("Rendering Time     : %f", info.RenderTime);

	ImGui::End();
}
