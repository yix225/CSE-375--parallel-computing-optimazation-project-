#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <iostream>
#include "sequential.h"
#include "concurrent.h"
#include "transaction.h"

// Define the number of threads and operations per thread
const int num_threads = 8;
const int num_ops_per_thread = 1000;

// Define the percentages of operations
const int contains_percentage = 80;
const int insert_percentage = 10;
const int remove_percentage = 10;

std::chrono::milliseconds seq_duration;
std::chrono::milliseconds con_duration;
std::chrono::milliseconds tra_duration;



// Function to generate random integers
int generate_random_int() {
    std::cout<<"Random"<<std::endl;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(1, 1000);
    return distrib(gen);
}

// Function to perform operations on the data structure for sequential hash set
void perform_operations_sequential(sequential<int>& my_set) {
    std::cout<<"Sequential Thread"<<std::endl;
    auto seq_start =std::chrono::steady_clock::now();

    for (int i = 0; i < num_ops_per_thread; ++i) {
        int rand_val = generate_random_int();
        int operation = generate_random_int() % 100; // Randomly choose an operation

        if (operation < contains_percentage) {
            my_set.contains(rand_val); // Perform contains operation
        } else if (operation < contains_percentage + insert_percentage) {
            my_set.add(rand_val); // Perform insert operation
        } else {
            my_set.remove(rand_val); // Perform remove operation
        }
    }
    auto seq_end = std::chrono::steady_clock::now();
    seq_duration = std::chrono::duration_cast<std::chrono::milliseconds>(seq_end - seq_start);

}

// Function to perform operations on the data structure for concurrent hash set
void perform_operations_concurrent(concurrent<int>& my_set) {
    std::cout<<"Concurrent Thread"<<std::endl;
    auto con_start = std::chrono::steady_clock::now();
    for (int i = 0; i < num_ops_per_thread; ++i) {
        int rand_val = generate_random_int();
        int operation = generate_random_int() % 100; // Randomly choose an operation

        if (operation < contains_percentage) {
            my_set.contains(rand_val); // Perform contains operation
        } else if (operation < contains_percentage + insert_percentage) {
            my_set.add(rand_val); // Perform insert operation
        } else {
            my_set.remove(rand_val); // Perform remove operation
        }
    }
    auto con_end = std::chrono::steady_clock::now();
    con_duration = std::chrono::duration_cast<std::chrono::milliseconds>(con_end - con_start);
}

// Function to perform operations on the data structure for transactional hash set
void perform_operations_transactional(transaction<int>& my_set) {
    std::cout<<"Transactional Thread"<<std::endl;
    auto tra_start = std::chrono::steady_clock::now();
    for (int i = 0; i < num_ops_per_thread; ++i) {
        int rand_val = generate_random_int();
        int operation = generate_random_int() % 100; // Randomly choose an operation

        if (operation < contains_percentage) {
            my_set.contains(rand_val); // Perform contains operation
        } else if (operation < contains_percentage + insert_percentage) {
            my_set.add(rand_val); // Perform insert operation
        } else {
            my_set.remove(rand_val); // Perform remove operation
        }
    }
    auto tra_end = std::chrono::steady_clock::now();
    tra_duration = std::chrono::duration_cast<std::chrono::milliseconds>(tra_end - tra_start);
}

int main() {
    // Create the hash sets
    sequential<int> sequential_set(1000, 10);
    concurrent<int> concurrent_set(1000, 10, 8,2,4);
    transaction<int> transactional_set(1000, 10, 2, 4);

    // Populate the hash sets
    sequential_set.populate(1000, generate_random_int);
    std::cout<<"1"<<std::endl;
    concurrent_set.populate();
    std::cout<<"2"<<std::endl;
    transactional_set.populate();
    std::cout<<"3"<<std::endl;

    // Create vector to hold threads
    std::vector<std::thread> threads;

    // Spawn threads for sequential hash set
    for (int i = 0; i < num_threads; ++i) {
        std::cout<<"4"<<std::endl;

        threads.emplace_back(perform_operations_sequential, std::ref(sequential_set));
    }

    // Spawn threads for concurrent hash set
    for (int i = 0; i < num_threads; ++i) {
        std::cout<<"5"<<std::endl;
        threads.emplace_back(perform_operations_concurrent, std::ref(concurrent_set));
    }

    // Spawn threads for transactional hash set
    for (int i = 0; i < num_threads; ++i) {
        std::cout<<"6"<<std::endl;
        threads.emplace_back(perform_operations_transactional, std::ref(transactional_set));
    }

    // Join threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Check if the sizes match the expected sizes
    std::cout << "Sequential Set Size: " << sequential_set.size() << std::endl;
    std::cout<<"Sequential Set Time: "<<seq_duration.count()<<std::endl;
    std::cout << "Concurrent Set Size: " << concurrent_set.size() << std::endl;
    std::cout<<"Concurrent Set Time: "<<con_duration.count()<<std::endl;
    std::cout << "Transactional Set Size: " << transactional_set.size() << std::endl;
    std::cout<<"Transactional Set Time: "<<tra_duration.count()<<std::endl;

    return 0;
}
