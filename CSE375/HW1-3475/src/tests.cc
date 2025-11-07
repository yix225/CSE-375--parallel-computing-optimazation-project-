// CSE 375/475 Assignment #1
// Spring 2024
//
// Description: This file implements a function 'run_custom_tests' that should be able to use
// the configuration information to drive tests that evaluate the correctness
// and performance of the map_t object.

#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <mutex>
#include <random>
#include "config_t.h"
#include "tests.h"
#include "simplemap.h"
	std::mutex mtx;

	void printer(int k, float v) {
			
			std::cout<<"<"<<k<<","<<v<<">"<< std::endl;
			
	}
	void deposit(simplemap_t<int,float>& map,  int id1, int id2, float amount) {
			float b1 = map.lookup(id1).first;
			float b2 = map.lookup(id2).first;
			map.update(id1, b1 - amount);
			map.update(id2, b2 + amount);
	}
	float balance(simplemap_t<int,float>& map) {
			float sum = 0;
			for (int i = 0; i < 100000; i++) {
				sum += map.lookup(i).first;

			}
			return sum;
	}
	
	std::chrono::milliseconds do_work(simplemap_t<int,float>& map, config_t& cfg){
		// sum is 100000
		std::chrono::milliseconds exec_time;
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < cfg.iters; i++) {
			int r = rand() % 100;
			if (r < 95) {
				int id1 = rand() % 100;
				int id2 = rand() % 100;
				float amount = rand() % 100;

				mtx.lock();
				deposit(map, id1, id2, amount);
				mtx.unlock();
			} else {
				mtx.lock();
				balance(map);
				mtx.unlock();
			}
		}
		auto end = std::chrono::high_resolution_clock::now();
		// calculate the elapsed time
		exec_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		 	
		return exec_time;
	}

	void run_custom_tests(config_t& cfg) {
		// Step 1
		// Define a simplemap_t of types <int,float>
		// this map represents a collection of bank accounts:
		// each account has a unique ID of type int;
		// each account has an amount of fund of type float.
		
		simplemap_t<int, float> map;
		int id;
		float amount;

		// Step 2
		// Populate the entire map with the 'insert' function
		// Initialize the map in a way the sum of the amounts of
		// all the accounts in the map is 100000

		for (int i = 0; i < 100; i++) {
			id = i;
			amount = 1000;
			map.insert(id, amount);
		}

	
		// Step 3
		// Define a function "deposit" that selects two random bank accounts
		// and an amount. This amount is subtracted from the amount
		// of the first account and summed to the amount of the second
		// account. In practice, give two accounts B1 and B2, and a value V,
		// the function performs B1-=V and B2+=V.
		// The execution of the whole function should happen atomically:
		// no operation should happen on B1 and B2 (or on the whole map?)
		// while the function executes.


		// Step 4
		// Define a function "balance" that sums the amount of all the
		// bank accounts in the map. In order to have a consistent result,
		// the execution of this function should happen atomically:
		// no other deposit operations should interleave.


		
		// Step 5
		// Define a function 'do_work', which has a for-loop that
		// iterates for config_t.iters times. In each iteration,
		// the function 'deposit' should be called with 95% of the probability;
		// otherwise (the rest 5%) the function 'balance' should be called.
		// The function 'do_work' should measure 'exec_time_i', which is the
		// time needed to perform the entire for-loop. This time will be shared with
		// the main thread once the thread executing the 'do_work' joins its execution
		// with the main thread.


		// Step 6
		// The evaluation should be performed in the following way:
		// - the main thread creates #threads threads (as defined in config_t)
		//   << use std:threds >>
		// - each thread executes the function 'do_work' until completion
		// - the (main) spawning thread waits for all the threads to be executed
		//   << use std::thread::join() >>
		//	 and collect all the 'exec_time_i' from each joining thread
		//   << consider using std::future for retireving 'exec_time_i' after the thread finishes its do_work>>
		// - once all the threads have joined, the function "balance" must be called

		// Make sure evey invocation of the "balance" function returns 100000.

		// use the global variable to replace the fucture and thread
		
		// write the sequential version of the code first

		
		// then write the parallel version of the code



		std::vector<std::thread> threads;
		std::vector<std::future <std::chrono::milliseconds>> futures;
		std::chrono::milliseconds exec_time;
		std::chrono::milliseconds exec_time_f;
		std::chrono::milliseconds tot_exec_time;
		std::chrono::milliseconds tot_exec_time_f;
		std::vector<std::chrono::milliseconds> thread_exec_times;
	
		for (int i = 0; i < cfg.threads; i++) {
			threads.emplace_back(std::thread([&, i]() {
				// mtx.lock();
				exec_time = do_work(map, cfg);
				// tot_exec_time += exec_time;
				// mtx.unlock();
			}));
    	}
		// std::cout << "2 " << std::endl;
		// std::cout << "2" << std::endl;
		
		
		for (int i = 0; i < cfg.threads; i++) {
			// mtx.lock();
			futures.push_back(std::async(do_work, std::ref(map), std::ref(cfg)));
			// mtx.unlock();

		}



		
		// Now thread_exec_times vector contains the execution time of each thread for all accounts

		for (auto& fut : futures) {
			
			exec_time_f = fut.get();
			tot_exec_time_f += exec_time_f;
			thread_exec_times.push_back(exec_time_f);
			
		}
		// Print thread execution times
		std::cout << "Thread Execution Times:" << std::endl;
		for (size_t i = 0; i < thread_exec_times.size(); ++i) {
			std::cout << "Thread " << i + 1 << ": " << thread_exec_times[i].count() << " ms" << std::endl;
		}

		for (auto& thd : threads) {
			thd.join();
		}
		

		float totbalance = balance(map);

		
		std::cout << "Total balance: " << totbalance << std::endl;
		// std::cout << "Total execution time: " << tot_exec_time.count() << std::endl;
		std::cout << "Total execution time future: " << tot_exec_time_f.count() << std::endl;
		std::cout << "Average execution time: " << tot_exec_time_f.count() / cfg.threads << std::endl;

		
		// Step 7
		// Now configure your application to perform the SAME TOTAL amount
		// of iterations just executed, but all done by a single thread.
		// Measure the time to perform them and compare with the time
		// previously collected.
		// Which conclusion can you draw?
		// Which optimization can you do to the single-threaded execution in
		// order to improve its performance?
		
		// share the lock, share the map, share the random number generator




		// Step 8
		// Remove all the items in the map by leveraging the 'remove' function of the map
		// Destroy all the allocated resources (if any)
		// Execution terminates.
		// If you reach this stage happy, then you did a good job!


		// Final step: Produce plot
		// I expect each submission to include at least one plot in which
		// the x-axis is the concurrent threads used {1;2;4;8}
		// the y-axis is the application execution time.
		// The performance at 1 thread must be the sequential
		// application without synchronization primitives

	


		
		map.apply(printer);

		for(int i = 0; i < cfg.iters; i++) {
			map.remove(i);
		}

	}
void run_custom_tests_sequential(config_t &cfg) {
    simplemap_t<int, float> map;
    int id, amount;
	// std::chrono::milliseconds tot_exec_time;
    for (int i = 0; i < 100; i++) {
        id = i;
        amount = 1000;
        map.insert(id, amount);
    }
	std::thread sequential_thread([&map, &cfg] {
		std::chrono::milliseconds exec_time = do_work(map, cfg);
		std::cout << "Sequential Execution Time: " << exec_time.count() << " ms" << std::endl;
	});
	sequential_thread.join();

    // std::chrono::milliseconds tot_exec_time = do_work(map, cfg);

    float totbalance = balance(map);

    std::cout << "Total balance: " << totbalance << std::endl;
    // std::cout << "Total execution time: " << tot_exec_time.count() << " ms" << std::endl;
}

void test_driver(config_t &cfg) {
	run_custom_tests(cfg);
	run_custom_tests_sequential(cfg);
}


// deadlock is a situation where a thread is waiting for a resource that is held by another thread, and this second thread is waiting for a resource held by the first thread.
// handle the dependencies between the threads to avoid deadlock

// lamda function to pass the function pointer to the thread
// std::thread t1([](int a, int b) {std::cout << a + b << std::endl;}, 1, 2);
// t1.join();

// binary search
// 


