#pragma once

#include "../processed-packets.h"

class Agent {
 public:
    /// DO NOT DELETE
    /// time in milliseconds after which the NextMove() answer is not sent to server, CAN BE CHANGED WHENEVER YOU WANT
    int skipResponse = 99;

	Agent();
	void Init(LobbyData lobbyData);
	ResponseVariant NextMove(GameState gameState);
	void OnGameEnded(EndGameLobby endGameLobby);
    void OnWarningReceived(WarningType warningType, std::optional<std::string>& message);
};
