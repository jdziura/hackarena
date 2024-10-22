#include "agent.h"
#include <utility>

void Agent::PrintMap(const std::vector<std::vector<Tile>>& tiles) {
    auto rows = tiles.size();
    auto cols = tiles[0].size();

    std::vector<std::vector<std::string>> map(rows, std::vector<std::string>(cols, " "));

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            const Tile& tile = tiles[i][j];

            // Check for tile objects
            bool hasObject = false;
            for (const TileVariant& object : tile.objects) {
                if (std::holds_alternative<Wall>(object)) {
                    map[i][j] = '#';
                    hasObject = true;
                    break; // Found wall, no need to check further
                } else if (std::holds_alternative<Tank>(object)) {
                    if (auto tankPtr = std::get_if<Tank>(&object)) {
                        auto tankId = tankPtr->ownerId;
                        // Use '@' for player's tank, 'T' for enemy tanks
                        std::string tankSymbol = (tankId != myId) ? "T" : "@";
                        tankSymbol += (tankPtr->direction == Direction::up) ? "^" :
                                      (tankPtr->direction == Direction::down) ? "v" :
                                      (tankPtr->direction == Direction::left) ? "<" : ">";
                        map[i][j] = tankSymbol;
                        hasObject = true;
                        break; // Found tank, no need to check further
                    }
                } else if (std::holds_alternative<Item>(object)) {
                    if (auto itemPtr = std::get_if<Item>(&object)) {
                        map[i][j] = (itemPtr->type == ItemType::radar) ? 'R' :
                                    (itemPtr->type == ItemType::doubleBullet) ? 'D' :
                                    (itemPtr->type == ItemType::mine) ? 'M' : 'L';
                        hasObject = true;
                    }
                } else if (std::holds_alternative<Mine>(object)) {
                    map[i][j] = 'X';
                    hasObject = true;
                } else if (std::holds_alternative<Laser>(object)) {
                    if (auto laserPtr = std::get_if<Laser>(&object)) {
                        map[i][j] = (laserPtr->orientation == LaserOrientation::horizontal) ? '-' : '!';
                        hasObject = true;
                    }
                } else if (std::holds_alternative<Bullet>(object)) {
                    if (auto bulletPtr = std::get_if<Bullet>(&object)) {
                        if (bulletPtr->type == BulletType::bullet) {
                            map[i][j] = (bulletPtr->direction == Direction::up) ? "^" :
                                        (bulletPtr->direction == Direction::down) ? "v" :
                                        (bulletPtr->direction == Direction::left) ? "<" : ">";
                        } else {
                            map[i][j] = (bulletPtr->direction == Direction::up) ? "^^" :
                                        (bulletPtr->direction == Direction::down) ? "vv" :
                                        (bulletPtr->direction == Direction::left) ? "<<" : ">>";
                        }
                        hasObject = true;
                    }
                }
                break; // Stop checking objects after the first found
            }

            // If no object found, check for None tile properties
            if (!hasObject) {
                if (tile.zoneName != '?') {
                    map[i][j] = tile.zoneName; // Print zone name
                } else if (tile.isVisible) {
                    map[i][j] = '.'; // Print visibility for visible None tile
                } else {
                    map[i][j] = ' '; // Not visible, so leave it as empty space
                }
            }
        }
    }

    // Print the map
    for (const auto& row : map) {
        bool firstTile = true; // Flag to handle spacing
        for (const auto& tile : row) {
            // Only print space before tile if it is not the first tile
            if (firstTile) {
                firstTile = false; // Reset the flag after the first tile is printed
            } else {
                // Check the length of the last printed tile
                if (tile.length() == 1) {
                    std::cout << ' '; // Print space for single character tiles
                }
            }
            std::cout << tile; // Print the tile character(s)
        }
        std::cout << '\n'; // New line after each row
    }
    std::cout << "---------------------------------------------------" << std::endl;
}

Agent::Agent() = default;
void Agent::Init(const LobbyData& lobbyData) {myId = lobbyData.myId; skipResponse = lobbyData.broadcastInterval - 1;}
ResponseVariant Agent::NextMove(const GameState& gameState) {
	PrintMap(gameState.map.tiles);
	return Wait{};
}
void Agent::OnWarningReceived(WarningType warningType, std::optional<std::string> &message) {}
void Agent::OnGameStarting() {}
void Agent::OnGameEnded(const EndGameLobby& endGameLobby) {}
