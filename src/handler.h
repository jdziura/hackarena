#pragma once

#include "pch.h"
#include "agent/agent.h"
#include "processed-packets.h"

class Handler {
 public:
	Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr, std::condition_variable *cvPtr);
	void HandleLobbyData(nlohmann::json payload);
	void HandleGameState(nlohmann::json payload);
	void HandleGameEnded(nlohmann::json payload);

 private:
	TileVariant ParseTileVariant(const nlohmann::json& tileJson);
	std::string ResponseToString(const ResponseVariant& response);
	void SendResponse(const ResponseVariant& response);
    void UpdateTilesAndVisibility(GameState& gameState);

	Agent *agentPtr;
	std::queue<std::string> *messagesToSendPtr;
	std::mutex *mtxPtr;
	std::condition_variable *cvPtr;
};
