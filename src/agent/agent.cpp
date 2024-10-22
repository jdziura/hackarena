#include "agent.h"
#include <utility>

void Agent::PrintMap(const std::vector<std::vector<Tile>>& tiles) {
    auto rows = tiles.size();
    auto cols = tiles[0].size();

    std::vector<std::vector<std::string>> map(rows, std::vector<std::string>(cols, " "));

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            for (const TileVariant& object : tiles[i][j].objects) {

				if (std::holds_alternative<Wall>(object)) {
					map[i][j] = '#';
				} else if (std::holds_alternative<Tank>(object)) {
					if (auto tankPtr = std::get_if<Tank>(&object)) {
						auto tankId = tankPtr->ownerId;
						if(tankId!=myId) {
							if(tankPtr->direction==Direction::up) { map[i][j] = "T^"; }
							else if(tankPtr->direction==Direction::down) { map[i][j] = "Tv"; }
							else if(tankPtr->direction==Direction::left) { map[i][j] = "T<"; }
							else if(tankPtr->direction==Direction::right) { map[i][j] = "T>"; }
						}
						else {
							if(tankPtr->direction==Direction::up) { map[i][j] = "@^"; }
							else if(tankPtr->direction==Direction::down) { map[i][j] = "@v"; }
							else if(tankPtr->direction==Direction::left) { map[i][j] = "@<"; }
							else if(tankPtr->direction==Direction::right) { map[i][j] = "@>"; }
						}
					}
				} else if (std::holds_alternative<None>(object)) {
					if (tiles[i][j].zoneName!='?'){
						map[i][j] = tiles[i][j].zoneName;
						std::cout << "name " << tiles[i][j].zoneName << std::endl;
					} else if (tiles[i][j].isVisible) {
						std::cout << "vis " << tiles[i][j].isVisible << std::endl;
						map[i][j] = '.';
					}
					else map[i][j] = ' ';
				} else if (std::holds_alternative<Item>(object)) {
					if (auto itemPtr = std::get_if<Item>(&object)) {
						if(itemPtr->type==ItemType::radar) { map[i][j] = 'R'; }
						else if(itemPtr->type==ItemType::doubleBullet) { map[i][j] = 'D'; }
						else if(itemPtr->type==ItemType::mine) { map[i][j] = 'M'; }
						else if(itemPtr->type==ItemType::laser) { map[i][j] = 'L'; }
					}
				} else if (std::holds_alternative<Mine>(object)) {
					map[i][j] = 'X';
				} else if (std::holds_alternative<Laser>(object)) {
					if (auto laserPtr = std::get_if<Laser>(&object)) {
						if(laserPtr->orientation==LaserOrientation::horizontal) { map[i][j] = '-'; }
						else { map[i][j] = '!'; }
					}
				} else if (std::holds_alternative<Bullet>(object)) {
					if (auto bulletPtr = std::get_if<Bullet>(&object)) {
						if(bulletPtr->type==BulletType::bullet){
							if(bulletPtr->direction==Direction::up) { map[i][j] = "^"; }
							else if(bulletPtr->direction==Direction::down) { map[i][j] = "v"; }
							else if(bulletPtr->direction==Direction::left) { map[i][j] = "<"; }
							else if(bulletPtr->direction==Direction::right) { map[i][j] = ">"; }
						}
						else{
							if(bulletPtr->direction==Direction::up) { map[i][j] = "^^"; }
							else if(bulletPtr->direction==Direction::down) { map[i][j] = "vv"; }
							else if(bulletPtr->direction==Direction::left) { map[i][j] = "<<"; }
							else if(bulletPtr->direction==Direction::right) { map[i][j] = ">>"; }
						}
					}
				}
                break;
            }
        }
    }

    for (const auto& row : map) {
        for (const auto& tile : row) {
            std::cout << tile << ' ';
        }
        std::cout << '\n';
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
