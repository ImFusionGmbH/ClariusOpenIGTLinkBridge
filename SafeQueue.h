#pragma once


#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <png.h>
#include <queue>
#include <sstream>
#include <thread>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
	SafeQueue()
		: q()
		, m()
		, c()
	{
	}

	~SafeQueue() = default;

	// Add an element to the queue.
	void enqueue(T t)
	{
		std::lock_guard<std::mutex> lock(m);
		q.push(t);
		c.notify_one();
	}

	// Get the "front"-element.
	// If the queue is empty, wait till a element is available.
	T dequeue()
	{
		std::unique_lock<std::mutex> lock(m);
		while (q.empty())
		{
			// release lock as long as the wait and require it afterwards.
			c.wait(lock);
		}
		T val = q.front();
		q.pop();
		return val;
	}

private:
	std::queue<T> q;
	mutable std::mutex m;
	std::condition_variable c;
};
