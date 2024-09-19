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
	Agent *agentPtr;
	std::queue<std::string> *messagesToSendPtr;
	std::mutex *mtxPtr;
	std::condition_variable *cvPtr;
};
