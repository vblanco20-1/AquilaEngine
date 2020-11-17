#pragma once
export  module appinfo ;
import <imgui/imgui.h>;

export struct ApplicationInfo {

	float deltaTime;
	float averagedDeltaTime;
	int Drawcalls;
	float SimTime;
	float RenderTime;
	int TotalEntities;
	int BoidEntities;

	float _SpawnRate = 1;
	bool bEnableCulling = true;
	bool bEnableBoids = true;
	bool bEnableSimulation = true;
};

export void AppInfoUI(ApplicationInfo& info) {
	ImGui::Begin("Application Info");

	ImGui::Text("Delta Time         : %f", info.deltaTime);
	ImGui::Text("Averaged Delta Time: %f", info.averagedDeltaTime);
	ImGui::Text("Drawcalls          : %i", info.Drawcalls);
	ImGui::Text("Total Entities     : %i", info.TotalEntities);
	ImGui::Text("Boids              : %i", info.BoidEntities);
	ImGui::Text("Simulation Time    : %f", info.SimTime);
	ImGui::Text("Rendering Time     : %f", info.RenderTime);

	ImGui::SliderFloat("Spawn Rate ", &info._SpawnRate, 0.1, 3);
	ImGui::Checkbox("Simulation Enabled", &info.bEnableSimulation);
	ImGui::Checkbox("Culling Enabled", &info.bEnableCulling);
	ImGui::Checkbox("Boids Enabled", &info.bEnableBoids);



	ImGui::End();
}
