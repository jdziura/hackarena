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

    LobbyData lobbyData;
    int dim;
    std::vector<std::vector<bool>> isWall;
    std::vector<std::vector<char>> zoneName;
    std::vector<GameState> statesSnapshots;
    OrientedPosition myPos;
    Item heldItem = Item{ItemType::unknown};

    void onFirstNextMove(const GameState& gameState);
    void initIsWall(const std::vector<std::vector<Tile>>& tiles);
    void initZoneName(const std::vector<std::vector<Tile>>& tiles);
    void initMyPos(const GameState& gameState);

    bool haveItem() {
        return heldItem.type != ItemType::unknown;
    }

    template<class F>
    std::optional<MoveOrRotation> bfs(const OrientedPosition& start, F&& f) {
        std::queue<OrientedPosition> q;
        std::vector<std::vector<std::vector<bool>>> visited(dim, std::vector<std::vector<bool>>(dim, std::vector<bool>(4, false)));
        std::vector<std::vector<std::vector<MoveOrRotation>>> from(dim, std::vector<std::vector<MoveOrRotation>>(dim, std::vector<MoveOrRotation>(4))); // i, j, dir
        q.push(start);
        visited[start.pos.x][start.pos.y][getDirId(start.dir)] = true;

        bool found = false;
        OrientedPosition finish;

        while (!q.empty()) {
            OrientedPosition pos = q.front();
            q.pop();

            if (f(pos)) {
                found = true;
                finish = pos;
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

                if (visited[x][y][dir]) {
                    continue;
                }

                visited[x][y][dir] = true;
                from[x][y][dir] = reversed(move);
                q.push(nextPos);
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
        return lastMove;
    }
};
