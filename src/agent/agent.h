#pragma once

#include "../processed-packets.h"

class Agent {
 public:
	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded();
};
