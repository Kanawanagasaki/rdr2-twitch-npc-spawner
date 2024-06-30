#pragma once

#include "game.h"

namespace Rewards
{
	void Tick();
	void Disconnect();
	void TryReconnect();
	void Fulfill(Game::Redemption* redemption);
	void Cancel(Game::Redemption* redemption);
	std::string GetState();
	long long GetLastMessageMs();
	long long GetNextRedemptionPull();
	long long GetLastRedemptionPull();
}
