#include "handler.h"
#include "packet.h"

TileVariant Handler::ParseTileVariant(const nlohmann::json& tileJson) {
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
		tank.direction = Direction{tileJson["payload"]["direction"].get<int>()};

		// Parse Turret (assuming it is mandatory in Tank)
		if (!tileJson["payload"].contains("turret") || tileJson["payload"]["turret"].is_null()) {
			throw std::runtime_error("Missing turret in tank payload.");
		}

		const auto& turretJson = tileJson["payload"]["turret"];

		// Check if turret direction exists and is not null
		if (!turretJson.contains("direction") || turretJson["direction"].is_null()) {
			throw std::runtime_error("Missing or null turret direction.");
		}
		tank.turret.direction = turretJson["direction"].get<Direction>();

		// Check if bulletCount exists and is not null
		if (turretJson.contains("bulletCount")) {
			tank.turret.bulletCount = turretJson["bulletCount"].get<int>();
		}

		// Optional health field
		if (tileJson["payload"].contains("health") && !tileJson["payload"]["health"].is_null()) {
			tank.health = tileJson["payload"]["health"].get<int>();
		}

        // Optional secondaryItem field
        if (tileJson["payload"].contains("secondaryItem") && !tileJson["payload"]["secondaryItem"].is_null()) {
            tank.secondaryItem = tileJson["payload"]["secondaryItem"].get<SecondaryItemType>();
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
		bullet.id = tileJson["payload"]["id"].get<int>();
		bullet.speed = tileJson["payload"]["speed"].get<double>();
		bullet.direction = tileJson["payload"]["direction"].get<Direction>();
        std::string temp = tileJson["payload"]["type"].get<std::string>();
        if(temp == "bullet") {
            bullet.type = BulletType::bullet;
        } else {
            bullet.type = BulletType::doubleBullet;
        }
		return bullet;
	}
    else if (type == "item") {
        // Parse Item
        Item item;
        item.type = tileJson["payload"]["type"].get<ItemType>();
        return item;
    }
    else if (type == "laser") {
        // Parse Item
        Laser laser;
        laser.id = tileJson["payload"]["id"].get<int>();
        laser.orientation = tileJson["payload"]["orientation"].get<LaserOrientation>();
        return laser;
    }
    else if (type == "mine") {
        // Parse Item
        Mine mine;
        mine.id = tileJson["payload"]["id"].get<int>();
        if (tileJson["payload"].contains("explosionRemainingTicks") && !tileJson["payload"]["explosionRemainingTicks"].is_null()) {
            mine.explosionRemainingTicks = tileJson["payload"]["explosionRemainingTicks"].get<int>();
        }
        return mine;
    }

	// Default case, throw an error if type doesn't match
	throw std::runtime_error("Unknown tile type: " + type);
}

// Function to convert ResponseVariant to string
std::string Handler::ResponseToString(const ResponseVariant& response, std::string& id) {
	nlohmann::ordered_json jsonResponse;

	std::visit([&jsonResponse, &id](const auto& resp) {
	  using T = std::decay_t<decltype(resp)>;
	  if constexpr (std::is_same_v<T, Rotate>) {
		  jsonResponse["type"] = PacketType::TankRotation;
          if (static_cast<int>(resp.tankRotation) == 2){
              jsonResponse["payload"]["tankRotation"] = nullptr;
          } else jsonResponse["payload"]["tankRotation"] = static_cast<int>(resp.tankRotation);
          if (static_cast<int>(resp.turretRotation) == 2){
              jsonResponse["payload"]["turretRotation"] = nullptr;
          } else jsonResponse["payload"]["turretRotation"] = static_cast<int>(resp.turretRotation);
          jsonResponse["payload"]["gameStateId"] = id;
	  } else if constexpr (std::is_same_v<T, Move>) {
		  jsonResponse["type"] = PacketType::TankMovement;
		  jsonResponse["payload"]["direction"] = static_cast<int>(resp.direction);
          jsonResponse["payload"]["gameStateId"] = id;
	  } else if constexpr (std::is_same_v<T, AbilityUse>) {
		  jsonResponse["type"] = PacketType::AbilityUse;
          jsonResponse["payload"]["abilityType"] = static_cast<int>(resp.type);;
		  jsonResponse["payload"]["gameStateId"] = id;
	  } else if constexpr (std::is_same_v<T, Wait>) {
		  jsonResponse["type"] = PacketType::ResponsePass;
          jsonResponse["payload"]["gameStateId"] = id;
	  }
	}, response);

	return jsonResponse.dump(); // Convert JSON object to string
}

// Example of sending the response over WebSocket
void Handler::SendResponse(const ResponseVariant& response, std::string& id) {
	std::string responseString = ResponseToString(response, id);
	// Send the response over the WebSocket
	{
		std::lock_guard<std::mutex> lock(*mtxPtr);
		messagesToSendPtr->push(responseString);
	}
	cvPtr->notify_one();
}

void Handler::UpdateTilesAndVisibility(GameState& gameState) {
    const auto& zones = gameState.map.zones;
    const auto& visibility = gameState.map.visibility;

    // Create a map for quick zone lookups
    std::unordered_map<std::pair<size_t, size_t>, char, boost::hash<std::pair<size_t, size_t>>> zoneMap;
    for (const auto& zone : zones) {
        for (int x = zone.x; x < zone.x + zone.width; ++x) {
            for (int y = zone.y; y < zone.y + zone.height; ++y) {
                zoneMap[{x, y}] = zone.name;  // Store the zone name for quick access
            }
        }
    }

    // Iterate through each tile in the tiles layer
    for (size_t row = 0; row < gameState.map.tiles.size(); ++row) {
        for (size_t col = 0; col < gameState.map.tiles[row].size(); ++col) {
            auto& tile = gameState.map.tiles[row][col];

            // Determine the zone name based on the zone map
            auto zoneIt = zoneMap.find({col, row});  // Notice the order: (x, y) corresponds to (col, row)
            char zoneName = zoneIt != zoneMap.end() ? zoneIt->second : '?'; // Default to '?' if no zone

            // Update tile based on its type and zone name
            std::visit([zoneName](auto& tileObj) {
                using T = std::decay_t<decltype(tileObj)>;
                if constexpr (std::is_same_v<T, Wall>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, Tank>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, Bullet>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, None>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, Mine>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, Laser>) {
                    tileObj.zoneName = zoneName;
                } else if constexpr (std::is_same_v<T, Item>) {
                    tileObj.zoneName = zoneName;
                }
            }, tile);

            // Update visibility based on the provided visibility data
            if (row < visibility.size() && col < visibility[row].size()) {
                char visibilityChar = visibility[row][col];

                // Update visibility based on the visibility character
                std::visit([visibilityChar](auto& tileObj) {
                    using T = std::decay_t<decltype(tileObj)>;
                    if constexpr (std::is_same_v<T, Tank>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    } else if constexpr (std::is_same_v<T, Bullet>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    } else if constexpr (std::is_same_v<T, None>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    } else if constexpr (std::is_same_v<T, Mine>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    } else if constexpr (std::is_same_v<T, Laser>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    } else if constexpr (std::is_same_v<T, Item>) {
                        tileObj.isVisible = (visibilityChar == '1');
                    }
                }, tile);
            }
        }
    }
}

void Handler::HandleGameState(nlohmann::json payload) {
	GameState gameState;

	// Parse playerId and tick (time)
	std::string id = payload["id"].get<std::string>();
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
        if (playerJson.contains("isUsingRadar") && !playerJson["isUsingRadar"].is_null()) {
            player.isUsingRadar = playerJson["isUsingRadar"].get<bool>();
        }

		gameState.players.push_back(player);
	}

	// Parse the map
	const auto& mapJson = payload["map"];

	// Parse zones
	for (const auto& zoneJson : mapJson["zones"]) {
		Zone zone;
		zone.x = zoneJson["x"].get<int>();
		zone.y = zoneJson["y"].get<int>();
		zone.width = zoneJson["width"].get<int>();
		zone.height = zoneJson["height"].get<int>();
		zone.name = zoneJson["index"].get<char>();

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

	gameState.map.visibility.resize(numCols, std::vector<char>(numRows));
    	for (size_t i = 0; i < numRows; ++i) {
		std::string row = visibilityJson[i].get<std::string>();
		for (size_t j = 0; j < numCols; ++j) {
			gameState.map.visibility[i][j] = row[j];
		}
	}

    const auto& layer = payload["map"]["tiles"];

    size_t numRows2 = layer.size();
    size_t numCols2 = layer[0].size(); // Assuming at least one row exists

    gameState.map.tiles.resize(numCols2, std::vector<TileVariant>(numRows2));

    // Loop through each row in the tiles layer
    for (size_t i = 0; i < numRows2; ++i) {
        const auto& rowJson = layer[i];

        // Loop through each tile in the row
        for (size_t j = 0; j < rowJson.size(); ++j) {
            const auto& tileJson = rowJson[j];

            if (tileJson.empty()) {
                // Handle empty tiles appropriately
                gameState.map.tiles[j][i] = None{false, 63};
                continue;
            }

            // Use the ParseTileVariant function to parse each tile
            gameState.map.tiles[j][i] = ParseTileVariant(tileJson[0]); // Access the first item in the tile array
        }
    }

    UpdateTilesAndVisibility(gameState);

	// If agent move takes less than 5 seconds send response
	auto start = std::chrono::high_resolution_clock::now();
	ResponseVariant response = agentPtr->NextMove(gameState);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> duration = end - start;

	if(duration.count() < agentPtr->skipResponse) SendResponse(response, id);
}

void Handler::HandleGameEnded(nlohmann::json payload) {
    EndGameLobby endGameLobby;

    // Extract players array and populate the players vector
    for (const auto& player : payload.at("players")) {
        EndGamePlayer lobbyPlayer;
        lobbyPlayer.id = player.at("id").get<std::string>();
        lobbyPlayer.nickname = player.at("nickname").get<std::string>();
        lobbyPlayer.color = player.at("color").get<uint32_t>();
        lobbyPlayer.score = player.at("score").get<int>();

        endGameLobby.players.push_back(lobbyPlayer);
    }

    agentPtr->OnGameEnded(endGameLobby);
}

void Handler::HandleLobbyData(nlohmann::json payload) {
	LobbyData lobbyData;

	// Extract the playerId
	lobbyData.myId = payload.at("playerId").get<std::string>();

	// Extract players array and populate the players vector
	for (const auto& player : payload.at("players")) {
		LobbyPlayer lobbyPlayer;
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
: agentPtr(agentPtr), messagesToSendPtr(messagesToSendPtr), mtxPtr(mtxPtr), cvPtr(cvPtr) {}

void Handler::OnWarningReceived(WarningType warningType, std::optional<std::string> message) {
    agentPtr->OnWarningReceived(warningType, message);
}
