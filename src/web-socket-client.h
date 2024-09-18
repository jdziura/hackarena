#pragma once

#include "pch.h"
#include "thread-timer.h"

class WebSocketClient {
 public:
	WebSocketClient(std::string  host, std::string  port, std::string  code = "", int timeoutNumber = 100);
	~WebSocketClient();

	std::future<bool> Connect();
	void Run();
	void Stop();
	static std::string ConstructUrl(const std::string& host, const std::string& port, const std::string& code);

 private:
	void DoConnect();
	void DoRead();
	void DoWrite();
	static void ProcessMessage(const std::string& message);
	static void ProcessTextMessage(const std::string& message);
	static void RespondToPing();
	void Reconnect();

	std::string host;
	std::string port;
	std::string code;

	boost::asio::io_context ioc;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
	std::thread workThread;
	ThreadTimer threadTimer;
	std::promise<bool> connectPromise;

	static std::queue<std::string> messagesToSend;
	static std::mutex mtx;
	static std::condition_variable cv;
	bool isPromiseSet = false;
	bool isReconnecting = false;
	std::chrono::steady_clock::time_point reconnectStartTime;
};
