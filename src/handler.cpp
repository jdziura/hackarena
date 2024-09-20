#include "handler.h"

void Handler::HandleGameState(nlohmann::json payload) {
	GameState gameState;

	//TODO: Process GameState packet here

	ResponseVariant response = agentPtr->NextMove(gameState);
}

void Handler::HandleGameEnded(nlohmann::json payload) {}

void Handler::HandleLobbyData(nlohmann::json payload) {
	LobbyData lobbyData;

	// Extract the playerId
	lobbyData.myId = payload.at("playerId").get<std::string>();

	// Extract players array and populate the players vector
	for (const auto& player : payload.at("players")) {
		LobbyPlayers lobbyPlayer;
		lobbyPlayer.id = player.at("id").get<std::string>();
		lobbyPlayer.nickname = player.at("nickname").get<std::string>();
		lobbyPlayer.color = player.at("color").get<uint32_t>();

		lobbyData.players.push_back(lobbyPlayer);
	}

	// Extract server settings from the nested object
	const auto& serverSettings = payload.at("serverSettings");

	lobbyData.gridDimension = serverSettings.at("gridDimension").get<int>();
	lobbyData.numberOfPlayers = serverSettings.at("numberOfPlayers").get<int>();
	lobbyData.seed = serverSettings.at("seed").get<int>();
	lobbyData.broadcastInterval = serverSettings.at("broadcastInterval").get<int>();
	lobbyData.eagerBroadcast = serverSettings.at("eagerBroadcast").get<bool>();

	// Initialize the agent with the parsed lobby data
	agentPtr->Init(lobbyData);
}

Handler::Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr,std::condition_variable *cvPtr)
: agentPtr(agentPtr), messagesToSendPtr(messagesToSendPtr), mtxPtr(mtxPtr), cvPtr(cvPtr) {};
