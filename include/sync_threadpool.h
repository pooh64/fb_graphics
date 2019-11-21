#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <functional>
#include <cassert>
#include <cstdint>

struct SyncThreadpoolEnv {
	friend struct SyncThreadpool;
private:
	uint32_t n_threads = 0;
	std::vector<std::thread> pool;
	std::atomic<int> task_id;
	std::function<void(int, int)> task;

	size_t run_gen = 0;
	std::atomic<int> n_run;
	std::condition_variable cv_run;
	std::mutex m_done;

	bool start = false;
	std::condition_variable cv_start;
	std::mutex m_start;
	bool ready = false;
	std::condition_variable cv_ready;
	std::mutex m_ready;
	bool finish = false;

	void worker(int worker_id);
	bool start_barrier();
	void run_barrier();
};

inline void SyncThreadpoolEnv::worker(int worker_id)
{
	while (start_barrier()) {
		int caught;
		while ((caught = --task_id) >= 0)
			task(worker_id, caught);

		run_barrier();
	}
}

inline bool SyncThreadpoolEnv::start_barrier()
{
	std::unique_lock<std::mutex> lk(m_start);
	while (!start)
		cv_start.wait(lk);

	if (finish)
		return false;
	return true;
}

inline void SyncThreadpoolEnv::run_barrier()
{
	std::unique_lock<std::mutex> lk(m_done);
	size_t gen = run_gen;
	if (--n_run == 0) {
		start = false;
		n_run = n_threads;
		++run_gen;
		cv_run.notify_all();
		{ /* Notify owner */
			std::unique_lock lr_ready(m_ready);
			ready = true;
		}
		cv_ready.notify_all();
		return;
	}

	while (gen == run_gen)
		cv_run.wait(lk);
}

struct SyncThreadpool {
private:
	struct SyncThreadpoolEnv env;
	bool running = false; /* Track run/wait completion */
public:
	SyncThreadpool()
	{
		env.n_threads = 0;
	}

	void add_concurrency(uint32_t n_thr)
	{
		if (running)
			wait_completion();

		for (int id = env.n_threads; id < env.n_threads + n_thr; ++id)
			env.pool.push_back(std::thread(
				&SyncThreadpoolEnv::worker, &env, id));

		env.n_threads += n_thr;
		env.n_run = env.n_threads;
	}

	uint32_t get_concurrency()
	{
		return env.n_threads;
	}

	~SyncThreadpool()
	{
		if (running)
			wait_completion();

		env.finish = true;
		run();
		for (auto &t : env.pool)
			t.join();
	}

	void set_tasks(std::function<void(int, int)> _task, int _task_id)
	{
		env.task = _task;
		env.task_id = _task_id;
	}

	void run()
	{
		assert(env.n_threads);

		if (running == true)
			assert(!"Already running!");
		running = true;

		{ /* Allow execution */
			std::lock_guard<std::mutex> lk(env.m_start);
			env.start = true;
		}
		env.cv_start.notify_all();
	}

	void wait_completion()
	{
		if (running == false)
			assert(!"Already finished!");
		running = false;

		/* Wait for and accept notification */
		std::unique_lock<std::mutex> lk(env.m_ready);
		while (!env.ready) env.cv_ready.wait(lk);
		env.ready = false;
	}
};
