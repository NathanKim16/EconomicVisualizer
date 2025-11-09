#include "Visualization.h"
#include "tree.h"
#include "hashTable.h"

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
#include <cmath>

using namespace std;

namespace Visualization {

    // UI Helpers
    struct Button {
        sf::RectangleShape box;
        sf::Text label;
        string id;
        bool contains(sf::Vector2f p) const { return box.getGlobalBounds().contains(p); }
    };

    struct InputBox {
        sf::RectangleShape box;
        sf::Text text;
        sf::Text placeholder;
        string value;
        bool focused = false;

        // "any" | "digits" | "letters" | "lettersOnly"
        string mode = "any";
        size_t maxLen = SIZE_MAX;
        bool   forceUpper = false;

        void setFocused(bool f){ focused = f; box.setOutlineColor(f ? sf::Color(120,160,255) : sf::Color(80,90,110)); }
        bool contains(sf::Vector2f p) const { return box.getGlobalBounds().contains(p); }
        void draw(sf::RenderWindow& win){
            win.draw(box);
            if (value.empty()) win.draw(placeholder); else win.draw(text);
        }
        void handleText(sf::Uint32 unicode){
            if (!focused) return;
            if (unicode == 8) { if (!value.empty()){ value.pop_back(); text.setString(value);} return; }
            if (unicode < 32 || unicode > 126) return;
            char ch = static_cast<char>(unicode);
            if (mode=="digits" && !isdigit((unsigned char)ch)) return;
            if (mode=="letters" && !(isalpha((unsigned char)ch) || ch==' ' || ch=='-' || ch==',')) return;
            if (mode=="lettersOnly" && !isalpha((unsigned char)ch)) return;
            if (forceUpper) ch = (char)toupper((unsigned char)ch);
            if (value.size() >= maxLen) return;
            value.push_back(ch); text.setString(value);
        }
    };

    // Color Helpers
    static inline unsigned packRGB(const sf::Color& c) {
        return (unsigned(c.r) << 16) | (unsigned(c.g) << 8) | unsigned(c.b);
    }
    static int hex2(char a, char b) {
        auto v = [&](char c)->int {
            if ('0'<=c && c<='9') return c - '0';
            if ('a'<=c && c<='f') return 10 + (c - 'a');
            if ('A'<=c && c<='F') return 10 + (c - 'A');
            return 0;
        };
        return (v(a) << 4) | (v(b));
    }
    static bool isHex7(const string& s) {
        if (s.size()!=7 || s[0] != '#') return false;
        auto ok=[&](char c){ return isdigit((unsigned char)c) || (c>='a'&&c<='f') || (c>='A'&&c<='F'); };
        for (int i=1;i<7;i++) if (!ok(s[i])) return false;
        return true;
    }
    static sf::Color HEX7(const string& s) {
        return sf::Color(hex2(s[1],s[2]), hex2(s[3],s[4]), hex2(s[5],s[6]));
    }
    static string trim(string s){
        auto issp=[](unsigned char c){ return isspace(c)!=0; };
        s.erase(s.begin(), find_if(s.begin(), s.end(), [&](char c){ return !issp((unsigned char)c); }));
        s.erase(find_if(s.rbegin(), s.rend(), [&](char c){ return !issp((unsigned char)c); }).base(), s.end());
        return s;
    }
    static string findFile(const char* name){
        const char* dirs[] = {"","./","./assets/","../","./cmake-build-debug/","../../","data/","./data/"};
        for (auto d: dirs){
            string p = string(d) + name;
            ifstream f(p.c_str(), ios::binary);
            if (f.good()) return p;
        }
        return {};
    }

    // US State Images and Borders
    static bool loadIdsCSV(
            const char* path,
            unordered_map<unsigned,string>& colorToAbbr,
            unordered_map<string,unsigned>& abbrToColor
    ){
        ifstream in(path, ios::binary);
        if (!in) return false;
        string line; bool any=false;
        while (getline(in, line)) {
            if (line.size()>=3 &&
                (unsigned char)line[0]==0xEF &&
                (unsigned char)line[1]==0xBB &&
                (unsigned char)line[2]==0xBF) line.erase(0,3);
            line = trim(line);
            if (line.empty() || line[0]=='#') continue;
            size_t h = line.find('#');
            if (h==string::npos || h+7>line.size()) continue;
            string hex = line.substr(h,7);
            if (!isHex7(hex)) continue;
            string ab = trim(line.substr(0,h));
            while (!ab.empty() && (ab.back()==','||ab.back()=='-'||ab.back()==':')) ab.pop_back();
            ab = trim(ab);
            if (ab.size()!=2) continue;
            for (auto& c: ab) c = (char)toupper((unsigned char)c);
            unsigned key = packRGB(HEX7(hex));
            colorToAbbr[key] = ab;
            abbrToColor[ab]  = key;
            any = true;
        }
        return any;
    }
    static unsigned nearestKey(const sf::Color& c, const vector<unsigned>& keys){
        int best = 1e9; unsigned bestKey = 0;
        for (unsigned k : keys){
            int r=(k>>16)&255, g=(k>>8)&255, b=k&255;
            int dr=int(c.r)-r, dg=int(c.g)-g, db=int(c.b)-b;
            int d2 = dr*dr + dg*dg + db*db;
            if (d2 < best){ best=d2; bestKey=k; }
        }
        return bestKey;
    }
    static vector<unsigned> classifyLabels(
            const sf::Image& idImg,
            const unordered_map<unsigned,string>& colorToAbbr,
            const vector<unsigned>& keys,
            unsigned char alphaMin = 16
    ){
        auto sz = idImg.getSize();
        vector<unsigned> labels(sz.x*sz.y, 0);
        for (unsigned y=0; y<sz.y; ++y){
            for (unsigned x=0; x<sz.x; ++x){
                sf::Color c = idImg.getPixel(x,y);
                if (c.a < alphaMin) { labels[y*sz.x+x] = 0; continue; }
                unsigned key = packRGB(c);
                if (colorToAbbr.count(key)) { labels[y*sz.x+x] = key; continue; }
                labels[y*sz.x+x] = nearestKey(c, keys);
            }
        }
        return labels;
    }
    static vector<unsigned char> buildBorderMaskFromLabels(
            const vector<unsigned>& labels, unsigned W, unsigned H
    ){
        vector<unsigned char> border(W*H, 0);
        auto at=[&](int x,int y)->unsigned{
            return (x>=0 && y>=0 && x<(int)W && y<(int)H) ? labels[y*W+x] : 0;
        };
        for (unsigned y=0; y<H; ++y){
            for (unsigned x=0; x<W; ++x){
                unsigned me = labels[y*W + x];
                if (me==0) continue;
                bool b = (at(x-1,y)!=me) || (at(x+1,y)!=me) || (at(x,y-1)!=me) || (at(x,y+1)!=me);
                border[y*W + x] = b ? 1 : 0;
            }
        }
        return border;
    }
    static void repaintFromLabels(
            const vector<unsigned>& labels,
            const vector<unsigned char>& border,
            unsigned W, unsigned H,
            const unordered_map<unsigned,sf::Color>& stateColorLUT,
            sf::Image& out
    ){
        out.create(W, H, sf::Color(0,0,0,0));
        for (unsigned y=0; y<H; ++y){
            for (unsigned x=0; x<W; ++x){
                unsigned i = y*W + x;
                unsigned key = labels[i];
                if (key==0) continue;
                if (border[i]) { out.setPixel(x,y, sf::Color::Black); continue; }
                auto it = stateColorLUT.find(key);
                out.setPixel(x,y, (it!=stateColorLUT.end()) ? it->second : sf::Color(0,0,0,0));
            }
        }
    }

    // Color Palette
    static array<sf::Color,5> buildShades(sf::Color base) {
        float factors[5] = {0.60f, 0.80f, 1.00f, 1.15f, 1.30f};
        array<sf::Color,5> shades{};
        auto clamp255 = [](int v){ return (v<0)?0:((v>255)?255:v); };
        for (int i=0;i<5;i++){
            int r = clamp255(int(base.r * factors[i]));
            int g = clamp255(int(base.g * factors[i]));
            int b = clamp255(int(base.b * factors[i]));
            shades[i] = sf::Color((sf::Uint8)r,(sf::Uint8)g,(sf::Uint8)b);
        }
        return shades;
    }
    static int bucketIndex(float x, float lo, float hi, int buckets=5){
        if (hi<=lo) return 0;
        float t = (x - lo) / (hi - lo);
        if (t<0) t=0; if (t>0.999999f) t=0.999999f;
        return int(t * buckets);
    }
    static string fmtPct(float x){
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f%%", x);
        return string(buf);
    }
    static string fmtNum(float x){
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3f", x);
        return string(buf);
    }

    // Attribute toggle function
    static const vector<string> kAttributes = {
            "Civilian_labor_force",
            "Employed",
            "Med_HH_Income_Percent_of_State_Total",
            "Median_Household_Income",
            "Metro",
            "Rural_Urban_Continuum_Code",
            "Unemployed",
            "Unemployment_Rate",
            "Urban_Influence_Code"
    };

    // State ordering
    static const vector<string> kStateOrder = {
            "AL","AK","AZ","AR","CA","CO","CT","DE","FL","GA",
            "HI","ID","IL","IN","IA","KS","KY","LA","ME","MD",
            "MA","MI","MN","MS","MO","MT","NE","NV","NH","NJ",
            "NM","NY","NC","ND","OH","OK","OR","PA","RI","SC",
            "SD","TN","TX","UT","VT","VA","WA","WV","WI","WY"
    };

    // MAIN STUFF
    int visualizer(Tree tree, hashTable hashData, vector<float> stateData){
        string pngPath = findFile("usa_color_ids.png"); if (pngPath.empty()) pngPath = findFile("data/usa_color_ids.png");
        string csvPath = findFile("ids.csv");          if (csvPath.empty())  csvPath = findFile("data/ids.csv");
        if (pngPath.empty() || csvPath.empty()){ cerr<<"Missing usa_color_ids.png or ids.csv\n"; return 1; }

        sf::Image idImage;
        if (!idImage.loadFromFile(pngPath)){ cerr<<"Failed to load "<<pngPath<<"\n"; return 1; }
        unsigned imgW = idImage.getSize().x, imgH = idImage.getSize().y;

        unordered_map<unsigned,string> colorToAbbr;
        unordered_map<string,unsigned> abbrToColor;
        if (!loadIdsCSV(csvPath.c_str(), colorToAbbr, abbrToColor)){ cerr<<"Cannot load ids.csv\n"; return 1; }

        vector<unsigned> keys; keys.reserve(colorToAbbr.size());
        for (auto& kv : colorToAbbr) keys.push_back(kv.first);

        vector<unsigned> labelImage = classifyLabels(idImage, colorToAbbr, keys, 16);
        vector<unsigned char> borderMask = buildBorderMaskFromLabels(labelImage, imgW, imgH);

        unordered_map<unsigned,sf::Color> stateColorLUT;
        sf::Image coloredImage;
        sf::Texture mapTexture;
        sf::Sprite mapSprite;

        // Window layout
        const unsigned WIN_W = 1200, WIN_H = 700;
        const float PAD = 12.f;
        const float HEADER_H = 56.f;
        const float SIDEBAR_W = 360.f;

        sf::RenderWindow win(sf::VideoMode(WIN_W, WIN_H), "Economic Visualizer", sf::Style::Close);
        const sf::Color WINDOW_BG(235,240,248);

        float mapAreaW = WIN_W - SIDEBAR_W - 3*PAD;
        float mapAreaH = WIN_H - HEADER_H - 2*PAD;
        float scale = std::min(mapAreaW / float(imgW), mapAreaH / float(imgH));
        float mapX = PAD;
        float mapY = HEADER_H + PAD + (mapAreaH - imgH*scale)*0.5f;

        // Font
        string fontPath = findFile("font.ttf"); if (fontPath.empty()) fontPath = findFile("data/font.ttf");
        if (fontPath.empty()){ cerr<<"Missing font.ttf\n"; return 1; }
        sf::Font uiFont; if (!uiFont.loadFromFile(fontPath)){ cerr<<"Font load failed\n"; return 1; }

        // Header
        sf::Text header; header.setFont(uiFont); header.setCharacterSize(22);
        header.setFillColor(sf::Color(30,40,55));
        header.setString("US Map - Need Index");
        header.setPosition(PAD, (HEADER_H - 22.f)/2.f - 1.f);

        // Sidebar
        const float sideX = WIN_W - SIDEBAR_W - PAD;
        const float sideY = HEADER_H;
        const float sideH = WIN_H - HEADER_H - PAD;

        sf::RectangleShape sidebarPanel;
        sidebarPanel.setPosition(sideX, sideY);
        sidebarPanel.setSize({SIDEBAR_W, sideH});
        sidebarPanel.setFillColor(sf::Color(18,18,22));
        sidebarPanel.setOutlineThickness(1.f);
        sidebarPanel.setOutlineColor(sf::Color(90,90,110));

        float y = sideY + 12.f;
        auto nextY = [&](float h){ float old=y; y += h + 10.f; return old; };

        sf::Text about; about.setFont(uiFont); about.setCharacterSize(16); about.setFillColor(sf::Color(230,230,235));
        about.setString("Economic Visualizer\nUse the search to find a certain\ndata point using both a hash\ntable and a B tree and compare\ntheir time complexities.");
        about.setPosition(sideX + 12.f, nextY(76.f));

        auto makeInput = [&](const string& ph, const string& mode)->InputBox{
            InputBox ib;
            ib.box.setSize({SIDEBAR_W - 24.f, 34.f});
            ib.box.setPosition(sideX + 12.f, 0.f);
            ib.box.setFillColor(sf::Color(245,245,248));
            ib.box.setOutlineThickness(1.f);
            ib.box.setOutlineColor(sf::Color(80,90,110));
            ib.text.setFont(uiFont); ib.text.setCharacterSize(18); ib.text.setFillColor(sf::Color(30,40,55));
            ib.placeholder.setFont(uiFont); ib.placeholder.setCharacterSize(18); ib.placeholder.setFillColor(sf::Color(150,160,175));
            ib.placeholder.setString(ph);
            ib.mode = mode;
            return ib;
        };

        InputBox yearInput   = makeInput("Year (ex: 2001)", "digits");
        yearInput.box.setPosition(sideX + 12.f, nextY(34.f));
        yearInput.text.setPosition(yearInput.box.getPosition().x + 10.f, yearInput.box.getPosition().y + 6.f);
        yearInput.placeholder.setPosition(yearInput.text.getPosition());

        InputBox stateInput  = makeInput("State (2 letters)", "lettersOnly");
        stateInput.maxLen = 2; stateInput.forceUpper = true;
        stateInput.box.setPosition(sideX + 12.f, nextY(34.f));
        stateInput.text.setPosition(stateInput.box.getPosition().x + 10.f, stateInput.box.getPosition().y + 6.f);
        stateInput.placeholder.setPosition(stateInput.text.getPosition());

        InputBox countyInput = makeInput("County (ex: Alachua)", "letters");
        countyInput.box.setPosition(sideX + 12.f, nextY(34.f));
        countyInput.text.setPosition(countyInput.box.getPosition().x + 10.f, countyInput.box.getPosition().y + 6.f);
        countyInput.placeholder.setPosition(countyInput.text.getPosition());

        // Attribute (click to cycle)
        Button attrBtn; attrBtn.id = "attr";
        attrBtn.box.setSize({SIDEBAR_W - 24.f, 34.f});
        attrBtn.box.setPosition(sideX + 12.f, nextY(34.f));
        attrBtn.box.setFillColor(sf::Color(245,245,248));
        attrBtn.box.setOutlineThickness(1.f);
        attrBtn.box.setOutlineColor(sf::Color(80,90,110));
        attrBtn.label.setFont(uiFont); attrBtn.label.setCharacterSize(12); attrBtn.label.setFillColor(sf::Color(30,40,55));
        size_t attrIdx=0;
        attrBtn.label.setString(string("Attribute: ") + kAttributes[attrIdx]);
        attrBtn.label.setPosition(attrBtn.box.getPosition().x + 10.f, attrBtn.box.getPosition().y + 6.f);

        Button searchBtn; searchBtn.id = "search";
        searchBtn.box.setSize({SIDEBAR_W - 24.f, 38.f});
        searchBtn.box.setPosition(sideX + 12.f, nextY(38.f));
        searchBtn.box.setFillColor(sf::Color(245,245,248));
        searchBtn.box.setOutlineThickness(1.f);
        searchBtn.box.setOutlineColor(sf::Color(80,90,110));
        searchBtn.label.setFont(uiFont); searchBtn.label.setCharacterSize(18); searchBtn.label.setFillColor(sf::Color(30,40,55));
        searchBtn.label.setString("Search");
        searchBtn.label.setPosition(searchBtn.box.getPosition().x + 10.f, searchBtn.box.getPosition().y + 7.f);

        // Output
        sf::RectangleShape outputPanel; outputPanel.setFillColor(sf::Color(24,24,30)); outputPanel.setOutlineThickness(1.f); outputPanel.setOutlineColor(sf::Color(90,90,110));
        outputPanel.setPosition(sideX + 12.f, nextY(56.f));
        outputPanel.setSize({SIDEBAR_W - 24.f, 56.f});
        sf::Text outputText; outputText.setFont(uiFont); outputText.setCharacterSize(22); outputText.setFillColor(sf::Color(230,230,235));
        outputText.setString("");
        outputText.setPosition(outputPanel.getPosition().x + 10.f, outputPanel.getPosition().y + 10.f);

        // Timings
        sf::RectangleShape cxPanel; cxPanel.setFillColor(sf::Color(24,24,30)); cxPanel.setOutlineThickness(1.f); cxPanel.setOutlineColor(sf::Color(90,90,110));
        cxPanel.setPosition(sideX + 12.f, nextY(70.f));
        cxPanel.setSize({SIDEBAR_W - 24.f, 70.f});
        sf::Text cxText; cxText.setFont(uiFont); cxText.setCharacterSize(14); cxText.setFillColor(sf::Color(230,230,235));
        cxText.setString("Hash:   time - ms\nB-tree: time - ms)");
        cxText.setPosition(cxPanel.getPosition().x + 10.f, cxPanel.getPosition().y + 6.f);

        // Key
        float remainingH = (sideY + sideH) - (y + 10.f);
        if (remainingH < 150.f) remainingH = 150.f;
        sf::RectangleShape legendPanel; legendPanel.setFillColor(sf::Color(24,24,30)); legendPanel.setOutlineThickness(1.f); legendPanel.setOutlineColor(sf::Color(90,90,110));
        legendPanel.setPosition(sideX + 12.f, y);
        legendPanel.setSize({SIDEBAR_W - 24.f, remainingH});

        sf::Text legendTitle; legendTitle.setFont(uiFont); legendTitle.setCharacterSize(16); legendTitle.setFillColor(sf::Color(230,230,235));
        legendTitle.setString("Key based on our Need Index");
        legendTitle.setPosition(legendPanel.getPosition().x + 10.f, legendPanel.getPosition().y + 6.f);

        array<sf::RectangleShape,5> swatch;
        array<sf::Text,5> swatchText{};
        auto shade = buildShades(sf::Color(220,60,60));
        float rowY = legendPanel.getPosition().y + 34.f;
        for (int i=0;i<5;i++){
            swatch[i].setSize({ 32.f, 18.f });
            swatch[i].setPosition(legendPanel.getPosition().x + 10.f, rowY + i*(18.f + 8.f));
            swatch[i].setOutlineThickness(1.f);
            swatch[i].setOutlineColor(sf::Color(40, 40, 50));
            swatch[i].setFillColor(shade[4-i]);
            swatchText[i].setFont(uiFont); swatchText[i].setCharacterSize(14); swatchText[i].setFillColor(sf::Color(230,230,235));
            swatchText[i].setString("-");
            swatchText[i].setPosition(swatch[i].getPosition().x + 32.f + 8.f, swatch[i].getPosition().y - 1.f);
        }

        // Name map for hover
        unordered_map<string,string> abbrName = {
                {"AL","Alabama"},{"AK","Alaska"},{"AZ","Arizona"},{"AR","Arkansas"},{"CA","California"},
                {"CO","Colorado"},{"CT","Connecticut"},{"DE","Delaware"},{"FL","Florida"},{"GA","Georgia"},
                {"HI","Hawaii"},{"ID","Idaho"},{"IL","Illinois"},{"IN","Indiana"},{"IA","Iowa"},
                {"KS","Kansas"},{"KY","Kentucky"},{"LA","Louisiana"},{"ME","Maine"},{"MD","Maryland"},
                {"MA","Massachusetts"},{"MI","Michigan"},{"MN","Minnesota"},{"MS","Mississippi"},{"MO","Missouri"},
                {"MT","Montana"},{"NE","Nebraska"},{"NV","Nevada"},{"NH","New Hampshire"},{"NJ","New Jersey"},
                {"NM","New Mexico"},{"NY","New York"},{"NC","North Carolina"},{"ND","North Dakota"},{"OH","Ohio"},
                {"OK","Oklahoma"},{"OR","Oregon"},{"PA","Pennsylvania"},{"RI","Rhode Island"},{"SC","South Carolina"},
                {"SD","South Dakota"},{"TN","Tennessee"},{"TX","Texas"},{"UT","Utah"},{"VT","Vermont"},
                {"VA","Virginia"},{"WA","Washington"},{"WV","West Virginia"},{"WI","Wisconsin"},{"WY","Wyoming"}
        };

        // Pair each state  with a value from stateData.
        unordered_map<string,float> perState;
        {
            size_t n = min(kStateOrder.size(), stateData.size());
            for (size_t i=0;i<n;i++){
                perState[kStateOrder[i]] = stateData[i];
            }
            // compute min/max for legend
            float lo=1e9f, hi=-1e9f;
            for (auto& kv: perState){ lo=min(lo, kv.second); hi=max(hi, kv.second); }
            if (!(lo<hi)) { lo=0.f; hi=10.f; }
            for (int i=0;i<5;i++){
                float a = lo + (hi-lo)* (i/5.f);
                float b = lo + (hi-lo)* ((i+1)/5.f);
                swatchText[i].setString(fmtNum(a) + " - " + fmtNum(b));
            }
            // LUT
            stateColorLUT.clear();
            for (auto& kv: perState){
                auto itKey = abbrToColor.find(kv.first);
                if (itKey==abbrToColor.end()) continue;
                int bi = bucketIndex(kv.second, lo, hi, 5);
                if (bi<0) bi=0; if (bi>4) bi=4;
                stateColorLUT[itKey->second] = shade[4-bi];
            }
            // paint once
            repaintFromLabels(labelImage, borderMask, imgW, imgH, stateColorLUT, coloredImage);
            mapTexture.loadFromImage(coloredImage); mapTexture.setSmooth(false);
            mapSprite.setTexture(mapTexture, true);
            mapSprite.setScale(scale, scale);
            mapSprite.setPosition(mapX, mapY);
        }

        // Hover tooltip
        sf::Text tip; tip.setFont(uiFont); tip.setCharacterSize(14); tip.setFillColor(sf::Color::White);
        sf::RectangleShape tipBg; tipBg.setFillColor(sf::Color(20,24,32,210)); tipBg.setOutlineThickness(1.f); tipBg.setOutlineColor(sf::Color(60,70,90));

        auto getStateStatString = [&](const string& ab)->string{
            auto it = perState.find(ab);
            if (it == perState.end()) return ab;
            return ab + " - " + fmtNum(it->second);
        };

        // Search
        auto doSearch = [&](){
            string yearStr = trim(yearInput.value);
            string st2 = trim(stateInput.value);
            string county = trim(countyInput.value);
            string attribute = kAttributes[attrIdx];

            if (yearStr.empty() || st2.size()!=2 || county.empty()){
                outputText.setString("");
                cxText.setString("Hash:   time - ms \nB-tree: time - ms");
                return;
            }

            string attrHash = (attribute == "Unemployment_Rate") ? "Unemployment_rate" : attribute;
            string attrTree = attrHash;

            auto trim_ic = [](string s){
                auto issp=[](unsigned char c){ return isspace(c)!=0; };
                s.erase(s.begin(), find_if(s.begin(), s.end(), [&](char c){ return !issp((unsigned char)c); }));
                s.erase(find_if(s.rbegin(), s.rend(), [&](char c){ return !issp((unsigned char)c); }).base(), s.end());
                return s;
            };
            auto toLower = [](string s){ for (char& c: s) c=(char)tolower((unsigned char)c); return s; };
            auto ends_with_ic = [&](const string& s, const string& suf){
                string A = toLower(s), B = toLower(suf);
                if (B.size()>A.size()) return false;
                return equal(B.rbegin(), B.rend(), A.rbegin());
            };
            auto titleCase = [](string s){
                bool nw=true;
                for (char& c: s){
                    unsigned char uc=c;
                    if (isalpha(uc)){
                        c = nw ? (char)toupper(uc) : (char)tolower(uc);
                        nw=false;
                    }else{
                        nw = (c==' '||c=='-'||c=='\t');
                    }
                }
                return s;
            };
            auto isNA = [&](const string& s){
                string t = toLower(trim_ic(s));
                return t.empty() ||
                       t=="n/a" || t=="na" || t=="null" || t=="none" ||
                       t=="not found" || t=="notfound" || t=="no data" || t=="missing";
            };

            vector<string> countyCands;
            string c0 = trim_ic(county);
            string c1 = titleCase(c0);
            countyCands.push_back(c0);
            if (!ends_with_ic(c0, " county")) countyCands.push_back(c0 + " County");
            if (c1 != c0) {
                countyCands.push_back(c1);
                if (!ends_with_ic(c1, " county")) countyCands.push_back(c1 + " County");
            }

            auto timeCall = [&](auto&& fn)->pair<string,double>{
                auto tA = std::chrono::steady_clock::now();
                string v = fn();
                auto tB = std::chrono::steady_clock::now();
                double ms = std::chrono::duration<double, std::milli>(tB - tA).count();
                return {v, ms};
            };
            auto [hv, hms] = timeCall([&](){
                for (const auto& c : countyCands){
                    string v = hashData.search(st2, c, attrHash, yearStr);
                    if (!isNA(v)) return v;
                }
                if (attrHash=="Unemployment_rate"){
                    for (const auto& c : countyCands){
                        string v = hashData.search(st2, c, "Unemployment_Rate", yearStr);
                        if (!isNA(v)) return v;
                    }
                }
                return string("");
            });

            auto [tv, tms] = timeCall([&](){
                for (const auto& c : countyCands){
                    string v = tree.searchValue(st2, c, attrTree, yearStr);
                    if (!isNA(v)) return v;
                }
                if (attrTree=="Unemployment_rate"){
                    for (const auto& c : countyCands){
                        string v = tree.searchValue(st2, c, "Unemployment_Rate", yearStr);
                        if (!isNA(v)) return v;
                    }
                }
                return string("");
            });

            string shown = !isNA(hv) ? hv : (!isNA(tv) ? tv : "");
            outputText.setString(shown);

            char buf[160];
            snprintf(buf, sizeof(buf), "Hash:   time %.3f ms\nB-tree: time %.3f ms", hms, tms);
            cxText.setString(buf);
        };

        // Event loop
        while (win.isOpen()){
            sf::Event e;
            while (win.pollEvent(e)){
                if (e.type==sf::Event::Closed) win.close();
                if (e.type==sf::Event::KeyPressed && e.key.code==sf::Keyboard::Escape) win.close();

                if (e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){
                    sf::Vector2f m(float(e.mouseButton.x), float(e.mouseButton.y));
                    auto clearFocus=[&](){ yearInput.setFocused(false); stateInput.setFocused(false); countyInput.setFocused(false); };
                    if (yearInput.contains(m)){ clearFocus(); yearInput.setFocused(true); }
                    else if (stateInput.contains(m)){ clearFocus(); stateInput.setFocused(true); }
                    else if (countyInput.contains(m)){ clearFocus(); countyInput.setFocused(true); }
                    else clearFocus();

                    if (attrBtn.contains(m)){ attrIdx = (attrIdx + 1) % kAttributes.size();
                        attrBtn.label.setString(string("Attribute: ") + kAttributes[attrIdx]);
                    }
                    if (searchBtn.contains(m)) doSearch();
                }
                if (e.type==sf::Event::TextEntered){
                    yearInput.handleText(e.text.unicode);
                    stateInput.handleText(e.text.unicode);
                    countyInput.handleText(e.text.unicode);
                }
                if (e.type==sf::Event::KeyPressed && e.key.code==sf::Keyboard::Enter){
                    if (yearInput.focused || stateInput.focused || countyInput.focused) doSearch();
                }
            }

            win.clear(WINDOW_BG);
            win.draw(header);
            win.draw(mapSprite);

            win.draw(sidebarPanel);
            win.draw(about);
            yearInput.draw(win);
            stateInput.draw(win);
            countyInput.draw(win);
            win.draw(attrBtn.box);  win.draw(attrBtn.label);
            win.draw(searchBtn.box);  win.draw(searchBtn.label);
            win.draw(outputPanel); win.draw(outputText);
            win.draw(cxPanel); win.draw(cxText);
            win.draw(legendPanel); win.draw(legendTitle);
            for (int i=0;i<5;i++){ win.draw(swatch[i]); win.draw(swatchText[i]); }

            // Hover
            string hover = "";
            sf::Vector2i mp = sf::Mouse::getPosition(win);
            float lx = (mp.x - mapSprite.getPosition().x) / scale;
            float ly = (mp.y - mapSprite.getPosition().y) / scale;
            if (lx>=0 && ly>=0 && lx<(float)imgW && ly<(float)imgH){
                unsigned ix = (unsigned)lx, iy = (unsigned)ly;
                unsigned key = labelImage[iy*imgW + ix];
                if (key != 0){
                    auto it = colorToAbbr.find(key);
                    if (it!=colorToAbbr.end()){
                        string ab = it->second;
                        string name = abbrName.count(ab)? abbrName[ab] : ab;
                        string stat = getStateStatString(ab);
                        hover = name + "  (" + stat + ")";
                    }
                }
            }
            if (!hover.empty()){
                tip.setString(hover);
                auto bounds = tip.getLocalBounds();
                sf::Vector2f pos(float(mp.x)+14.f, float(mp.y)+14.f);
                float w = bounds.width + 16.f, h = bounds.height + 10.f;
                if (pos.x + w > WIN_W - 8) pos.x = WIN_W - 8 - w;
                if (pos.y + h > WIN_H - 8) pos.y = WIN_H - 8 - h;
                tipBg.setPosition(pos); tipBg.setSize({w,h});
                tip.setPosition(pos.x + 8.f - bounds.left, pos.y + 5.f - bounds.top);
                win.draw(tipBg); win.draw(tip);
            }

            win.display();
        }

        return 0;
    }

} // namespace Visualization
