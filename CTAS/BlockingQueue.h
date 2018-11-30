#pragma once
#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>

// Reference material: https://github.com/juanchopanza/cppblog/tree/master/Concurrency/Queue
template <typename T>
class BlockingQueue {
public:
	BlockingQueue ();
	~BlockingQueue ();

	T Pop ();
	void Push (T&& value);
	size_t Count ();

private:
	std::queue<T> queue;
	std::mutex mtx;
	std::condition_variable conditional_var;
};

template<typename T>
inline BlockingQueue<T>::BlockingQueue () {}

template<typename T>
inline BlockingQueue<T>::~BlockingQueue () {}

template<typename T>
inline T BlockingQueue<T>::Pop () {
	std::unique_lock<std::mutex> mlock (mtx);

	conditional_var.wait (mlock, [&] () {return !queue.empty (); });

	T ret_val = std::move (queue.front ());
	queue.pop ();

	return ret_val;
}

template<typename T>
inline void BlockingQueue<T>::Push (T && value) {
	std::lock_guard<std::mutex> mlock (mtx);

	queue.push (value);

	conditional_var.notify_one ();
}

template<typename T>
inline size_t BlockingQueue<T>::Count () {
	return queue.size ();
}

#endif // !BLOCKINGQUEUE_H