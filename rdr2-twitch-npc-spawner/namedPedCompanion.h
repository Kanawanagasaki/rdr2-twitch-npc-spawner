#pragma once

#include "namedPed.h"
#include "game.h"

class NamedPedCompanion : public NamedPed
{
private:
	static Hash GetRelationshipGroup();

	int lastGroupResetTime = 0;
	int lastLeaveVehTime = 0;

public:
	NamedPedCompanion(Ped handle, std::string viewerId, std::string nickname);

	bool ShouldDelete() override;

	void Tick() override;

	~NamedPedCompanion() override;

	static bool TryCreate(Game::Redemption* redemption, NamedPed** res);
};
