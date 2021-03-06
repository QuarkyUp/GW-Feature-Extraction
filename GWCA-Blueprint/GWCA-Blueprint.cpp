#include <iostream>
#include <map>
#include <string>
#include <iterator>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <initializer_list>
#include <Windows.h>

#include <GWCA/GWCA.h>

#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/SkillbarMgr.h>

#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Map.h>
#include <GWCA/GameEntities/Skill.h>

#include <cereal/cereal.hpp>
#include <json.hpp>

#define INIT_CONSOLE(fh) AllocConsole(); SetConsoleTitleA("GWCA++ Debug Console"); freopen_s(&fh,"CONOUT$","w",stdout); freopen_s(&fh, "CONOUT$", "w", stderr);
#define KILL_CONSOLE(fh) fclose(fh); fclose(stdout); fclose(stderr); FreeConsole();
#define NAME_OF( v ) #v

using SkillID = GW::Constants::SkillID;
using Profession = GW::Constants::Profession;
using Skillbar = GW::Skillbar;
using nlohmann::json;

using hr_clock = std::chrono::high_resolution_clock;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

HANDLE main_thread = nullptr;
FILE* console;

float feature_nearestEnemyDistance() {
	GW::AgentArray agents = GW::Agents::GetAgentArray();
	if (!agents.valid()) return -1;

	int closest = -1;
	float distance = GW::Constants::SqrRange::Compass;
	float enemy_distance_nearest = -1;

	for (size_t i = 0; i < agents.size(); ++i) {
		if (agents[i] == nullptr) continue;
		if (agents[i]->Allegiance != 0x3) continue;
		if (agents[i]->GetIsDead()) continue;
		
		GW::Agent* player = GW::Agents::GetPlayer();
		float newDistance = GW::Agents::GetSqrDistance(player->pos, agents[i]->pos);
		if (distance > newDistance) {
			distance = newDistance;
			closest = i;
		}
	}
	if (closest != -1) {
		enemy_distance_nearest = GW::Agents::GetSqrDistance(GW::Agents::GetPlayer()->pos, agents[closest]->pos);
	}

	return enemy_distance_nearest;
}

float feature_playerHealth(GW::Agent*& player) {
	return player->HP;
}

float feature_playerEnergy(GW::Agent*& player) {
	return player->Energy;
}

int feature_skillCanCast(Skillbar skillbar, SkillID skillID) {
	return (skillbar.GetSkillById(skillID).Value().GetRecharge() == 0) ? 1 : 0;
}

std::string feature_class_whichSkillUsed(GW::Agent*& player) {
	std::map<int, std::string> map_skill;
	map_skill.insert(std::make_pair(static_cast<int>(SkillID::Deadly_Paradox), "Deadly_Paradox"));
	map_skill.insert(std::make_pair(static_cast<int>(SkillID::Shadow_Form), "Shadow_Form"));
	map_skill.insert(std::make_pair(static_cast<int>(SkillID::Shroud_of_Distress), "Shroud_of_Distress"));
	map_skill.insert(std::make_pair(static_cast<int>(SkillID::Way_of_Perfection), "Way_of_Perfection"));
	
	std::vector<SkillID> v_skillToScan{ SkillID::Deadly_Paradox, SkillID::Shadow_Form, SkillID::Shroud_of_Distress, SkillID::Way_of_Perfection };

	WORD skillIdUsedWORD = player->Skill;
	SkillID skillUsedID = SkillID::No_Skill;
	if (skillIdUsedWORD != 0) skillUsedID = static_cast<SkillID>(player->Skill);

	auto it = std::find(v_skillToScan.begin(), v_skillToScan.end(), skillUsedID);
	if (it != v_skillToScan.end()) {
		return map_skill.at(static_cast<int>(v_skillToScan.at((it - v_skillToScan.begin()))));
	}
	
	return "None_Skill";
}

int feature_enemyGroupHasCaster() {
	GW::AgentArray agents = GW::Agents::GetAgentArray();
	if (!agents.valid()) return -1;

	int enemyMesmerCount = 0;
	int enemyElementalistCount = 0;
	int enemyNecromancerCount = 0;
	int enemyRitualistCount = 0;

	for (size_t i = 0; i < agents.size(); ++i) {
		if (agents[i] == nullptr) continue;
		if (agents[i]->Allegiance != 0x3) continue;
		if (agents[i]->GetIsDead()) continue;
		if (GW::Agents::GetSqrDistance(GW::Agents::GetPlayer()->pos, agents[i]->pos) > GW::Constants::SqrRange::Spellcast) continue;
		
		if (static_cast<Profession>(agents[i]->Primary) == Profession::Mesmer) enemyMesmerCount++;
		if (static_cast<Profession>(agents[i]->Primary) == Profession::Elementalist) enemyElementalistCount++;
		if (static_cast<Profession>(agents[i]->Primary) == Profession::Necromancer) enemyNecromancerCount++;
		if (static_cast<Profession>(agents[i]->Primary) == Profession::Ritualist) enemyRitualistCount++;
	}

	std::vector<int> enemyCasterCountVector {enemyElementalistCount, enemyMesmerCount, enemyNecromancerCount, enemyRitualistCount};
	if (std::find(enemyCasterCountVector.begin(), enemyCasterCountVector.end(), 0) == enemyCasterCountVector.end()) {
		return 1;
	}

	return 0;
}

int main(HMODULE hModule) {
	Sleep(1000);
	//GW::Agent* player = GW::Agents::GetPlayer();
	//printf("Player: %f %f\n", player->pos.x, player->pos.y);
	//GW::Map::Travel(GW::Constants::MapID::Great_Temple_of_Balthazar_outpost, GW::Map::GetDistrict());
	//do {
	//	Sleep(1000);
	//	printf("Waiting...");
	//} while (GW::Map::IsMapLoaded());

	
	/*
	for (const auto& skill : skillbar.Skills) {
		auto it_map_skill = map_skill.find(NAME_OF(skill.SkillId));

		if (it_map_skill != map_skill.end()) {
			SkillID skillID = it_map_skill->second;
			auto skill_recharge_time = skillbar.GetSkillById(skillID).Value().GetRecharge();
		}
	}
	*/

	json features_dataset;
	features_dataset = {};

	auto timer_init = hr_clock::now();
	auto timer_backup = hr_clock::now();

	bool run = true;
	while (run) {
		Sleep(100);
		if (GetAsyncKeyState(VK_ESCAPE)) { std::cout << "Closing...\n"; Sleep(1500);  run = !run; }
		if (GW::Map::GetInstanceType() == GW::Constants::InstanceType::Loading || GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost) { continue; }
		
		// Get features
		GW::Agent* player = GW::Agents::GetPlayer();
		Skillbar skillbar = Skillbar::GetPlayerSkillbar();
		float nearestEnemyDistance = feature_nearestEnemyDistance();
		float playerEnergy = feature_playerEnergy(player);
		float playerHP = feature_playerHealth(player);
		int skillDeadlyParadoxCanCast = feature_skillCanCast(skillbar, SkillID::Deadly_Paradox);
		int skillShadowFormCanCast = feature_skillCanCast(skillbar, SkillID::Shadow_Form);
		int enemyGroupHasCaster = feature_enemyGroupHasCaster();
		std::string whichSkillUsed = feature_class_whichSkillUsed(player);
		int skillShroudOfDistress = feature_skillCanCast(skillbar, SkillID::Shroud_of_Distress);
		int skillWayOfPerfection = feature_skillCanCast(skillbar, SkillID::Way_of_Perfection);

		// Store features in JSON
		json feature_element;
		feature_element["nearest_enemy_distance"] = (nearestEnemyDistance != -1) ? nearestEnemyDistance : -1;
		feature_element["skill_player_deadly_paradox_can_cast"] = skillDeadlyParadoxCanCast;
		feature_element["skill_player_shadow_of_form_can_cast"] = skillShadowFormCanCast;
		feature_element["enemy_group_has_caster"] = enemyGroupHasCaster;
		feature_element["class_skill_used"] = whichSkillUsed;
		feature_element["skill_player_shroud_of_distress_can_cast"] = skillShroudOfDistress;
		feature_element["skill_player_way_of_perfection_can_cast"] = skillWayOfPerfection;
		
		features_dataset.push_back(feature_element);

		// Backup JSON every 10 seconds
		int elapsed_init = static_cast<int>(duration_cast<milliseconds>(hr_clock::now() - timer_init).count());
		int elapsed_backup = static_cast<int>(duration_cast<milliseconds>(hr_clock::now() - timer_backup).count());
		if (elapsed_backup >= 10000) {
			std::cout << "Backup..." << std::endl;
			std::ofstream file("split_" + std::to_string((int)(elapsed_init/100)) + "dataset.json");
			file << std::setw(4) << features_dataset << std::endl;
			features_dataset.clear();
			timer_backup = hr_clock::now();
		}
	}

	std::ofstream file("split_last_dataset.json");
	file << std::setw(4) << features_dataset << std::endl;
	features_dataset.clear();

	return 1;
}

DWORD WINAPI init(HMODULE hModule) {
	INIT_CONSOLE(console);
	if (!GW::Initialize()) {
		return FALSE;
	}
	main(hModule);
	GW::Terminate();
	KILL_CONSOLE(console);
	FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);
	return 0;
}

BOOL WINAPI DllMain(_In_ HMODULE _HDllHandle, _In_ DWORD _Reason, _In_opt_ LPVOID _Reserved) {
	if (_Reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(_HDllHandle);

		if (*(DWORD*)0x00DE0000 != NULL) {
			MessageBoxA(0, "Error: Guild Wars already injected!", "GWCA++", 0);
			FreeLibraryAndExitThread(_HDllHandle, EXIT_SUCCESS);
		}
		main_thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)init, _HDllHandle, 0, 0);
	}
	else if (_Reason == DLL_PROCESS_DETACH) {
		if (main_thread) {
			TerminateThread(main_thread, EXIT_SUCCESS);
		}
	}
	return TRUE;
}
