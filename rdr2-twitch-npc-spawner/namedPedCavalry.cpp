#include "namedPedCavalry.h"
#include "util.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <vector>
#include <cmath>

std::vector<std::string> mountsModels = { "A_C_Horse_AmericanPaint_Greyovero","A_C_Horse_AmericanPaint_Overo","A_C_Horse_AmericanPaint_SplashedWhite","A_C_Horse_AmericanPaint_Tobiano","A_C_Horse_AmericanStandardbred_Black","A_C_Horse_AmericanStandardbred_Buckskin","A_C_Horse_AmericanStandardbred_Lightbuckskin","A_C_Horse_AmericanStandardbred_PalominoDapple","A_C_Horse_AmericanStandardbred_SilverTailBuckskin","A_C_Horse_Andalusian_DarkBay","A_C_Horse_Andalusian_Perlino","A_C_Horse_Andalusian_Perlino","A_C_Horse_Andalusian_RoseGray","A_C_Horse_Appaloosa_BlackSnowflake","A_C_Horse_Appaloosa_Blanket","A_C_Horse_Appaloosa_BrownLeopard","A_C_Horse_Appaloosa_FewSpotted_PC","A_C_Horse_Appaloosa_Leopard","A_C_Horse_Appaloosa_LeopardBlanket","A_C_Horse_Arabian_Black","A_C_Horse_Arabian_Grey","A_C_Horse_Arabian_RedChestnut","A_C_Horse_Arabian_RedChestnut_PC" };

NamedPedCavalry::NamedPedCavalry(Ped handle, Ped mount, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	this->mount = mount;

	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, false, false);

	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, TRUE);
	PED::SET_PED_FLEE_ATTRIBUTES(handle, 0, 0);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, TRUE);
	PED::SET_PED_CONFIG_FLAG(handle, 2, FALSE);

	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(mount, false, false);

	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(mount, TRUE);
	PED::SET_PED_FLEE_ATTRIBUTES(mount, 0, 0);
	PED::SET_PED_COMBAT_ATTRIBUTES(mount, 46, TRUE);
	PED::SET_PED_CONFIG_FLAG(mount, 2, FALSE);

	Hash relationshipGroup;
	PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED", &relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(3, Util::GetHashKey("PLAYER"), relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, relationshipGroup, Util::GetHashKey("PLAYER"));
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, relationshipGroup);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(mount, relationshipGroup);
}

bool NamedPedCavalry::ShouldDelete()
{
	return NamedPed::ShouldDelete();
}

void NamedPedCavalry::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	auto now = MISC::GET_GAME_TIMER();
	if (now - taskResetTime < 2500)
		return;
	taskResetTime = now;

	if (!PED::IS_PED_ON_MOUNT(handle))
	{
		if (!ENTITY::DOES_ENTITY_EXIST(mount) || ENTITY::IS_ENTITY_DEAD(mount))
		{
			Vector3 spot = {};
			if (!FindSpawnSpot(&spot))
				return;

			unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
			std::shuffle(std::begin(mountsModels), std::end(mountsModels), std::default_random_engine(seed));
			auto mountModel = mountsModels[(MISC::GET_GAME_TIMER() / 10) % mountsModels.size()];
			auto mountHash = MISC::GET_HASH_KEY(mountModel.c_str());

			STREAMING::REQUEST_MODEL(mountHash, true);
			while (!STREAMING::HAS_MODEL_LOADED(mountHash)) WAIT(0);

			mount = PED::CREATE_PED(mountHash, spot.x, spot.y, spot.z + 1.0f, 0.0f, false, false, false, false);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(mountHash);
			if (!ENTITY::DOES_ENTITY_EXIST(mount))
				return;
			PED::_SET_RANDOM_OUTFIT_VARIATION(mount, false);
		}
		else
		{
			auto distSq = Game::DistanceSq(handle, mount);
			if (100.0f < distSq)
			{
				TASK::TASK_GO_TO_ENTITY(handle, mount, -1, 6.0f, 4.0f, 10.0f, 0);
				TASK::TASK_GO_TO_ENTITY(mount, handle, -1, 6.0f, 1.0f, 10.0f, 0);
			}
			else
			{
				TASK::CLEAR_PED_TASKS(mount, true, true);
				TASK::TASK_MOUNT_ANIMAL(handle, mount, 10000, -1, 4.0f, 0, 0, 0);
			}
		}
	}
	else if (!ENTITY::DOES_ENTITY_EXIST(mount) || ENTITY::IS_ENTITY_DEAD(mount))
	{
		TASK::TASK_DISMOUNT_ANIMAL(handle, 0, 0, 0, 0, 0);
	}
	else
	{
		auto mountPos = ENTITY::GET_ENTITY_COORDS(mount, true, true);

		auto plPed = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_ON_MOUNT(plPed))
		{
			auto followMount = PED::GET_MOUNT(plPed);
			auto speed = ENTITY::GET_ENTITY_SPEED(followMount);
			auto pos = ENTITY::GET_ENTITY_COORDS(followMount, true, true);

			auto vx = pos.x - mountPos.x;
			auto vy = pos.y - mountPos.y;
			auto vLen = sqrt(vx * vx + vy * vy);
			vx /= vLen;
			vy /= vLen;

			TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(mount, followMount, -vx * 3.0f, -vy * 3.0f, 0.0f, max(1.0f, speed), -1, 5.0f, true, false, false, false, false, false);
		}
		else if (PED::IS_PED_IN_ANY_VEHICLE(plPed, false))
		{
			auto followVeh = PED::GET_VEHICLE_PED_IS_IN(plPed, true);
			auto speed = ENTITY::GET_ENTITY_SPEED(followVeh);
			auto pos = ENTITY::GET_ENTITY_COORDS(followVeh, true, true);

			auto vx = pos.x - mountPos.x;
			auto vy = pos.y - mountPos.y;
			auto vLen = sqrt(vx * vx + vy * vy);
			vx /= vLen;
			vy /= vLen;

			TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(mount, followVeh, -vx * 3.0f, -vy * 3.0f, 0.0f, max(1.0f, speed), -1, 5.0f, true, false, false, false, false, false);
		}
	}
}

NamedPedCavalry::~NamedPedCavalry()
{
	if (ENTITY::DOES_ENTITY_EXIST(mount))
	{
		ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&mount);
		TASK::CLEAR_PED_TASKS(mount, true, true);
	}
	if (ENTITY::DOES_ENTITY_EXIST(handle))
	{
		ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&handle);
		TASK::CLEAR_PED_TASKS(handle, true, true);
	}
}

bool NamedPedCavalry::TryCreate(Game::Redemption* redemption, NamedPed** res)
{
	static std::vector<Hash> modelHashes = { 0x157604BC, 0x97990286, 0x1754F82F, 0x1A2459CB, 0x8F03BE01, 0xF21ED93D, 0x77807351 };

	auto plPed = PLAYER::PLAYER_PED_ID();
	if (!PED::IS_PED_ON_MOUNT(plPed) && !PED::IS_PED_IN_ANY_VEHICLE(plPed, false))
		return false;

	Vector3 spot = {};
	if (!FindSpawnSpot(&spot))
		return false;

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

	// Mount
	std::shuffle(std::begin(mountsModels), std::end(mountsModels), std::default_random_engine(seed));
	auto mountModel = mountsModels[(MISC::GET_GAME_TIMER() / 10) % mountsModels.size()];
	auto mountHash = MISC::GET_HASH_KEY(mountModel.c_str());

	STREAMING::REQUEST_MODEL(mountHash, true);
	while (!STREAMING::HAS_MODEL_LOADED(mountHash)) WAIT(0);

	auto mount = PED::CREATE_PED(mountHash, spot.x, spot.y, spot.z + 1.0f, 0.0f, false, false, false, false);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(mountHash);
	if (!ENTITY::DOES_ENTITY_EXIST(mount))
		return false;
	ENTITY::PLACE_ENTITY_ON_GROUND_PROPERLY(mount, true);
	PED::_SET_RANDOM_OUTFIT_VARIATION(mount, false);


	// Ped
	std::shuffle(std::begin(modelHashes), std::end(modelHashes), std::default_random_engine(seed));
	auto pedHash = modelHashes[(MISC::GET_GAME_TIMER() / 10) % modelHashes.size()];

	STREAMING::REQUEST_MODEL(pedHash, true);
	while (!STREAMING::HAS_MODEL_LOADED(pedHash)) WAIT(0);

	auto ped = PED::CREATE_PED(pedHash, spot.x, spot.y, spot.z + 1.0f, 0.0f, false, false, false, false);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedHash);
	if (!ENTITY::DOES_ENTITY_EXIST(ped))
	{
		ENTITY::DELETE_ENTITY(&mount);
		return false;
	}
	ENTITY::PLACE_ENTITY_ON_GROUND_PROPERLY(ped, true);
	PED::_SET_RANDOM_OUTFIT_VARIATION(ped, false);

	// named ped
	*res = new NamedPedCavalry(ped, mount, redemption->userId, redemption->userName);

	// Game::ShowNotification(redemption->userName + " galloped up");
	Game::ShowNotification("Gallops announced " + redemption->userName + "'s presence");

	return true;
}
