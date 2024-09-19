#pragma once

#include "pch.h"
#include "agent/agent.h"

class Handler {
 public:
	Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr, std::condition_variable *cvPtr);
	void LobbyData();
	void GameState();
	void GameEnded();

 private:
	Agent *agentPtr;
	std::queue<std::string> *messagesToSendPtr;
	std::mutex *mtxPtr;
	std::condition_variable *cvPtr;
};
