#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include "sequential.h" // Assuming this is the header file for your sequential hash set implementation

int main() {
    constexpr int NUM_THREADS = 8; // Number of threads
    constexpr int NUM_OPERATIONS_PER_THREAD = 1000;
    constexpr double CONTAINS_PERCENTAGE = 0.8;
    constexpr double INSERT_PERCENTAGE = 0.1;
    constexpr double REMOVE_PERCENTAGE = 0.1;
    constexpr int NUM_OPERATIONS = NUM_THREADS * NUM_OPERATIONS_PER_THREAD;
    constexpr int CAPACITY = 1000; // Adjust the capacity according to your implementation
    constexpr int LIMIT = 3; // Adjust the limit according to your implementation

    // Initialize the hash table
    sequential<int> hashTable(CAPACITY, LIMIT);

    // Function to generate random integers
    auto generateRandomInt = []() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> dis(1, 10000);
        return dis(gen);
    };
    hashTable.populate(10000, generateRandomInt); // Populate the hash table with random values
    // Vector to hold threads
    std::vector<std::thread> threads;
    std::vector<long long> threadTimes; // Vector to hold execution times of each thread

    auto startTime = std::chrono::high_resolution_clock::now();

    // Create and start threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        auto threadStartTime = std::chrono::high_resolution_clock::now();
        threads.emplace_back([&hashTable, NUM_OPERATIONS_PER_THREAD, &generateRandomInt]() {
            for (int j = 0; j < NUM_OPERATIONS_PER_THREAD; ++j) {
                double choice = static_cast<double>(rand()) / RAND_MAX;

                if (choice < CONTAINS_PERCENTAGE) {
                    // Perform contains operation
                    hashTable.contains(generateRandomInt());
                } else if (choice < CONTAINS_PERCENTAGE + INSERT_PERCENTAGE) {
                    // Perform insert operation
                    hashTable.add(generateRandomInt());
                } else {
                    // Perform remove operation
                    hashTable.remove(generateRandomInt());
                }
            }
        });
        auto threadEndTime = std::chrono::high_resolution_clock::now();
        threadTimes.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(threadEndTime - threadStartTime).count());
    }
    // std::cout << "Threads created" << std::endl;
    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto endTime = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Total execution time: " << duration.count() << " milliseconds" << std::endl;

    // Show each thread's execution time
    for (int i = 0; i < NUM_THREADS; ++i) {
        std::cout << "Thread " << i << " execution time: " << threadTimes[i] << " milliseconds" << std::endl;
    }

    return 0;
}
