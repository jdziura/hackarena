#include "web-socket-client.h"

#include <utility>
#include "packet.h"

WebSocketClient::WebSocketClient(std::string  host, std::string  port, std::string nickname, std::string  code, int timeoutNumber)
	: host(std::move(host)), port(std::move(port)), nickname(std::move(nickname)), code(std::move(code)), timeoutNumber(timeoutNumber),
	  ws(ioc), handler(&agent, &messagesToSend, &mtx, &cv) {}

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
	#ifdef _WIN64
		// Windows 64-bit specific code
		TerminateThread(workThread.native_handle(), 0);
	#elif defined(__linux__)
		// Linux-specific code
		pthread_cancel(workThread.native_handle());
	#elif defined(__APPLE__)
		// macOS-specific code
	#else
		#error "Unsupported platform"
	#endif
}

std::string WebSocketClient::ConstructUrl(const std::string& code, const std::string& nickname, bool quickJoin)
{
	std::string url = "/?nickname=" + nickname;

	if (!code.empty()) {
		url += "&joinCode=" + code;
	}

	if (quickJoin) {
		url += "&quickJoin=true";
	}

	return url;
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

		std::string path = ConstructUrl(code, nickname, true);
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

	std::thread processingThread([this]() {
	  SendToProcessing();
	});

	std::thread readThread([this]() {
	  DoRead();
	});

	std::thread writeThread([this]() {
	  DoWrite();
	});

	processingThread.join();
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

			std::lock_guard<std::mutex> lock(mtxR);
			messagesReceived.push(message);
			cvR.notify_one();
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

void WebSocketClient::SendToProcessing()
{
	try {
		while (true) {
			std::unique_lock<std::mutex> lock(mtxR);
			cvR.wait(lock, [this]() { return !messagesReceived.empty(); });

			while (!messagesReceived.empty()) {
				std::string message = messagesReceived.front();
				messagesReceived.pop();
				lock.unlock();

				auto start = std::chrono::high_resolution_clock::now();

				std::thread processMessageThread([this, message]() {
				  ProcessMessage(message);
				});

				#ifdef _WIN64
				// Windows 64-bit specific code
					if (WaitForSingleObject(processMessageThread.native_handle(), timeoutNumber) != 0x00000000L) {
						TerminateThread(processMessageThread.native_handle(), 1);
					}
				#elif defined(__linux__)
				// Linux-specific code
					struct timespec ts{};
					clock_gettime(CLOCK_REALTIME, &ts);
					ts.tv_sec += timeoutNumber / 1000;

					if (pthread_timedjoin_np(processMessageThread.native_handle(), nullptr, &ts) != 0) {
						pthread_cancel(processMessageThread.native_handle());
					}
				#elif defined(__APPLE__)
					// macOS-specific code
				#else
					#error "Unsupported platform"
				#endif

				processMessageThread.join();

				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> duration = end - start; // Duration in milliseconds

				// Log the duration
				std::cout << "NextMove took " << duration.count() << " ms." << std::endl;

				lock.lock();
			}
		}
	} catch (std::exception& e) {
		std::cerr << "Send to processing exception: " << e.what() << std::endl;
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
		packet.packetType = static_cast<PacketType>(jsonMessage.at("type").get<uint64_t>());
		packet.payload = jsonMessage.at("payload");

		// Process based on PacketType
		switch (packet.packetType) {
		case PacketType::Ping:
			// Handle Ping
			RespondToPing();
			std::cout << "Received Ping" << std::endl;
			break;
		case PacketType::Pong:
			// Handle Pong
			std::cout << "Received Pong" << std::endl;
			break;
		case PacketType::GameStart:
			// Handle GameStart
			std::cout << "Received GameStart" << std::endl;
			break;
		case PacketType::GameState:
			// Handle GameState
			handler.HandleGameState(packet.payload);
			std::cout << "Received GameState" << std::endl;
			break;
		case PacketType::LobbyData:
			// Handle LobbyData
			handler.HandleLobbyData(packet.payload);
			std::cout << "Received LobbyData" << std::endl;
			break;
		case PacketType::Ready:
			// Handle Ready
			std::cout << "Received Ready" << std::endl;
			break;
		case PacketType::GameEnded:
			// Handle GameEnded
			handler.HandleGameEnded(packet.payload);
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
		jsonResponse["type"] = static_cast<uint64_t>(pongPacket.packetType);
		jsonResponse["payload"] = nlohmann::json::object();

		std::string responseString = jsonResponse.dump();

		// Send the response over the WebSocket
		std::lock_guard<std::mutex> lock(mtx);
		messagesToSend.push(responseString);
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