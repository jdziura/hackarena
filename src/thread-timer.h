#pragma once

class ThreadTimer
{
 private:
	int timeoutNumber;
	static void ThreadProcessTimeout(const std::function<void()>& handler, int waitFor);

 public:
	explicit ThreadTimer(int timeoutNumber);

	template<typename Function, typename... Args>
	void SetTimeout(Function&& func, Args&&... args);
};
