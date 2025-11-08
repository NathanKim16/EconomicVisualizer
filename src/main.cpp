//Developed by Nathan Kim, Ashtyn Connoley,
//and Nicole Wu for Project 2 of COP3503 at 
//the University of Florida.

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include "tree.h"

using namespace std;

int main() {
    //State abbreviation to full name map
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
    //Load Data
    vector<vector<string>> unemploymentData;
    //NOTE: Replace file path with your own local path to the data file
    ifstream file("C:/Users/Nathan Kim/Documents/Coding/C++/COP3530/EconomicVisualizer/data/cleanedUnemployment2023.csv");
    if (!file.is_open()) {
        cerr << "Error opening file." << endl;
        return 1;
    }
    string line;
    while (getline(file, line)) {
        vector<string> row;
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ',')) {
            //Strip quotes if present
            if (!cell.empty() && cell.front() == '"' && cell.back() == '"') {
                cell = cell.substr(1, cell.size() - 2);
            }
            row.push_back(cell);
        }
        if (!row.empty()){
            // //Print row for debugging
            // for (const auto& c : row) {
            //     cout << c << " | ";
            // }
            // cout << endl;
            unemploymentData.push_back(row);
        }
    }
    file.close();
    cout << unemploymentData.size() << " rows loaded from unemployment data file." << endl;

    //Map to hold data: path -> seriesName -> year -> value
    map<string, map<string, map<int, float>>> allData;

    for (size_t i = 1; i < unemploymentData.size(); ++i) {
        const auto& row = unemploymentData[i];
        if (row.size() < 5 || row[0].empty()){
            cout << "Skipping invalid row " << i << endl;
            continue;
        }

        string stateAbbrev = row[1];
        auto it = abbrevToFull.find(stateAbbrev);
        if (it == abbrevToFull.end()){
            cout << "Unknown state abbreviation in row " << i << endl;
            continue;
        }
        string fullState = it->second;

        string county = row[2];

        string path = fullState + "/" + county;

        string attr = row[3];
        if (attr.empty()){
            cout << "Skipping empty attribute in row " << i << endl;
            continue;
        }
        string valStr = row[4];
        float value;
        try {
            value = stof(valStr);
        } catch (const std::exception& e) {
            cout << "Invalid value in row " << i << endl;
            continue;
        }

        ///Parse attribute for base and year
        size_t usPos = attr.rfind('_');
        if (usPos == string::npos || attr.length() - usPos - 1 != 4) {
            //No year or invalid, skip or handle as no year
            cout << "Invalid attribute format in row " << i << endl;
            continue;
        }
        string yearStr = attr.substr(usPos + 1);
        int year;
        try {
            year = stoi(yearStr);
        } catch (const std::exception& e) {
            cout << "Invalid year in row " << i << endl;
            continue;
        }
        string base = attr.substr(0, usPos);

        //Store data
        allData[path][base][year] = value;
    }

    //Push data into tree structure
    Tree tree;
    for (const auto& pd : allData) {
        const string& path = pd.first;
        for (const auto& sd : pd.second) {
            const string& dataType = sd.first;
            const auto& yearMap = sd.second;
            vector<pair<int, float>> yearValues;
            for (const auto& yv : yearMap) {
                yearValues.push_back({yv.first, yv.second});
            }
            //Sort by year
            sort(yearValues.begin(), yearValues.end());
            vector<float> values;
            vector<string> labels;
            for (const auto& yv : yearValues) {
                values.push_back(yv.second);
                labels.push_back(to_string(yv.first));
            }
            tree.insert(path, dataType, values, labels);
        }
    }
    //Dearest Nicole, 
    //the following line returns the values for each state
    //in alphabetical order. For example, stateData[0] is Alabama.
    //Should be pretty easy to connect to the GUI from here. You 
    //will need to modify the path to the database, which you can 
    //do on Line 35. I also have no idea what you need for the CMAKE
    //to make this code work, I used VSCode. Good Luck!
    vector<float> stateData = tree.getDisplayData();

    cout << "States in display data: " << stateData.size() << endl;

    for(int i = 0; i < stateData.size(); ++i){
        cout << stateData[i] << ", ";
    }


    return 0;
}