#include "handler.h"

void Handler::HandleGameState(nlohmann::json payload) {
	GameState gameState;

	//TODO: Process GameState packet here

	ResponseVariant response = agentPtr->NextMove(gameState);
}

void Handler::HandleGameEnded(nlohmann::json payload) {}

void Handler::HandleLobbyData(nlohmann::json payload) {
	LobbyData lobbyData;

	lobbyData.myId = payload.at("playerId").get<std::string>();

	// Extract players array and populate the players vector
	for (const auto& player : payload.at("players")) {
		LobbyPlayers lobbyPlayer;
		lobbyPlayer.id = player.at("id").get<std::string>();
		lobbyPlayer.nickname = player.at("nickname").get<std::string>();
		lobbyPlayer.color = player.at("color").get<uint32_t>();

		lobbyData.players.push_back(lobbyPlayer);
	}

	lobbyData.gridDimension = payload.at("gridDimension").get<int>();
	lobbyData.seed = payload.at("seed").get<int>();
	lobbyData.broadcastInterval = payload.at("broadcastInterval").get<int>();

	agentPtr->Init(lobbyData);
}

Handler::Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr,std::condition_variable *cvPtr)
: agentPtr(agentPtr), messagesToSendPtr(messagesToSendPtr), mtxPtr(mtxPtr), cvPtr(cvPtr) {};
