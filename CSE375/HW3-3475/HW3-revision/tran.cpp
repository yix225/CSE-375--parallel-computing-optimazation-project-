#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <random>
#include "transaction.h"

constexpr int NUM_THREADS = 8;            // Number of threads
constexpr int OPERATIONS_PER_THREAD = 1000; // Number of operations per thread
constexpr double CONTAINS_PERCENTAGE = 0.8;
constexpr double INSERT_PERCENTAGE = 0.1;
constexpr double REMOVE_PERCENTAGE = 0.1;
auto con_duration = std::chrono::milliseconds::zero();
auto generateRandomInt = []() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(1, 10000);
    return dis(gen);
};

// Function to execute operations on the hash set
void executeOperations(transaction<int>& hashSet, int threadId) {
    auto con_start = std::chrono::steady_clock::now();
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        int randValue = generateRandomInt();
        int operation = rand() % 10; // Generate random operation type (0-9)

        if (operation < 8) { // 80% contains operation
            hashSet.contains(randValue);
        } else if (operation < 9) { // 10% insert operation
            hashSet.add(randValue);
        } else { // 10% remove operation
            hashSet.remove(randValue);
        }
    }
     // std::cout<<"transaction Thread"<<std::endl;
    auto con_end = std::chrono::steady_clock::now();
    con_duration = std::chrono::duration_cast<std::chrono::milliseconds>(con_end - con_start);
    std::cout << "Thread " << threadId << " execution time: " << con_duration.count() << " ms" << std::endl;
}

int main() {
    transaction<int> hashSet(1000,10,2,4); // Create a transaction hash set

    std::vector<std::thread> threads; // Vector to store threads
    std::cout << "Populating hash set..." << std::endl;
    hashSet.populate(10000, generateRandomInt); // Populate the hash set with random values
    // std::cout << "Populated hash set" << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(std::bind(executeOperations, std::ref(hashSet),i));
    }
   
   

    // std::cout << "Threads spawned" << std::endl; 

    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout<<"Total execution time: " << con_duration.count() << " ms" << std::endl;


    return 0;
}
