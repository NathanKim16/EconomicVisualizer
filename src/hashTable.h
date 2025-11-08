#ifndef HEAP_H
#define HEAP_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>



class hashTable {
private:
    float maxLoadFactor;
    int buckets;
    int fullBuckets = 0;
    std::vector<std::pair<std::string, std::string>>* arr = new std::vector<std::pair<std::string, std::string>>[buckets];
    int hash(const std::string& key, int buckets);
    void resize();

public:
    hashTable(float maxLoadFactor);
    hashTable();
    bool insert(const std::string& key, const std::string& value);
    bool remove(const std::string& key);
    std::string search(const std::string& state, const std::string& county, const std::string& attribute, const std::string& year);
    ~hashTable();
};



#endif //HEAP_H
