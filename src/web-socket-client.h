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
	WebSocketClient(const std::string& host, const std::string& port, const std::string& code = "");
	~WebSocketClient();

	std::future<bool> Connect();
	void Run();
	static std::string ConstructUrl(const std::string& host, const std::string& port, const std::string& code);

 private:
	void DoConnect();
	void DoRead();
	void DoWrite();
	void ProcessMessage(const std::string& message);
	void ProcessTextMessage(const std::string& message);
	void RespondToPing();

	std::string host;
	std::string port;
	std::string code;

	boost::asio::io_context ioc;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
	std::thread workThread;
	std::promise<bool> connectPromise;

	std::queue<std::string> messagesToSend;
	std::mutex mtx;
	std::condition_variable cv;
	bool shouldStop = false;
};
