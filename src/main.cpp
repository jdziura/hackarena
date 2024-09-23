#include "pch.h"
#include "web-socket-client.h"

int main(int argc, char** argv) {
	// Default values
	std::string host = "localhost";
	std::string port = "5000";
	std::string nickname = "empty";
	std::string code = "";
    std::string debugQuickJoinString;
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
    if (args.count("debug-quick-join")) debugQuickJoinString = args["debug-quick-join"];
    if(debugQuickJoinString == "true") debugQuickJoin = true;

	// Print the values
	std::cout << "Host: " << host << "\n";
	std::cout << "Port: " << port << "\n";
	std::cout << "Nickname: " << nickname << "\n";
	std::cout << "Code: " << code << "\n";

	// Placeholder for actual websocket client logic
	std::cout << "Starting client...\n";

	// Create the WebSocket client
	WebSocketClient client(host, port, nickname, code, debugQuickJoin);

	// Connect to the WebSocket server asynchronously
	auto connect_future = client.Connect();

	// Wait for the connection to be established
	if (!connect_future.get()) {
		std::cerr << "Failed to Connect to the WebSocket server.\n";
		client.Stop();
		return EXIT_FAILURE;
	}

	std::cout << "Connected successfully. Running the client...\n";

	return EXIT_SUCCESS;
}
