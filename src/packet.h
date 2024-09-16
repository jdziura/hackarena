#pragma once
#include <nlohmann/json.hpp>

enum class PacketType {
	Unknown = 0,
	Ping = 1,
	Pong = 2,
	TankMovement = 11,
	TankRotation = 12,
	TankShoot = 13,
	GameState = 21,
	LobbyData = 31,
	Ready = 102,
	GameEnded = 103
};

struct Packet {
	PacketType packet_type;
	nlohmann::json payload;

	static Packet ConstructPongPacket() {
		return { PacketType::Pong, nlohmann::json{} };
	}
};

