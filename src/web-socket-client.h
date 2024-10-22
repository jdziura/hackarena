#pragma once

#include "pch.h"
#include "handler.h"
#include "agent/agent.h"

class WebSocketClient {
 public:
	WebSocketClient(std::string  host, std::string  port, std::string nickname, std::string  code);
	~WebSocketClient();

	std::future<bool> Connect();
	void Run();
	static void SignalHandler(int signal);
	static void Stop();
	std::string ConstructUrl();

 private:
	void DoConnect();
	void DoRead();
	void SendToProcessing();
	void DoWrite();
	void ProcessMessage(const std::string& message);
	void RespondToPing();
    void SendLobbyRequest();

	std::string host;
	std::string port;
	std::string nickname;
	std::string code;

	static boost::asio::io_context ioc;
	static boost::asio::ip::tcp::socket socket;
	static boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
	static std::thread workThread;
	Handler handler;
	Agent agent;
	std::promise<bool> connectPromise;

	std::queue<std::string> messagesToSend;
	std::mutex mtx;
	std::condition_variable cv;
	std::queue<std::string> messagesReceived;
	std::mutex mtxR;
	std::condition_variable cvR;
};
