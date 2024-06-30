#include "game.h"
#include "rewards.h"
#include "util.h"
#include "easing.h"

#include "namedPedAnimal.h"
#include "namedPedCavalry.h"
#include "namedPedCompanion.h"

#include <queue>

#include "inc/enums.h"
#include "inc/natives.h"

namespace Game
{
	std::unordered_map<int, Redemption*> redemptions;

	std::unordered_map<Ped, NamedPed*> peds = {};
	std::unordered_set<std::string> spawnedViewerIds = {};
	std::unordered_map<int, std::queue<Ped>*> pedsQueue = {};

	std::unordered_set<Ped> companions = {};

	std::vector<Ped> pedsToDespawn = {};
	bool shouldDespawnAllPeds = false;

	std::queue<std::string> notificationsQueue = {};
	std::string notification = "";
	int lastNotificationTime = -6666;

	Ped* pedsHandles = new Ped[1024];

	void Process(Redemption* toProcess)
	{
		if (redemptions.contains(toProcess->rewardType))
			delete redemptions[toProcess->rewardType];
		redemptions[toProcess->rewardType] = toProcess;
	}

	const std::vector<const char*> FontList = { "body", "body1", "catalog1", "catalog2", "catalog3", "catalog4", "catalog5", "chalk",
	"Debug_BOLD", "FixedWidthNumbers", "Font5", "gamername", "handwritten", "ledger", "RockstarTAG", "SOCIAL_CLUB_COND_BOLD", "title", "wantedPostersGeneric" };
	void DrawFormattedText(const std::string& text, Font font, int red, int green, int blue, int alpha, Alignment align, int textSize, float posX, float posY)
	{
		const std::string fontStr = FontList[(int)font];

		if (align == Alignment::Right) { posX = 0.0f; }
		if (align == Alignment::Center) { posX = posX * 2.0f - 1.0f; }

		UIDEBUG::_BG_SET_TEXT_COLOR(red, green, blue, alpha);

		const std::string alignStr = (align == Alignment::Left ? "Left" : align == Alignment::Right ? "Right" : "Center");
		const std::string sizeStr = std::to_string(textSize);

		std::string formattedText = "<TEXTFORMAT><P ALIGN='" + alignStr + "'><FONT FACE='$" + fontStr + "' SIZE='" + sizeStr + "'>~s~" + text + "</FONT></P><TEXTFORMAT>";

		UIDEBUG::_BG_DISPLAY_TEXT(MISC::VAR_STRING(10, "LITERAL_STRING", formattedText.c_str()), posX, posY);
	}

	void ShowNotification(std::string text)
	{
		notificationsQueue.push(text);
	}

	float DistanceSq(Entity entityA, Entity entityB)
	{
		auto entityAPos = ENTITY::GET_ENTITY_COORDS(entityA, true, true);
		auto entityBPos = ENTITY::GET_ENTITY_COORDS(entityB, true, true);

		auto vx = entityAPos.x - entityBPos.x;
		auto vy = entityAPos.y - entityBPos.y;
		auto vz = entityAPos.z - entityBPos.z;

		return vx * vx + vy * vy + vz * vz;
	}

	std::unordered_map<Ped, NamedPed*>* GetSpawnedPeds()
	{
		return &peds;
	}
	int GetSpawnedPedsCount()
	{
		return peds.size();
	}

	std::unordered_map<int, Redemption*>* GetPendingRedemptions()
	{
		return &redemptions;
	}

	int CompanionsCount()
	{
		return companions.size();
	}
	bool IsPedCompanion(Ped ped)
	{
		return companions.contains(ped);
	}
	std::unordered_set<Ped> GetCompanions()
	{
		return companions;
	}

	typedef bool (*TryCreate)(Redemption*, NamedPed**);
	TryCreate createFuncs[] = { NamedPedAnimal::TryCreate, NamedPedCavalry::TryCreate, NamedPedCompanion::TryCreate };
	void Tick()
	{
		int now = MISC::GET_GAME_TIMER();
		int notifDiff = now - lastNotificationTime;
		if (notifDiff < 6666)
		{
			auto y = 32.0f / 720.0f;
			if (notifDiff < 300)
				y = Easing::QuadOut(notifDiff / 300.0f) * 64.0f / 720.0f - 32.0f / 720.0f;
			else if (6366 < notifDiff)
				y = Easing::QuadOut(1.0f - (notifDiff - 6366) / 300.0f) * 64.0f / 720.0f - 32.0f / 720.0f;

			GRAPHICS::DRAW_SPRITE("generic_textures", "hud_menu_4a", 0.825f, y, 0.3f, 24.0f / 720.0f, 0.0f, 10, 10, 10, 240, false);
			DrawFormattedText(notification, Font::Body, 244, 244, 244, 244, Alignment::Center, 16, 0.825f, y - 8.0f / 720.0f);
		}
		else if (0 < notificationsQueue.size())
		{
			notification = notificationsQueue.front();
			notificationsQueue.pop();
			lastNotificationTime = now;
		}

		companions.clear();
		auto plPed = PLAYER::PLAYER_PED_ID();
		auto plGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());

		int pedsCount = worldGetAllPeds(pedsHandles, 1024);
		for (int i = 0; i < pedsCount; i++)
		{
			auto ped = pedsHandles[i];
			if (ped == plPed)
				continue;
			if (peds.contains(ped))
				continue;
			if (PED::IS_PED_GROUP_MEMBER(ped, plGroup, false) || PED::GET_RELATIONSHIP_BETWEEN_PEDS(ped, plPed) <= 2)
				companions.insert(ped);
		}

		if (!HUD::IS_RADAR_HIDDEN())
		{
			for (int i = 0; i < 3; i++)
			{
				if (!redemptions.contains(i))
					continue;

				auto redemption = redemptions[i];
				NamedPed* namedPed;
				if (spawnedViewerIds.contains(redemption->userId))
				{
					Rewards::Cancel(redemption);

					delete redemption;
					redemptions.erase(i);
				}
				else if (createFuncs[i](redemption, &namedPed))
				{
					peds[namedPed->GetHandle()] = namedPed;
					if (!pedsQueue.contains(i))
						pedsQueue[i] = new std::queue<Ped>;
					pedsQueue[i]->push(namedPed->GetHandle());
					spawnedViewerIds.insert(redemption->userId);
					Rewards::Fulfill(redemption);

					delete redemption;
					redemptions.erase(i);
				}
			}
		}

		for (auto it = peds.begin(); it != peds.end(); )
		{
			if (it->second->ShouldDelete())
			{
				spawnedViewerIds.erase(it->second->GetViewerId());

				delete it->second;
				it = peds.erase(it);
			}
			else
			{
				it->second->Tick();
				++it;
			}
		}

		for (int i = 0; i <= 2; i++)
		{
			if (!pedsQueue.contains(i))
				continue;
			auto queue = pedsQueue[i];
			if (queue->size() == 0)
				continue;
			auto frontPed = queue->front();
			if (ENTITY::DOES_ENTITY_EXIST(frontPed))
				continue;

			if (peds.contains(frontPed))
			{
				auto namedPed = peds[frontPed];
				spawnedViewerIds.erase(namedPed->GetViewerId());

				delete namedPed;
				peds.erase(frontPed);
			}
			queue->pop();

		}

		for (int i = 0; i <= 2; i++)
		{
			if (!pedsQueue.contains(i))
				continue;

			auto queue = pedsQueue[i];
			while ((i == 2 ? 2 : 4) < queue->size())
			{
				auto ped = queue->front();
				if (peds.contains(ped))
				{
					auto namedPed = peds[ped];
					spawnedViewerIds.erase(namedPed->GetViewerId());

					delete namedPed;
					peds.erase(ped);
				}
				queue->pop();
			}
		}

		if (0 < pedsToDespawn.size())
		{
			for (const auto& ped : pedsToDespawn)
			{
				auto pedRef = ped;
				if (ENTITY::DOES_ENTITY_EXIST(pedRef))
				{
					ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, true, true);
					ENTITY::DELETE_ENTITY(&pedRef);
				}
			}
			pedsToDespawn.clear();
		}

		if (shouldDespawnAllPeds)
		{
			for (const auto& pair : peds)
			{
				auto ped = pair.first;
				if (ENTITY::DOES_ENTITY_EXIST(ped))
				{
					ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, true, true);
					ENTITY::DELETE_ENTITY(&ped);
				}
			}

			shouldDespawnAllPeds = false;
		}
	}

	void DespawnPed(Ped ped)
	{
		pedsToDespawn.push_back(ped);
	}
	void DespawnAllPeds()
	{
		shouldDespawnAllPeds = true;
	}
}
