#include "pch.h"

#pragma once

/// First received list of players
struct LobbyPlayers {
	std::string id;
	std::string nickname;
	uint32_t color;
};

struct LobbyData {
	std::string myId;
	std::vector<LobbyPlayers> players;
	int gridDimension;
	int numberOfPlayers;
	int seed;
    /// how many milliseconds in tick
	int broadcastInterval;
	bool eagerBroadcast;
};

/// Turret struct for tanks
struct Turret {
	int direction;
    /// Not present in enemies
	std::optional<int> bulletCount;
    /// Not present in enemies
	std::optional<int> ticksToRegenBullet;
};

/// TankPayload struct
struct Tank {
	std::string ownerId;
	int direction;
	Turret turret;
    /// Not present in enemies
	std::optional<int> health;
};

/// BulletPayload struct
struct Bullet {
	int id;
	double speed;
	int direction;
};

/// ZoneStatus struct to represent various zone states
struct ZoneStatus {
	std::string type;
    /// Used in "beingCaptured" and "beingRetaken"
	std::optional<int> remainingTicks;
    /// Used in "beingCaptured" and "captured"
	std::optional<std::string> playerId;
    /// Used in "beingContested" and "beingRetaken"
	std::optional<std::string> capturedById;
    /// Used in "beingRetaken"
	std::optional<std::string> retakenById;
};

/// Zone struct to represent a zone on the map
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
    /// Not present in enemies
	std::optional<int> score;
    /// Optional because it might be null
	std::optional<int> ticksToRegen;
};

struct Wall {};

using TileVariant = std::variant<Wall, Tank, Bullet>;

/// Map struct:
/// Tiles are stored in a 2D array
/// where each tile is a list of items in this tile
/// Currently there can be at most one item in a tile
/// Outer array represents columns of the map
/// Inner arrays represent rows of the map
/// Item with index [0][0] represents top-left corner of the map
struct Map {
	/// A 3D vector to hold variants of tile objects
	std::vector<std::vector<std::vector<TileVariant>>> tiles;
	std::vector<Zone> zones;
    /// 2D array of chars ('0' or '1') same as tiles
	std::vector<std::vector<char>> visibility;
};

/// GameState struct
struct GameState {
	std::string playerId;
	int time; /// tick number
	std::vector<Player> players;
	Map map;
};

enum class RotationDirection {
	left = 0,
	right = 1
};

enum class MoveDirection {
	forward = 0,
	backward = 1
};

struct Rotate {
	RotationDirection tankRotation;
	RotationDirection turretRotation;
};

struct Move {
	MoveDirection direction;
};

struct Shoot {};

struct Wait {};

using ResponseVariant = std::variant<Rotate, Move, Shoot, Wait>;