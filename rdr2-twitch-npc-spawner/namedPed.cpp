#include "namedPed.h"
#include "game.h"
#include "util.h"

#include <algorithm>

NamedPed::NamedPed(Ped handle, std::string viewerId, std::string nickname)
{
	this->handle = handle;
	this->viewerId = viewerId;
	this->nickname = nickname;

	this->formattedNickname = "<TEXTFORMAT><P ALIGN='Center'><FONT FACE='$body' SIZE='16'>~s~" + nickname + "</FONT></P><TEXTFORMAT>";

	auto model = ENTITY::GET_ENTITY_MODEL(handle);
	Vector3 min;
	Vector3 max;
	MISC::GET_MODEL_DIMENSIONS(model, &min, &max);

	modelHeight = max.z;
}

bool NamedPed::LoadAnimDict(std::string dict)
{
	if (STREAMING::HAS_ANIM_DICT_LOADED(dict.c_str()))
	{
		if (requestedDicts.contains(dict))
			requestedDicts.erase(dict);
		return true;
	}

	if (!requestedDicts.contains(dict))
	{
		STREAMING::REQUEST_ANIM_DICT(dict.c_str());
		requestedDicts.insert(dict);
	}

	return false;
}

bool NamedPed::FindSpawnSpot(Vector3* spot)
{
	Vector3 camPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
	Vector3 camRot = CAM::GET_FINAL_RENDERED_CAM_ROT(2);

	float pitch = camRot.x / 180.0f * 3.14159265f;
	float yaw = (camRot.z + 90.0f) / 180.0f * 3.14159265f;

	auto vx = cos(yaw) * cos(pitch);
	auto vy = sin(yaw) * cos(pitch);

	auto len = sqrt(vx * vx + vy * vy);

	auto nx = vx / len;
	auto ny = vy / len;

	auto plPed = PLAYER::PLAYER_PED_ID();
	auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true, true);
	auto spotX = plPos.x - nx * 10.0f;
	auto spotY = plPos.y - ny * 10.0f;
	auto spotZ = plPos.z + 10.0f;

	float groundZ;
	if (!MISC::GET_GROUND_Z_FOR_3D_COORD(spotX, spotY, spotZ, &groundZ, true))
		return false;
	if (groundZ == 0.0f)
		return false;
	if (7.0f < abs(plPos.z - groundZ))
		return false;

	groundZ += 1.0f;

	auto rayX = plPos.x - nx * 15.0f;
	auto rayY = plPos.y - ny * 15.0f;
	auto hRaycast = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(camPos.x, camPos.y, camPos.z, rayX, rayY, groundZ, 0x1FF, plPed, 7);

	BOOL hits;
	Vector3 hitRes;
	Vector3 surfaceNormal;
	Entity hitEntity;
	if (SHAPETEST::GET_SHAPE_TEST_RESULT(hRaycast, &hits, &hitRes, &surfaceNormal, &hitEntity) == 2 && hits)
		return false;

	spot->x = spotX;
	spot->y = spotY;
	spot->z = groundZ;

	return true;
}

bool NamedPed::ShouldDelete()
{
	return !ENTITY::DOES_ENTITY_EXIST(handle) || ENTITY::IS_ENTITY_DEAD(handle);
}

void NamedPed::Tick()
{
	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	auto cameraPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
	auto pedPos = ENTITY::GET_ENTITY_COORDS(handle, true, true);

	auto vx = cameraPos.x - pedPos.x;
	auto vy = cameraPos.y - pedPos.y;
	auto vz = cameraPos.z - pedPos.z;

	auto distSq = vx * vx + vy * vy + vz * vz;

	if (distSq < 400.0f)
	{
		auto distPercent = std::clamp((400.0f - distSq) / 400.0f, 0.0f, 1.0f);

		float worldZ = pedPos.z + modelHeight + 0.45f - 0.15f * distPercent;
		float screenX;
		float screenY;
		if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(pedPos.x, pedPos.y, worldZ, &screenX, &screenY))
		{
			auto camPos = CAM::GET_GAMEPLAY_CAM_COORD();

			vx = camPos.x - pedPos.x;
			vy = camPos.y - pedPos.y;
			vz = camPos.z - pedPos.z;

			distSq = vx * vx + vy * vy + vz * vz;

			auto opacity = std::clamp((100.0f - (distSq - 300.0f)) / 100.0f, 0.0f, 1.0f);
			auto scale = 0.175f + distPercent * 0.075f;

			UIDEBUG::_BG_SET_TEXT_SCALE(scale, scale);
			UIDEBUG::_BG_SET_TEXT_COLOR(255, 255, 255, std::clamp(static_cast<int>(opacity * 255.0f), 0, 255));
			UIDEBUG::_BG_DISPLAY_TEXT(MISC::VAR_STRING(10, "LITERAL_STRING", formattedNickname.c_str()), screenX * 2.0f - 1.0f, screenY);
		}
	}
}

NamedPed::~NamedPed()
{
	if (ENTITY::DOES_ENTITY_EXIST(handle))
		ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&handle);
}
