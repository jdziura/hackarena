#pragma once

#include "pch.h"
#include "agent/agent.h"
#include "processed-packets.h"
#include "packet.h"

class Handler {
 public:
	Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr, std::condition_variable *cvPtr);
	void HandleLobbyData(nlohmann::json payload);
	void HandleGameState(nlohmann::json payload);
	void HandleGameEnded(nlohmann::json payload);
    void HandleGameStarting();
    void OnWarningReceived(WarningType warningType, std::optional<std::string> message);

 private:
	static std::string ResponseToString(const ResponseVariant& response, std::string& id);
	void SendResponse(const ResponseVariant& response, std::string& id);
    static void UpdateTilesAndVisibility(GameState& gameState);

	Agent *agentPtr;
	std::queue<std::string> *messagesToSendPtr;
	std::mutex *mtxPtr;
	std::condition_variable *cvPtr;
};
