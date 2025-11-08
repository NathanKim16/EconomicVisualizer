#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>
#include "tree.h"

using namespace std;

Tree::Tree() : root(new GeoNode("United States")) {}

Tree::~Tree() {
    delete root;
}

bool Tree::insert(string fullPath, string dataType, vector<float> values, vector<string> labels) {
    GeoNode* current = root;
    stringstream ss(fullPath);
    string segment;

    //Split by '/'
    vector<string> parts;
    while (getline(ss, segment, '/')) {
        if (!segment.empty()) parts.push_back(segment);
    }

    //Walk or create the hierarchy
    for (const auto& part : parts) {
        Node* found = current->findChild(part);
        if (found) {
            current = dynamic_cast<GeoNode*>(found);
        } else {
            current = current->emplaceChild<GeoNode>(part, current);
        }
    }

    //Add data node under this geo node
    current->emplaceChild<DataNode>(dataType, values, labels, current);
    return true;
}

void Tree::print() const {
    cout << "=== Economic Tree ===\n";
    printNode(root, 0);
    cout << "======================\n";
}

void Tree::printNode(const Node* n, int depth) const {
    if (!n) return;

    string indent(depth * 2, ' ');

    if (const auto* geo = dynamic_cast<const GeoNode*>(n)) {
        cout << indent << "Geo: " << geo->name << "  [" << geo->path() << "]\n";
        for (const auto& ch : geo->children)
            printNode(ch.get(), depth + 1);
    }
    else if (const auto* dat = dynamic_cast<const DataNode*>(n)) {
        cout << indent << "Data: " << dat->dataType << " (" << dat->values.size() << " years)\n";
        if (!dat->values.empty()) {
            cout << indent << "  values: [";
            size_t show = min<size_t>(3, dat->values.size());
            for (size_t i = 0; i < show; ++i)
                cout << fixed << setprecision(2) << dat->values[i] << (i+1 < show ? ", " : "");
            if (dat->values.size() > 6)
                cout << " ... ";
            if (dat->values.size() > 3) {
                for (size_t i = dat->values.size() - 3; i < dat->values.size(); ++i)
                    cout << fixed << setprecision(2) << dat->values[i] << (i+1 < dat->values.size() ? ", " : "");
            }
            cout << "]\n";
        }
    }
}

vector<float> Tree::getDisplayData() const
{
    vector<float> displayData;

    const GeoNode* us = root;
    for (const auto& stateUPtr : us->children) {
        const GeoNode* stateNode = dynamic_cast<const GeoNode*>(stateUPtr.get());
        if (!stateNode) continue;

        // attribute name = <sum of averages, number of counties that have it>
        map<string, pair<float, int>> attrStats;

        //Walk through every county of this state
        for (const auto& countyUPtr : stateNode->children) {
            const GeoNode* countyNode = dynamic_cast<const GeoNode*>(countyUPtr.get());
            if (!countyNode) continue;

            for (const auto& childUPtr : countyNode->children) {
                const DataNode* data = dynamic_cast<const DataNode*>(childUPtr.get());
                if (!data || data->values.empty()) continue;

                //average of the time-series for this attribute
                float sum = 0.0f;
                for (float v : data->values) sum += v;
                float avg = sum / static_cast<float>(data->values.size());

                //update running statistics for this attribute
                auto& p = attrStats[data->dataType];
                p.first  += avg;          // sum of averages
                p.second += 1;            // how many counties contributed
            }
        }

        //Build the final per-attribute state averages
        const vector<string> requiredOrder = {
            "Civilian_labor_force",
            "Employed",
            "Med_HH_Income_Percent_of_State_Total",
            "Median_Household_Income",
            "Metro",
            "Rural_Urban_Continuum_Code",
            "Unemployed",
            "Unemployment_rate",
            "Urban_Influence_Code"
        };

        vector<float> stateValues;
        stateValues.reserve(requiredOrder.size());

        for (const auto& attrName : requiredOrder) {
            auto it = attrStats.find(attrName);
            if (it == attrStats.end() || it->second.second == 0) {
                stateValues.push_back(0.0f);
            } else {
                float stateAvg = it->second.first / static_cast<float>(it->second.second);
                stateValues.push_back(stateAvg);
            }
        }

        //Debug
        // cout << stateNode->name << " Vector: ";
        // for (float v : stateValues) cout << v << " ";
        // cout << endl;

        //Special UNEMPLOYMENT Algorithm
        float stateTotal = 0.0f;
        if (stateValues.size() == 9) {
            stateTotal +=
                stateValues[0] * 0.00001f +   // Civilian_labor_force
                stateValues[1] * 0.00001f +   // Employed
                stateValues[2] * 0.001f  +   // Med_HH_Income_Percent_of_State_Total
                stateValues[3] * 0.00001f +   // Median_Household_Income
                stateValues[4] * 1.0f    +   // Metro
                stateValues[5] * 0.1f    +   // Rural_Urban_Continuum_Code
                stateValues[6] * 0.0001f +   // Unemployed
                stateValues[7] * 0.1f    +   // Unemployment_rate
                stateValues[8] * 0.1f;       // Urban_Influence_Code
        }

        displayData.push_back(stateTotal);
    }

    return displayData;
}

string Tree::searchValue(const string& stateAbbrev, const string& countyName, const string& dataType, string yearString) const {
    map<string, string> abbrevToFull = {
        {"AL", "Alabama"}, {"AK", "Alaska"}, {"AZ", "Arizona"}, {"AR", "Arkansas"},
        {"CA", "California"}, {"CO", "Colorado"}, {"CT", "Connecticut"}, {"DE", "Delaware"},
        {"DC", "District of Columbia"}, {"FL", "Florida"}, {"GA", "Georgia"}, {"HI", "Hawaii"},
        {"ID", "Idaho"}, {"IL", "Illinois"}, {"IN", "Indiana"}, {"IA", "Iowa"},
        {"KS", "Kansas"}, {"KY", "Kentucky"}, {"LA", "Louisiana"}, {"ME", "Maine"},
        {"MD", "Maryland"}, {"MA", "Massachusetts"}, {"MI", "Michigan"}, {"MN", "Minnesota"},
        {"MS", "Mississippi"}, {"MO", "Missouri"}, {"MT", "Montana"}, {"NE", "Nebraska"},
        {"NV", "Nevada"}, {"NH", "New Hampshire"}, {"NJ", "New Jersey"}, {"NM", "New Mexico"},
        {"NY", "New York"}, {"NC", "North Carolina"}, {"ND", "North Dakota"}, {"OH", "Ohio"},
        {"OK", "Oklahoma"}, {"OR", "Oregon"}, {"PA", "Pennsylvania"}, {"RI", "Rhode Island"},
        {"SC", "South Carolina"}, {"SD", "South Dakota"}, {"TN", "Tennessee"}, {"TX", "Texas"},
        {"UT", "Utah"}, {"VT", "Vermont"}, {"VA", "Virginia"}, {"WA", "Washington"},
        {"WV", "West Virginia"}, {"WI", "Wisconsin"}, {"WY", "Wyoming"}
    };
    const GeoNode* current = root;
    int year = stoi(yearString);
    vector<string> parts;

    auto it = abbrevToFull.find(stateAbbrev);
    if (it == abbrevToFull.end()){
        cout << "Unknown state abbreviation" << endl;
    }
    string stateName = it->second;

    string fullCountyName = countyName + " County";

    parts.push_back(stateName);
    parts.push_back(fullCountyName);

    //Walk the hierarchy
    for (const auto& part : parts) {
        Node* found = current->findChild(part);
        if (found) {
            current = dynamic_cast<const GeoNode*>(found);
        } else {
            return "N/A";
        }
    }

    //Find the DataNode with the specified dataType
    for (const auto& childUPtr : current->children) {
        const DataNode* data = dynamic_cast<const DataNode*>(childUPtr.get());
        if (data && data->dataType == dataType) {
            if (year >= 2000 && year < 2000 + static_cast<int>(data->values.size())) {
                // Year starts from 2000
                return to_string(data->values[year-2000]); 
            }
        }
    }

    return "N/A"; // Data not found
}