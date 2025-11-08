
#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <array>
#include <chrono>
#include <random>
using namespace std;

class Visualization {
        static inline unsigned packRGB(const sf::Color& c);
        static int hex2(char a, char b);
        static bool isHex7(const string& s);
        static sf::Color HEX7(const string& s);
        static string trim(string s);
        static void upper2_inplace(string& s);
        static string findFile(const char* name);
        static bool loadIdsCSV(
                const char* path,
                unordered_map<unsigned,string>& C2A,
                unordered_map<string,unsigned>& A2C
        );
        static unsigned nearestKey(const sf::Color& c, const vector<unsigned>& keys);
        static vector<unsigned> classifyLabels(
                const sf::Image& idImg,
                const unordered_map<unsigned,string>& C2A,
                const vector<unsigned>& keys,
                unsigned char alphaMin = 16
        );

        static vector<unsigned char> buildBorderMaskFromLabels(
                const vector<unsigned>& L, unsigned W, unsigned H
        );

        static void repaintFromLabels(
                const vector<unsigned>& L,
                const vector<unsigned char>& border,
                unsigned W, unsigned H,
                const unordered_map<unsigned,sf::Color>& lut,
                sf::Image& out
        );

        struct Button {
                sf::RectangleShape box;
                sf::Text label;
                string id;
        };

        static array<sf::Color,5> buildShades(sf::Color base);

        static int bucketIndex(float x, float lo, float hi, int buckets=5);

        static string fmtPct(float x);

public:

        static int visualizer();
};

#endif //VISUALIZATION_H
