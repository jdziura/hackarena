#include "pch.h"
#include "agent/agent.h"

#pragma once

namespace Handlers {
	static void LobbyData() {};
	static void GameState(Agent *agent) {
		*agent = Agent("test Id", "test Nickname");
		agent->NextMove();
	};
	static void GameEnded() {};
}
