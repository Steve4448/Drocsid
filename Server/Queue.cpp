//Queue code from https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
#include "Queue.h"
template <typename T>
T Queue<T>::pop()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	while (queue_.empty())
	{
		cond_.wait(mlock);
	}
	auto item = queue_.front();
	queue_.pop();
	return item;
}

template <typename T>
void Queue<T>::pop(T& item)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	while (queue_.empty())
	{
		cond_.wait(mlock);
	}
	item = queue_.front();
	queue_.pop();
}

template <typename T>
void Queue<T>::push(const T& item)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	queue_.push(item);
	mlock.unlock();
	cond_.notify_one();
}

template <typename T>
void Queue<T>::push(T&& item)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	queue_.push(std::move(item));
	mlock.unlock();
	cond_.notify_one();
}

template <typename T>
bool Queue<T>::isEmpty() {
	return queue_.empty();
}