#pragma once

#include "../processed-packets.h"

class Agent {
 public:
    /// DO NOT DELETE
    /// lifetime of a thread with NextMove() in milliseconds
    int timeoutNumber = 200;
    /// DO NOT DELETE
    /// time in milliseconds after which the NextMove() answer is not sent to server
    int skipResponse = 100;

	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded();
};
