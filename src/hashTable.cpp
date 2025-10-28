#include "hashTable.h"

hashTable::hashTable(float maxLoadFactor) {
    hashTable::maxLoadFactor = maxLoadFactor;
}

hashTable::~hashTable() {
    delete[] arr;
}

bool hashTable::insert(const string& key, const string& value) {
    vector<pair<string, string>>& curr = arr[hash(key, buckets)];
    for (auto & i : curr) {
        if (i.first == key) {
            return false; // key has already been inserted
        }
    }
    if (curr.empty()) {
        fullBuckets++;
    }
    curr.push_back(pair(key, value));
    if (static_cast<float>(fullBuckets)/static_cast<float>(buckets) > maxLoadFactor) { // Need to check load factor on insert
        resize();
    }
    return true;
}

bool hashTable::remove(const string& key) {
    vector<pair<string, string>>& curr = arr[hash(key, buckets)];
    for (int i = 0; i < curr.size(); i++) {
        if (curr[i].first == key) {
            for (int j = i+1; j < curr.size(); j++) {
                curr[j-1] = curr[j]; // Shifting elements forward to fill the gap
            }
            curr.pop_back();
            fullBuckets--;
            return true;
        }
    }
    return false;
}

string hashTable::search(const string& key) {
    vector<pair<string, string>>& curr = arr[hash(key, buckets)];
    for (auto & i : curr) {
        if (i.first == key) {
            return i.second;
        }
    }
    return "";
}

int hashTable::hash(const string& key, int buckets) {
    return -1;
}

void hashTable::resize() {
    int newSize = buckets*2;
    auto* newArr = new vector<pair<string, string>>[newSize];
    for (int i = 0; i < buckets; i++) {
        for (auto& item : arr[i]) {
            vector<pair<string, string>>& curr = newArr[hash(item.first, newSize)];
            curr[curr.size()] = pair(item.first, item.second); // Don't need to check duplicates
        }
    }
    delete[] arr;
    arr = newArr;
    buckets = newSize;
}

