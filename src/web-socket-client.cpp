#include "web-socket-client.h"

#include <utility>
#include "packet.h"

WebSocketClient::WebSocketClient(std::string  host, std::string  port, std::string nickname, std::string  code, bool debugQuickJoin)
	: host(std::move(host)), port(std::move(port)), nickname(std::move(nickname)), code(std::move(code)), debugQuickJoin(debugQuickJoin),
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
		std::cerr << "Error closing WebSocket: " << e.what() << std::endl << std::flush;
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

std::string WebSocketClient::ConstructUrl()
{
	std::string url = "/?nickname=" + nickname;

	if (!code.empty()) {
		url += "&joinCode=" + code;
	}

	if (debugQuickJoin) {
		url += "&quickJoin=true";
	}

    url+= "&playerType=hackatonBot";

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

		std::string path = ConstructUrl();
		ws.handshake(host, path);
        connectPromise.set_value(true);

		// Start reading and writing threads
		Run();
	} catch (std::exception& e) {
		std::cerr << "Connection failed: " << e.what() << std::endl << std::flush;
        connectPromise.set_value(false);
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
		std::cerr << "Read error: " << e.message() << std::endl << std::flush;
	} catch (std::exception& e) {
		std::cerr << "Read exception: " << e.what() << std::endl << std::flush;
		Stop();
	}
	catch (...) {
		std::cerr << "Exception!!!" <<  std::endl << std::flush;
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

				std::thread processMessageThread([this, message]() {
				  ProcessMessage(message);
				});

				#ifdef _WIN64
				// Windows 64-bit specific code
					if (WaitForSingleObject(processMessageThread.native_handle(), agent.timeoutNumber) != 0x00000000L) {
						TerminateThread(processMessageThread.native_handle(), 1);
					}
				#elif defined(__linux__)
				// Linux-specific code
					struct timespec ts{};
					clock_gettime(CLOCK_REALTIME, &ts);
					ts.tv_sec += agent.timeoutNumber / 1000;

					if (pthread_timedjoin_np(processMessageThread.native_handle(), nullptr, &ts) != 0) {
						pthread_cancel(processMessageThread.native_handle());
					}
				#elif defined(__APPLE__)
					// macOS-specific code
				#else
					#error "Unsupported platform"
				#endif

				processMessageThread.join();

				lock.lock();
			}
		}
	} catch (std::exception& e) {
		std::cerr << "Send to processing exception: " << e.what() << std::endl << std::flush;
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
		std::cerr << "Write error: " << e.message() << std::endl << std::flush;
	} catch (std::exception& e) {
		std::cerr << "Write exception: " << e.what() << std::endl << std::flush;
		Stop();
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
        if (jsonMessage.contains("payload")) packet.payload = jsonMessage.at("payload");

		// Process based on PacketType
		switch (packet.packetType) {
		case PacketType::Ping:
			RespondToPing();
			break;
		case PacketType::Pong:
			break;
		case PacketType::GameStart:
			break;
		case PacketType::GameState:
			handler.HandleGameState(packet.payload);
			break;
		case PacketType::LobbyData:
			handler.HandleLobbyData(packet.payload);
			break;
		case PacketType::Ready:
			break;
		case PacketType::GameEnded:
			handler.HandleGameEnded(packet.payload);
            Stop();
			break;
        case PacketType::ConnectionOkay:
            break;
        case PacketType::ConnectionRejected:
            std::cerr << "Connection Rejected: " << packet.payload["reason"].get<std::string>() << std::endl << std::flush;
            Stop();
            break;
		default:
			std::cerr << "Unknown packet type: " << message << std::endl << std::flush;
			break;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error processing message: " << e.what() << std::endl << std::flush;
	}
}

void WebSocketClient::RespondToPing()
{
	try {
		// Serialize Packet to JSON
		nlohmann::json jsonResponse;
		jsonResponse["type"] = static_cast<uint64_t>(PacketType::Pong);

		std::string responseString = jsonResponse.dump();

		// Send the response over the WebSocket
		std::lock_guard<std::mutex> lock(mtx);
		messagesToSend.push(responseString);
		cv.notify_one();
	} catch (const std::exception& e) {
		std::cerr << "Error responding to Ping: " << e.what() << std::endl << std::flush;
	}
}