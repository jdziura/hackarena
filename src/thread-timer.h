#include "pch.h"

#pragma once

class ThreadTimer
{
 private:
	static void ThreadProcessTimeout(const std::function<void()>& handler, int waitFor);

 public:
	int timeoutNumber;
	explicit ThreadTimer(int timeoutNumber);

	template<typename Function, typename... Args>
	void SetTimeout(Function&& func, Args&&... args);
};

template<typename Function, typename... Args>
void ThreadTimer::SetTimeout(Function&& func, Args&&... args) {
	auto boundFunction = std::bind(std::forward<Function>(func), std::forward<Args>(args)...);

	std::thread processMessageThread([this, boundFunction]() {
	  ThreadProcessTimeout(boundFunction, timeoutNumber);
	});
	processMessageThread.detach();
}