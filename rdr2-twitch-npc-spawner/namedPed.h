#pragma once

#include <string>
#include <set>

#include "inc/natives.h"

class NamedPed
{
protected:
	Ped handle;
	std::string viewerId;
	std::string nickname;
	std::string formattedNickname;
	float modelHeight;

	std::set<std::string> requestedDicts;
	bool LoadAnimDict(std::string dict);
	static bool FindSpawnSpot(Vector3* spot);

public:
	NamedPed(Ped handle, std::string viewerId, std::string nickname);

	std::string GetViewerId() { return viewerId; }

	Ped GetHandle() { return handle; }
	std::string GetNickname() { return nickname; }

	virtual bool ShouldDelete();

	virtual void Tick();

	virtual ~NamedPed();
};
