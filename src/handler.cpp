#include "handler.h"

TileVariant Handler::ParseTileVariant(const nlohmann::json& tileJson) {
	// Check the type of the tile
	std::string type = tileJson["type"].get<std::string>();

	if (type == "wall") {
		return Wall();  // Wall has no additional properties
	}
	else if (type == "tank") {
		// Parse Tank
		Tank tank;

		// Check if ownerId exists and is not null
		if (!tileJson["payload"].contains("ownerId") || tileJson["payload"]["ownerId"].is_null()) {
			throw std::runtime_error("Missing or null ownerId in tank payload.");
		}
		tank.ownerId = tileJson["payload"]["ownerId"].get<std::string>();

		// Check if direction exists and is not null
		if (!tileJson["payload"].contains("direction") || tileJson["payload"]["direction"].is_null()) {
			throw std::runtime_error("Missing or null direction in tank payload.");
		}
		tank.direction = tileJson["payload"]["direction"].get<int>();

		// Parse Turret (assuming it is mandatory in Tank)
		if (!tileJson["payload"].contains("turret") || tileJson["payload"]["turret"].is_null()) {
			throw std::runtime_error("Missing turret in tank payload.");
		}

		const auto& turretJson = tileJson["payload"]["turret"];

		// Check if turret direction exists and is not null
		if (!turretJson.contains("direction") || turretJson["direction"].is_null()) {
			throw std::runtime_error("Missing or null turret direction.");
		}
		tank.turret.direction = turretJson["direction"].get<int>();

		// Check if bulletCount exists and is not null
		if (turretJson.contains("bulletCount")) {
			tank.turret.bulletCount = turretJson["bulletCount"].get<int>();
		}

		// Optional health field
		if (tileJson["payload"].contains("health") && !tileJson["payload"]["health"].is_null()) {
			tank.health = tileJson["payload"]["health"].get<int>();
		}

		// Optional field for ticksToRegenBullet in Turret
		if (turretJson.contains("ticksToRegenBullet") && !turretJson["ticksToRegenBullet"].is_null()) {
			tank.turret.ticksToRegenBullet = turretJson["ticksToRegenBullet"].get<int>();
		}

		return tank;
	}
	else if (type == "bullet") {
		// Parse Bullet
		Bullet bullet;

		// Check if id exists and is not null
		if (!tileJson["payload"].contains("id") || tileJson["payload"]["id"].is_null()) {
			throw std::runtime_error("Missing or null id in bullet payload.");
		}
		bullet.id = tileJson["payload"]["id"].get<int>();

		// Check if speed exists and is not null
		if (!tileJson["payload"].contains("speed") || tileJson["payload"]["speed"].is_null()) {
			throw std::runtime_error("Missing or null speed in bullet payload.");
		}
		bullet.speed = tileJson["payload"]["speed"].get<double>();

		// Check if direction exists and is not null
		if (!tileJson["payload"].contains("direction") || tileJson["payload"]["direction"].is_null()) {
			throw std::runtime_error("Missing or null direction in bullet payload.");
		}
		bullet.direction = tileJson["payload"]["direction"].get<int>();

		return bullet;
	}

	// Default case, throw an error if type doesn't match
	throw std::runtime_error("Unknown tile type: " + type);
}

// Function to convert ResponseVariant to string
std::string Handler::ResponseToString(const ResponseVariant& response) {
	nlohmann::json jsonResponse;

	std::visit([&jsonResponse](const auto& resp) {
	  using T = std::decay_t<decltype(resp)>;
	  if constexpr (std::is_same_v<T, Rotate>) {
		  jsonResponse["type"] = 66; // PacketType for TankRotation
		  jsonResponse["payload"]["tankRotation"] = static_cast<int>(resp.tankRotation);
		  jsonResponse["payload"]["turretRotation"] = static_cast<int>(resp.turretRotation);
	  } else if constexpr (std::is_same_v<T, Move>) {
		  jsonResponse["type"] = 65; // PacketType for TankMovement
		  jsonResponse["payload"]["direction"] = static_cast<int>(resp.direction);
	  } else if constexpr (std::is_same_v<T, Shoot>) {
		  jsonResponse["type"] = 67; // PacketType for TankShoot
		  jsonResponse["payload"] = nlohmann::json::object(); // Empty payload
	  } else if constexpr (std::is_same_v<T, Wait>) {
		  jsonResponse["type"] = 66; // Same as TankRotation
		  jsonResponse["payload"]["tankRotation"] = nullptr; // Null for tankRotation
		  jsonResponse["payload"]["turretRotation"] = nullptr; // Null for turretRotation
	  }
	}, response);

	return jsonResponse.dump(); // Convert JSON object to string
}

// Example of sending the response over WebSocket
void Handler::SendResponse(const ResponseVariant& response) {
	std::string responseString = ResponseToString(response);

	// Send the response over the WebSocket
	{
		std::lock_guard<std::mutex> lock(*mtxPtr);
		messagesToSendPtr->push(responseString);
	}
	cvPtr->notify_one();
}

void Handler::HandleGameState(nlohmann::json payload) {
	GameState gameState;

	// Parse playerId and tick (time)
	gameState.playerId = payload["playerId"].get<std::string>();
	gameState.time = payload["tick"].get<int>();

	// Parse the players
	for (const auto& playerJson : payload["players"]) {
		Player player;
		player.id = playerJson["id"].get<std::string>();
		player.nickname = playerJson["nickname"].get<std::string>();
		player.color = playerJson["color"].get<uint32_t>();
		player.ping = playerJson["ping"].get<int>();

		// Optional fields
		if (playerJson.contains("score") && !playerJson["score"].is_null()) {
			player.score = playerJson["score"].get<int>();
		}
		if (playerJson.contains("ticksToRegen") && !playerJson["ticksToRegen"].is_null()) {
			player.ticksToRegen = playerJson["ticksToRegen"].get<int>();
		}

		gameState.players.push_back(player);
	}

	// Parse the map
	const auto& mapJson = payload["map"];

	// Parse the tiles (3D vector)
	for (const auto& layer : mapJson["tiles"]) {
		std::vector<std::vector<TileVariant>> tileLayer;
		for (const auto& row : layer) {
			std::vector<TileVariant> tileRow;
			for (const auto& tile : row) {
				TileVariant tileVariant;
				// Assuming you have a method to handle TileVariant
				tileVariant = ParseTileVariant(tile);
				tileRow.push_back(tileVariant);
			}
			tileLayer.push_back(tileRow);
		}
		gameState.map.tiles.push_back(tileLayer);
	}

	// Parse zones
	for (const auto& zoneJson : mapJson["zones"]) {
		Zone zone;
		zone.x = zoneJson["x"].get<int>();
		zone.y = zoneJson["y"].get<int>();
		zone.width = zoneJson["width"].get<int>();
		zone.height = zoneJson["height"].get<int>();
		zone.index = zoneJson["index"].get<int>();

		// Parse ZoneStatus (with optional fields)
		ZoneStatus zoneStatus;
		zoneStatus.type = zoneJson["status"]["type"].get<std::string>();

		// Optional fields for ZoneStatus based on its type
		if (zoneJson["status"].contains("remainingTicks") && !zoneJson["status"]["remainingTicks"].is_null()) {
			zoneStatus.remainingTicks = zoneJson["status"]["remainingTicks"].get<int>();
		}
		if (zoneJson["status"].contains("playerId") && !zoneJson["status"]["playerId"].is_null()) {
			zoneStatus.playerId = zoneJson["status"]["playerId"].get<std::string>();
		}
		if (zoneJson["status"].contains("capturedById") && !zoneJson["status"]["capturedById"].is_null()) {
			zoneStatus.capturedById = zoneJson["status"]["capturedById"].get<std::string>();
		}
		if (zoneJson["status"].contains("retakenById") && !zoneJson["status"]["retakenById"].is_null()) {
			zoneStatus.retakenById = zoneJson["status"]["retakenById"].get<std::string>();
		}

		zone.status = zoneStatus;
		gameState.map.zones.push_back(zone);
	}

	// Parse visibility (2D array of chars)
	const auto& visibilityJson = mapJson["visibility"];
	size_t numRows = visibilityJson.size();
	size_t numCols = visibilityJson[0].get<std::string>().size();

	// Initialize the transposed visibility array
	gameState.map.visibility.resize(numCols, std::vector<char>(numRows));

	// Fill the transposed visibility array
	for (size_t i = 0; i < numRows; ++i) {
		std::string row = visibilityJson[i].get<std::string>();
		for (size_t j = 0; j < numCols; ++j) {
			gameState.map.visibility[j][i] = row[j];  // Transpose the data
		}
	}

	// If agent move takes less than 5 seconds send response
	auto start = std::chrono::high_resolution_clock::now();
	ResponseVariant response = agentPtr->NextMove(gameState);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> duration = end - start;

	if(duration.count() < agentPtr->skipResponse) SendResponse(response);
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
