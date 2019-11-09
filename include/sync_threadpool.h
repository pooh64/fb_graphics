#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <functional>
#include <cassert>

struct SyncThreadpoolEnv {
	friend struct SyncThreadpool;
private:
	unsigned n_threads;
	std::vector<std::thread> pool;

	std::atomic<int> task_id;
	std::function<void(int, int)> task;

	bool start = false;
	std::condition_variable cv_start;
	std::mutex m_start;

	bool done = false;
	std::atomic<int> n_done;
	std::condition_variable cv_done;
	std::mutex m_done;

	bool ready = false;
	std::condition_variable cv_ready;
	std::mutex m_ready;

	bool finish = false;

	SyncThreadpoolEnv()
	{
		n_done = 0;
	}

	void worker(int worker_id)
	{
		while (1) {
			{
				std::unique_lock<std::mutex> lk(m_start);
				while (!start) cv_start.wait(lk);
			}
			if (finish)
				return;

			int caught;
			while ((caught = --task_id) >= 0)
				task(worker_id, caught);

			if (++n_done == n_threads) {
				start = false;	       /* Lock start */

				n_done = 0;
				done = true;	       /* Open barrier */
				cv_done.notify_all();

				ready = true;	       /* Notify owner */
				cv_ready.notify_all();
			} else {
				std::unique_lock<std::mutex> lk(m_done);
				while(!done) cv_done.wait(lk);
			}
		}
	}
};

struct SyncThreadpool {
private:
	struct SyncThreadpoolEnv env;
	bool running = false; /* Track run/wait completion */
public:
	SyncThreadpool(unsigned _n_threads)
	{
		env.n_threads = _n_threads;

		for (int id = 0; id < env.n_threads; ++id)
			env.pool.push_back(std::thread(
				&SyncThreadpoolEnv::worker, &env, id));
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
		if (running == true)
			assert(!"Already running!");
		running = true;

		/* Allow execution */
		env.start = true;
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
