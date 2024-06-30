#include "namedPedAnimal.h"
#include "util.h"
#include "inc/main.h"

#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <cmath>

std::vector<std::string> animalModels = { "A_C_Alligator_01","A_C_Alligator_02","A_C_Alligator_03","A_C_Armadillo_01","A_C_Badger_01","A_C_Bat_01","A_C_Bear_01","A_C_BearBlack_01","A_C_Beaver_01","A_C_BigHornRam_01","A_C_BlueJay_01","A_C_Boar_01","A_C_BoarLegendary_01","A_C_Buck_01","A_C_Buffalo_01","A_C_Buffalo_Tatanka_01","A_C_Bull_01","A_C_CaliforniaCondor_01","A_C_Cardinal_01","A_C_CarolinaParakeet_01","A_C_Cat_01","A_C_CedarWaxwing_01","A_C_Chicken_01","A_C_Chipmunk_01","A_C_Cormorant_01","A_C_Cougar_01","A_C_Cow","A_C_Coyote_01","A_C_Crab_01","A_C_CraneWhooping_01","A_C_Crawfish_01","A_C_Crow_01","A_C_Deer_01","A_C_DogAmericanFoxhound_01","A_C_DogAustralianSheperd_01","A_C_DogBluetickCoonhound_01","A_C_DogCatahoulaCur_01","A_C_DogChesBayRetriever_01","A_C_DogCollie_01","A_C_DogHobo_01","A_C_DogHound_01","A_C_DogHusky_01","A_C_DogLab_01","A_C_DogLion_01","A_C_DogPoodle_01","A_C_DogRufus_01","A_C_DogStreet_01","A_C_Donkey_01","A_C_Duck_01","A_C_EAGLE_01","A_C_Eagle_01","A_C_Egret_01","A_C_Elk_01","A_C_Fox_01","A_C_FrogBull_01","A_C_GilaMonster_01","A_C_Goat_01","A_C_GooseCanada_01","A_C_Hawk_01","A_C_Heron_01","A_C_Iguana_01","A_C_IguanaDesert_01","A_C_Javelina_01","A_C_LionMangy_01","A_C_Loon_01","A_C_Moose_01","A_C_Muskrat_01","A_C_Oriole_01","A_C_Owl_01","A_C_Ox_01","A_C_Panther_01","A_C_Parrot_01","A_C_Pelican_01","A_C_Pheasant_01","A_C_Pig_01","A_C_Pigeon","A_C_Possum_01","A_C_PrairieChicken_01","A_C_Pronghorn_01","A_C_Quail_01","A_C_Rabbit_01","A_C_Raccoon_01","A_C_Rat_01","A_C_Raven_01","A_C_RedFootedBooby_01","A_C_Robin_01","A_C_Rooster_01","A_C_RoseateSpoonbill_01","A_C_Seagull_01","A_C_Sheep_01","A_C_Skunk_01","A_C_Snake_01","A_C_SnakeBlackTailRattle_01","A_C_SnakeFerDeLance_01","A_C_SnakeRedBoa_01","A_C_SnakeRedBoa10ft_01","A_C_SongBird_01","A_C_Sparrow_01","A_C_Squirrel_01","A_C_Toad_01","A_C_Turkey_01","A_C_Turkey_02","A_C_TurkeyWild_01","A_C_TurtleSnapping_01","A_C_Vulture_01","A_C_Wolf","A_C_Wolf_Medium","A_C_Wolf_Small","A_C_Woodpecker_01","A_C_Woodpecker_02" };
std::vector<std::string> animalModelsWihtoutPredators = { "A_C_Armadillo_01","A_C_Badger_01","A_C_Bat_01","A_C_Beaver_01","A_C_BigHornRam_01","A_C_BlueJay_01","A_C_Boar_01","A_C_BoarLegendary_01","A_C_Buck_01","A_C_Buffalo_01","A_C_Buffalo_Tatanka_01","A_C_Bull_01","A_C_CaliforniaCondor_01","A_C_Cardinal_01","A_C_CarolinaParakeet_01","A_C_Cat_01","A_C_CedarWaxwing_01","A_C_Chicken_01","A_C_Chipmunk_01","A_C_Cormorant_01","A_C_Cow","A_C_Crab_01","A_C_CraneWhooping_01","A_C_Crawfish_01","A_C_Crow_01","A_C_Deer_01","A_C_DogAmericanFoxhound_01","A_C_DogAustralianSheperd_01","A_C_DogBluetickCoonhound_01","A_C_DogCatahoulaCur_01","A_C_DogChesBayRetriever_01","A_C_DogCollie_01","A_C_DogHobo_01","A_C_DogHound_01","A_C_DogHusky_01","A_C_DogLab_01","A_C_DogLion_01","A_C_DogPoodle_01","A_C_DogRufus_01","A_C_DogStreet_01","A_C_Donkey_01","A_C_Duck_01","A_C_EAGLE_01","A_C_Eagle_01","A_C_Egret_01","A_C_Elk_01","A_C_Fox_01","A_C_FrogBull_01","A_C_GilaMonster_01","A_C_Goat_01","A_C_GooseCanada_01","A_C_Hawk_01","A_C_Heron_01","A_C_Iguana_01","A_C_IguanaDesert_01","A_C_Javelina_01","A_C_Loon_01","A_C_Moose_01","A_C_Muskrat_01","A_C_Oriole_01","A_C_Owl_01","A_C_Ox_01","A_C_Parrot_01","A_C_Pelican_01","A_C_Pheasant_01","A_C_Pig_01","A_C_Pigeon","A_C_Possum_01","A_C_PrairieChicken_01","A_C_Pronghorn_01","A_C_Quail_01","A_C_Rabbit_01","A_C_Raccoon_01","A_C_Rat_01","A_C_Raven_01","A_C_RedFootedBooby_01","A_C_Robin_01","A_C_Rooster_01","A_C_RoseateSpoonbill_01","A_C_Seagull_01","A_C_Sheep_01","A_C_Skunk_01","A_C_SongBird_01","A_C_Sparrow_01","A_C_Squirrel_01","A_C_Toad_01","A_C_Turkey_01","A_C_Turkey_02","A_C_TurkeyWild_01","A_C_TurtleSnapping_01","A_C_Vulture_01","A_C_Woodpecker_01","A_C_Woodpecker_02"};

NamedPedAnimal::NamedPedAnimal(Ped handle, std::string viewerId, std::string nickname) : NamedPed(handle, viewerId, nickname)
{
	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, false, false);
	TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, true, true);
	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, TRUE);

	TASK::TASK_GO_TO_ENTITY(handle, PLAYER::PLAYER_PED_ID(), -1, 4.0f, 4.0f, 6.0f, 0);
	taskResetTime = MISC::GET_GAME_TIMER() + 5000;

	Hash relationshipGroup;
	PED::ADD_RELATIONSHIP_GROUP("_NAMED_PED", &relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(3, Util::GetHashKey("PLAYER"), relationshipGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(1, relationshipGroup, Util::GetHashKey("PLAYER"));
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(handle, relationshipGroup);
}

bool NamedPedAnimal::ShouldDelete()
{
	if (10000.0f < Game::DistanceSq(PLAYER::PLAYER_PED_ID(), handle))
		return true;

	return NamedPed::ShouldDelete();
}

void NamedPedAnimal::Tick()
{
	NamedPed::Tick();

	if (!ENTITY::DOES_ENTITY_EXIST(handle))
		return;

	auto now = MISC::GET_GAME_TIMER();
	if (2500 < now - taskResetTime)
	{
		auto plPed = PLAYER::PLAYER_PED_ID();

		if (Game::DistanceSq(plPed, handle) < 100)
		{
			TASK::TASK_TURN_PED_TO_FACE_ENTITY(handle, plPed, -1, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			TASK::CLEAR_PED_TASKS(handle, true, true);

			if (ENTITY::_GET_IS_BIRD(handle))
			{
				auto pos = ENTITY::GET_ENTITY_COORDS(handle, true, true);
				auto plPos = ENTITY::GET_ENTITY_COORDS(plPed, true, true);

				auto vx = plPos.x - pos.x;
				auto vy = plPos.y - pos.y;
				auto lenSq = vx * vx + vy * vy;
				auto len = sqrt(lenSq);

				vx /= len;
				vy /= len;

				auto x = plPos.x - vx * 3.0f;
				auto y = plPos.y - vy * 3.0f;
				float z = 0;
				if (!MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, plPos.z, &z, true))
					z = plPos.z;

				TASK::TASK_FLY_TO_COORD(handle, 4.0f, x, y, z, true, true);
			}
			else
			{
				TASK::TASK_GO_TO_ENTITY(handle, plPed, -1, 6.0f, 4.0f, 10.0f, 0);
			}
		}

		taskResetTime = now;
	}
}

NamedPedAnimal::~NamedPedAnimal()
{
	if (ENTITY::DOES_ENTITY_EXIST(handle))
		ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&handle);
}

bool NamedPedAnimal::TryCreate(Game::Redemption* redemption, NamedPed** res)
{
	auto plPed = PLAYER::PLAYER_PED_ID();

	if (PED::IS_PED_IN_ANY_VEHICLE(plPed, true))
		return false;

	if (PED::IS_PED_ON_MOUNT(plPed))
	{
		auto mount = PED::GET_MOUNT(plPed);
		if (3.0f < ENTITY::GET_ENTITY_SPEED(mount))
			return false;
	}

	Vector3 spot = {};
	if (!FindSpawnSpot(&spot))
		return false;

	std::string model;
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::string userInput;
	std::copy_if(redemption->userInput.begin(), redemption->userInput.end(), std::back_inserter(userInput), [](char c) { return std::isalpha(c); });
	if (redemption->extra == "no-predators")
	{
		auto filteredModels = FilterByUserInput(animalModelsWihtoutPredators, userInput);

		if (0 < filteredModels.size())
		{
			std::shuffle(std::begin(filteredModels), std::end(filteredModels), std::default_random_engine(seed));
			model = filteredModels[(MISC::GET_GAME_TIMER() / 10) % filteredModels.size()];
		}
		else
		{
			std::shuffle(std::begin(animalModelsWihtoutPredators), std::end(animalModelsWihtoutPredators), std::default_random_engine(seed));
			model = animalModelsWihtoutPredators[(MISC::GET_GAME_TIMER() / 10) % animalModelsWihtoutPredators.size()];
		}
	}
	else
	{
		auto filteredModels = FilterByUserInput(animalModels, userInput);

		if (0 < filteredModels.size())
		{
			std::shuffle(std::begin(filteredModels), std::end(filteredModels), std::default_random_engine(seed));
			model = filteredModels[(MISC::GET_GAME_TIMER() / 10) % filteredModels.size()];
		}
		else
		{
			std::shuffle(std::begin(animalModels), std::end(animalModels), std::default_random_engine(seed));
			model = animalModels[(MISC::GET_GAME_TIMER() / 10) % animalModels.size()];
		}
	}

	auto hash = MISC::GET_HASH_KEY(model.c_str());

	STREAMING::REQUEST_MODEL(hash, true);
	while (!STREAMING::HAS_MODEL_LOADED(hash)) WAIT(0);

	auto ped = PED::CREATE_PED(hash, spot.x, spot.y, spot.z, 0.0f, false, false, false, false);
	if (!ENTITY::DOES_ENTITY_EXIST(ped))
		return false;

	PED::_SET_RANDOM_OUTFIT_VARIATION(ped, true);
	ENTITY::PLACE_ENTITY_ON_GROUND_PROPERLY(ped, true);

	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(hash);

	*res = new NamedPedAnimal(ped, redemption->userId, redemption->userName);

	Game::ShowNotification(redemption->userName + " looks very cute today");

	return true;
}

std::vector<std::string> NamedPedAnimal::FilterByUserInput(std::vector<std::string>& models, std::string& userInput)
{
	auto toLower = [](std::string str)
		{
			std::string result = str;
			std::transform(result.begin(), result.end(), result.begin(), [](char c) { return std::tolower(c); });
			return result;
		};

	std::vector<std::string> result;
	std::string lowerUserInput = toLower(userInput);
	for (const auto& model : models)
	{
		std::string lowerModel = toLower(model);
		if (lowerModel.find(lowerUserInput) != std::string::npos)
			result.push_back(model);
	}
	return result;
}
