#include <iostream>
#include <vector>

using namespace std;

struct geoNode {
    string locationName;
    geoNode* left;
    geoNode* right;

    geoNode(string locationName){
        this->locationName = locationName;
        left = nullptr;
        right = nullptr;
        int height; // New node is initially added at leaf
    }
};

struct dataNode {
    string dataType;
    vector<double> dataValues;
    dataNode* left;
    dataNode* right;

    dataNode(string dataType, vector<double> dataValues){
        this->dataType = dataType;
        this->dataValues = dataValues;
        left = nullptr;
        right = nullptr;
        int height; // New node is initially added at leaf
    }
};
