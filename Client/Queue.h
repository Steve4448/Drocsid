//Queue code from https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
#ifndef QUEUE_H
#define QUEUE_H
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
template <typename T>
class Queue
{
public:
	T pop();
	void pop(T& item);
	void push(const T& item);
	void push(T&& item);
	bool isEmpty();

private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
};
#endif