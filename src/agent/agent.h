#include "../processed-packets.h"

#pragma once

class Agent {
 public:
	void NextMove();
	void OnGameEnded();
 private:
	std::string myId;
	std::string myNickname;
};
