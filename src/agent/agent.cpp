#include "agent.h"
#include <utility>

Agent::Agent(std::string myId, std::string myNickname) : myId(std::move(myId)), myNickname(std::move(myNickname)) {}
void Agent::NextMove() {

}
void Agent::OnGameEnded() {

}
