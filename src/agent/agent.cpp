#include "agent.h"
#include <utility>

Agent::Agent() = default;
void Agent::Init(const LobbyData& lobbyData) {}
ResponseVariant Agent::NextMove(const GameState& gameState) {return Wait{};}
void Agent::OnWarningReceived(WarningType warningType, std::optional<std::string> &message) {}
void Agent::OnGameEnded(const EndGameLobby& endGameLobby) {}
