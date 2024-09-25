#pragma once

#include "../processed-packets.h"

class Agent {
 public:
    /// DO NOT DELETE
    /// time in milliseconds after which the NextMove() answer is not sent to server, CAN BE CHANGED WHENEVER YOU WANT
    int skipResponse = 90; // 100 - 10 to make sure overhead is included (100 is standard tick)

	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded(EndGameLobby endGameLobby);
};
