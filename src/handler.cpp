#include "handler.h"

void Handler::GameState() {
	agentPtr->myNickname = "nickname";
	agentPtr->myId = "ID";
	Agent::NextMove();
}

void Handler::GameEnded() {

}

void Handler::LobbyData() {

}

Handler::Handler(Agent *agentPtr, std::queue<std::string> *messagesToSendPtr, std::mutex *mtxPtr,std::condition_variable *cvPtr)
: agentPtr(agentPtr), messagesToSendPtr(messagesToSendPtr), mtxPtr(mtxPtr), cvPtr(cvPtr) {};
