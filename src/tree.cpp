#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "tree.h"

using namespace std;

Tree::Tree() : root(nullptr) {};

Tree::~Tree(){
    destroy(root);
}

void Tree::destroy(Tree::GeoNode* node) {
    if (node) {
        for (auto& child : node->children) {
            if (auto* geoChild = dynamic_cast<GeoNode*>(child.get())) {
                destroy(geoChild);
            }
        }
        delete node;
    }
}

bool Tree::insert(string name, string parent, string dataType, vector<float> values, vector<string> labels){
        // Split the 'name' by '/' to get the hierarchy
        size_t pos = 0;
        string token;
        GeoNode* current = root;

        while ((pos = name.find('/')) != string::npos) {
            token = name.substr(0, pos);
            Node* child = current->findChild(token);
            if (child) {
                current = dynamic_cast<GeoNode*>(child);
            } else {
                current = current->emplaceChild<GeoNode>(token);
            }
            name.erase(0, pos + 1);
        }
        // Handle the last token
        Node* child = current->findChild(name);
        if (child) {
            current = dynamic_cast<GeoNode*>(child);
        } else {
            current = current->emplaceChild<GeoNode>(name);
        }
        // Now add the DataNode under 'current'
        current->emplaceChild<DataNode>(dataType, values, labels);
        return true;

        //NOTE: Different Version
          // // Split the 'name' by '/' to get the hierarchy
        // size_t pos = 0;
        // string token;
        // GeoNode* current = root;

        // while ((pos = name.find('/')) != string::npos) {
        //     token = name.substr(0, pos);
        //     Node* child = current->findChild(token);
        //     if (!child) {
        //         // Create new GeoNode if it doesn't exist
        //         current = current->emplaceChild<GeoNode>(token);
        //     } else {
        //         current = dynamic_cast<GeoNode*>(child);
        //     }
        //     name.erase(0, pos + 1);
        // }

        // // Finally, insert the DataNode under the last GeoNode
        // current->emplaceChild<DataNode>(dataType, values, labels);
        // return true;
    }

