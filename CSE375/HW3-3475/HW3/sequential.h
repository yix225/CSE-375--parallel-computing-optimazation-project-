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

        void resize(){
            if(!is_resize){
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



    public:
        sequential(int capacity, int limit){
            this -> capacity = capacity;
            this -> limit = limit;
            
        }

        ~sequential(){
            for (auto& val: tables[0]){
                val.clear();
            }
            for (auto& val: tables[1]){
                val.clear();
            }
            
        }
        bool contains(T x){
            int hash_index0 = hash0(x);
            for (const auto& element : tables[0][hash_index0]) {
                if (element == x) {
                    return true;
                }
            }

            int hash_index1 = hash1(x);
            for (const auto& element : tables[1][hash_index1]) {
                if (element == x) {
                    return true;
                }
            }
            return false;
        }
        bool add(T x){
            if (contains(x)) {
                return false;
            }
            T temp = x;
            int index = hash0(temp);
            for (int i=0; i<limit; i++){
                if(tables[0][index].empty()){
                    tables[0][index].push_back(temp);
                    return true;
                }else{
                    std::swap(temp, tables[0][index][0]);
                }
                index = hash1(temp);
                if(tables[1][index].empty()){
                    tables[1][index].push_back(temp);
                    return true;
                }else{
                    std::swap(temp, tables[1][index][0]);
                }

            }

            
            resize();
            add(x);
            return true;
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

    void populate(int n, T (*gen)()){
        int count = 0;
        srand(time(NULL));
        while(count < 1024){
            if(add(rand())){
                count++;
            }
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