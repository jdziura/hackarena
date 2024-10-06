#include "agent.h"
#include <utility>

Agent::Agent() = default;
void Agent::Init(LobbyData lobbyData) {}
ResponseVariant Agent::NextMove(GameState gameState) {return Wait{};}
void Agent::OnWarningReceived(WarningType warningType, std::optional<std::string> &message) {}
void Agent::OnGameEnded(EndGameLobby endGameLobby) {}
