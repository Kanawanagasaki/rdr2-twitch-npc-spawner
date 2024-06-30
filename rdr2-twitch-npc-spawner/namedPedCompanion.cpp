#include "namedPedCompanion.h"
#include "util.h"
#include "inc/enums.h"

#include <algorithm>
#include <random>
#include <chrono>

Hash NamedPedCompanion::GetRelationshipGroup()
{
	static Hash relationshipGroup = 0;

	if (relationshipGroup == 0)
	{
		PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED_TYPE_2", &relationshipGroup);
		PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, relationshipGroup, Util::GetHashKey("PLAYER"));
		PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, Util::GetHashKey("PLAYER"), relationshipGroup);
	}
	return relationshipGroup;
}

NamedPedCompanion::NamedPedCompanion(Ped handle, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, GetRelationshipGroup());

	PED::SET_PED_HEARING_RANGE(handle, 9999.f);
	PED::SET_PED_CONFIG_FLAG(handle, 281, true);
	PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(handle, false);

	PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
	PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), false);
	// PED::SET_GROUP_FORMATION(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 1);
	// PED::SET_GROUP_FORMATION_SPACING(PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), 10.0f, 10.0f, 200.0f);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true);
	PED::SET_PED_ACCURACY(handle, 100);
}

bool NamedPedCompanion::ShouldDelete()
{
	return NamedPed::ShouldDelete();
}

void NamedPedCompanion::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto distSq = Game::DistanceSq(plPed, handle);

	if (100000.0f < distSq)
	{
		auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true, true);

		Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
		Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);

		float pitch = camRot.x / 180.0f * 3.14159265f;
		float yaw = (camRot.z + 90.0f) / 180.0f * 3.14159265f;

		auto vx = cos(yaw) * cos(pitch);
		auto vy = sin(yaw) * cos(pitch);

		auto len = sqrt(vx * vx + vy * vy);

		auto nx = vx / len;
		auto ny = vy / len;

		auto spotX = plPos.x - nx * 100.0f;
		auto spotY = plPos.y - ny * 100.0f;
		auto spotZ = PATHFIND::GET_APPROX_FLOOR_FOR_POINT(spotX, spotY) + 1.0f;
		float groundZ;
		if (MISC::GET_GROUND_Z_FOR_3D_COORD(spotX, spotY, spotZ, &groundZ, true) && 0.0f < groundZ)
			spotZ = groundZ;

		Vector3 node;
		if (PATHFIND::GET_NTH_CLOSEST_VEHICLE_NODE(spotX, spotY, spotZ, 1, &node, 1, 3.0f, 0.0f))
		{
			spotX = node.x;
			spotY = node.y;
			spotZ = node.z + 1.0f;

			if (PATHFIND::GET_SAFE_COORD_FOR_PED(spotX, spotY, spotZ, true, &node, 0))
			{
				spotX = node.x;
				spotY = node.y;
				spotZ = node.z + 1.0f;
			}
		}

		PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(handle, spotX, spotY, spotZ, true, true, true);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, true, true);
		PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));
		PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()), false);
		TASK::TASK_GO_TO_ENTITY(handle, PLAYER::PLAYER_PED_ID(), -1, 10.0f, 4.0f, 20.0f, 0);
	}

	if (MISC::GET_GAME_TIMER() - lastGroupResetTime < 5000)
	{
		PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, GetRelationshipGroup());
		PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
		PED::SET_PED_AS_GROUP_MEMBER(handle, PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID()));

		lastGroupResetTime = MISC::GET_GAME_TIMER();
	}

	for (const auto& companion : Game::GetCompanions())
	{
		if (PED::GET_RELATIONSHIP_BETWEEN_PEDS(handle, companion) != 0)
		{
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, GetRelationshipGroup(), PED::GET_PED_RELATIONSHIP_GROUP_HASH(companion));
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(0, PED::GET_PED_RELATIONSHIP_GROUP_HASH(companion), GetRelationshipGroup());
		}
	}

	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
	{
		auto veh = PED::GET_VEHICLE_PED_IS_IN(plPed, true);
		auto vehModel = ENTITY::GET_ENTITY_MODEL(veh);
		auto numSeats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(vehModel);

		auto plGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
		int groupMembersCount = 0;
		for (const auto& companion : Game::GetCompanions())
			if (PED::IS_PED_GROUP_MEMBER(companion, plGroup, false) || PED::GET_RELATIONSHIP_BETWEEN_PEDS(companion, plPed) == 0)
				groupMembersCount++;

		if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
		{
			int seat = -1;
			for (int i = 0; i <= 2; i++)
				if (!VEHICLE::IS_VEHICLE_SEAT_FREE(veh, i) && VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, i) == handle)
					seat = i;

			if (numSeats - 1 <= groupMembersCount || seat < groupMembersCount)
			{
				if (ENTITY::GET_ENTITY_SPEED(veh) < 3.0f)
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 0);
				else
					TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
			}
		}
		else
		{
			auto veh2 = PED::GET_VEHICLE_PED_IS_ENTERING(handle);
			auto seat = PED::GET_SEAT_PED_IS_TRYING_TO_ENTER(handle);
			if (veh2 == veh && ENTITY::DOES_ENTITY_EXIST(veh2) && (seat < groupMembersCount || !VEHICLE::IS_VEHICLE_SEAT_FREE(veh, seat)))
			{
				TASK::CLEAR_PED_TASKS(handle, true, true);
				int trySeat = max(groupMembersCount, seat + 1) % 3;
				if (groupMembersCount <= trySeat && VEHICLE::IS_VEHICLE_SEAT_FREE(veh, trySeat))
					TASK::TASK_ENTER_VEHICLE(handle, veh, -1, trySeat, 4.0f, 1, 0);
			}
		}
	}
	else if (PED::IS_PED_IN_ANY_VEHICLE(handle, true))
	{
		auto veh = PED::GET_VEHICLE_PED_IS_IN(handle, true);

		if (ENTITY::GET_ENTITY_SPEED(veh) < 3.0f)
			TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 0);
		else
			TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
	}
}

NamedPedCompanion::~NamedPedCompanion()
{
	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, true, true);
	WEAPON::REMOVE_ALL_PED_WEAPONS(handle, true, true);
	PED::SET_PED_AS_GROUP_MEMBER(handle, 0);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, 0);

	PED::SET_PED_CONFIG_FLAG(handle, 281, false);
	PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(handle, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 5, false);
	PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, false);

	if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
		TASK::TASK_LEAVE_ANY_VEHICLE(handle, 0, 4160);
}

bool NamedPedCompanion::TryCreate(Game::Redemption* redemption, NamedPed** res)
{
	static std::vector<Hash> modelHashes = { 0x157604BC, 0x97990286, 0x1754F82F, 0x1A2459CB, 0x8F03BE01, 0xF21ED93D, 0x77807351 };

	auto plPed = PLAYER::PLAYER_PED_ID();
	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, false))
	{
		auto veh = PED::GET_VEHICLE_PED_IS_IN(plPed, false);
		auto vehPos = ENTITY::GET_ENTITY_COORDS(veh, true, true);

		for (int i = 0; i <= 2; i++)
		{
			if (!VEHICLE::IS_VEHICLE_SEAT_FREE(veh, i))
				continue;

			unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
			std::shuffle(std::begin(modelHashes), std::end(modelHashes), std::default_random_engine(seed));
			auto pedHash = modelHashes[(MISC::GET_GAME_TIMER() / 10) % modelHashes.size()];

			STREAMING::REQUEST_MODEL(pedHash, true);
			while (!STREAMING::HAS_MODEL_LOADED(pedHash)) WAIT(0);

			auto ped = PED::CREATE_PED(pedHash, vehPos.x, vehPos.y, vehPos.z + 1.0f, 0.0f, false, false, false, false);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedHash);
			if (!ENTITY::DOES_ENTITY_EXIST(ped))
				return false;
			ENTITY::PLACE_ENTITY_ON_GROUND_PROPERLY(ped, true);
			PED::_SET_RANDOM_OUTFIT_VARIATION(ped, false);

			PED::SET_PED_INTO_VEHICLE(ped, veh, i);
			*res = new NamedPedCompanion(ped, redemption->userId, redemption->userName);

			Game::ShowNotification(redemption->userName + " ready to fight");

			return true;
		}

		return false;
	}
	else
	{
		Vector3 spot = {};
		if (!FindSpawnSpot(&spot))
			return false;

		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::shuffle(std::begin(modelHashes), std::end(modelHashes), std::default_random_engine(seed));
		auto pedHash = modelHashes[(MISC::GET_GAME_TIMER() / 10) % modelHashes.size()];

		STREAMING::REQUEST_MODEL(pedHash, true);
		while (!STREAMING::HAS_MODEL_LOADED(pedHash)) WAIT(0);

		auto ped = PED::CREATE_PED(pedHash, spot.x, spot.y, spot.z + 1.0f, 0.0f, false, false, false, false);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pedHash);
		if (!ENTITY::DOES_ENTITY_EXIST(ped))
			return false;
		ENTITY::PLACE_ENTITY_ON_GROUND_PROPERLY(ped, true);
		PED::_SET_RANDOM_OUTFIT_VARIATION(ped, false);

		*res = new NamedPedCompanion(ped, redemption->userId, redemption->userName);

		Game::ShowNotification(redemption->userName + " ready to fight");

		return true;
	}
}
