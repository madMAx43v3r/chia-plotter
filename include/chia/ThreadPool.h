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
				const int num_threads, const std::string& name = "")
		:	output(output),
			execute(func)
	{
		if(num_threads < 1) {
			throw std::logic_error("num_threads < 1");
		}
		for(int i = 0; i < num_threads; ++i) {
			threads.push_back(std::make_shared<thread_t>());
		}
		for(int i = 0; i < num_threads; ++i) {
			threads[i]->thread = std::make_shared<Thread<T>>(
					std::bind(&ThreadPool::wrapper, this,
							threads[i].get(),
							threads[((i + num_threads) - 1) % num_threads].get(),
							std::placeholders::_1),
					name.empty() ? name : name + "/" + std::to_string(i));
		}
	}
	
	~ThreadPool() {
		close();
	}
	
	// NOT thread-safe
	void take(T& data) override {
		const auto& state = threads[next % threads.size()];
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
		for(const auto& state : threads) {
			state->thread->wait();
		}
	}
	
	// NOT thread-safe
	void close() {
		wait();
		for(const auto& state : threads) {
			state->thread->close();
		}
		threads.clear();
	}
	
	// NOT thread-safe
	size_t num_threads() const {
		return threads.size();
	}
	
	// NOT thread-safe
	L& get_local(size_t index) {
		const auto& state = threads[index];
		state->thread->wait();
		return state->local;
	}
	
	// NOT thread-safe
	void set_local(size_t index, L&& value) {
		const auto& state = threads[index];
		state->thread->wait();
		state->local = value;
	}
	
private:
	void wrapper(thread_t* state, thread_t* prev, T& input)
	{
		uint64_t job = -1;
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			job = state->job;
		}
		S out;
		execute(input, out, state->local);
		{
			std::unique_lock<std::mutex> lock(prev->mutex);
			while(prev->job < job) {
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
