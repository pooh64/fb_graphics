#include <include/sync_threadpool.h>

#include <atomic>
#include <cassert>
#include <iostream>

std::atomic<int> my_counter;

void my_task(int x)
{
	//std::cout << x << " ";
	++my_counter;
}

void test(int n)
{
	{
		sync_threadpool tp(n);
		my_counter = 0;

		for (int i = 0; i < 10; ++i) {
			tp.set_tasks(my_task, n * 10);
			tp.run();
			tp.wait_completion();
		}
	}

	std::cout << std::flush << std::endl <<
		"Done: " << my_counter << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		std::cout << "Usage: argv[1] = n_threads" << std::endl;
	else
		test(atoi(argv[1]));
	return 0;
}
