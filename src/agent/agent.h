#pragma once

#include "../processed-packets.h"

class Agent {
 public:
	Agent();
	void Init(LobbyData lobbyData);
	void NextMove(GameState gameState);
	void OnGameEnded();
};
