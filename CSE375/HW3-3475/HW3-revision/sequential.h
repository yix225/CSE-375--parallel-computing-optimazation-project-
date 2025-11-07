#include <iostream>
#include <vector>
#include <mutex>
#include <random>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>


template <typename T> 
class sequential {
    private:
        std::vector<std::vector<T>> tables[2];
        int capacity; // num of table
        int limit; // limit
        // int threshold;
        // int pro_size;
        bool is_resize = true;

        
        

        
        int hash0(T x) {
            int value = std::hash<T>{}(x);
            return value % capacity;
        }

        int hash1(T x) {
            int value =std::hash<T>{}(x);
            
            return abs((value * 1913) % 19841);
        }

        void resize() {
            // std::cout << "Resizing tables..." << std::endl;

            int new_capacity = capacity * 2;
            std::vector<std::vector<T>> new_table0(new_capacity);
            std::vector<std::vector<T>> new_table1(new_capacity);
            // std::cout << "New capacity: " << new_capacity << std::endl;
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < capacity; ++j) {
                    for (const auto& value : tables[i][j]) {
                        // std::cout << "Resizing value: " << value << std::endl;
                        int index = (i == 0) ? hash0(value) % new_capacity: hash1(value) %new_capacity;
                        if (i == 0) {
                            // std::cout << "Adding value to new table 0" << std::endl;
                            new_table0[index].push_back(value);
                        } else {
                            // std::cout << "Adding value to new table 1" << std::endl;
                            new_table1[index].push_back(value);
                        }
                    }
                }
            }
            // std::cout << "Tables resized" << std::endl;
            tables[0] = std::move(new_table0);
            tables[1] = std::move(new_table1);
            capacity = new_capacity;
        }




    public:
        sequential(int capacity, int limit) : capacity(capacity), limit(limit) {
            tables[0].resize(capacity);
            tables[1].resize(capacity);
        }


        ~sequential(){
            for (auto& val: tables[0]){
                val.clear();
            }
            for (auto& val: tables[1]){
                val.clear();
            }
            
        }
        bool contains(T x) {
            int hash_index0 = hash0(x) % capacity;
            // std::cout<<"capacity: "<<capacity<<std::endl;
            // std::cout << "Hash index 0: " << hash_index0 << std::endl;
            if (hash_index0 < 0 || hash_index0 >= capacity) {
                // std::cout << "Hash index 0 is out of bounds" << std::endl;
                return false; // Hash index 0 is out of bounds
            }
            // std::cout << "Checking table 0" << std::endl;
            // std::cout << "Table 0 size: " << tables[0].size() << std::endl;
            if (hash_index0 >= tables[0].size()) {
                // std::cerr << "Error: hash_index0 out of range for tables[0]" << std::endl;
                return false;
            }
            if (!tables[0][hash_index0].empty()) {
                // std::cout << "Table 0 is not empty" << std::endl;
                for (const auto& element : tables[0][hash_index0]) {
                    if (element == x) {
                        return true;
                    }
                }
            }
            
            int hash_index1 = hash1(x) % capacity;
            if (hash_index1 < 0 || hash_index1 >= capacity) {
                return false; // Hash index 1 is out of bounds
            }

            if (!tables[1][hash_index1].empty()) {
                for (const auto& element : tables[1][hash_index1]) {
                    if (element == x) {
                        return true;
                    }
                }
            }

            return false;
        }


        bool add(T x) {
            // std::cout << "Adding" << std::endl;
            if (contains(x)) {
                return false;
            }

            // std::cout << "Value not found, adding value: " << x << std::endl;
            T temp = x;
            // std::cout << "Adding value: " << temp << std::endl;
            int index = hash0(temp);
            // std::cout << "Hash index 0: " << index << std::endl;

            for (int i = 0; i < limit; i++) {
                if (tables[0][index].empty()) {
                    // std::cout << "Table 0 is empty, adding value: " << temp << std::endl;
                    tables[0][index].push_back(temp);
                    return true;
                } else {
                    // std::cout << "Swapping value in table 0" << std::endl;
                    std::swap(temp, tables[0][index][0]);
                }
                index = hash1(temp)%capacity;
                // std::cout << "Hash index 1: " << index << std::endl;
                // std::cout << "Hash table 1 size : " << tables[1].size() << std::endl;
                if (tables[1][index].empty()) {
                    // std::cout << "Table 1 is empty, adding value: " << temp << std::endl;
                    tables[1][index].push_back(temp);
                    return true;
                } else {
                    // std::cout << "Swapping value in table 1" << std::endl;
                    std::swap(temp, tables[1][index][0]);
                }
            }
            // std::cout << "Resizing tables..." << std::endl;
            resize(); // Perform resize operation

            // Retry adding after resizing
            return add(x);
        }


        bool remove(T x){
            int index = hash0(x);
            if(!tables[0][index].empty()){
                for (auto element : tables[0][index]){
                    if(element == x){
                        tables[0][index].clear();
                        return true;
                    }
                }
            }
            index = hash1(x);
            if(!tables[1][index].empty() ){
                for (auto element : tables[1][index]){
                    if(element == x){
                        tables[1][index].clear();
                        return true;
                    }
                }
                
            }
            return false;
        } 
      
        int size(){
            int count = 0;
            for (int i=0; i < capacity;i++){
                if(!tables[0][i].empty()){
                    count++;
                }
                if(!tables[1][i].empty()){
                    count++;
                }
            }
            return count;
        }
        void populate(int n, T (*gen)()) {
            for (int i = 0; i < n; ++i) {
                T value = gen();
                // std::cerr << "Adding value: " << value << std::endl;
                add(value);
            }
        }


    void print(){
        for (int i=0; i<capacity; i++){
            if(tables[0][i] != NULL){
                std::cout << tables[0][i] << std::endl;
            }
            if(tables[1][i] != NULL){
                std::cout << tables[1][i] << std::endl;
            }
        }
    }

};