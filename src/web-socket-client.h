#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <memory>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>

class WebSocketClient {
 public:
	WebSocketClient(std::string  host, std::string  port, std::string  code = "");
	~WebSocketClient();

	std::future<bool> Connect();
	void Run();
	void Stop();
	static std::string ConstructUrl(const std::string& host, const std::string& port, const std::string& code);

 private:
	void DoConnect();
	void DoRead();
	void DoWrite();
	void ProcessMessage(const std::string& message);
	void ProcessTextMessage(const std::string& message);
	void RespondToPing();
	void Reconnect();

	std::string host;
	std::string port;
	std::string code;

	boost::asio::io_context ioc;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
	std::thread workThread;
	std::promise<bool> connectPromise;

	std::queue<std::string> messagesToSend;
	std::mutex mtx;
	std::condition_variable cv;
	bool isPromiseSet = false;
	bool isReconnecting = false;
	std::chrono::steady_clock::time_point reconnectStartTime;
};
