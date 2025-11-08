#include "hashTable.h"
using namespace std;

hashTable::hashTable(float maxLoadFactor) {
    hashTable::maxLoadFactor = maxLoadFactor;
}

hashTable::hashTable() {
    maxLoadFactor = 0.7f;
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
    curr.push_back(pair<string, string>(key, value));
    if (static_cast<float>(fullBuckets)/static_cast<float>(buckets) >= maxLoadFactor) { // Need to check load factor on insert
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
    stringstream ss(key);
    vector<string> keyData; // keyData[0] = State, keyData[1] = county, keyData[2] = attribute, keyData[3] = year
    string data;
    while (getline(ss, data, '-')) {
        keyData.push_back(data);
    }

    // Did some research on the golden ratio's usage in scattering:
    // https://softwareengineering.stackexchange.com/a/402543
    // Inspiration also taken from using ASCII values of letters as taught in class.

    const unsigned long base = 131;
    unsigned long hash1 = stoi(keyData[3]);

    unsigned long hash2 = 1;
    for (char c : keyData[0]) {
        hash2 *= base + isalpha(c);
    }

    unsigned long hash3 = 1;
    for (char c : keyData[1]) {
        hash3 *= base + isalpha(c);
    }

    unsigned long hash4 = 1;
    for (char c : keyData[2]) {
        hash4 *= base + isalpha(c);
    }

    // Use golden ratio fractional for a better spread.
    hash1 *= 0x9e3779b9 + hash2;
    hash1 *= 0x9e3779b9 + hash3;
    hash1 += 0x9e3779b9 + hash4;

    return hash1 % buckets;
}

void hashTable::resize() {
    int newSize = buckets*2;
    auto* newArr = new vector<pair<string, string>>[newSize];
    for (int i = 0; i < buckets; i++) {
        for (auto& item : arr[i]) {
            vector<pair<string, string>>& curr = newArr[hash(item.first, newSize)];
            curr[curr.size()] = pair<string, string>(item.first, item.second); // Don't need to check duplicates
        }
    }
    delete[] arr;
    arr = newArr;
    buckets = newSize;
}




