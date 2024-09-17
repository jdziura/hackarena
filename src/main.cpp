#include <cstdlib>
#include <iostream>
#include <string>
#include "web-socket-client.h"

int main(int argc, char** argv)
{
	// Default values
	std::string host = "localhost";
	std::string port = "5000";
	std::string code;

	// Check command line arguments and update default values if provided
	if (argc > 1) host = argv[1];
	if (argc > 2) port = argv[2];
	if (argc > 3) code = argv[3];

	// Print the values
	std::cout << "Host: " << host << "\n";
	std::cout << "Port: " << port << "\n";
	std::cout << "Code: " << code << "\n";

	// Placeholder for actual websocket client logic
	std::cout << "Starting client...\n";

	// Create the WebSocket client
	WebSocketClient client(host, port, code);

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