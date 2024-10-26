#pragma once

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