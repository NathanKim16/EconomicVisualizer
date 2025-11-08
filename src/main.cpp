//Developed by Nathan Kim, Ashtyn Connoley,
//and Nicole Wu for Project 2 of COP3503 at 
//the University of Florida.

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "tree.h" 

using namespace std;

int main() {
    //////////////////////////////////
    /////Load data from CSV files/////
    //////////////////////////////////

    //Poverty Data
    vector<vector<string>> data;
    //NOTE: Replace file path with your own local path to the data file
    ifstream file("C:/Users/Nathan Kim/Documents/Coding/C++/COP3530/EconomicVisualizer/data/cleanedPoverty2023.csv");
    if (!file.is_open()) {
        cerr << "Error opening poverty data file." << endl;
        return 1;
    }
    string line;
    while (getline(file, line)) {
        vector<string> row;
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
    file.close();

    vector<string> removeKeyStrings = {"United States", "Alabama", "Alaska", "Arizona", "Arkansas", "California", "Colorado",
                                        "Connecticut", "Delaware", "District of Columbia", "Florida", "Georgia", "Hawaii",
                                        "Idaho", "Illinois", "Indiana", "Iowa", "Kansas", "Kentucky", "Louisiana", "Maine", 
                                        "Maryland", "Massachusetts", "Michigan", "Minnesota", "Mississippi", "Missouri", "Montana", 
                                        "Nebraska", "Nevada", "New Hampshire", "New Jersey", "New Mexico", "New York", 
                                        "North Carolina", "North Dakota", "Ohio", "Oklahoma", "Oregon", "Pennsylvania", "Rhode Island",
                                        "South Carolina", "South Dakota", "Tennessee", "Texas", "Utah", "Vermont", "Virginia",
                                        "Washington", "West Virginia", "Wisconsin", "Wyoming"};
    cout << data.size() << " rows loaded from poverty data file." << endl;
    if (!data.empty()) {
        // // Remove rows with unwanted geographic names
        // for(int i=0; i<data.size(); i++){
        //     string geoName = data[i][2];
        //     if (find(removeKeyStrings.begin(), removeKeyStrings.end(), geoName) != removeKeyStrings.end()) {
        //         data.erase(data.begin() + i);
        //         i--; // Adjust index after removal
        //     }
        // }


    }
    //Save cleaned data to new CSV file
    // ofstream cleanedfile("C:/Users/Nathan Kim/Documents/Coding/C++/COP3530/EconomicVisualizer/data/cleanedUnemployment2023.csv");
    // for (const auto& row : data) {
    //     for (size_t i = 0; i < row.size(); ++i) {
    //         cleanedfile << row[i];
    //         if (i < row.size() - 1) {
    //             cleanedfile << ",";
    //         }
    //     }
    //     cleanedfile << "\n";
    // }
    // cleanedfile.close();    
    // cout << data.size() << " rows loaded to poverty data file." << endl;

    //Push data into tree structure

}