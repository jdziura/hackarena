#pragma once
#include <nlohmann/json.hpp>

enum class PacketType {
	Unknown = 0,
	Ping = 17,
	Pong = 18,
	GameStart = 49,
	TankMovement = 73,
	TankRotation = 74,
	TankShoot = 75,
	GameState = 58,
	LobbyData = 41,
	Ready = 102,
	GameEnded = 59
};

struct Packet {
	PacketType packetType;
	nlohmann::json payload;
};

