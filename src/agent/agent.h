#pragma once

#include "../processed-packets.h"

class Agent {
 public:
	Agent(std::string myId = "", std::string myNickname = "");
	static void NextMove();
	static void OnGameEnded();
	std::string myId;
	std::string myNickname;
};
