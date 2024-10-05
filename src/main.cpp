#include "pch.h"
#include "web-socket-client.h"

int main(int argc, char** argv) {
	// Default values
	std::string host = "localhost";
	std::string port = "5000";
	std::string nickname = "empty";
	std::string code = "";
    bool debugQuickJoin = false;

	// Parse command line arguments into a map
	std::unordered_map<std::string, std::string> args;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg.starts_with("--")) {
			if (i + 1 < argc) {
				args[arg.substr(2)] = argv[++i]; // Store key-value pairs
			}
		}
	}

	// Update default values based on provided arguments
	if (args.count("host")) host = args["host"];
	if (args.count("port")) port = args["port"];
	if (args.count("nickname")) nickname = args["nickname"];
	if (args.count("code")) code = args["code"];

	// Print the values
	std::cout << "Host: " << host << std::endl << std::flush;
	std::cout << "Port: " << port << std::endl << std::flush;
	std::cout << "Nickname: " << nickname << std::endl << std::flush;
	std::cout << "Code: " << code << std::endl << std::flush;

	// Placeholder for actual websocket client logic
	std::cout << "Starting client..." << std::endl << std::flush;

	// Create the WebSocket client
	WebSocketClient client(host, port, nickname, code);

	// Connect to the WebSocket server asynchronously
	auto connect_future = client.Connect();

	// Wait for the connection to be established
	if (!connect_future.get()) {
		std::cerr << "Failed to Connect to the WebSocket server." << std::endl << std::flush;
		client.Stop();
		return EXIT_FAILURE;
	}

	std::cout << "Connected successfully. Running the client..." << std::endl << std::flush;

	return EXIT_SUCCESS;
}
