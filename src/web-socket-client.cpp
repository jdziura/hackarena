#include "web-socket-client.h"
#include "packet.h"

std::queue<std::string> WebSocketClient::messagesToSend;
std::mutex WebSocketClient::mtx;
std::condition_variable WebSocketClient::cv;

WebSocketClient::WebSocketClient(std::string  host, std::string  port, std::string  code, int timeoutNumber)
	: host(std::move(host)), port(std::move(port)), code(std::move(code)), ws(ioc),
	  threadTimer(timeoutNumber) {}

WebSocketClient::~WebSocketClient()
{
	if (workThread.joinable())
		workThread.join();
}

void WebSocketClient::Stop()
{
	try {
		// Close the WebSocket connection
		if (ws.is_open()) {
			ws.close(boost::beast::websocket::close_code::normal);
		}
	} catch (const std::exception& e) {
		std::cerr << "Error closing WebSocket: " << e.what() << std::endl;
	}
	TerminateThread(workThread.native_handle(), 0);
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
		boost::asio::connect(ws.next_layer(), results.begin(), results.end());

		std::string url = ConstructUrl(host, port, code);
		std::cout << "Connecting to WebSocket server at: " << url << std::endl;

		std::string path = code.empty() ? "/" : "/?joinCode=" + code;
		ws.handshake(host, path);

		// Set the promise value only once
		if (!isPromiseSet) {
			connectPromise.set_value(true);
			isPromiseSet = true;
		}
		isReconnecting = false;
		// Start reading and writing threads
		Run();
	} catch (std::exception& e) {
		std::cerr << "Connection failed: " << e.what() << std::endl;

		// Set the promise to false only on the first failure
		if (!isPromiseSet) {
			connectPromise.set_value(false);
			isPromiseSet = true;
		}

		// Start the reconnection timer
		if (!isReconnecting) {
			reconnectStartTime = std::chrono::steady_clock::now();
			isReconnecting = true;
		}
		Reconnect();
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
		while (true) {
			boost::beast::flat_buffer buffer;
			ws.read(buffer);
			std::string message = boost::beast::buffers_to_string(buffer.data());

			// Process the message
			/*std::thread processMessageThread([this, message]() {
			  ProcessMessage(message);
			});
			processMessageThread.detach();*/

			threadTimer.SetTimeout(ProcessMessage, message);
		}
	} catch (boost::beast::error_code& e) {
		std::cerr << "Read error: " << e.message() << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Read exception: " << e.what() << std::endl;
		// Start the reconnection timer
		if (!isReconnecting) {
			reconnectStartTime = std::chrono::steady_clock::now();
			isReconnecting = true;
		}
		Reconnect();
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
			cv.wait(lock, [this]() { return !messagesToSend.empty(); });

			while (!messagesToSend.empty()) {
				std::string message = messagesToSend.front();
				messagesToSend.pop();
				lock.unlock();

				ws.write(boost::asio::buffer(message));
				lock.lock();
			}
		}
	} catch (boost::beast::error_code& e) {
		std::cerr << "Write error: " << e.message() << std::endl;
	} catch (std::exception& e) {
		std::cerr << "Write exception: " << e.what() << std::endl;
		// Start the reconnection timer
		if (!isReconnecting) {
			reconnectStartTime = std::chrono::steady_clock::now();
			isReconnecting = true;
		}
		Reconnect();
	}
}

void WebSocketClient::ProcessMessage(const std::string& message)
{
	try {
		// Parse JSON message
		auto jsonMessage = nlohmann::json::parse(message);

		// Deserialize Packet
		Packet packet;
		packet.packet_type = static_cast<PacketType>(jsonMessage.at("type").get<uint64_t>());
		packet.payload = jsonMessage.at("payload");

		// Process based on PacketType
		switch (packet.packet_type) {
		case PacketType::Ping:
			// Handle Ping
			RespondToPing();
			std::cout << "Received Ping" << std::endl;
			break;
		case PacketType::Pong:
			// Handle Pong
			std::cout << "Received Pong" << std::endl;
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

void WebSocketClient::RespondToPing()
{
	try {
		// Construct a Pong packet
		Packet pongPacket = Packet::ConstructPongPacket();

		// Serialize Packet to JSON
		nlohmann::ordered_json jsonResponse;
		jsonResponse["type"] = static_cast<uint64_t>(pongPacket.packet_type);
		jsonResponse["payload"] = nlohmann::json::object();

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

void WebSocketClient::Reconnect()
{
	// Check elapsed time
	auto elapsedTime = std::chrono::steady_clock::now() - reconnectStartTime;
	if (std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() > 10) {
		std::cerr << "\nReconnection attempts timed out after 10 seconds.\n";
		Stop();
		return;
	}

	// Reset the promise for a new connection attempt
	connectPromise = std::promise<bool>();
	isPromiseSet = false; // Reset the flag

	try {
		DoConnect();
	} catch (std::exception& e) {
		std::cerr << "Reconnection attempt failed: " << e.what() << std::endl;
		Reconnect(); // Recursive call to keep trying to reconnect
	}
}