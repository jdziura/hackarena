#include "../processed-packets.h"

#pragma once

class Agent {
 public:
	Agent(std::string myId, std::string myNickname);
	static void NextMove();
	static void OnGameEnded();
 private:
	std::string myId;
	std::string myNickname;
};
