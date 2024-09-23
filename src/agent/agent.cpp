#include "agent.h"
#include <utility>

Agent::Agent() = default;
void Agent::Init(LobbyData lobbyData) {}
ResponseVariant Agent::NextMove(GameState gameState) {return Wait{};}
void Agent::OnGameEnded(EndGameLobby endGameLobby) {}
