/*
 * ThreadPool.h
 *
 *  Created on: May 24, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_THREADPOOL_H_
#define INCLUDE_CHIA_THREADPOOL_H_

#include <chia/Thread.h>

#include <vector>
#include <memory>


template<typename T, typename S, typename L = size_t>
class ThreadPool : public Processor<T> {
private:
	struct thread_t {
		uint64_t job = -1;
		std::mutex mutex;
		std::condition_variable signal;
		std::shared_ptr<Thread<T>> thread;
		L local;
	};
	
public:
	ThreadPool(	const std::function<void(T&, S&, L&)>& func, Processor<S>* output,
				int num_threads, const std::string& name = "")
		:	output(output),
			execute(func)
	{
		if(num_threads < 1) {
			throw std::logic_error("num_threads < 1");
		}
		for(int i = 0; i < num_threads; ++i) {
			auto state = std::make_shared<thread_t>();
			state->thread = std::make_shared<Thread<T>>(
					std::bind(&ThreadPool::wrapper, this, i, std::placeholders::_1),
					name.empty() ? name : name + "/" + std::to_string(i));
			threads.push_back(state);
		}
	}
	
	~ThreadPool() {
		close();
	}
	
	// NOT thread-safe
	void take(T& data) override {
		auto state = threads[next % threads.size()];
		state->thread->wait();
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			state->job = next;
		}
		state->thread->take(data);
		next++;
	}
	
	// NOT thread-safe
	void wait() {
		for(auto state : threads) {
			state->thread->wait();
		}
	}
	
	// NOT thread-safe
	void close() {
		wait();
		for(auto state : threads) {
			state->thread->close();
		}
		threads.clear();
	}
	
	size_t num_threads() const {
		return threads.size();
	}
	
	// NOT thread-safe
	L& get_local(size_t index) {
		return threads[index]->local;
	}
	
	// NOT thread-safe
	void set_local(size_t index, L&& value) {
		threads[index]->local = value;
	}
	
private:
	void wrapper(const size_t index, T& input)
	{
		auto state = threads[index];
		
		S out;
		execute(input, out, state->local);
		
		auto prev = threads[((index + threads.size()) - 1) % threads.size()];
		{
			std::unique_lock<std::mutex> lock(prev->mutex);
			while(prev->job < state->job) {
				prev->signal.wait(lock);
			}
		}
		if(output) {
			output->take(out);	// only one thread can be at this position
		}
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			state->job = -1;
		}
		state->signal.notify_all();
	}
	
private:
	uint64_t next = 0;
	Processor<S>* output = nullptr;
	std::function<void(T&, S&, L&)> execute;
	std::vector<std::shared_ptr<thread_t>> threads;
	
};



#endif /* INCLUDE_CHIA_THREADPOOL_H_ */
