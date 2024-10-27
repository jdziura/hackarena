#pragma once

#include <set>

#include "../processed-packets.h"

constexpr inline int getDirId(const Direction& dir) {
    return static_cast<std::underlying_type<Direction>::type>(dir);
}

constexpr inline int getRotDirId(const RotationDirection& dir) {
    return static_cast<std::underlying_type<RotationDirection>::type>(dir);
}

constexpr inline int getZoneId(char c) {
    return c - 'A';
}

constexpr inline Direction getBackwardDir(const Direction& dir) {
    return static_cast<Direction>((getDirId(dir) + 2) % 4);
}

inline void rotate(Direction& dir, const RotationDirection& rotDir) {
    assert(rotDir != RotationDirection::none);

    if (rotDir == RotationDirection::left) {
        dir = static_cast<Direction>((getDirId(dir) + 3) % 4);
    } else if (rotDir == RotationDirection::right) {
        dir = static_cast<Direction>((getDirId(dir) + 1) % 4);
    }
}

inline Direction rotated(Direction dir, const RotationDirection& rotDir) {
    rotate(dir, rotDir);
    return dir;
}

struct Position {
    static constexpr std::pair<int, int> DIRECTIONS[] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
    int x, y;

    Position(int _x, int _y) : x(_x), y(_y) {}

    Position() : Position(0, 0) {}

    auto operator<=>(const Position&) const = default;

    void move(const Direction& dir) {
        const auto idx = getDirId(dir);
        const auto &[dx, dy] = DIRECTIONS[idx];
        x += dx;
        y += dy;
    }
};

using MoveOrRotation = std::variant<MoveDirection, RotationDirection>;

constexpr inline MoveOrRotation reversed(const MoveOrRotation& action) {
    if (std::holds_alternative<MoveDirection>(action)) {
        return std::get<MoveDirection>(action) == MoveDirection::forward ? MoveDirection::backward : MoveDirection::forward;
    } else {
        return std::get<RotationDirection>(action) == RotationDirection::left ? RotationDirection::right : RotationDirection::left;
    }
}

constexpr MoveOrRotation ALL_ACTIONS[] = {
    MoveDirection::forward, 
    MoveDirection::backward, 
    RotationDirection::left, 
    RotationDirection::right
};

constexpr inline bool isValid(const Position& pos, int dim) {
    return pos.x >= 0 && pos.x < dim && pos.y >= 0 && pos.y < dim;
}

struct OrientedPosition {
    Position pos;
    Direction dir;

    OrientedPosition(const Position& _pos, const Direction& _dir) : pos(_pos), dir(_dir) {}

    OrientedPosition() : OrientedPosition(Position(), Direction::up) {}

    auto operator<=>(const OrientedPosition&) const = default;

    MoveOrRotation getMoveFollowing(const Direction& moveDir) {
        auto backwardDir = getBackwardDir(dir);

        if (moveDir == dir) {
            return MoveDirection::forward;
        } else if (moveDir == backwardDir) {
            return MoveDirection::backward;
        } else {
            return (getDirId(dir) + 1) % 4 == getDirId(moveDir) ? RotationDirection::right : RotationDirection::left;
        }
    }

    void move(const MoveDirection& moveDir) {
        if (moveDir == MoveDirection::forward) {
            pos.move(dir);
        } else {
            pos.move(getBackwardDir(dir));
        }
    }

    void rotate(const RotationDirection& rotDir) {
        ::rotate(dir, rotDir);
    }

    void move(const MoveOrRotation& action) {
        if (std::holds_alternative<MoveDirection>(action)) {
            move(std::get<MoveDirection>(action));
        } else {
            rotate(std::get<RotationDirection>(action));
        }
    }
};

constexpr inline bool isValid(const OrientedPosition& pos, int dim) {
    return isValid(pos.pos, dim);
}

inline Position afterMove(Position pos, const Direction& dir) {
    pos.move(dir);
    return pos;
}

inline OrientedPosition afterMove(OrientedPosition pos, const MoveOrRotation& action) {
    pos.move(action);
    return pos;
}

constexpr inline bool isParallel(const Direction& dir1, const Direction& dir2) {
    int id1 = getDirId(dir1);
    int id2 = getDirId(dir2);
    return id1 == id2 || id1 == (id2 + 2) % 4;
}

struct KnowledgeTileVariant {
    int lastSeen;
    TileVariant object;
    auto operator<=>(const KnowledgeTileVariant&) const = default;
};

struct KnowledgeTileVariantComparator {
    bool operator()(const KnowledgeTileVariant& lhs, const KnowledgeTileVariant& rhs) const {
        return lhs.lastSeen < rhs.lastSeen;
    }
};

struct KnowledgeTile {
    std::set<KnowledgeTileVariant, KnowledgeTileVariantComparator> objects;
    auto operator<=>(const KnowledgeTile&) const = default;
};

struct KnowledgeMap {
    static constexpr int MAX_TRACK_TIME = 5;

    std::vector<std::vector<KnowledgeTile>> tiles;

    void init(int dim) {
        tiles = std::vector<std::vector<KnowledgeTile>>(dim, std::vector<KnowledgeTile>(dim));
    }

    void update(const GameState& gameState) {
        std::vector<std::pair<Bullet, Position>> bullets;

        for (int i = 0; i < gameState.map.tiles.size(); ++i) {
            for (int j = 0; j < gameState.map.tiles[i].size(); ++j) {
                if (gameState.map.tiles[i][j].isVisible) {
                    tiles[i][j].objects.clear();
                    for (const TileVariant& object : gameState.map.tiles[i][j].objects) {
                        if (!std::holds_alternative<Wall>(object)) {
                            tiles[i][j].objects.insert({gameState.time, object});
                        }
                    }
                }
                else {
                    for (auto it = tiles[i][j].objects.begin(); it != tiles[i][j].objects.end();) {
                        if (std::holds_alternative<Bullet>(it->object)) {
                            bullets.emplace_back(std::get<Bullet>(it->object), Position(i, j));
                            it = tiles[i][j].objects.erase(it);
                        } else if (gameState.time - it->lastSeen > MAX_TRACK_TIME) {
                            it = tiles[i][j].objects.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }

        for (auto [bullet, pos] : bullets) {
            auto [dx, dy] = Position::DIRECTIONS[getDirId(bullet.direction)];
            for (int i = 0; i < 2; i++) {
                pos.x += dx;
                pos.y += dy;
                if (!isValid(pos, tiles.size())) {
                    break;
                }
                tiles[pos.x][pos.y].objects.insert({gameState.time, bullet});
            }
        }
    }
};

inline Position closestBullet(const GameState& gameState, const Position& myPos) {
    Position closestBulletPos = Position(1e9, 1e9);
    double closestBulletDist = 1e9;

    // Position result = Position(1e9, 1e9);
    for (int i = 0; i < myPos.x; i++) {
        if (gameState.map.tiles[i][myPos.y].objects.size() > 0) {
            for (const auto& obj : gameState.map.tiles[i][myPos.y].objects) {
                if (std::holds_alternative<Bullet>(obj)) {
                    const auto& bullet = std::get<Bullet>(obj);
                    if (bullet.direction == Direction::down) {
                        if (myPos.x - i < closestBulletDist) {
                            closestBulletDist = myPos.x - i;
                            closestBulletPos = Position(i, myPos.y);
                        }
                    }
                }
            }
        }
    }
    for (int i = myPos.x + 1; i < gameState.map.tiles.size(); i++) {
        if (gameState.map.tiles[i][myPos.y].objects.size() > 0) {
            for (const auto& obj : gameState.map.tiles[i][myPos.y].objects) {
                if (std::holds_alternative<Bullet>(obj)) {
                    const auto& bullet = std::get<Bullet>(obj);
                    if (bullet.direction == Direction::up) {
                        if (i - myPos.x < closestBulletDist) {
                            closestBulletDist = i - myPos.x;
                            closestBulletPos = Position(i, myPos.y);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < myPos.y; i++) {
        if (gameState.map.tiles[myPos.x][i].objects.size() > 0) {
            for (const auto& obj : gameState.map.tiles[myPos.x][i].objects) {
                if (std::holds_alternative<Bullet>(obj)) {
                    const auto& bullet = std::get<Bullet>(obj);
                    if (bullet.direction == Direction::right) {
                        if (myPos.y - i < closestBulletDist) {
                            closestBulletDist = myPos.y - i;
                            closestBulletPos = Position(myPos.x, i);
                        }
                    }
                }
            }
        }
    }
    for (int i = myPos.y + 1; i < gameState.map.tiles[0].size(); i++) {
        if (gameState.map.tiles[myPos.x][i].objects.size() > 0) {
            for (const auto& obj : gameState.map.tiles[myPos.x][i].objects) {
                if (std::holds_alternative<Bullet>(obj)) {
                    const auto& bullet = std::get<Bullet>(obj);
                    if (bullet.direction == Direction::left) {
                        if (i - myPos.y < closestBulletDist) {
                            closestBulletDist = i - myPos.y;
                            closestBulletPos = Position(myPos.x, i);
                        }
                    }
                }
            }
        }
    }

    return closestBulletPos;
}

inline bool isOnBulletLine(Position bullet, Position myPos) {
    return bullet.x == myPos.x || bullet.y == myPos.y;
}


inline ResponseVariant RotateInDirection(const Direction& myDir, const Direction& targetDir) {
    if ((getDirId(myDir) - getDirId(targetDir)) % 4 == 3) {
        return Rotate{RotationDirection::none, RotationDirection::left};
    } else if ((getDirId(targetDir) - getDirId(myDir)) % 4 == 0) {
        return Wait{};
    } else {
        return Rotate{RotationDirection::none, RotationDirection::right};
    }
}
