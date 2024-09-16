#include "web-socket-client.h"
#include <iostream>
#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <string>
#include "packet.h"
#include <nlohmann/json.hpp>  // Include this for JSON parsing
#include <stdexcept>

WebSocketClient::WebSocketClient(const std::string& host, const std::string& port, const std::string& code)
	: host(host), port(port), code(code), ws_(ioc)
{
}

WebSocketClient::~WebSocketClient()
{
	if (workThread.joinable())
		workThread.join();
}

std::string WebSocketClient::ConstructUrl(const std::string& host, const std::string& port, const std::string& code)
{
	if (code.empty()) {
		return "ws://" + host + ":" + port;
	}
	return "ws://" + host + ":" + port + "/?joinCode=" + code;
}

std::future<bool> WebSocketClient::Connect()
{
	auto future = connectPromise.get_future();

	workThread = std::thread([this]() {
	  DoConnect();
	});

	return future;
}

void WebSocketClient::DoConnect()
{
	try {
		boost::asio::ip::tcp::resolver resolver(ioc);
		auto const results = resolver.resolve(host, port);
		boost::asio::connect(ws_.next_layer(), results.begin(), results.end());

		std::string url = ConstructUrl(host, port, code);
		std::cout << "Connecting to WebSocket server at: " << url << std::endl;

		std::string path = code.empty() ? "/" : "/?joinCode=" + code;
		ws_.handshake(host, path);

		connectPromise.set_value(true);
	} catch (std::exception& e) {
		std::cerr << "Connection failed: " << e.what() << std::endl;
		connectPromise.set_value(false);
	}
}

void WebSocketClient::Run()
{
	ioc.run();

	std::thread readThread([this]() {
	  DoRead();
	});

	std::thread writeThread([this]() {
	  DoWrite();
	});

	readThread.join();
	writeThread.join();
}

void WebSocketClient::DoRead()
{
	try {
		for (;;) {
			boost::beast::flat_buffer buffer;
			ws_.read(buffer);
			std::string message = boost::beast::buffers_to_string(buffer.data());

			// Process the message
			std::thread processMessageThread([this, message]() {
			  ProcessMessage(message);
			});
			processMessageThread.detach();
		}
	} catch (boost::beast::error_code& e) {
		std::cerr << "Read error: " << e.message() << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Read exception: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Exception!!!" <<  std::endl;
	}
}

void WebSocketClient::DoWrite()
{
	try {
		while (true) {
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this]() { return !messagesToSend.empty() || shouldStop; });

			if (shouldStop && messagesToSend.empty()) {
				break;
			}

			while (!messagesToSend.empty()) {
				std::string message = messagesToSend.front();
				messagesToSend.pop();
				lock.unlock();

				ws_.write(boost::asio::buffer(message));
				lock.lock();
			}
		}
	} catch (boost::beast::error_code& e) {
		std::cerr << "Write error: " << e.message() << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Write exception: " << e.what() << std::endl;
	}
}

void WebSocketClient::ProcessMessage(const std::string& message)
{
	try {
		// Parse JSON message
		auto json_message = nlohmann::json::parse(message);

		// Deserialize Packet
		Packet packet;
		packet.packet_type = static_cast<PacketType>(json_message.at("type").get<uint64_t>());
		packet.payload = json_message.at("payload");

		// Process based on PacketType
		switch (packet.packet_type) {
		case PacketType::Ping:
			// Handle Ping
			RespondToPing();
			break;
		case PacketType::Pong:
			// Handle Pong
			std::cout << "Received Pong" << std::endl;
			break;
		case PacketType::TankMovement:
			// Handle TankMovement
			std::cout << "Received TankMovement" << std::endl;
			break;
		case PacketType::TankRotation:
			// Handle TankRotation
			std::cout << "Received TankRotation" << std::endl;
			break;
		case PacketType::TankShoot:
			// Handle TankShoot
			std::cout << "Received TankShoot" << std::endl;
			break;
		case PacketType::GameState:
			// Handle GameState
			std::cout << "Received GameState" << std::endl;
			break;
		case PacketType::LobbyData:
			// Handle LobbyData
			std::cout << "Received LobbyData" << std::endl;
			break;
		case PacketType::Ready:
			// Handle Ready
			std::cout << "Received Ready" << std::endl;
			break;
		case PacketType::GameEnded:
			// Handle GameEnded
			std::cout << "Received GameEnded" << std::endl;
			break;
		default:
			std::cerr << "Unknown packet type" << std::endl;
			break;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error processing message: " << e.what() << std::endl;
	}
}

void WebSocketClient::ProcessTextMessage(const std::string& message)
{
	// Process the text message
	std::cout << "Processing Text Message: " << message << std::endl;
	// Example: respond to specific message types or handle them
}

void WebSocketClient::RespondToPing()
{
	try {
		// Construct a Pong packet
		Packet pongPacket = Packet::ConstructPongPacket();

		// Serialize Packet to JSON
		nlohmann::json jsonResponse;
		jsonResponse["type"] = static_cast<uint64_t>(pongPacket.packet_type);
		jsonResponse["payload"] = pongPacket.payload;

		std::string responseString = jsonResponse.dump();

		// Send the response over the WebSocket
		{
			std::lock_guard<std::mutex> lock(mtx);
			messagesToSend.push(responseString);
		}
		cv.notify_one();
	} catch (const std::exception& e) {
		std::cerr << "Error responding to Ping: " << e.what() << std::endl;
	}
}

