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
struct Tank {
	std::string ownerId;
	int direction;
	Turret turret;
	std::optional<int> health; // Optional because it's not always present
};

// BulletPayload struct
struct Bullet {
	int id;
	double speed;
	int direction;
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

// Player struct
struct Player {
	std::string id;
	std::string nickname;
	uint32_t color;
	int ping;
	std::optional<int> score;  // Optional because it's not always present
	std::optional<double> regenProgress; // Optional because it might be null
};

struct Wall {
	// No additional properties needed for walls in this case
};

// Define a variant to hold any type of tile object
using TileVariant = std::variant<Wall, Tank, Bullet>;

// Map struct
struct Map {
	// A 3D vector to hold variants of tile objects
	std::vector<std::vector<std::vector<TileVariant>>> tiles;
	std::vector<Zone> zones;
	std::vector<std::vector<char>> visibility; // 2D array of chars ('0' or '1')
};

// GameState struct
struct GameState {
	std::string playerId;
	double time;
	std::vector<Player> players;
	Map map;
};