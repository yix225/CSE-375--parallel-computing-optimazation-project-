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
        // bool is_resize = true;

        
        

        
        int hash0(T x) {
            int value = hash<T>{}(x);
            return value % capacity;
        }

        int hash1(T x) {
            int value = hash<T>{}(x);
            const uint32_t HASH_CONSTANT = 0x9e3779b9;

            value = (value ^ (value >> 16)) * HASH_CONSTANT;
            value = (value ^ (value >> 13)) * HASH_CONSTANT;
            value = value ^ (value >> 16);
            return value % capacity;
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
            
            for(int i = 0;i<2 ; i++){
                for (int j=0; j<capacity; j++){
                    tables[i][j]= NULL;
                }
            }
        }

        ～sequential(){
            for (auto& val: tables[0]){
                val.clear();
            }
            for (auto& val: tables[1]){
                val.clear();
            }
            
        }
        bool contains(T x){
            if(tables[0][hash0(x)] != NULL && tables[0][hash0(x)] == x){
                return true;
            }
            if(tables[1][hash1(x)] != NULL && tables[1][hash1(x)] == x){
                return true;
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
                if(tables[0][index] == NULL){
                    tables[0][index].push_back(temp);
                    return true;
                }else{
                    std::swap(temp, tables[0][index][0]);
                }
                index = hash1(temp);
                if(tables[1][index] == NULL){
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
            if(table0[0][index]!=NULL && table0[index].value == x){
                return true;
            }
            index = hash1(x);
            if(table1[1][index]!=NULL&& table1[index].value == x){
                return true;
            }
            return false;
        } 
      
        int size(){
            int count = 0;
            for (int i=0; i < capacity;i++){
                if(tables[0][i] != NULL){
                    count++;
                }
                if(tables[1][i] != NULL){
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