#include <functional>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>

template <typename T>
class transaction
{
    private:
        vector<vector<vector<T*>>> tables;
        int capacity;
        bool isResize;

        int N;
        int LIMIT;
        int THRESHOLD;
        int PROBE_SIZE;

        int hash0(T x) {
            int hashvalue = hash<T>{}(x);
            return abs((hashvalue * 1567) % 16759);
        }

        int hash1(T x) {
            int hashvalue = hash<T>{}(x);
            return abs((hashvalue * 1913) % 19841);
        }

        void resizeTables(int oldCapacity){

            vector<vector<vector<T*>>> tmp_tables = tables;

            __transaction_atomic{
                if(oldCapacity != capacity){
                    return;
                }

                capacity *= 2;

                tables.resize(2);
                for(int i = 0; i < 2; ++i){
                    tables[i].resize(capacity);
                    for(int j = 0; j < capacity; ++j){
                        tables[i][j].resize(PROBE_SIZE, nullptr);
                    }
                }
            }

            for(int i = 0; i < 2; ++i){
                for(int j = 0; j < oldCapacity; ++j){
                    for(int z = 0; z < PROBE_SIZE; ++z){
                        if(tmp_tables[i][j][z] != NULL){
                            add(*tmp_tables[i][j][z]);
                        }
                    }
                }
            }
        }

        void resize(int oldCapacity, bool useLock = true){
            resizeTables(oldCapacity);
        }

        int probe_set_size(vector<T*>& s){
            int res = 0;
            for(size_t i = 0; i < s.size(); ++i) {
                if(s[i] != nullptr) {
                    res++;
                }
            }
            return res;
        }

        bool probe_set_add(vector<T*>& s, T x){
            for(size_t i = 0; i < s.size(); ++i){
                if(s[i] == nullptr){
                    s[i] = new T(x);
                    return true;
                }
            }
            return false;
        }

        bool probe_set_remove(vector<T*>& s, T x){
            for(size_t i = 0; i < s.size(); ++i){
                if(s[i] != nullptr && *s[i] == x){
                    delete s[i];
                    for(size_t j = i + 1; j < s.size(); ++j) {
                        s[j - 1] = s[j];
                    }
                    s[s.size() - 1] = nullptr;
                    return true;
                }
            }
            return false;
        }

        bool probe_set_contains(vector<T*>& s, T x){
            for(size_t i = 0; i < s.size(); ++i){
                if(s[i] != nullptr && *s[i] == x){
                    return true;
                }
            }
            return false;
        }

        bool relocate(int i, int hi, int oldCapacity){
            if(oldCapacity != capacity){
                return true;
            }

            int hj = 0;
            int j = 1 - i;

            for(int round = 0; round < LIMIT; ++round){

                vector<T*>& iSet = tables[i][hi];

                if(iSet.empty() || iSet[0] == nullptr){
                    return true;
                }

                T y = *iSet[0];

                __transaction_atomic{

                    if(oldCapacity != capacity){
                        return true;
                    }

                    if(i==0) hj = hash1(y) % capacity;
                    else if(i==1) hj = hash0(y) % capacity;

                    vector<T*>& jSet = tables[j][hj];

                    if(jSet.empty()){
                        return false;
                    }

                    if(probe_set_remove(iSet, y)){
                        if(probe_set_size(jSet) < THRESHOLD){
                            probe_set_add(jSet, y);
                            return true;
                        }
                        else if(probe_set_size(jSet) < PROBE_SIZE){
                            probe_set_add(jSet, y);
                            i = 1 - i;
                            hi = hj;
                            j = 1 - j;
                        }
                        else{
                            probe_set_add(iSet, y);
                            return false;
                        }
                    }
                    else if(probe_set_size(iSet) >= THRESHOLD){
                        continue;
                    }
                    else{
                        return true;
                    }
                }
            }

            return false;
        }

    public:
        TransactionalPhasedCuckooHashSet(int n = 1000, int limit = 10, int threshold = 2, int probe_size = 4){
            N = n;
            LIMIT = limit;
            THRESHOLD = threshold;
            PROBE_SIZE = probe_size;

            capacity = N;

            tables.resize(2);
            for(int i = 0; i < 2; ++i){
                tables[i].resize(capacity);
                for(int j = 0; j < capacity; ++j){
                    tables[i][j].resize(PROBE_SIZE, nullptr);
                }
            }
        }

        bool contains(T x){
            if(probe_set_contains(tables[0][hash0(x) % capacity], x)){
                return true;
            }
            if(probe_set_contains(tables[1][hash1(x) % capacity], x)){
                return true;
            }
            return false;
        }

        bool add(T x){
            bool mustResize = false;
            int oldCapacity;
            int i = -1, h = -1;

            __transaction_atomic{
                oldCapacity = capacity;

                if(contains(x)){
                    return false;
                }

                T *tmp = new T(x);

                int h0 = hash0(x) % capacity, h1 = hash1(x) % capacity;

                vector<T*>& set0 = tables[0][h0];
                vector<T*>& set1 = tables[1][h1];

                if(probe_set_size(set0) < THRESHOLD){
                    probe_set_add(set0, x);
                    return true;
                }
                else if(probe_set_size(set1) < THRESHOLD){
                    probe_set_add(set1, x);
                    return true;
                }
                else if(probe_set_size(set0) < PROBE_SIZE){
                    probe_set_add(set0, x);
                    i = 0;
                    h = h0;
                }
                else if(probe_set_size(set1) < PROBE_SIZE){
                    probe_set_add(set1, x);
                    i = 1;
                    h = h1;
                }
                else{
                    mustResize = true;
                }
            }

            if(mustResize){
                resize(oldCapacity);
                return add(x);    
            }
            else if(!relocate(i,h,oldCapacity)){
                resize(oldCapacity);
            }

            return true;
        }

        bool remove(T x){
            __transaction_atomic{
                int h0 = hash0(x) % capacity;
                if(probe_set_remove(tables[0][h0], x)){
                    return true;
                }
                int h1 = hash1(x) % capacity;
                if(probe_set_remove(tables[1][h1], x)){
                    return true;
                }
            }
            return false;
        }

        int size(){
            int res = 0;

            for(int i = 0; i < capacity; ++i){
                res += probe_set_size(tables[0][i]);
                res += probe_set_size(tables[1][i]);
            }

            return res;
        }

        void populate(){
            int count = 0;

            srand(time(NULL));
            while(count < 1024){
                if(add(rand())){
                    count++;
                }
            }
        }

        void print(){
            for(int  i = 0; i < 2; ++i){
                for(int  j = 0; j < capacity; ++j){
                    if(!tables[i][j].empty()){
                        for(size_t z = 0; z < tables[i][j].size(); ++z){
                            if(tables[i][j][z] != nullptr){
                                cout << *tables[i][j][z] << ",";
                            } else {
                                cout << "NULL,";
                            }
                        }
                    }
                    else{
                        for(int z = 0; z < PROBE_SIZE; ++z){
                            cout << "NULL,";
                        }
                    }
                    cout << endl;
                }
                cout << endl;
            }
        }
    };
            
