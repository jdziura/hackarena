#pragma once

#include "../processed-packets.h"

class Agent {
 public:
	Agent();
	void NextMove(GameState gameState);
	void OnGameEnded();
	LobbyData lobbyData;
};
