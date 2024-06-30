#pragma once

#include "game.h"
#include "namedPed.h"

class NamedPedAnimal : public NamedPed
{
private:
	long long taskResetTime = 0;

public:
	NamedPedAnimal(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;

	~NamedPedAnimal() override;

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);

	static std::vector<std::string> FilterByUserInput(std::vector<std::string>& names, std::string& userInput);
};

