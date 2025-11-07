#include <functional>
#include <iostream>
#include <time.h>
#include <cstring>
#include <chrono>
#include <vector>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <mutex>
// after acquire the lock , we could read this lock 

template <class T>
class transaction{
    private: 
        std::vector<std::vector<T*>> tables[2];
        int capacity;
        bool isResize;
        std::mutex resizeMtx;
        std::vector<std::vector<std::mutex>> tableLocks;

        int N;
        int limit;
        int prob_size;
        int threshold;

        int hash0(T x){
            int hashvalue = std::hash<T>{}(x);
            return hashvalue % capacity;
        }

        int hash1(T x){
            int hashvalue = std::hash<T>{}(x);
            return abs((hashvalue * 1913) % 19841);
        }
        //  __attribute__ ((transaction_pure))
        void resizeTables(int oldCapacity){
            std::vector<std::vector<T*>> tmp_tables[2];
            tmp_tables[0] = tables[0];
            tmp_tables[1] = tables[1];

            std::mutex mtx;
            std::lock_guard<std::mutex> lock(resizeMtx);
            // __transaction_atomic{
                if(oldCapacity != capacity){
                    return;
                }

                capacity *= 2;

                // tables.resize(2);
                for(int i = 0; i < 2; ++i){
                    tables[i].resize(capacity);
                    for(int j = 0; j < capacity; ++j){
                        tables[i][j].resize(prob_size, nullptr);
                    }
                }
            // }

            for(int i = 0; i < 2; ++i){
                for(int j = 0; j < oldCapacity; ++j){
                    for(int z = 0; z < prob_size; ++z){
                        if(tmp_tables[i][j][z] != NULL){
                            add(*tmp_tables[i][j][z]);
                        }
                    }
                }
            }
        }

        void resize(int oldCapacity){
            if(!isResize){
                return;
            }

            std::vector<std::vector<T>> new_table0(capacity * 2);
            std::vector<std::vector<T>> new_table1(capacity * 2);

            for(int i=0; i< 2 ; ++i){
                for(int j=0; j < capacity/2; ++j){
                    for(const auto& value : tables[i][j]){
                        int index = i == 0 ? hash0(value) : hash1(value);
                        new_table0[index].push_back(value);
                    }
                }
            }

            tables[0] = std::move(new_table0);
            tables[1] = std::move(new_table1);
            
        }
        int prob_sizes(std::vector<T*>& s){
            int res = 0;
            if(s.size() == 0){
                return 0;
            }
            for(int i = 0; i < prob_size; i++) {
                if(s[i] != nullptr) {
                    res++;
                }
            }
            return res ;
        }

        bool prob_add(std::vector<T*>& s, T x){
            for(int i = 0; i < prob_size; i++){
                if(s[i] == nullptr){
                    s[i] = new T(x);
                    return true;
                }
            }
            return false;
        }

        bool prob_remove(std::vector<T*>& s, T x){
            for(int i = 0; i < prob_size; i++){
                if(s[i] != nullptr && *s[i] == x){
                    delete s[i];
                    for(int j = i + 1; j < prob_size; j++) {
                        s[j - 1] = s[j];
                    }
                    s[prob_size - 1] = nullptr;
                    return true;
                }
            }
            return false;
        }

        bool prob_contains(std::vector<T*>& s, T x){
            for(int i = 0; i < prob_size; i++){
                if(s[i] != nullptr && *s[i] == x){
                    return true;
                }
            }
            return false;
        }

        //  __attribute__ ((transaction_pure))
        bool relocate(int index, int h_index, int old_capacity){
            if (old_capacity != capacity){
                return false;
            }
            // _transaction_atomic{
                int index1 = 1- index;
                int h_index1 = 0;

                // lock for each one 
                std::lock_guard<std::mutex> lock(tableLocks[0][h_index]);
                std::lock_guard<std::mutex> lock1(tableLocks[1][h_index1]);

            
                for (int round = 0; round < capacity; round++){
                    std::vector<T*>& value = tables[index][h_index]; 
                    if (value.empty() || value[0] == nullptr){ 
                        return true;
                    }
                    T temp = *value[0];
                    // _transaction_atomic{
                        if(old_capacity != capacity){
                            return false;
                        }  
                        if(index == 0){
                            h_index1 = hash1(temp) % capacity;
                        }else {
                            h_index1 = hash0(temp) % capacity;
                        }
                        std::vector<T*>& value1 = tables[index1][h_index1];
                        if (value1.empty() || value1[0] == nullptr){
                        return false;
                        }
                        if(prob_remove(value, temp)){
                            if(prob_sizes(value1) < threshold){
                                if(prob_add(value1, temp)){
                                    return true;
                                }
                            }else if(prob_sizes(value1)< prob_size){
                                if(prob_add(value1, temp)){
                                    index = 1 - index;
                                    h_index = h_index1;
                                    index1 = 1 - index;

                                }
                            }else{
                                prob_add(value, temp);
                                return false;
                            }
                        }else if(prob_sizes(value) >= threshold){
                            continue;
                        } else{
                            return true;
                        } 
                    // }
                    
                }
            // }
            return false;
           
        }

        public:
            //n = 1000, int limit = 10, int threshold = 2, int prob_size = 4
            transaction(int n = 1000, int limit = 10, int threshold = 2, int prob_size = 4) : N(n), limit(limit), threshold(threshold), prob_size(prob_size) {
                this->N = n;
                this->limit = limit;
                this->threshold = threshold;
                this->prob_size = prob_size;
                capacity = n;
                isResize = true;

                // Initialize tables
                for (int i = 0; i < 2; ++i) {
                    tables[i].resize(capacity);
                    for (int j = 0; j < capacity; ++j) {
                        tables[i][j].resize(prob_size, nullptr);
                    }
                }
            }


            bool contains(T x){
                if(prob_contains(tables[0][hash0(x) % capacity], x)){
                    return true;
                }
                if(prob_contains(tables[1][hash1(x) % capacity], x)){
                    return true;
                }
                return false;
            }   

            bool add(T x){
                bool mustResize = false;
                int oldCapacity;
                int i = -1, h = -1;

            
                // __transaction_atomic{
                    oldCapacity = capacity;
                
                    if(contains(x)){
                        return false;
                    }

                    T *tmp = new T(x);
                    int h0 = hash0(x) % capacity;
                    int h1 = hash1(x) % capacity;
                    std::cout<<"add"<<std::endl;
                    std::cout<<"h0: "<<h0<<std::endl;
                    std::cout<<"h1: "<<h1<<std::endl;
                    std::cout<<"h0 size: "<<prob_sizes(tables[0][h0])<<std::endl;
                    std::lock_guard<std::mutex> lock0(tableLocks[0][h0]);
                    std::lock_guard<std::mutex> lock1(tableLocks[1][h1]);
                    std::cout<<"add"<<std::endl;
                    if(prob_sizes(tables[0][h0]) < threshold){
                        if(prob_add(tables[0][h0], x)){
                            return true;
                        }
                    }else if(prob_sizes(tables[1][h1]) < threshold){
                        if(prob_add(tables[1][h1], x)){
                            return true;
                        }
                    }else if (prob_sizes(tables[0][h0]) < prob_size){
                        if(prob_add(tables[0][h0], x)){
                            i = 0;
                            h = h0;
                        }
                    }else if(prob_sizes(tables[1][h1]) < prob_size){
                        if(prob_add(tables[1][h1], x)){
                            i = 1;
                            h = h1;
                        }
                    }else{
                        mustResize = true;
                    }
                // }
                std::cout<<"must resize: "<<mustResize<<std::endl;
                if(mustResize){
                    resizeTables(oldCapacity);
                    return add(x);
                }else if(!relocate(i, h, oldCapacity)){
                    resizeTables(oldCapacity);
                    return false;
                }else{
                    return true;
                }

                return true;
            }

            bool remove(T x){
                std::lock_guard<std::mutex> lock0(tableLocks[0][hash0(x) % capacity]);
                std::lock_guard<std::mutex> lock1(tableLocks[1][hash1(x) % capacity]);

                // transaction_atomic{

                    if(prob_remove(tables[0][hash0(x) % capacity], x)){
                        return true;
                    }
                    if(prob_remove(tables[1][hash1(x) % capacity], x)){
                        return true;
                    }
                // }
                return false;
            }

            int size(){
                int count = 0;
                for(int i = 0; i < capacity; i++){
                    count += prob_sizes(tables[0][i]);
                    count += prob_sizes(tables[1][i]);
                }
                return count;
            }

            void populate(int n, T (*gen)()){
                for (int i = 0; i < n; ++i) {
                    add(gen());
                }
            }

                void print(){
                for(int  i = 0; i < 2; ++i){
                    for(int  j = 0; j < capacity; ++j){
                        if(!tables[i][j].empty()){
                            for(size_t z = 0; z < tables[i][j].size(); ++z){
                                if(tables[i][j][z] != nullptr){
                                    std::cout << *tables[i][j][z] << ",";
                                } else {
                                    std::cout  << "NULL,";
                                }
                            }
                        }
                        else{
                            for(int z = 0; z < prob_size; ++z){
                                std::cout  << "NULL,";
                            }
                        }
                        std::cout  << std::endl;
                    }
                    std::cout  << std::endl;
                }
            }
};