/*
 * Thread.h
 *
 *  Created on: May 23, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_THREAD_H_
#define INCLUDE_CHIA_THREAD_H_

#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <functional>
#include <condition_variable>

#include <pthread.h>

template<typename T>
class Processor {
public:
	virtual ~Processor() {}
	
	virtual void take(T& data) = 0;
	
	void take_copy(const T& data) {
		T copy = data;
		take(copy);
	}
};

template<typename T>
class Thread : public Processor<T> {
public:
	Thread(const std::function<void(T&)>& func, const std::string& name = "")
		:	execute(func)
	{
		thread = std::thread(&Thread::loop, this, name);
	}
	
	virtual ~Thread() {
		close();
	}
	
	// thread-safe
	void take(T& data) override {
		{
			std::unique_lock<std::mutex> lock(mutex);
			if(is_fail) {
				throw std::runtime_error("thread failed with: " + ex_what);
			}
			while(is_busy) {
				signal.wait(lock);
			}
			input = std::move(data);
			is_busy = true;
		}
		signal.notify_all();
	}
	
	// thread-safe
	void wait() {
		std::unique_lock<std::mutex> lock(mutex);
		while(do_run && is_busy) {
			signal.wait(lock);
		}
	}
	
	// NOT thread-safe
	void close() {
		wait();
		std::unique_lock<std::mutex> lock(mutex);
		do_run = false;
		if(thread.joinable()) {
			lock.unlock();
			signal.notify_all();
			thread.join();
		}
	}
	
private:
	void loop(const std::string& name) noexcept
	{
		if(!name.empty()) {
			std::string thread_name = name;
			// limit the name to 15 chars, otherwise pthread_setname_np() fails
			if(thread_name.size() > 15) {
				thread_name.resize(15);
			}
			pthread_setname_np(pthread_self(), thread_name.c_str());
		}
		std::unique_lock<std::mutex> lock(mutex);
		while(!is_fail) {
			while(do_run && !is_busy) {
				signal.wait(lock);
			}
			if(!do_run) {
				break;
			}
			lock.unlock();
			try {
				execute(input);
			} catch(const std::exception& ex) {
				is_fail = true;
				ex_what = ex.what();
				std::cerr << ex.what();
			}
			lock.lock();
			is_busy = false;
			signal.notify_all();
		}
		signal.notify_all();
	}
	
private:
	T input;
	bool do_run = true;
	bool is_fail = false;
	bool is_busy = false;
	std::mutex mutex;
	std::thread thread;
	std::condition_variable signal;
	std::function<void(T&)> execute;
	std::string ex_what;
	
};


#endif /* INCLUDE_CHIA_THREAD_H_ */
