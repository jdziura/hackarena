#pragma once

#include "../processed-packets.h"

class Agent {
 public:
    int timeoutNumber = 100; // lifetime of a thread with NextMove() in milliseconds

	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded();
};
