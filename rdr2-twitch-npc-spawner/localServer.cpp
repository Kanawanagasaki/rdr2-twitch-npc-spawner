#include "localServer.h"

#include <thread>

#define _NO_ASYNCRTIMP
#include "inc/cpprest/http_listener.h"
#include "game.h"
#include "auth.h"
#include "rewards.h"

using namespace web;
using namespace http;
using namespace utility;
using namespace utility::conversions;
using namespace http::experimental::listener;
using namespace Concurrency;

namespace LocalServer
{
	struct postData
	{
		std::string path;
		task<json::value> body;
	};

	http_listener* listener = nullptr;
	bool isStopping = false;
	task<void> closeTask;
	std::vector<postData> postRequests;

	void AddCORS(http_response& response)
	{
		response.headers().add(U("Allow"), U("GET, POST, OPTIONS"));
		response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
		response.headers().add(U("Access-Control-Allow-Methods"), U("GET, POST, OPTIONS"));
		response.headers().add(U("Access-Control-Allow-Headers"), U("Content-Type"));
	}

	void HandleOptions(http_request request)
	{
		http_response response(status_codes::OK);
		AddCORS(response);
		request.reply(response);
	}

	void HandleGet(http_request request)
	{
		auto uri = request.request_uri();
		if (uri.is_path_empty())
		{
			http_response response(status_codes::NoContent);
			AddCORS(response);
			request.reply(response);
		}
		else
		{
			auto path = utility::conversions::to_utf8string(uri.path());
			if (path == "/state")
			{
				auto status = json::value::string(U("unknown"));
				switch (Auth::ValidateJwt())
				{
				case Auth::eValidationStatus::Authorised: status = json::value::string(U("authorized")); break;
				case Auth::eValidationStatus::Unauthorised: status = json::value::string(U("unauthorized")); break;
				}

				std::vector<json::value> spawnedPedsJson;
				auto spawnedPeds = Game::GetSpawnedPeds();
				for (const auto& pair : *spawnedPeds)
				{
					json::value spawnedPed;
					spawnedPed[U("handle")] = json::value::number(pair.first);
					spawnedPed[U("viewerId")] = json::value::string(to_string_t(pair.second->GetViewerId()));
					spawnedPed[U("nickname")] = json::value::string(to_string_t(pair.second->GetNickname()));
					spawnedPedsJson.push_back(spawnedPed);
				}

				std::vector<json::value> pendingRedemptionsJson;
				auto pendingRedemptions = Game::GetPendingRedemptions();
				for (const auto& pair : *pendingRedemptions)
				{
					json::value redemption;
					redemption[U("type")] = json::value::number(pair.first);
					redemption[U("id")] = json::value::string(to_string_t(pair.second->id));
					redemption[U("userId")] = json::value::string(to_string_t(pair.second->userId));
					redemption[U("userInput")] = json::value::string(to_string_t(pair.second->userInput));
					redemption[U("userLogin")] = json::value::string(to_string_t(pair.second->userLogin));
					redemption[U("userName")] = json::value::string(to_string_t(pair.second->userName));
					pendingRedemptionsJson.push_back(redemption);
				}

				json::value eventSub;
				eventSub[U("state")] = json::value::string(to_string_t(Rewards::GetState()));
				eventSub[U("lastMessageMs")] = json::value::number(Rewards::GetLastMessageMs());
				eventSub[U("lastRedemptionPull")] = json::value::number(Rewards::GetLastRedemptionPull());
				eventSub[U("nextRedemptionPull")] = json::value::number(Rewards::GetNextRedemptionPull());

				http_response response(status_codes::OK);
				AddCORS(response);
				json::value json;
				json[U("status")] = status;
				json[U("spawnedPeds")] = json::value::array(spawnedPedsJson);
				json[U("pendingRedemptions")] = json::value::array(pendingRedemptionsJson);
				json[U("eventSub")] = eventSub;
				response.set_body(json);
				request.reply(response);
			}
			else if (path == "/getjwt")
			{
				auto jwt = Auth::GetJwt();

				http_response response(status_codes::OK);
				AddCORS(response);
				response.set_body(jwt);
				request.reply(response);
			}
			else if (path == "/disconnect")
			{
				Rewards::Disconnect();

				http_response response(status_codes::NoContent);
				AddCORS(response);
				request.reply(response);
			}
			else if (path == "/tryreconnect")
			{
				Rewards::TryReconnect();

				http_response response(status_codes::NoContent);
				AddCORS(response);
				request.reply(response);
			}
			else if (path == "/kickall")
			{
				Game::DespawnAllPeds();

				http_response response(status_codes::NoContent);
				AddCORS(response);
				request.reply(response);
			}
			else if (path.starts_with("/kick/"))
			{
				try
				{
					auto handleStr = path.substr(6);
					auto handle = std::stoi(handleStr);
					Game::DespawnPed(handle);
				}
				catch (...) {}

				http_response response(status_codes::NoContent);
				AddCORS(response);
				request.reply(response);
			}
			else if (path == "/logout")
			{
				Auth::SaveJwt("");
				Rewards::Disconnect();

				http_response response(status_codes::NoContent);
				AddCORS(response);
				request.reply(response);
			}
			else
			{
				http_response response(status_codes::MethodNotAllowed);
				AddCORS(response);
				request.reply(response);
			}
		}
	}

	void HandlePost(http_request request)
	{
		auto headers = request.headers();
		auto contentType = headers.content_type();
		auto contentTypeStr = utility::conversions::to_utf8string(contentType);

		if (contentTypeStr.find("json") == std::string::npos)
		{
			http_response response(status_codes::BadRequest);
			AddCORS(response);
			request.reply(response);
			return;
		}

		auto jsonTask = request.extract_json();

		auto uri = request.request_uri();
		auto path = to_utf8string(uri.path());

		postRequests.push_back(postData
			{
				.path = path,
				.body = jsonTask
			}
		);

		http_response response(status_codes::Accepted);
		AddCORS(response);
		request.reply(response);
	}

	bool IsRunning()
	{
		return listener != nullptr;
	}

	void Start()
	{
		if (listener != nullptr)
			return;
		if (isStopping)
			return;

		listener = new http_listener(U("http://127.0.0.1:41078"));
		listener->support(methods::GET, HandleGet);
		listener->support(methods::POST, HandlePost);
		listener->support(methods::OPTIONS, HandleOptions);
		listener->open();
	}

	void Stop()
	{
		if (listener == nullptr)
			return;
		if (isStopping)
			return;

		closeTask = listener->close();
		isStopping = true;
	}

	void Tick()
	{
		if (listener == nullptr)
			return;

		if (isStopping && closeTask.is_done())
		{
			delete listener;
			listener = nullptr;
			isStopping = false;
			return;
		}

		for (auto it = postRequests.begin(); it != postRequests.end();)
		{
			if (it->body.is_done())
			{
				auto json = it->body.get();

				if (it->path == "/setjwt" && json.has_field(U("token")))
				{
					auto jwt = to_utf8string(json.at(U("token")).as_string());
					Auth::SaveJwt(jwt);
				}

				it = postRequests.erase(it);
			}
			else
				++it;
		}
	}
}
