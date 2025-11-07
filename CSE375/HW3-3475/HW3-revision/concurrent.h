#include <vector>
#include <functional>
#include <iostream>
#include <shared_mutex>
#include <mutex>

template <typename T> 
class concurrent {
    private:
        int capacity; // number
        int limit;
        int threshold;
        int pro_size;
        int locks;
        bool is_Resize = false;
        
        // cuckoo hasing we need to table to hash
        std::vector<std::vector<T>> tables[2];

        // lock the table also two 
        std::vector<std::recursive_mutex> locks_table[2];
        
        // table 0
        int hash0(T x){
            int value = std::hash<T>{}(x);
            return value % capacity;
        }

        // table 1
        int hash1(T x) {
            int value = std::hash<T>{}(x);
            return abs((value * 1913) % 19841);

        }
        void aquire(T value){
            int index0 = hash0(value) % locks;
            int index1 = hash1(value) % locks;

            while(true){
                locks_table[0][index0].lock();
                if (locks_table[1][index1].try_lock()){
                    break;
                }else{
                    locks_table[0][index0].unlock();// release the firt lock
                }

            }
            std::this_thread::yield(); // yield the thread
            
            // locks_table[1][index1].lock();
            
        }
        void release(T value){
            int index0 = hash0(value) % locks;
            int index1 = hash1(value) % locks;

            locks_table[0][index0].unlock();
            
            locks_table[1][index1].unlock();
        }

        void resize(){
            is_Resize = true;
            int old_size = capacity;
            capacity = capacity * 2;
            // track the size of the table
            for (int i=0; i< locks; i++){
                locks_table[0][i].lock();
                locks_table[1][i].lock();
            }
            if (capacity != old_size){
                for (int i=0; i< locks; i++){
                    locks_table[0][i].unlock();
                    locks_table[1][i].unlock();
                }
                return;
            }
            std::vector<std::vector<T>> old0 = tables[0];
            std::vector<std::vector<T>> old1 = tables[1];
            
            tables[0] = std::vector<std::vector<T>>(capacity);
            tables[1] = std::vector<std::vector<T>>(capacity);

            for (int i=0; i< old_size; i++){
                for (int j=0; j< old0[i].size(); j++){
                    add(old0[i][j]);
                }
                for (int j=0; j< old1[i].size(); j++){
                    add(old1[i][j]);
                }
            }
            for (int i=0; i< locks; i++){
                locks_table[0][i].unlock();
                locks_table[1][i].unlock();
            }
        }

        bool relocate(int index, int h_index){
            int index1 = 1- index;
            int h_index1 = 0;

            for (int round = 0; round < limit; round++){
                T value = tables[index][h_index].at(0);
                if(index){
                    h_index1 = hash0(value);
                }else {
                    h_index1 = hash1(value);
                }
                aquire(value);

                bool success = false;
                for (int i=0; i< tables[index][h_index].size(); i++){
                    if(tables[index][h_index].at(i) == value){
                        tables[index][h_index].erase(tables[index][h_index].begin() + i);
                        success = true;
                        break;
                    }
                }
                if (success){
                    if (tables[index1][h_index1].size()< threshold){
                        tables[index1][h_index1].push_back(value);
                        release(value);
                        return true;
                    }else if (tables[index1][h_index1].size() < pro_size){
                        tables[index1][h_index1].push_back(value);
                        release(value);
                        index = 1- index;
                        h_index = h_index1;
                        index1 = 1- index1;
                    }else{
                        tables[index][h_index].push_back(value);
                        release(value);
                        return false;
                    }
                }else if (tables[index][h_index].size() >= threshold){
                    release(value);
                    continue;
                }else{
                    release(value);
                    return true;
                }
            }

            return false;
        }

    public:
        concurrent(int capacity, int limit, int num_locks, int threshold, int pro_size){
            this->capacity = capacity;
            this->limit = limit;
            this->locks = num_locks;
            this->threshold = threshold;
            this->pro_size = pro_size;

            // for(int i=0; i<2; i++){
            //     for (int j=0; j< capacity; j++){
            //         tables[i][j]=NULL;
            //     }
            // }
            tables[0].resize(capacity);
            tables[1].resize(capacity);

            std::vector<std::recursive_mutex> locks_table0(num_locks);
            std::vector<std::recursive_mutex> locks_table1(num_locks);

            locks_table[0].swap(locks_table0);
            locks_table[1].swap(locks_table1);

        }
       bool contains(T value) {
            aquire(value);
            int index0 = hash0(value) % capacity;
            for (const auto& element : tables[0][index0]) {
                if (element == value) {
                    release(value);
                    return true;
                }
            }
            // std::cout<<"Contains1"<<std::endl;
            int index1 = hash1(value) % capacity;
            for (const auto& element : tables[1][index1]) {
                if (element == value) {
                    release(value);
                    return true;
                }
            }
            
            release(value);
            return false;
        }

        bool add(T value){
            aquire(value);
            if(contains(value)){
                release(value);
                return false;
            }
            // std::cout<<"Add1"<<std::endl;
            int index0 = hash0(value)% capacity;
            int index1 = hash1(value)% capacity;

            bool resize_needed = false;

            int t_index = -1;
            int h_index = -1;

            if (tables[0][index0].size() < threshold){
                tables[0][index0].push_back(value);
                release(value);
                return true;
            }else if (tables[1][index1].size() < threshold){
                tables[1][index1].push_back(value);
                release(value);
                return true;
            }else if (tables[0][index0].size() < pro_size){
                tables[0][index0].push_back(value);
                release(value);
                return true;
            }else if (tables[1][index1].size() < pro_size){
                tables[1][index1].push_back(value);
                t_index = 1;
                h_index = index1;
            }else{
                resize_needed = true;
            }
            if (resize_needed){
                resize();
                add(value);
            }else if(!relocate(t_index, h_index)){
                resize();
                release(value);
                return false;

            }
            // else{
            //     continue;
            // }
            release(value);
            return true;
        }


        bool remove(T value){
            aquire(value);
            int index0 = hash0(value)% capacity;
            

            // for (auto it = tables[0][index0].begin(); it != tables[0][index0].end(); it++){
            //     if (*it == value){
            //         tables[0][index0].erase(it);
            //         release(value);
            //         return true;
            //     }
            // }
            
            for (const auto& element : tables[0][index0]) {
                if (element == value) {
                    release(value);
                    return true;
                }
            }

            int index1 = hash1(value)% capacity;

            // for (auto it = tables[1][index1].begin(); it != tables[1][index1].end(); it++){
            //     if (*it == value){
            //         tables[1][index1].erase(it);
            //         release(value);
            //         return true;
            //     }
            // }   
            for (const auto& element : tables[1][index1]) {
                if (element == value) {
                    release(value);
                    return true;
                }
            }

            release(value);
            return false;
        }


        int size(){
            int count = 0;
            for (int i=0; i < capacity;i++){
                // if(tables[0][i] != NULL){
                //     count++;
                // }
                // if(tables[1][i] != NULL){
                //     count++;
                // }
                count += tables[0][i].size();
                count += tables[1][i].size();
            }
            return count;
        }

        void populate(int n, T (*gen)()){
            for (int i = 0; i < n; ++i) {
                add(gen());
            }
        }


        void print(){
            for (int i=0; i<2; i++){
                for (int j=0; j< capacity; j++){
                    for (int k=0; k< pro_size; k++){
                        std::cout << tables[i][j][k] << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }






};