#include <include/sync_threadpool.h>

#include <atomic>
#include <cassert>
#include <iostream>

std::atomic<int> my_counter;

std::vector<int> thread_stat;

void my_task(int id, int x)
{
	//std::cout << x << " ";
	++my_counter;
	thread_stat[id]++;
}

void test(int n)
{
	{
		thread_stat.resize(n);
		SyncThreadpool tp(n);
		my_counter = 0;

		for (int i = 0; i < 10; ++i) {
			tp.set_tasks(my_task, n * 10);
			tp.run();
			tp.wait_completion();
		}
	}

	std::cout << std::flush << std::endl <<
		"Done: " << my_counter << std::endl;
	for (int i = 0; i < n; ++i)
		std::cout << "thread[" << i << "] " <<
			thread_stat[i] << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		std::cout << "Usage: argv[1] = n_threads" << std::endl;
	else
		test(atoi(argv[1]));
	return 0;
}
