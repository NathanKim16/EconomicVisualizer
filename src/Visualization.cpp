#include "Visualization.h"
#include <fstream>

using namespace std;

// Helpers
inline unsigned Visualization::packRGB(const sf::Color& c) {
    return (unsigned(c.r) << 16) | (unsigned(c.g) << 8) | unsigned(c.b);
}
int Visualization::hex2(char a, char b) {
    auto v = [&](char c)->int {
        if ('0'<=c && c<='9') return c - '0';
        if ('a'<=c && c<='f') return 10 + (c - 'a');
        if ('A'<=c && c<='F') return 10 + (c - 'A');
        return 0;
    };
    return (v(a) << 4) | v(b);
}
bool Visualization::isHex7(const string& s) {
    if (s.size()!=7 || s[0] != '#') return false;
    auto ok=[&](char c){ return isdigit((unsigned char)c) || (c>='a'&&c<='f') || (c>='A'&&c<='F'); };
    for (int i=1;i<7;i++) if (!ok(s[i])) return false;
    return true;
}
sf::Color Visualization::HEX7(const string& s) {
    return sf::Color(hex2(s[1],s[2]), hex2(s[3],s[4]), hex2(s[5],s[6]));
}
string Visualization::trim(string s){
    auto issp=[](unsigned char c){ return isspace(c)!=0; };
    s.erase(s.begin(), find_if(s.begin(), s.end(), [&](char c){ return !issp((unsigned char)c); }));
    s.erase(find_if(s.rbegin(), s.rend(), [&](char c){ return !issp((unsigned char)c); }).base(), s.end());
    return s;
}
void Visualization::upper2_inplace(string& s){
    string t;
    for (char c : s) if (isalpha((unsigned char)c)) t.push_back((char)toupper((unsigned char)c));
    s = t.size()>=2 ? t.substr(0,2) : t;
}
string Visualization::findFile(const char* name){
    const char* dirs[] = {"","./","./assets/","../","./cmake-build-debug/","../../"};
    for (auto d: dirs){
        string p = string(d) + name;
        ifstream f(p.c_str(), ios::binary);
        if (f.good()) return p;
    }
    return {};
}

// State IDs
bool Visualization::loadIdsCSV(
        const char* path,
        unordered_map<unsigned,string>& C2A,
        unordered_map<string,unsigned>& A2C
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
        upper2_inplace(ab);
        if (ab.size()!=2) continue;

        unsigned key = packRGB(HEX7(hex));
        C2A[key] = ab;
        A2C[ab]  = key;
        any = true;
    }
    return any;
}

// Finds nearest color from hex value
unsigned Visualization::nearestKey(const sf::Color& c, const vector<unsigned>& keys){
    int best = 1e9; unsigned bestKey = 0;
    for (unsigned k : keys){
        int r=(k>>16)&255, g=(k>>8)&255, b=k&255;
        int dr=int(c.r)-r, dg=int(c.g)-g, db=int(c.b)-b;
        int d2 = dr*dr + dg*dg + db*db;
        if (d2 < best){ best=d2; bestKey=k; }
    }
    return bestKey;
}

// Makes each color a label
vector<unsigned> Visualization::classifyLabels(
        const sf::Image& idImg,
        const unordered_map<unsigned,string>& C2A,
        const vector<unsigned>& keys,
        unsigned char alphaMin
){
    auto sz = idImg.getSize();
    vector<unsigned> L(sz.x*sz.y, 0);

    for (unsigned y=0; y<sz.y; ++y){
        for (unsigned x=0; x<sz.x; ++x){
            sf::Color c = idImg.getPixel(x,y);
            if (c.a < alphaMin) { L[y*sz.x+x] = 0; continue; }

            unsigned key = packRGB(c);
            if (C2A.count(key)) { L[y*sz.x+x] = key; continue; }

            L[y*sz.x+x] = nearestKey(c, keys);
        }
    }
    return L;
}

// Creates mask based on different colors
vector<unsigned char> Visualization::buildBorderMaskFromLabels(
        const vector<unsigned>& L, unsigned W, unsigned H
){
    vector<unsigned char> B(W*H, 0);
    auto at=[&](int x,int y)->unsigned{
        return (x>=0 && y>=0 && x<(int)W && y<(int)H) ? L[y*W+x] : 0;
    };
    for (unsigned y=0; y<H; ++y){
        for (unsigned x=0; x<W; ++x){
            unsigned me = L[y*W + x];
            if (me==0) continue;
            bool b = (at(x-1,y)!=me) || (at(x+1,y)!=me) || (at(x,y-1)!=me) || (at(x,y+1)!=me);
            B[y*W + x] = b ? 1 : 0;
        }
    }
    return B;
}

// Change the colors based on the labels (able to change state color)
void Visualization::repaintFromLabels(
        const vector<unsigned>& L,
        const vector<unsigned char>& border,
        unsigned W, unsigned H,
        const unordered_map<unsigned,sf::Color>& lut,
        sf::Image& out
){
    out.create(W, H, sf::Color(0,0,0,0));
    for (unsigned y=0; y<H; ++y){
        for (unsigned x=0; x<W; ++x){
            unsigned i = y*W + x;
            unsigned key = L[i];
            if (key==0) continue;
            if (border[i]) { out.setPixel(x,y, sf::Color::Black); continue; }
            auto it = lut.find(key);
            out.setPixel(x,y, (it!=lut.end()) ? it->second : sf::Color(0,0,0,0));
        }
    }
}


// Key shades
array<sf::Color,5> Visualization::buildShades(sf::Color base) {
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

// Range for the keys
int Visualization::bucketIndex(float x, float lo, float hi, int buckets){
    if (hi<=lo) return 0;
    float t = (x - lo) / (hi - lo);
    if (t<0) t=0; if (t>0.999999f) t=0.999999f;
    return int(t * buckets);            // 0..4
}

// Formatting percentage in key
string Visualization::fmtPct(float x){
    char buf[32]; snprintf(buf, sizeof(buf), "%.1f%%", x);
    return string(buf);
}

int Visualization::visualizer(Tree tree, hashTable table, vector<float> stateDataTree){
    string pngPath = findFile("data/usa_color_ids.png");
    string csvPath = findFile("data/ids.csv");
    if (pngPath.empty() || csvPath.empty()){ cerr<<"Missing usa_color_ids.png or ids.csv\n"; return 1; }

    sf::Image idImg;
    if (!idImg.loadFromFile(pngPath)){ cerr<<"Failed to load "<<pngPath<<"\n"; return 1; }

    unsigned imgW = idImg.getSize().x, imgH = idImg.getSize().y;

    unordered_map<unsigned,string> C2A;
    unordered_map<string,unsigned> A2C;
    if (!loadIdsCSV(csvPath.c_str(), C2A, A2C)){ cerr<<"Cannot load ids.csv\n"; return 1; }

    vector<unsigned> keys; keys.reserve(C2A.size());
    for (auto& kv : C2A) keys.push_back(kv.first);

    auto labels = classifyLabels(idImg, C2A, keys, 16);
    auto border = buildBorderMaskFromLabels(labels, imgW, imgH);

    // Starts with uncolored map
    unordered_map<unsigned,sf::Color> lut;

    sf::Image colored;
    repaintFromLabels(labels, border, imgW, imgH, lut, colored);

    sf::Texture mapTex; mapTex.loadFromImage(colored); mapTex.setSmooth(false);
    sf::Sprite  mapSpr(mapTex);

    const unsigned WIN_W = 1200, WIN_H = 700;
    const float HEADER_H  = 64.f;
    const float SIDEBAR_W = 300.f;
    const float MARGIN_L  = 16.f;
    const float MARGIN_R  = 16.f;
    const float MARGIN_Y  = 12.f;

    sf::RenderWindow win(sf::VideoMode(WIN_W, WIN_H), "US Unemployment – Hash vs B-Tree", sf::Style::Close);

    float usableW = WIN_W - SIDEBAR_W - MARGIN_L - MARGIN_R;
    float usableH = WIN_H - HEADER_H - 2*MARGIN_Y;

    float s = std::min( usableW / float(imgW), usableH / float(imgH) ) * 0.92f;
    float mapX = MARGIN_L;
    float mapY = HEADER_H + (usableH - imgH*s)*0.5f + MARGIN_Y;

    mapSpr.setScale(s, s);
    mapSpr.setPosition(mapX, mapY);

    auto recolorAndUpload = [&](){
        repaintFromLabels(labels, border, imgW, imgH, lut, colored);
        mapTex.loadFromImage(colored);
        mapSpr.setTexture(mapTex, true);
    };

    // Font
    string fontPath = findFile("data/font.ttf");
    if (fontPath.empty()){ cerr<<"Missing font.ttf\n"; return 1; }
    sf::Font font; if (!font.loadFromFile(fontPath)){ cerr<<"Font load failed\n"; return 1; }

    // Buttons: Hash / B-Tree / Reset
    auto makeBtn = [&](const string& text, const string& id, float x) {
        Button b;
        b.id = id;
        b.box.setSize({180.f, 40.f});
        b.box.setPosition(x, (HEADER_H-40.f)/2.f);
        b.box.setFillColor(sf::Color(245,245,248));
        b.box.setOutlineThickness(1.f);
        b.box.setOutlineColor(sf::Color(80,90,110));
        b.label.setFont(font);
        b.label.setString(text);
        b.label.setCharacterSize(18);
        b.label.setFillColor(sf::Color(30,40,55));
        b.label.setPosition(x + 10.f, (HEADER_H-40.f)/2.f + 9.f);
        return b;
    };

    vector<Button> btns;
    float bx = 16.f;
    btns.push_back(makeBtn("Hash","hash",bx)); bx += 190.f;
    btns.push_back(makeBtn("B-Tree","btree",bx)); bx += 190.f;
    btns.push_back(makeBtn("Reset","reset",bx));

    // Background
    const sf::Color BG(235,240,248);

    // Info box (left panel text)
    sf::Text info;
    info.setFont(font);
    info.setCharacterSize(20);
    info.setFillColor(sf::Color(230,230,235));
    info.setString(
            "Economic Visualizer\n\n"
            "This program will analyze the unemployment\n"
            "rate of each state using hash tables and\n"
            "B trees and see which algorithm runs faster.\n\n"
            "Click one of the algorithms below to see the\n"
            "run time."
    );
    sf::RectangleShape infoBox;
    const float panelX = WIN_W - SIDEBAR_W - 25.f;
    const float panelY = HEADER_H + 100.f;
    const float panelW = SIDEBAR_W - 16.f;
    infoBox.setPosition(panelX, panelY);
    infoBox.setSize({panelW, 220.f});
    infoBox.setFillColor(sf::Color(18,18,22));
    infoBox.setOutlineThickness(1.f);
    infoBox.setOutlineColor(sf::Color(90,90,110));
    info.setPosition(panelX + 12.f, panelY + 12.f);

    // Key panel
    sf::RectangleShape keyPanel;
    keyPanel.setPosition(panelX, panelY + 240.f);
    keyPanel.setSize({panelW, 250.f});
    keyPanel.setFillColor(sf::Color(18, 18, 22));
    keyPanel.setOutlineThickness(1.f);
    keyPanel.setOutlineColor(sf::Color(90, 90, 110));

    sf::Text keyTitle;
    keyTitle.setFont(font);
    keyTitle.setString("Unemployment Key");
    keyTitle.setCharacterSize(20);
    keyTitle.setFillColor(sf::Color(230,230,235));
    keyTitle.setPosition(panelX + 12.f, panelY + 250.f);

    array<sf::RectangleShape,5> swatch;
    array<sf::Text,5> swatchText{};
    auto shades = buildShades(sf::Color(220,60,60)); // red gradient
    for (int i=0;i<5;i++){
        swatch[i].setSize({ 36.f, 24.f });
        swatch[i].setPosition(panelX + 12.f, panelY + 290.f + i*(24.f + 12.f));
        swatch[i].setOutlineThickness(1.f);
        swatch[i].setOutlineColor(sf::Color(40, 40, 50));
        swatch[i].setFillColor(shades[i]);

        swatchText[i].setFont(font);
        swatchText[i].setCharacterSize(16);
        swatchText[i].setFillColor(sf::Color(220,220,225));
        swatchText[i].setString(to_string(i+1)); // will update with ranges after first run
        swatchText[i].setPosition(panelX + 12.f + 36.f + 10.f, panelY + 290.f + i*(24.f + 12.f));
    }

    // Runtime label
    sf::Text runtime;
    runtime.setFont(font);
    runtime.setCharacterSize(18);
    runtime.setFillColor(sf::Color(30,40,55));
    runtime.setPosition(16.f, HEADER_H + 8.f);
    runtime.setString("");

    // Hover tooltip
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
    sf::Text tip; tip.setFont(font); tip.setCharacterSize(16); tip.setFillColor(sf::Color::White);
    sf::RectangleShape tipBg; tipBg.setFillColor(sf::Color(20,24,32,210)); tipBg.setOutlineThickness(1.f); tipBg.setOutlineColor(sf::Color(60,70,90));
    const float TPADX=8.f, TPADY=5.f;

    // ---- PLACEHOLDERS: plug your structures here ---------------------------------
    // Hash-based index hook: map state -> vector of unemployment samples
    unordered_map<string, vector<float>> HASH_VECTORS; // TODO: fill during your hash build

    // B-tree hook: you will iterate (state, month) in-order to produce per-state vectors.
    // This placeholder stores the same end result (state -> vector) for wiring.
    unordered_map<string, vector<float>> BTREE_VECTORS; // TODO: populate from your B-tree range scans
    // -----------------------------------------------------------------------------

    // Deterministic demo fallback if vectors are empty (so UI works now).
    auto demoRates = [&](const string& ab)->float{
        // simple seeded pseudo-values in [3%, 12%]
        unsigned h = 2166136261u;
        for (char c: ab){ h ^= (unsigned)toupper((unsigned char)c); h *= 16777619u; }
        return 3.f + float(h % 900) / 100.f; // 3.00 .. 12.00
    };

    auto computePerState = [&](const string& algo)->unordered_map<string,float>{
        unordered_map<string,float> out;
        out.reserve(64);
        if (algo=="hash"){
            if (!HASH_VECTORS.empty()){
                for (auto& kv : HASH_VECTORS){
                    const auto& vec = kv.second;
                    if (vec.empty()) continue;
                    double s=0; for (float x: vec) s+=x;
                    out[kv.first] = float(s / vec.size());
                }
                return out;
            }
        } else if (algo=="btree"){
            if (!BTREE_VECTORS.empty()){
                for (auto& kv : BTREE_VECTORS){
                    const auto& vec = kv.second;
                    if (vec.empty()) continue;
                    double s=0; for (float x: vec) s+=x;
                    out[kv.first] = float(s / vec.size());
                }
                return out;
            }
        }
        // Fallback demo: produce a single average per state
        for (auto& kv : A2C){
            out[kv.first] = demoRates(kv.first);
        }
        return out;
    };

    auto applyUnemploymentToMap = [&](const unordered_map<string,float>& avg){
        // compute min/max for buckets
        float lo=1e9f, hi=-1e9f;
        for (auto& kv: avg){ lo=min(lo, kv.second); hi=max(hi, kv.second); }
        if (!(lo<hi)) { lo=0.f; hi=10.f; }

        // update key labels with ranges
        for (int i=0;i<5;i++){
            float a = lo + (hi-lo)* (i   /5.f);
            float b = lo + (hi-lo)* ((i+1)/5.f);
            swatchText[i].setString(fmtPct(a) + " – " + fmtPct(b));
        }

        // set LUT by bucket -> shade
        lut.clear();
        for (auto& kv: avg){
            auto itKey = A2C.find(kv.first);
            if (itKey==A2C.end()) continue;
            int bi = bucketIndex(kv.second, lo, hi, 5); // 0..4
            if (bi<0) bi=0; if (bi>4) bi=4;
            lut[itKey->second] = shades[bi];
        }
        recolorAndUpload();
    };

    auto resetMap = [&](){
        lut.clear();
        recolorAndUpload();
        runtime.setString("");
        // Keep info/key visible; map becomes uncolored again.
    };

    // Initial paint (uncolored)
    recolorAndUpload();

    while (win.isOpen()){
        sf::Event e;
        while (win.pollEvent(e)){
            if (e.type==sf::Event::Closed) win.close();
            if (e.type==sf::Event::KeyPressed && e.key.code==sf::Keyboard::Escape) win.close();

            if (e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){
                sf::Vector2f m(float(e.mouseButton.x), float(e.mouseButton.y));
                for (auto& b : btns){
                    if (b.box.getGlobalBounds().contains(m)){
                        if (b.id=="reset"){
                            resetMap();
                        } else {
                            // time the selected path
                            auto tA = std::chrono::steady_clock::now();
                            auto per = computePerState(b.id); // "hash" or "btree"
                            auto tB = std::chrono::steady_clock::now();
                            double ms = std::chrono::duration<double, std::milli>(tB - tA).count();

                            applyUnemploymentToMap(per);
                            runtime.setString(string("Run time (") + b.id + "): " + to_string(ms) + " ms");
                        }
                    }
                }
            }
        }

        win.clear(BG);

        // Header buttons
        for (auto& b : btns){ win.draw(b.box); win.draw(b.label); }

        // Map
        win.draw(mapSpr);

        // Info + Key
        win.draw(infoBox);
        win.draw(info);
        win.draw(keyPanel);
        win.draw(keyTitle);
        for (int i=0;i<5;i++){ win.draw(swatch[i]); win.draw(swatchText[i]); }

        // runtime line (top-left)
        win.draw(runtime);

        // hover tooltip
        string hover = "";
        sf::Vector2i mp = sf::Mouse::getPosition(win);
        float localX = (mp.x - mapSpr.getPosition().x) / s;
        float localY = (mp.y - mapSpr.getPosition().y) / s;

        if (localX>=0 && localY>=0 && localX<(float)imgW && localY<(float)imgH){
            unsigned ix = (unsigned)localX, iy = (unsigned)localY;
            unsigned key = labels[iy*imgW + ix];
            if (key != 0){
                auto it = C2A.find(key);
                if (it!=C2A.end()){
                    string ab = it->second;
                    auto f = abbrName.find(ab);
                    hover = (f!=abbrName.end()) ? f->second : ab;
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
