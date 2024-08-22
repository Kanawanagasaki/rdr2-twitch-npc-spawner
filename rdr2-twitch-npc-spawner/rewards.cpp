#define _NO_ASYNCRTIMP
#include "inc/cpprest/ws_client.h"
#include "inc/cpprest/http_client.h"

#include "rewards.h"
#include "auth.h"
#include "util.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace utility::conversions;
using namespace web::websockets::client;
using namespace Concurrency;
using namespace utility::conversions;

#define STATE_DISCONNECTED -1
#define STATE_VALIDATE_JWT 0
#define STATE_SYNC_REWARDS 1
#define STATE_WAIT_SYNC_REWARDS 2
#define STATE_CONNECT 3
#define STATE_WAIT_CONNECTION 4
#define STATE_WAIT_SUBSCRIBE 5
#define STATE_RECEIVE_MESSAGE 6
#define STATE_WAIT_MESSAGE 7
#define STATE_WAIT_EXTRACT_MESSAGE 8
#define STATE_CLOSE 9
#define STATE_WAIT_CLOSE 10

namespace Rewards
{
	std::string endpoint = "wss://eventsub.wss.twitch.tv/ws";

	websocket_client* client;
	int state = STATE_VALIDATE_JWT;
	std::string sessionId = "";
	bool shouldAnnounceConnection = true;
	bool shouldReconnect = true;
	bool shouldDisconnect = false;
	bool shouldSkipSendingSubscriptions = false;
	long long lastMessageMs = 0;
	long long nextRedemptionPull = 0;
	long long lastRedemptionPull = 0;

	task<http_response> syncTask;
	task<void> connectTask;
	task<websocket_incoming_message> receiveMessageTask;
	task<std::string> messageTask;
	task<void> closeTask;
	task<http_response> subscribeTask;
	bool subscribeTaskIsRunning = false;
	task<http_response> redemptionTask;
	bool redemptionTaskIsRunning = false;
	task<json::value> redemptionJsonTask;
	bool redemptionJsonTaskIsRunning = false;

	void ProcessWelcome(json::value json)
	{
		if (!json.has_field(U("payload")))
			return;
		auto payload = json[U("payload")];
		if (!payload.has_field(U("session")))
			return;
		auto session = payload[U("session")];
		if (!session.has_string_field(U("id")))
			return;

		sessionId = to_utf8string(session[U("id")].as_string());

		if (shouldSkipSendingSubscriptions)
		{
			shouldSkipSendingSubscriptions = false;
			return;
		}

		http_request request(methods::POST);
		request.headers().add(to_string_t("Authorization"), to_string_t(Auth::GetJwt()));
		request.headers().add(to_string_t("Content-Type"), to_string_t("application/json"));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Event/Subscribe"));

		json::value postData;
		postData[U("sessionId")] = json::value::string(to_string_t(sessionId));
		request.set_body(postData);

		http_client client(to_string_t(Util::GetServerUrl()));
		subscribeTask = client.request(request);
		subscribeTaskIsRunning = true;
	}
	void WaitSubscribe()
	{
		if (!subscribeTask.is_done())
			return;

		subscribeTaskIsRunning = false;

		auto res = subscribeTask.get();
		if (200 <= res.status_code() && res.status_code() < 300)
		{
			if (shouldAnnounceConnection)
			{
				Game::ShowNotification("Twitch NPC Spawner 1.0.1: Connected");
				shouldAnnounceConnection = false;
			}
			state = STATE_RECEIVE_MESSAGE;
		}
		else
		{
			Game::ShowNotification("Twitch NPC Spawner: Failed to connected to EventSub");
			shouldReconnect = false;
			state = STATE_CLOSE;
		}
	}

	void ProcessReconnect(json::value json)
	{
		if (!json.has_field(U("payload")))
			return;
		auto payload = json[U("payload")];
		if (!payload.has_field(U("session")))
			return;
		auto session = payload[U("session")];
		if (!session.has_string_field(U("reconnect_url")))
			return;

		endpoint = to_utf8string(session[U("reconnect_url")].as_string());
		state = STATE_CLOSE;
		shouldReconnect = true;
		shouldAnnounceConnection = false;
		shouldSkipSendingSubscriptions = true;
	}

	void PullRedemption()
	{
		auto now = Util::Now();
		if (now - lastRedemptionPull < 5000)
			return;
		lastRedemptionPull = now;

		http_request request(methods::GET);
		request.headers().add(to_string_t("Authorization"), to_string_t(Auth::GetJwt()));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Reward/Redemption"));

		http_client client(to_string_t(Util::GetServerUrl()));
		redemptionTask = client.request(request);
		redemptionTaskIsRunning = true;
	}
	void WaitRedemption()
	{
		if (!redemptionTask.is_done())
			return;
		redemptionTaskIsRunning = false;

		auto res = redemptionTask.get();
		if (res.status_code() != 200)
			nextRedemptionPull = Util::Now() + 300000;
		else
		{
			nextRedemptionPull = Util::Now() + 30000;
			redemptionJsonTask = res.extract_json();
			redemptionJsonTaskIsRunning = true;
		}
	}
	void WaitRedemptionJson()
	{
		if (!redemptionJsonTask.is_done())
			return;
		redemptionJsonTaskIsRunning = false;

		auto json = redemptionJsonTask.get();
		auto arr = json.as_array();

		for (auto& item : arr)
		{
			auto redemption = new Game::Redemption
			{
				.id = item.has_string_field(U("id")) ? to_utf8string(item[U("id")].as_string()) : "",
				.userId = item.has_string_field(U("user_id")) ? to_utf8string(item[U("user_id")].as_string()) : "",
				.userLogin = item.has_string_field(U("user_login")) ? to_utf8string(item[U("user_login")].as_string()) : "",
				.userName = item.has_string_field(U("user_name")) ? to_utf8string(item[U("user_name")].as_string()) : "",
				.userInput = item.has_string_field(U("user_input")) ? to_utf8string(item[U("user_input")].as_string()) : "",
				.rewardType = item.has_integer_field(U("reward_type")) ? item[U("reward_type")].as_integer() : 0,
				.extra = item.has_string_field(U("extra")) ? to_utf8string(item[U("extra")].as_string()) : "",
			};

			Game::Process(redemption);
		}
	}

	void ProcessNotification(json::value json)
	{
		if (!json.has_field(U("metadata")))
			return;
		auto metadata = json[U("metadata")];
		if (!metadata.has_string_field(U("subscription_type")))
			return;

		auto subscriptionType = to_utf8string(metadata[U("subscription_type")].as_string());
		if (subscriptionType != "channel.channel_points_custom_reward_redemption.add")
			return;

		PullRedemption();
	}

	void ValidateJwt()
	{
		switch (Auth::ValidateJwt())
		{
		case Auth::eValidationStatus::Authorised:
			lastMessageMs = Util::Now();
			state = STATE_SYNC_REWARDS;
			break;
		case Auth::eValidationStatus::Unauthorised:
			Game::ShowNotification("Twitch NPC Spawner: Authorization required");
			state = STATE_DISCONNECTED;
			break;
		}
	}
	void SyncRewards()
	{
		http_request request(methods::GET);
		request.headers().add(to_string_t("Authorization"), to_string_t(Auth::GetJwt()));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Reward/Sync"));
		http_client client(to_string_t(Util::GetServerUrl()));
		syncTask = client.request(request);

		state = STATE_WAIT_SYNC_REWARDS;
	}
	void WaitSyncRewards()
	{
		if (!syncTask.is_done())
			return;

		auto response = syncTask.get();
		if (200 <= response.status_code() && response.status_code() < 300)
			state = STATE_CONNECT;
		else
		{
			Game::ShowNotification("Twitch NPC Spawner: Failed to synchronize rewards");
			state = STATE_DISCONNECTED;
		}
	}
	void Connect()
	{
		client = new websocket_client();
		connectTask = client->connect(to_string_t(endpoint));
		state = STATE_WAIT_CONNECTION;
	}
	void WaitConnection()
	{
		if (!connectTask.is_done())
			return;

		state = STATE_RECEIVE_MESSAGE;
	}
	void ReceiveMessage()
	{
		receiveMessageTask = client->receive();
		state = STATE_WAIT_MESSAGE;
	}
	void WaitForMessage()
	{
		if (!receiveMessageTask.is_done())
			return;

		auto message = receiveMessageTask.get();

		switch (message.message_type())
		{
		case websocket_message_type::text_message:
			messageTask = message.extract_string();
			state = STATE_WAIT_EXTRACT_MESSAGE;
			break;
		case websocket_message_type::close:
			state = STATE_CLOSE;
			break;
		case websocket_message_type::ping:
		{
			auto pongMsg = websocket_outgoing_message();
			pongMsg.set_pong_message();
			client->send(pongMsg);
			state = STATE_RECEIVE_MESSAGE;
			break;
		}
		default:
			state = STATE_RECEIVE_MESSAGE;
			break;
		}
	}
	void WaitExtractMessage()
	{
		if (!messageTask.is_done())
			return;

		auto message = messageTask.get();

		if (shouldDisconnect)
		{
			state = STATE_CLOSE;
			shouldDisconnect = false;
			return;
		}
		else
			state = STATE_RECEIVE_MESSAGE;

		auto json = json::value::parse(message);

		if (!json.has_field(U("metadata")))
			return;
		auto metadata = json[U("metadata")];
		if (!metadata.has_string_field(U("message_type")))
			return;

		lastMessageMs = Util::Now();

		auto messageType = to_utf8string(metadata[U("message_type")].as_string());
		if (messageType == "session_welcome")
			ProcessWelcome(json);
		else if (messageType == "session_reconnect")
			ProcessReconnect(json);
		else if (messageType == "revocation")
			state = STATE_CLOSE;
		else if (messageType == "notification")
			ProcessNotification(json);
	}
	void Close()
	{
		closeTask = client->close();
		state = STATE_WAIT_CLOSE;
	}
	void WaitClose()
	{
		if (!closeTask.is_done())
			return;

		delete client;
		client = nullptr;

		if (shouldReconnect)
			state = STATE_VALIDATE_JWT;
		else
		{
			Game::ShowNotification("Twitch NPC Spawner: Disconnected");
			state = STATE_DISCONNECTED;

			shouldAnnounceConnection = true;
		}
	}

	void Tick()
	{
		if (state == STATE_RECEIVE_MESSAGE && 300000 < Util::Now() - lastMessageMs)
		{
			state = STATE_CLOSE;
			shouldAnnounceConnection = false;
			shouldReconnect = true;
		}

		try
		{
			switch (state)
			{
			case STATE_VALIDATE_JWT: ValidateJwt(); break;
			case STATE_SYNC_REWARDS: SyncRewards(); break;
			case STATE_WAIT_SYNC_REWARDS: WaitSyncRewards(); break;
			case STATE_CONNECT: Connect(); break;
			case STATE_WAIT_CONNECTION: WaitConnection(); break;
			case STATE_RECEIVE_MESSAGE: ReceiveMessage(); break;
			case STATE_WAIT_MESSAGE: WaitForMessage(); break;
			case STATE_WAIT_EXTRACT_MESSAGE: WaitExtractMessage(); break;
			case STATE_CLOSE: Close(); break;
			case STATE_WAIT_CLOSE: WaitClose(); break;
			}
		}
		catch (...)
		{
			Game::ShowNotification("Twitch NPC Spawner: Internal error, disconnected");

			if (client != nullptr)
				delete client;
			state = STATE_DISCONNECTED;
		}

		if (subscribeTaskIsRunning)
			WaitSubscribe();

		if (state == STATE_RECEIVE_MESSAGE || state == STATE_WAIT_MESSAGE || state == STATE_WAIT_EXTRACT_MESSAGE)
		{
			if (redemptionTaskIsRunning)
				WaitRedemption();
			else if (redemptionJsonTaskIsRunning)
				WaitRedemptionJson();
			else if (nextRedemptionPull <= Util::Now())
				PullRedemption();
		}
	}

	void Disconnect()
	{
		if (state != STATE_DISCONNECTED)
		{
			shouldDisconnect = true;
			shouldReconnect = false;
			shouldAnnounceConnection = true;
			shouldSkipSendingSubscriptions = false;
			endpoint = "wss://eventsub.wss.twitch.tv/ws";
		}
	}
	void TryReconnect()
	{
		if (state == STATE_DISCONNECTED)
			state = STATE_VALIDATE_JWT;
	}

	void Fulfill(Game::Redemption* redemption)
	{
		http_request request(methods::POST);
		request.headers().add(to_string_t("Authorization"), to_string_t(Auth::GetJwt()));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Reward/Fulfill"));

		std::string form = "redemptionId=" + redemption->id + "&rewardType=" + std::to_string(redemption->rewardType);
		request.set_body(to_string_t(form), to_string_t("application/x-www-form-urlencoded"));

		http_client client(to_string_t(Util::GetServerUrl()));
		client.request(request);
	}

	void Cancel(Game::Redemption* redemption)
	{
		http_request request(methods::POST);
		request.headers().add(to_string_t("Authorization"), to_string_t(Auth::GetJwt()));
		request.set_request_uri(to_string_t(Util::GetServerUrl() + "/Reward/Cancel"));

		std::string form = "redemptionId=" + redemption->id + "&rewardType=" + std::to_string(redemption->rewardType);
		request.set_body(to_string_t(form), to_string_t("application/x-www-form-urlencoded"));

		http_client client(to_string_t(Util::GetServerUrl()));
		client.request(request);
	}

	std::string GetState()
	{
		switch (state)
		{
		case STATE_DISCONNECTED: return "DISCONNECTED";
		case STATE_VALIDATE_JWT: return "VALIDATE_JWT";
		case STATE_SYNC_REWARDS: return "SYNC_REWARDS";
		case STATE_WAIT_SYNC_REWARDS: return "WAIT_SYNC_REWARDS";
		case STATE_CONNECT: return "CONNECT";
		case STATE_WAIT_CONNECTION: return "WAIT_CONNECTION";
		case STATE_RECEIVE_MESSAGE: return "RECEIVE_MESSAGE";
		case STATE_WAIT_MESSAGE: return "WAIT_MESSAGE";
		case STATE_WAIT_EXTRACT_MESSAGE: return "WAIT_EXTRACT_MESSAGE";
		case STATE_CLOSE: return "CLOSE";
		case STATE_WAIT_CLOSE: return "WAIT_CLOSE";
		}

		return "UNKNOWN";
	}

	long long GetLastMessageMs()
	{
		return lastMessageMs;
	}
	long long GetLastRedemptionPull()
	{
		return lastRedemptionPull;
	}
	long long GetNextRedemptionPull()
	{
		return nextRedemptionPull;
	}
}
