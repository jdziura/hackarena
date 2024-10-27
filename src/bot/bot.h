#pragma once

#include "../processed-packets.h"
#include "utils.h"

#include <vector>


class Bot {
 public:
    /// DO NOT DELETE
    /// time in milliseconds after which the NextMove() answer is not sent to server, CAN BE CHANGED WHENEVER YOU WANT
    int skipResponse = 99;
	std::string myId;

	Bot();
	void Init(const LobbyData& _lobbyData);
	ResponseVariant NextMove(const GameState& gameState);
	void OnGameEnded(const EndGameLobby& endGameLobby);
    void OnWarningReceived(WarningType warningType, std::optional<std::string>& message);
    void OnGameStarting();
	void PrintMap(const std::vector<std::vector<Tile>>& tiles);

    /// END

    ResponseVariant RandomMove(const GameState& gameState);
    ResponseVariant BeDrunkInsideZone(const GameState& gameState);

    LobbyData lobbyData;
    int dim;
    KnowledgeMap knowledgeMap;
    std::vector<std::vector<bool>> isWall;
    std::vector<std::vector<char>> zoneName;
    // std::vector<GameState> statesSnapshots;
    Tank myTank;
    OrientedPosition myPos;
    Direction myTurretDir;
    int myBulletCount = 3;
    SecondaryItemType heldItem = SecondaryItemType::unknown;

    // strategies
    std::optional<ResponseVariant> dropMineIfPossible(const GameState& gameState);
    std::optional<ResponseVariant> dropMineIfReasonable(const GameState& gameState);
    std::optional<ResponseVariant> useRadarIfPossible(const GameState& gameState);

    std::optional<ResponseVariant> goForItem(const GameState& gameState);

    std::optional<ResponseVariant> shootIfSeeingEnemy(
        const GameState& gameState, 
        bool useLaserIfPossible = true, 
        bool useDoubleBulletIfPossible = true
    );

    std::optional<ResponseVariant> shootIfWillFireHitForSure(
        const GameState& gameState, 
        bool useLaserIfPossible = true, 
        bool useDoubleBulletIfPossible = true
    );

    std::optional<ResponseVariant> dodgeIfNoAmmoAndWillBeHit(const GameState& gameState);

    std::optional<ResponseVariant> rotateToEnemy(const GameState& gameState);

    void onFirstNextMove(const GameState& gameState);
    void initIsWall(const std::vector<std::vector<Tile>>& tiles);
    void initZoneName(const std::vector<std::vector<Tile>>& tiles);
    void initMyTank(const GameState& gameState);
    void initMyTankHelper(const GameState& gameState);

    bool canSeeEnemy(const GameState& gameState) const;
    bool willFireHitForSure(const GameState& gameState) const;
    bool willBeHitByBullet(const GameState& gameState, const OrientedPosition& pos) const;

    bool canMoveForwardInsideZone(const OrientedPosition& pos) const;
    bool canMoveBackwardInsideZone(const OrientedPosition& pos) const;
    bool knowWhereIs(const TileVariant& object, const GameState& gamestate) const;

    struct BfsResult {
        MoveOrRotation move;
        OrientedPosition finalPos;
        int eta;
    };

    template<class F>
    std::optional<BfsResult> bfs(const OrientedPosition& start, F&& f) {
        std::queue<std::pair<OrientedPosition, int>> q;
        std::vector<std::vector<std::vector<bool>>> visited(dim, std::vector<std::vector<bool>>(dim, std::vector<bool>(4, false)));
        std::vector<std::vector<std::vector<MoveOrRotation>>> from(dim, std::vector<std::vector<MoveOrRotation>>(dim, std::vector<MoveOrRotation>(4))); // i, j, dir
        q.push({start, 0});
        visited[start.pos.x][start.pos.y][getDirId(start.dir)] = true;

        bool found = false;
        OrientedPosition finish;
        int eta = -1;

        while (!q.empty()) {
            auto [pos, timer] = q.front();
            q.pop();

            if (f(pos, timer)) {
                found = true;
                finish = pos;
                eta = timer;
                break;
            }

            for (const MoveOrRotation &move : ALL_ACTIONS) {
                auto nextPos = afterMove(pos, move);
                if (!isValid(nextPos, dim)) {
                    continue;
                }

                int x = nextPos.pos.x;
                int y = nextPos.pos.y;
                int dir = getDirId(nextPos.dir);

                if (isWall[x][y]) {
                    continue;
                }

                if (knowledgeMap.containsMine(nextPos.pos)) {
                    continue;
                }

                if (knowledgeMap.isOnBulletTraj(x, y)) {
                    continue;
                }

                if (visited[x][y][dir]) {
                    continue;
                }

                visited[x][y][dir] = true;
                from[x][y][dir] = reversed(move);
                q.push({nextPos, timer + 1});
            }
        }

        if (!found) {
            return std::nullopt;
        }

        OrientedPosition cur = finish;
        MoveOrRotation lastMove = from[cur.pos.x][cur.pos.y][getDirId(cur.dir)];

        while (cur != start) {
            lastMove = from[cur.pos.x][cur.pos.y][getDirId(cur.dir)];
            cur.move(lastMove);
        }

        lastMove = reversed(lastMove);
        return BfsResult{lastMove, finish, eta};
    }

    template<class F>
    std::optional<ResponseVariant> shootIf(
            const GameState& gameState, 
            F&& f, 
            bool useLaserIfPossible = true, 
            bool useDoubleBulletIfPossible = true
    ) {
        bool ignoreBulletCount = (heldItem == SecondaryItemType::DoubleBullet && useDoubleBulletIfPossible);
        ignoreBulletCount |=     (heldItem == SecondaryItemType::Laser && useLaserIfPossible);

        if (myBulletCount == 0 && !ignoreBulletCount)
        {
            return std::nullopt;
        }

        if (!f(gameState)) {
            return std::nullopt;
        }

        if (useLaserIfPossible && heldItem == SecondaryItemType::Laser) {
            return AbilityUse{AbilityType::useLaser};
        }

        if (useDoubleBulletIfPossible && heldItem == SecondaryItemType::DoubleBullet) {
            return AbilityUse{AbilityType::fireDoubleBullet};
        }
        
        return AbilityUse{AbilityType::fireBullet};
    }

    template<class F>
    std::optional<ResponseVariant> bfsStrategy(const GameState& gameState, F&& f) {
        auto result = bfs(myPos, f);
        if (!result) {
            return std::nullopt;
        }

        auto nxtMove = result->move;
        auto eta = result->eta;
        auto nextPos = afterMove(myPos, nxtMove);

        bool hitNow = willBeHitByBullet(gameState, myPos);
        bool hitNxt = willBeHitByBullet(gameState, nextPos);

        if (hitNxt && !hitNow) {
            // TODO: wait or think about rotating?
            return Wait{};
        }

        if (std::holds_alternative<MoveDirection>(nxtMove)) {
            return Move{std::get<MoveDirection>(nxtMove)};
        } else {
            return Rotate{std::get<RotationDirection>(nxtMove), RotationDirection::none};
        }
    }
};
