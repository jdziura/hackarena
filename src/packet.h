#pragma once
#include <nlohmann/json.hpp>

enum class PacketType {
	Unknown = 0,
	Ping = 17,
	Pong = 18,
	GameStart = 49,
	TankMovement = 65,
	TankRotation = 66,
	TankShoot = 67,
	GameState = 50,
	LobbyData = 33,
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

