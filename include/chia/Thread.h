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

#ifdef _GNU_SOURCE
#include <pthread.h>
#endif


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
		std::unique_lock<std::mutex> lock(mutex);
		while(do_run && is_avail) {
			signal.wait(lock);
		}
		if(!do_run) {
			return;
		}
		is_avail = true;
		input = std::move(data);
		
		if(is_busy) {
			// wait for thread to take new input (no triple buffering)
			while(do_run && is_avail && is_busy) {
				signal.notify_all();
				signal.wait(lock);
			}
		} else {
			// simple notify since thread is just waiting for new input
			lock.unlock();
			signal.notify_all();
		}
	}
	
	// wait for thread to finish all pending input [thread-safe]
	void wait() {
		std::unique_lock<std::mutex> lock(mutex);
		while(do_run && (is_avail || is_busy)) {
			signal.wait(lock);
		}
		if(is_fail) {
			throw std::runtime_error("thread failed with: " + ex_what);
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
#ifdef _GNU_SOURCE
			pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
		}
		std::unique_lock<std::mutex> lock(mutex);
		while(true) {
			while(do_run && !is_avail) {
				signal.notify_all();	// notify about is_busy change
				signal.wait(lock);
			}
			if(!do_run) {
				break;
			}
			T tmp = std::move(input);
			is_avail = false;
			is_busy = true;
			lock.unlock();
			signal.notify_all();		// notify about is_busy + is_avail change
			try {
				execute(tmp);
				lock.lock();
			} catch(const std::exception& ex) {
				lock.lock();
				do_run = false;
				is_fail = true;
				ex_what = ex.what();
			}
			is_busy = false;
		}
		signal.notify_all();		// notify about do_run + is_fail + is_busy change
	}
	
private:
	T input;
	bool do_run = true;
	bool is_fail = false;
	bool is_busy = false;
	bool is_avail = false;
	std::mutex mutex;
	std::thread thread;
	std::condition_variable signal;
	std::function<void(T&)> execute;
	std::string ex_what;
	
};


#endif /* INCLUDE_CHIA_THREAD_H_ */
