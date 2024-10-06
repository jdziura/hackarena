#pragma once
#include <nlohmann/json.hpp>

enum class PacketType {
    // Mask for packet type indicating that it has a payload
    HasPayload = 0x8,

    // Communication group (range: 0x10 - 0x1F)
    CommunicationGroup = 0x10,
    Ping = CommunicationGroup | 0x1,
    Pong = CommunicationGroup | 0x2,
    ConnectionAccepted = CommunicationGroup | 0x3,
    ConnectionRejected = CommunicationGroup | HasPayload | 0x4,

    // Lobby group (range: 0x20 - 0x2F)
    LobbyGroup = 0x20,
    LobbyData = LobbyGroup | HasPayload | 0x1,
    LobbyDeleted = LobbyGroup | 0x2,

    // GameState group (range: 0x30 - 0x3F)
    GameStateGroup = 0x30,
    GameStart = GameStateGroup | 0x1,
    GameState = GameStateGroup | HasPayload | 0x2,
    GameEnded = GameStateGroup | HasPayload | 0x3,

    // Player response group (range: 0x40 - 0x4F)
    PlayerResponseActionGroup = 0x40,
    TankMovement = PlayerResponseActionGroup | HasPayload | 0x1,
    TankRotation = PlayerResponseActionGroup | HasPayload | 0x2,
    TankShoot = PlayerResponseActionGroup | HasPayload | 0x3,
    ResponsePass = PlayerResponseActionGroup | HasPayload | 0x7,

    // Warning group (range: 0xE0 - 0xEF)
    WarningGroup = 0xE0,
    CustomWarning = WarningGroup | HasPayload | 0x1,
    PlayerAlreadyMadeActionWarning = WarningGroup | 0x2,
    ActionIgnoredDueToDeadWarning = WarningGroup | 0x3,
    SlowResponseWarning = WarningGroup | 0x4,

    // Error group (range: 0xF0 - 0xFF)
    ErrorGroup = 0xF0,
    InvalidPacketTypeError = ErrorGroup | 0x1,
    InvalidPacketUsageError = ErrorGroup | 0x2
};


struct Packet {
	PacketType packetType;
	nlohmann::json payload;
};

