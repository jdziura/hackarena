#pragma once

#include "../processed-packets.h"

class Agent {
 public:
    // Take overhead into account !!!

    /// DO NOT DELETE
    /// lifetime of a thread with NextMove() in milliseconds, CAN BE CHANGED WHENEVER YOU WANT
    int timeoutNumber = 200;
    /// DO NOT DELETE
    /// time in milliseconds after which the NextMove() answer is not sent to server, CAN BE CHANGED WHENEVER YOU WANT
    int skipResponse = 100;

	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded();
};
