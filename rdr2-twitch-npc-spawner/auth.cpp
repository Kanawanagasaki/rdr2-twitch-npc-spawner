#include "auth.h"
#include "game.h"
#include "rewards.h"
#include "util.h"

#include <cstdint>
#include <fstream>
#include <algorithm>

#define _NO_ASYNCRTIMP
#include "inc/cpprest/http_client.h"

using namespace web::http;
using namespace web::http::client;
using namespace utility::conversions;

namespace Auth
{
	const char* filename = "rdr2-twitch-npc-spawner.bin";
	std::string jwtCached;
	bool isCacheValid = false;
	bool isValidating = false;
	Concurrency::task<http_response> requestTask;
	long long lastValidationCheckMs = 0;
	eValidationStatus lastValidationStatus = eValidationStatus::Pending;

	std::string GetJwt()
	{
		if (isCacheValid)
			return jwtCached;

		std::ifstream inputFile(filename, std::ios::binary | std::ios::ate);

		if (!inputFile)
			return "";

		auto fileLength = inputFile.tellg();
		inputFile.seekg(0, std::ios::beg);
		auto bytes = new char[fileLength];
		if (1024 < fileLength || !inputFile.read(bytes, fileLength))
		{
			inputFile.close();
			delete[] bytes;
			return "";
		}
		inputFile.close();

		jwtCached = std::string(bytes, fileLength);
		isCacheValid = true;
		delete[] bytes;
		return jwtCached;
	}

	void SaveJwt(std::string jwt)
	{
		jwtCached = jwt;
		isCacheValid = true;

		std::ofstream outputFile(filename, std::ios::binary | std::ios::out);
		if (!outputFile)
		{
			Game::ShowNotification("Twitch NPC Spawner: Save failed");
			return;
		}

		outputFile.write(jwt.c_str(), jwt.size());
		outputFile.close();

		lastValidationStatus = eValidationStatus::Pending;
		lastValidationCheckMs = 0;

		Rewards::TryReconnect();
	}

	eValidationStatus ValidateJwt()
	{
		auto now = Util::Now();
		if (now - lastValidationCheckMs < 1800000)
			return lastValidationStatus;

		if (isValidating)
		{
			if (requestTask.is_done())
			{
				isValidating = false;

				auto response = requestTask.get();
				lastValidationStatus = 200 <= response.status_code() && response.status_code() < 300
					? eValidationStatus::Authorised
					: eValidationStatus::Unauthorised;
				lastValidationCheckMs = now;
				return lastValidationStatus;
			}

			return eValidationStatus::Pending;
		}

		auto jwt = GetJwt();
		http_request request(methods::GET);
		request.headers().add(to_string_t("Authorization"), to_string_t(jwt));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Auth/Validate"));
		http_client client(to_string_t(Util::GetServerUrl()));
		requestTask = client.request(request);
		isValidating = true;

		return eValidationStatus::Pending;
	}
}
