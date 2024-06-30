#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "namedPed.h"
#include "inc/types.h"

namespace Game
{
	struct Redemption
	{
		std::string id;
		std::string userId;
		std::string userLogin;
		std::string userName;
		std::string userInput;
		int rewardType;
		std::string extra;
	};

	void Process(Redemption* redemption);
	void ShowNotification(std::string text);
	float DistanceSq(Entity entityA, Entity entityB);
	std::unordered_map<Ped, NamedPed*>* GetSpawnedPeds();
	int GetSpawnedPedsCount();
	std::unordered_map<int, Redemption*>* GetPendingRedemptions();
	int CompanionsCount();
	bool IsPedCompanion(Ped ped);
	std::unordered_set<Ped> GetCompanions();
	void Tick();
	void DespawnPed(Ped ped);
	void DespawnAllPeds();
}
