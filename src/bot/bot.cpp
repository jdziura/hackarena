#include "bot.h"
#include <utility>
#include <random>

void Bot::OnWarningReceived(WarningType warningType, std::optional<std::string> &message) {}
void Bot::OnGameStarting() {}
void Bot::OnGameEnded(const EndGameLobby& endGameLobby) {}

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
    // for (const auto& row : map) {
    //     bool firstTile = true; // Flag to handle spacing
    //     for (const auto& tile : row) {
    //         // Only print space before tile if it is not the first tile
    //         if (firstTile) {
    //             firstTile = false; // Reset the flag after the first tile is printed
    //         } else {
    //             // Check the length of the last printed tile
    //             if (tile.length() == 1) {
    //                 std::cout << ' '; // Print space for single character tiles
    //             }
    //         }
    //         std::cout << tile; // Print the tile character(s)
    //     }
    //     std::cout << '\n'; // New line after each row
    // }
    // std::cout << "---------------------------------------------------" << std::endl;
}

Bot::Bot() = default;

void Bot::Init(const LobbyData& _lobbyData) { 
    lobbyData = _lobbyData;
    dim = lobbyData.gridDimension;
    myId = lobbyData.myId;
    skipResponse = lobbyData.broadcastInterval - 1;
    knowledgeMap.init(dim);
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

ResponseVariant Bot::BeDrunkInsideZone(const GameState& gameState) {
    std::random_device rd;
    std::mt19937 gen(rd());

    if (gen() % 2 == 0) {
        bool canMoveForward = canMoveForwardInsideZone(myPos);
        bool canMoveBackward = canMoveBackwardInsideZone(myPos);
        if (canMoveForward && canMoveBackward) {
            auto randomMove = static_cast<MoveDirection>(gen() % 2);
            return Move{randomMove};
        } else if (canMoveForward) {
            return Move{MoveDirection::forward};
        } else if (canMoveBackward) {
            return Move{MoveDirection::backward};
        }
    }

    auto randomRotation1 = static_cast<RotationDirection>(gen() % 2);
    auto randomRotation2 = static_cast<RotationDirection>(gen() % 2);
    return Rotate{randomRotation1, randomRotation2};
}

ResponseVariant Bot::NextMove(const GameState& gameState) {
    // clock_t start = clock();

    std::random_device rd;
    std::mt19937 gen(rd());

    if (gameState.time == 1) {
        onFirstNextMove(gameState);
    }

    auto lastPos = myPos;
    initMyTank(gameState);
    knowledgeMap.update(gameState);

    std::optional<ResponseVariant> response;

    response = shootIfWillFireHitForSure(gameState);
    if (response.has_value()) 
        return response.value();


    // TODO: jakieś gówno XD niech będzie na razie ale potem można wyjebać
    if (lastPos == myPos && gen() % 4 == 0) {
        response = shootIfSeeingEnemy(gameState);
        if (response.has_value())
            return response.value();
        return BeDrunkInsideZone(gameState);
    }

    response = dodgeIfNoAmmoAndWillBeHit(gameState);
    if (response.has_value()) 
        return response.value();

    auto bullet = closestBullet(gameState, myPos.pos);
    auto isOnBulletLine = [&](const OrientedPosition& oPos, int timer) {
        return oPos.pos.x != bullet.x && oPos.pos.y != bullet.y;
    };

    if (bullet.x != 1e9) {
        response = bfsStrategy(gameState, isOnBulletLine);
        if (response.has_value())
            return response.value();
    }

    response = dropMineIfReasonable(gameState);
    if (response.has_value()) 
        return response.value();

    response = useRadarIfPossible(gameState);
    if (response.has_value()) 
        return response.value();

    response = goForItem(gameState);
    if (response.has_value()) 
        return response.value();

    auto isZone = [&](const OrientedPosition& oPos, int timer) {
        return zoneName[oPos.pos.x][oPos.pos.y] != '?';
    };

    if (isZone(myPos, 0)) {
        response = shootIfSeeingEnemy(gameState, false, false);
        if (response.has_value())
            return response.value();

        response = rotateToEnemy(gameState);
        if (response.has_value()) {
            return response.value();
        }

        return BeDrunkInsideZone(gameState);
    }

    response = bfsStrategy(gameState, isZone);
    if (response.has_value())
        return response.value();
    
    // TODO: dziwny przypadek, jestśmy odizolowanie od wszystkich stref, można przemyśleć co z tym zrobić
    return BeDrunkInsideZone(gameState);
}

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
        }
    }
}

void Bot::initMyTankHelper(const GameState& gameState) {
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            for (const TileVariant& object : gameState.map.tiles[i][j].objects) {
                if (std::holds_alternative<Tank>(object)) {
                    const Tank& tank = std::get<Tank>(object);
                    if (tank.ownerId == myId) {
                        myTank = tank;
                        myPos = OrientedPosition{Position(i, j), tank.direction};
                        return;
                    }
                }
            }
        }
    }
}

void Bot::initMyTank(const GameState& gameState) {
    initMyTankHelper(gameState);
    myTurretDir = myTank.turret.direction;
    assert (myTank.turret.bulletCount.has_value());
    heldItem = myTank.secondaryItem.value_or(SecondaryItemType::unknown);
    myBulletCount = myTank.turret.bulletCount.value();
}

void Bot::onFirstNextMove(const GameState& gameState) {
    initIsWall(gameState.map.tiles);
    initZoneName(gameState.map.tiles);
}

bool Bot::canSeeEnemy(const GameState& gameState) const {
    int x = myPos.pos.x;
    int y = myPos.pos.y;
    int dir = getDirId(myTurretDir);

    auto [dx, dy] = Position::DIRECTIONS[dir];

    for (int i = 1; i < dim; ++i) {
        x += dx;
        y += dy;
        if (!isValid(Position(x, y), dim)) {
            break;
        }
        if (isWall[x][y]) {
            break;
        }
        if (!gameState.map.tiles[x][y].isVisible) {
            break;
        }
        for (const TileVariant& object : gameState.map.tiles[x][y].objects) {
            if (std::holds_alternative<Tank>(object)) {
                const Tank& tank = std::get<Tank>(object);
                if (tank.ownerId != myId) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Bot::willFireHitForSure(const GameState& gameState) const {
    int x = myPos.pos.x;
    int y = myPos.pos.y;
    int dir = getDirId(myTurretDir);

    auto [dx, dy] = Position::DIRECTIONS[dir];

    int bound = 2;
    if (heldItem == SecondaryItemType::Laser) {
        bound = dim;
    }

    for (int i = 1; i < bound; i++) {
        x += dx;
        y += dy;
        if (!isValid(Position(x, y), dim)) {
            break;
        }
        if (isWall[x][y]) {
            break;
        }
        if (!gameState.map.tiles[x][y].isVisible) {
            break;
        }
        for (const TileVariant& object : gameState.map.tiles[x][y].objects) {
            if (std::holds_alternative<Tank>(object)) {
                const Tank& tank = std::get<Tank>(object);
                if (tank.ownerId == myId) {
                    continue;
                }
                if (isParallel(myTurretDir, tank.direction)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Bot::canMoveForwardInsideZone(const OrientedPosition& pos) const {
    auto [dx, dy] = Position::DIRECTIONS[getDirId(pos.dir)];
    auto nextPos = pos;
    nextPos.pos.x += dx;
    nextPos.pos.y += dy;
    return isValid(nextPos.pos, dim) && !isWall[nextPos.pos.x][nextPos.pos.y] && zoneName[nextPos.pos.x][nextPos.pos.y] != '?';
}

bool Bot::canMoveBackwardInsideZone(const OrientedPosition& pos) const {
    auto [dx, dy] = Position::DIRECTIONS[getDirId(pos.dir)];
    auto nextPos = pos;
    nextPos.pos.x -= dx;
    nextPos.pos.y -= dy;
    return isValid(nextPos.pos, dim) && !isWall[nextPos.pos.x][nextPos.pos.y] && zoneName[nextPos.pos.x][nextPos.pos.y] != '?';
}

std::optional<ResponseVariant> Bot::dropMineIfPossible(const GameState& gameState) {
    if (heldItem != SecondaryItemType::Mine) {
        return std::nullopt;
    }

    auto [dx, dy] = Position::DIRECTIONS[getDirId(myPos.dir)];

    Position minePos = myPos.pos;
    minePos.x -= dx;
    minePos.y -= dy;

    if (!isValid(minePos, dim) || isWall[minePos.x][minePos.y]) {
        return std::nullopt;
    }

    knowledgeMap.notifyMine(gameState, minePos);

    std::cerr << "[EVENT] Dropping mine at " << minePos.x << " " << minePos.y << std::endl;

    return AbilityUse{AbilityType::dropMine};
}

std::optional<ResponseVariant> Bot::dropMineIfReasonable(const GameState& gameState) {
    if (heldItem == SecondaryItemType::Mine && (isBetweenWalls(myPos.pos, isWall, dim) || zoneName[myPos.pos.x][myPos.pos.y] != '?')) {
        auto [dx, dy] = Position::DIRECTIONS[getDirId(myPos.dir)];

        Position minePos = myPos.pos;
        minePos.x -= dx;
        minePos.y -= dy;

        if (!isValid(minePos, dim) || isWall[minePos.x][minePos.y]) {
            return std::nullopt;
        }

        knowledgeMap.notifyMine(gameState, minePos);

        return AbilityUse{AbilityType::dropMine};
    }

    return std::nullopt;
}

std::optional<ResponseVariant> Bot::useRadarIfPossible(const GameState& gameState) {
    if (heldItem == SecondaryItemType::Radar) {
        return AbilityUse{AbilityType::useRadar};
    }

    return std::nullopt;
}

std::optional<ResponseVariant> Bot::goForItem(const GameState& gameState) {
    if (knowWhereIs(Item{}, gameState) && heldItem == SecondaryItemType::unknown) {
        auto isItem = [&](const OrientedPosition& oPos, int timer) {
            int DOUBLE_BULLET_RANGE = 4;
            int OTHER_ITEM_RANGE = 10;
            for (const TileVariant& object : gameState.map.tiles[oPos.pos.x][oPos.pos.y].objects) {
                if (std::holds_alternative<Item>(object) && std::get<Item>(object).type == ItemType::mine) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                } else if (std::holds_alternative<Item>(object) && std::get<Item>(object).type == ItemType::radar) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                } else if (std::holds_alternative<Item>(object) && std::get<Item>(object).type == ItemType::doubleBullet) {
                    if (timer < DOUBLE_BULLET_RANGE) return true;
                } else if (std::holds_alternative<Item>(object) && std::get<Item>(object).type == ItemType::laser) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                }
            }
            for (auto object: knowledgeMap.tiles[oPos.pos.x][oPos.pos.y].objects) {
                if (std::holds_alternative<Item>(object.object) && std::get<Item>(object.object).type == ItemType::mine) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                } else if (std::holds_alternative<Item>(object.object) && std::get<Item>(object.object).type == ItemType::radar) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                } else if (std::holds_alternative<Item>(object.object) && std::get<Item>(object.object).type == ItemType::doubleBullet) {
                    if (timer < DOUBLE_BULLET_RANGE) return true;
                } else if (std::holds_alternative<Item>(object.object) && std::get<Item>(object.object).type == ItemType::laser) {
                    if (timer < OTHER_ITEM_RANGE) return true;
                }
            }
            return false;
        };
        return bfsStrategy(gameState, isItem);
    }
    return std::nullopt;
}

std::optional<ResponseVariant> Bot::shootIfSeeingEnemy(
        const GameState& gameState, 
        bool useLaserIfPossible, 
        bool useDoubleBulletIfPossible
) {
    return shootIf(gameState, [&](const GameState& gameState) {
        return canSeeEnemy(gameState);
    }, useLaserIfPossible, useDoubleBulletIfPossible);
}

std::optional<ResponseVariant> Bot::shootIfWillFireHitForSure(
        const GameState& gameState, 
        bool useLaserIfPossible, 
        bool useDoubleBulletIfPossible
) {
    return shootIf(gameState, [&](const GameState& gameState) {
        return willFireHitForSure(gameState);
    }, useLaserIfPossible, useDoubleBulletIfPossible);
}

std::optional<ResponseVariant> Bot::rotateToEnemy(const GameState& gameState) {
    auto isVisibleEnemy = [&](const OrientedPosition& pos, int timer) {
        if (!gameState.map.tiles[pos.pos.x][pos.pos.y].isVisible)
            return false;

        for (const TileVariant& object : gameState.map.tiles[pos.pos.x][pos.pos.y].objects) {
            if (std::holds_alternative<Tank>(object)) {
                const Tank& tank = std::get<Tank>(object);
                if (tank.ownerId != myId) {
                    return true;
                }
            }
        }

        return false;
    };

    auto isPotentialEnemy = [&](const OrientedPosition& pos, int timer) {
        if (gameState.map.tiles[pos.pos.x][pos.pos.y].isVisible)
            return false;

        for (const KnowledgeTileVariant& object : knowledgeMap.tiles[pos.pos.x][pos.pos.y].objects) {
            if (std::holds_alternative<Tank>(object.object)) {
                const Tank& tank = std::get<Tank>(object.object);
                if (tank.ownerId != myId) {
                    return true;
                }
            }
        }

        return false;
    };

    auto result = bfs(myPos, isVisibleEnemy);
    if (!result.has_value()) {
        result = bfs(myPos, isPotentialEnemy);
        if (!result.has_value()) {
            return std::nullopt;
        }
    }

    auto enemyPos = result.value().finalPos;

    auto dx = enemyPos.pos.x - myPos.pos.x;
    auto dy = enemyPos.pos.y - myPos.pos.y;

    Direction desiredTankDir;
    Direction desiredTurretDir;

    if (dx >= abs(dy)) {
        desiredTurretDir = Direction::down;
        if (dy >= 0) {
            desiredTankDir = Direction::right;
        } else {
            desiredTankDir = Direction::left;
        }
    } else if (dx <= -abs(dy)) {
        desiredTurretDir = Direction::up;
        if (dy >= 0) {
            desiredTankDir = Direction::right;
        } else {
            desiredTankDir = Direction::left;
        }
    } else if (dy >= abs(dx)) {
        desiredTurretDir = Direction::right;
        if (dx >= 0) {
            desiredTankDir = Direction::down;
        } else {
            desiredTankDir = Direction::up;
        }
    } else if (dy <= -abs(dx)) {
        desiredTurretDir = Direction::left;
        if (dx >= 0) {
            desiredTankDir = Direction::down;
        } else {
            desiredTankDir = Direction::up;
        }
    }

    auto tankRot = getRotationTo(myPos.dir, desiredTankDir);
    auto turretRot = getRotationTo(myTurretDir, desiredTurretDir);

    if (tankRot == RotationDirection::none && turretRot == RotationDirection::none) {
        std::random_device rd;
        std::mt19937 gen(rd());

        if (gen() % 4 != 0) {
            auto nxtPos = afterMove(myPos, MoveDirection::forward);
            if (isValid(nxtPos.pos, dim) && !isWall[nxtPos.pos.x][nxtPos.pos.y]) {
                return Move{MoveDirection::forward};
            }
            return BeDrunkInsideZone(gameState);
        }

        auto nxtPos = afterMove(myPos, MoveDirection::backward);
        switch (gen() % 3) {
            case 0:
                if (isValid(nxtPos.pos, dim) && !isWall[nxtPos.pos.x][nxtPos.pos.y]) {
                    return Move{MoveDirection::backward};
                }
                return BeDrunkInsideZone(gameState);
            case 1:
                return Wait{};
            case 2:
                return BeDrunkInsideZone(gameState);
        }
    }

    return Rotate{tankRot, turretRot};
}

bool Bot::willBeHitByBullet(const GameState& gameState, const OrientedPosition& pos) const {
    int x = pos.pos.x;
    int y = pos.pos.y;
    return knowledgeMap.willBeHitByBulletInNextMove(x, y);
}

bool Bot::knowWhereIs(const TileVariant& object, const GameState& gamestate) const {
    for (auto row: gamestate.map.tiles) {
        for (auto tile: row) {
            for (auto obj: tile.objects) {
                if (obj.index() == object.index()) {
                    if (std::holds_alternative<Tank>(obj)) {
                        if (std::get<Tank>(obj).ownerId != myId) {
                            return true;
                        }
                    } else {
                        return true;
                    }
                }
            }
        }
    }
    for (auto row: knowledgeMap.tiles) {
        for (auto tile: row) {
            for (auto obj: tile.objects) {
                if (obj.object.index() == object.index()) {
                    if (std::holds_alternative<Tank>(obj.object)) {
                        if (std::get<Tank>(obj.object).ownerId != myId) {
                            return true;
                        }
                    } else {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

std::optional<ResponseVariant> Bot::dodgeIfNoAmmoAndWillBeHit(const GameState& gameState) {
    for (int i = 0; i < 4; i++) {
        auto [dx, dy] = Position::DIRECTIONS[i];

        if (i == getDirId(myTurretDir)) {
            if (myBulletCount > 0) {
                continue;
            }
            if (heldItem == SecondaryItemType::Laser) {
                continue;
            }
            if (heldItem == SecondaryItemType::DoubleBullet) {
                continue;
            }
        }

        for (int j = 1; j <= 2; j++) {
            auto nextPos = myPos;
            nextPos.pos.x += j * dx;
            nextPos.pos.y += j * dy;

            if (!isValid(nextPos.pos, dim)) {
                break;
            }

            if (isWall[nextPos.pos.x][nextPos.pos.y]) {
                break;
            }

            for (const TileVariant& object : gameState.map.tiles[nextPos.pos.x][nextPos.pos.y].objects) {
                if (std::holds_alternative<Tank>(object)) {
                    const Tank& tank = std::get<Tank>(object);
                    if (tank.ownerId != myId) {
                        if (getDirId(tank.turret.direction) == (i + 2) % 4) {
                            if (!isParallel(myPos.dir, tank.turret.direction)) {
                                for (auto mov : {MoveDirection::forward, MoveDirection::backward}) {
                                    auto nxtPos = afterMove(myPos, mov);
                                    if (isValid(nxtPos.pos, dim) 
                                     && !isWall[nxtPos.pos.x][nxtPos.pos.y]
                                     && !willBeHitByBullet(gameState, nxtPos)
                                     && !knowledgeMap.containsMine(nxtPos.pos)) 
                                    {
                                        return Move{mov};
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return std::nullopt;
}
