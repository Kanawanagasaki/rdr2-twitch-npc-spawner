#pragma once

#include "game.h"
#include "namedPed.h"

class NamedPedCavalry : public NamedPed
{
private:
	Ped mount;
	long long taskResetTime = 0;

public:
	NamedPedCavalry(Ped handle, Ped mount, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;

	~NamedPedCavalry() override;

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);
};

