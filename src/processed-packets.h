#include "pch.h"

#pragma once

// First received list of players
struct LobbyPlayers {
	std::string id;
	std::string nickname;
	uint32_t color;
};

struct LobbyData {
	std::string myId;
	std::vector<LobbyPlayers> players;
	int gridDimension;
	int seed;
	int broadcastInterval;
};

// Turret struct for tanks
struct Turret {
	int direction;
	std::optional<int> bulletCount; // Optional because it's not always present
	std::optional<double> bulletRegenProgress; // Optional because it might be null
};

// TankPayload struct
struct TankPayload {
	std::string ownerId;
	int direction;
	Turret turret;
	std::optional<int> health; // Optional because it's not always present
};

// BulletPayload struct
struct BulletPayload {
	int id;
	double speed;
	int direction;
};

// TileObject struct to represent different objects that can appear on a tile
struct TileObject {
	std::string type;
	std::optional<TankPayload> tankPayload; // Optional because it's only present for "tank"
	std::optional<BulletPayload> bulletPayload; // Optional because it's only present for "bullet"
};

// ZoneStatus struct to represent various zone states
struct ZoneStatus {
	std::string type;
	std::optional<int> remainingTicks; // Used in "beingCaptured" and "beingRetaken"
	std::optional<std::string> playerId; // Used in "beingCaptured" and "captured"
	std::optional<std::string> capturedById; // Used in "beingContested" and "beingRetaken"
	std::optional<std::string> retakenById; // Used in "beingRetaken"
};

// Zone struct to represent a zone on the map
struct Zone {
	int x;
	int y;
	int width;
	int height;
	int index;
	ZoneStatus status;
};

// Map struct
struct Map {
	std::vector<std::vector<std::vector<TileObject>>> tiles; // 3D array to represent the grid (columns, rows, items in tile)
	std::vector<Zone> zones;
	std::vector<std::vector<char>> visibility; // 2D array of chars ('0' or '1') map.visibility[row][column]
};

// Player struct
struct Player {
	std::string id;
	std::string nickname;
	uint32_t color;
	int ping;
	std::optional<int> score;  // Optional because it's not always present
	std::optional<double> regenProgress; // Optional because it might be null
};

// GameState struct
struct GameState {
	std::string playerId;
	double time;
	std::vector<Player> players;
	Map map;
};
