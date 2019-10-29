#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <functional>
#include <cassert>

struct sync_threadpool_env {
	friend struct sync_threadpool;
private:
	unsigned n_threads;
	std::vector<std::thread> pool;

	std::atomic<int> task_id;
	std::function<void(int)> task;

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

	sync_threadpool_env()
	{
		n_done = 0;
	}

	void worker()
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
				task(caught);

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

struct sync_threadpool {
private:
	struct sync_threadpool_env env;
	bool running = false; /* Track run/wait completion */
public:
	sync_threadpool(unsigned _n_threads)
	{
		env.n_threads = _n_threads;

		for (int i = 0; i < env.n_threads; ++i)
			env.pool.push_back(std::thread(
				&sync_threadpool_env::worker, &env));
	}

	~sync_threadpool()
	{
		if (running)
			wait_completion();

		env.finish = true;
		run();
		for (auto &t : env.pool)
			t.join();
	}

	void set_tasks(std::function<void(int)> _task, int _task_id)
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
