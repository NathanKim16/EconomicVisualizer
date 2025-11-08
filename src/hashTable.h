#ifndef HEAP_H
#define HEAP_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
using namespace std;

class hashTable {
private:
    float maxLoadFactor;
    int buckets = 100;
    int fullBuckets = 0;
    vector<pair<string, string>>* arr = new vector<pair<string, string>>[buckets];
    int hash(const string& key, int buckets);
    void resize();

public:
    hashTable(float maxLoadFactor);
    hashTable();
    bool insert(const string& key, const string& value);
    bool remove(const string& key);
    string search(const string& key);
    ~hashTable();
};



#endif //HEAP_H
