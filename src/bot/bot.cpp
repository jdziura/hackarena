#include "bot.h"
#include <utility>
#include <random>

void Bot::PrintMap(const std::vector<std::vector<Tile>>& tiles) {
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

Bot::Bot() = default;

void Bot::Init(const LobbyData& _lobbyData) { 
    lobbyData = _lobbyData;
    dim = lobbyData.gridDimension;
    myId = lobbyData.myId;
    skipResponse = lobbyData.broadcastInterval - 1;
}

ResponseVariant Bot::RandomMove(const GameState& gameState) {
    // Create a random device and generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Define the distributions for each random selection
    std::uniform_int_distribution<int> actionDist(0, 3); // 4 possible actions
    std::uniform_int_distribution<int> directionDist(0, 1); // for MoveDirection and RotationDirection
    std::uniform_int_distribution<int> abilityDist(0, 4); // 5 possible abilities

    // Randomly choose the type of action
    int action = actionDist(gen);

    switch (action) {
        case 0: { // Rotate
            RotationDirection tankRot = static_cast<RotationDirection>(directionDist(gen));
            RotationDirection turretRot = static_cast<RotationDirection>(directionDist(gen));
            return Rotate{tankRot, turretRot};
        }
        case 1: { // Move
            MoveDirection moveDir = static_cast<MoveDirection>(directionDist(gen));
            return Move{moveDir};
        }
        case 2: { // AbilityUse
            AbilityType ability = static_cast<AbilityType>(abilityDist(gen));
            return AbilityUse{ability};
        }
        case 3: // Wait
        default:
            return Wait{};
    }
}

ResponseVariant Bot::NextMove(const GameState& gameState) {
    clock_t start = clock();
    std::cerr << " Round: " << gameState.time << std::endl;
    PrintMap(gameState.map.tiles);

    if (gameState.time == 1) {
        onFirstNextMove(gameState);
    }

    auto isZone = [&](const OrientedPosition& oPos) {
        return zoneName[oPos.pos.x][oPos.pos.y] != '?';
    };

    auto nxtMove = bfs(myPos, isZone);
    
    if (!nxtMove) {
        std::cerr << " >>> Jajco, can't move to zone <<< " << std::endl;
        return Wait{};
    }

    if (std::holds_alternative<MoveDirection>(*nxtMove)) {
        if (std::get<MoveDirection>(*nxtMove) == MoveDirection::forward) {
            std::cerr << " > move forward < " << std::endl;
        } else {
            std::cerr << " > move backward < " << std::endl;
        }
    } else {
        if (std::get<RotationDirection>(*nxtMove) == RotationDirection::left) {
            std::cerr << " > rotate left < " << std::endl;
        } else {
            std::cerr << " > rotate right < " << std::endl;
        }
    }

    myPos = afterMove(myPos, *nxtMove);
    std::cerr << " > myPos: " << myPos.pos.x << " " << myPos.pos.y << " " << static_cast<int>(myPos.dir) << std::endl;

    if (std::holds_alternative<MoveDirection>(*nxtMove)) {
        return Move{std::get<MoveDirection>(*nxtMove)};
    } else {
        return Rotate{std::get<RotationDirection>(*nxtMove), RotationDirection::none};
    }
}

void Bot::OnWarningReceived(WarningType warningType, std::optional<std::string> &message) {}
void Bot::OnGameStarting() {}
void Bot::OnGameEnded(const EndGameLobby& endGameLobby) {}

void Bot::initIsWall(const std::vector<std::vector<Tile>>& tiles) {
    isWall = std::vector<std::vector<bool>>(dim, std::vector<bool>(dim, false));
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            isWall[i][j] = std::any_of(tiles[i][j].objects.begin(), tiles[i][j].objects.end(), [](const TileVariant& object) {
                return std::holds_alternative<Wall>(object);
            });
        }
    }
}

void Bot::initZoneName(const std::vector<std::vector<Tile>>& tiles) {
    zoneName = std::vector<std::vector<char>>(dim, std::vector<char>(dim, '?'));
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            zoneName[i][j] = tiles[i][j].zoneName;
            std::cerr << zoneName[i][j] << " ";
        }
        std::cerr << std::endl;
    }
}

void Bot::initMyPos(const GameState& gameState) {
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            for (const TileVariant& object : gameState.map.tiles[i][j].objects) {
                if (std::holds_alternative<Tank>(object)) {
                    const Tank& tank = std::get<Tank>(object);
                    if (tank.ownerId == myId) {
                        myPos = OrientedPosition(Position(i, j), tank.direction);
                        return;
                    }
                }
            }
        }
    }
}

void Bot::onFirstNextMove(const GameState& gameState) {
    initIsWall(gameState.map.tiles);
    initZoneName(gameState.map.tiles);
    initMyPos(gameState);
}