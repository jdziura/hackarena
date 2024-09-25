#include "pch.h"

#pragma once

/// First received list of players
struct LobbyPlayer {
	std::string id;
	std::string nickname;
	uint32_t color;
};

/// Player received at game end
struct EndGamePlayer {
    std::string id;
    std::string nickname;
    uint32_t color;
    int score;
};

struct EndGameLobby {
    std::vector<EndGamePlayer> players;
};

struct LobbyData {
	std::string myId;
	std::vector<LobbyPlayer> players;
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
    bool isVisible;
    char zoneName; // '?' or 63 for no zone
};

enum class BulletDirection {
    up = 0,
    right = 1,
    down = 2,
    left = 3
};

/// BulletPayload struct
struct Bullet {
	int id;
	double speed;
    BulletDirection direction;
    bool isVisible;
    char zoneName; // '?' or 63 for no zone
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
	char name;
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

struct Wall {
    char zoneName; // '?' or 63 for no zone
};

struct None {
    bool isVisible;
    char zoneName; // '?' or 63 for no zone
};

using TileVariant = std::variant<Wall, Tank, Bullet, None>;

/// Map struct:
/// Tiles are stored in a 2D array
/// Inner array represents columns of the map
/// Outer arrays represent rows of the map
/// Item with index [0][0] represents top-left corner of the map
struct Map {
	/// A 3D vector to hold variants of tile objects
	std::vector<std::vector<TileVariant>> tiles;
	std::vector<Zone> zones;
    /// 2D array of chars ('0' or '1') same as tiles
	std::vector<std::vector<char>> visibility;
};

/// GameState struct
struct GameState {
    /// tick number
	int time;
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