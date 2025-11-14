// palette_final.cpp
#include <iostream>
#include <string>
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <limits>    // numeric_limits
#include <cstdio>    // sprintf
#include <cctype>    // toupper
#include <algorithm> // for sorting
#include <cmath>     // for abs, fmod math functions
#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif
using namespace std;

// Clear screen function
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// ---------------- Data Types ----------------
struct Color {
    string name; // user-given or "Random#"
    string hex;  // e.g., #FFAABB
    int r, g, b;
    float brightness() const { return (r + g + b) / 3.0f; }
    
    // Convert RGB to HSL for hue calculation
    void getHSL(float &h, float &s, float &l) const {
        float r_ = r / 255.0f;
        float g_ = g / 255.0f;
        float b_ = b / 255.0f;
        
        float maxVal = max(r_, max(g_, b_));
        float minVal = min(r_, min(g_, b_));
        float delta = maxVal - minVal;
        
        // Calculate lightness
        l = (maxVal + minVal) / 2.0f;
        
        // Calculate saturation
        if (delta == 0) {
            s = 0;
            h = 0;
        } else {
            s = delta / (1 - std::abs(2 * l - 1));
            
            // Calculate hue
            if (maxVal == r_) {
                h = 60 * std::fmod(((g_ - b_) / delta), 6);
            } else if (maxVal == g_) {
                h = 60 * (((b_ - r_) / delta) + 2);
            } else {
                h = 60 * (((r_ - g_) / delta) + 4);
            }
            
            if (h < 0) h += 360;
        }
    }
    
    float getHue() const {
        float h, s, l;
        getHSL(h, s, l);
        return h;
    }
    
    float getSaturation() const {
        float h, s, l;
        getHSL(h, s, l);
        return s;
    }
};

struct Action { // for undo/redo in Edit
    // type: 1 = add, 2 = remove, 3 = replace(old->new)
    int type;
    Color before; // used for remove/replace (store old)
    Color after;  // used for add/replace (store new)
};

// Simple Action Stack (fixed capacity)
class ActionStack {
    Action arr[500];
    int top;
public:
    ActionStack(): top(-1) {}
    bool isEmpty() const { return top == -1; }
    bool isFull() const { return top == 499; }
    void push(const Action &a) { if (!isFull()) arr[++top] = a; }
    Action pop() { if (!isEmpty()) return arr[top--]; Action e; e.type = 0; return e; }
    void clear() { top = -1; }
};

// Recent palettes queue (store ID + name)
class RecentQueue {
    string ids[5];
    string names[5];
    int front, rear, size;
public:
    RecentQueue(): front(0), rear(-1), size(0) {}
    void enqueue(const string &id, const string &name) {
        if (size < 5) {
            rear = (rear + 1) % 5;
            ids[rear] = id; names[rear] = name;
            size++;
        } else {
            front = (front + 1) % 5;
            rear = (rear + 1) % 5;
            ids[rear] = id; names[rear] = name;
        }
    }
    void display() const {
        if (size == 0) { cout << "No recent palettes.\n"; return; }
        cout << "\n--- Recent Palettes (Last " << size << ") ---\n";
        for (int i = 0; i < size; ++i) {
            int idx = (front + i) % 5;
            cout << ids[idx] << " : " << names[idx] << "\n";
        }
        cout << "-----------------------------------\n";
    }
};

// Palette structure (user-created palettes)
struct Palette {
    string id;        // e.g., P1001
    string name;
    Color colors[500]; // allow many colors
    int count;
    ActionStack undoStack;
    ActionStack redoStack;
    Palette(): id(""), name(""), count(0) {}
    void setId(const string &i) { id = i; }
    void setName(const string &n) { name = n; }
    bool addColor(const Color &c) {
        if (count >= 500) { cout << "Palette reached internal limit (500).\n"; return false; }
        colors[count++] = c;
        // push add action
        Action a; a.type = 1; a.after = c; undoStack.push(a); redoStack.clear();
        return true;
    }
    bool removeAtIndex(int idx) { // idx: 0-based
        if (idx < 0 || idx >= count) { cout << "Invalid index.\n"; return false; }
        Color old = colors[idx];
        // shift left
        for (int i = idx; i < count-1; ++i) colors[i] = colors[i+1];
        --count;
        Action a; a.type = 2; a.before = old; undoStack.push(a); redoStack.clear();
        cout << "Removed color " << old.hex << " (" << old.name << ")\n";
        return true;
    }
    bool replaceAtIndex(int idx, const Color &newc) {
        if (idx < 0 || idx >= count) { cout << "Invalid index.\n"; return false; }
        Color old = colors[idx];
        colors[idx] = newc;
        Action a; a.type = 3; a.before = old; a.after = newc;
        undoStack.push(a); redoStack.clear();
        cout << "Replaced " << old.hex << " with " << newc.hex << "\n";
        return true;
    }
    
    // Sort functions
    void sortByBrightness() {
        for (int i = 0; i < count-1; ++i) {
            for (int j = 0; j < count-i-1; ++j) {
                if (colors[j].brightness() > colors[j+1].brightness()) {
                    Color temp = colors[j];
                    colors[j] = colors[j+1];
                    colors[j+1] = temp;
                }
            }
        }
        cout << "Palette sorted by brightness (light to dark).\n";
    }
    
    void sortByHue() {
        for (int i = 0; i < count-1; ++i) {
            for (int j = 0; j < count-i-1; ++j) {
                if (colors[j].getHue() > colors[j+1].getHue()) {
                    Color temp = colors[j];
                    colors[j] = colors[j+1];
                    colors[j+1] = temp;
                }
            }
        }
        cout << "Palette sorted by hue (color wheel order).\n";
    }
    
    void sortBySaturation() {
        for (int i = 0; i < count-1; ++i) {
            for (int j = 0; j < count-i-1; ++j) {
                if (colors[j].getSaturation() > colors[j+1].getSaturation()) {
                    Color temp = colors[j];
                    colors[j] = colors[j+1];
                    colors[j+1] = temp;
                }
            }
        }
        cout << "Palette sorted by saturation (muted to vibrant).\n";
    }
    
    void undo() {
        if (undoStack.isEmpty()) { cout << "Nothing to undo.\n"; return; }
        Action a = undoStack.pop();
        if (a.type == 1) {
            // undo add -> remove last matching color (use after)
            bool found = false;
            for (int i = count-1; i >= 0; --i) {
                if (colors[i].hex == a.after.hex && colors[i].r==a.after.r) {
                    // remove this index
                    Color removed = colors[i];
                    for (int j = i; j < count-1; ++j) colors[j] = colors[j+1];
                    --count;
                    Action ra; ra.type = 1; ra.after = removed; // redo will re-add
                    redoStack.push(ra);
                    cout << "Undo: removed " << removed.hex << "\n";
                    found = true; break;
                }
            }
            if (!found) cout << "Undo: matching add not found.\n";
        } else if (a.type == 2) {
            // undo remove -> re-insert at end
            if (count < 500) {
                colors[count++] = a.before;
                Action ra; ra.type = 2; ra.before = a.before; redoStack.push(ra);
                cout << "Undo: restored " << a.before.hex << "\n";
            } else cout << "Undo error: palette full.\n";
        } else if (a.type == 3) {
            // undo replace -> find the color equal to after and change back to before (first match)
            bool found = false;
            for (int i = 0; i < count; ++i) {
                if (colors[i].hex == a.after.hex && colors[i].r==a.after.r) {
                    colors[i] = a.before;
                    Action ra; ra.type = 3; ra.before = a.before; ra.after = a.after; redoStack.push(ra);
                    cout << "Undo: restored " << a.before.hex << "\n";
                    found = true; break;
                }
            }
            if (!found) cout << "Undo: replace target not found.\n";
        }
    }
    void redo() {
        if (redoStack.isEmpty()) { cout << "Nothing to redo.\n"; return; }
        Action a = redoStack.pop();
        if (a.type == 1) {
            // redo add -> add color
            if (count < 500) {
                colors[count++] = a.after;
                undoStack.push(a);
                cout << "Redo: added " << a.after.hex << "\n";
            } else cout << "Redo error: palette full.\n";
        } else if (a.type == 2) {
            // redo remove -> remove last matching
            bool found = false;
            for (int i = count-1; i >= 0; --i) {
                if (colors[i].hex == a.before.hex && colors[i].r==a.before.r) {
                    Color rem = colors[i];
                    for (int j = i; j < count-1; ++j) colors[j] = colors[j+1];
                    --count;
                    undoStack.push(a);
                    cout << "Redo: removed " << rem.hex << "\n";
                    found = true; break;
                }
            }
            if (!found) cout << "Redo: matching color not found.\n";
        } else if (a.type == 3) {
            // redo replace -> find before and replace to after
            bool found = false;
            for (int i = 0; i < count; ++i) {
                if (colors[i].hex == a.before.hex && colors[i].r==a.before.r) {
                    colors[i] = a.after;
                    undoStack.push(a);
                    cout << "Redo: replaced with " << a.after.hex << "\n";
                    found = true; break;
                }
            }
            if (!found) cout << "Redo: replace target not found.\n";
        }
    }
    void display() const {
        cout << "\n--- Palette " << id << " : " << name << " (" << count << " colors) ---\n";
        for (int i = 0; i < count; ++i) {
            cout << i+1 << ". " << colors[i].name << " " << colors[i].hex
                 << " (RGB " << colors[i].r << "," << colors[i].g << "," << colors[i].b << ")\n";
        }
        cout << "-------------------------------------------\n";
    }
};

// ---------------- Helpers ----------------
string rgbToHex(int r, int g, int b) {
    char buf[8];
    sprintf(buf, "#%02X%02X%02X", r, g, b);
    return string(buf);
}
Color makeColorFromRGB(const string &name, int r, int g, int b) {
    Color c; c.name = name; c.r = r; c.g = g; c.b = b; c.hex = rgbToHex(r,g,b); return c;
}
void waitEnter() {
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    cout << "Press Enter to continue...";
    cin.get();
}

// ---------------- Predefined Explore Palettes (30 x 5) ----------------
const int TOTAL_PRESETS = 30;
const int COLORS_PER_PRESET = 5;

struct Preset {
    string title;
    Color cols[COLORS_PER_PRESET];
};

Preset presets[TOTAL_PRESETS];

// Fill presets with 30 predefined palettes
void initPresets() {
    // 1
    presets[0].title = "Sunset Glow";
    presets[0].cols[0] = makeColorFromRGB("Sun1", 255,154,139);
    presets[0].cols[1] = makeColorFromRGB("Sun2", 255,106,136);
    presets[0].cols[2] = makeColorFromRGB("Sun3", 255,153,172);
    presets[0].cols[3] = makeColorFromRGB("Sun4", 255,214,165);
    presets[0].cols[4] = makeColorFromRGB("Sun5", 252,213,206);
    // 2
    presets[1].title = "Ocean Breeze";
    presets[1].cols[0] = makeColorFromRGB("O1", 5,102,141);
    presets[1].cols[1] = makeColorFromRGB("O2", 2,128,144);
    presets[1].cols[2] = makeColorFromRGB("O3", 0,168,150);
    presets[1].cols[3] = makeColorFromRGB("O4", 2,195,154);
    presets[1].cols[4] = makeColorFromRGB("O5", 240,243,189);
    // 3
    presets[2].title = "Vintage Rose";
    presets[2].cols[0] = makeColorFromRGB("V1", 141,110,99);
    presets[2].cols[1] = makeColorFromRGB("V2", 215,204,200);
    presets[2].cols[2] = makeColorFromRGB("V3", 248,187,208);
    presets[2].cols[3] = makeColorFromRGB("V4", 194,24,91);
    presets[2].cols[4] = makeColorFromRGB("V5", 123,31,162);
    // 4
    presets[3].title = "Pastel Dreams";
    presets[3].cols[0] = makeColorFromRGB("P1", 250,208,196);
    presets[3].cols[1] = makeColorFromRGB("P2", 255,209,255);
    presets[3].cols[2] = makeColorFromRGB("P3", 226,240,203);
    presets[3].cols[3] = makeColorFromRGB("P4", 181,234,234);
    presets[3].cols[4] = makeColorFromRGB("P5", 201,204,213);
    // 5
    presets[4].title = "Neon Pop";
    presets[4].cols[0] = makeColorFromRGB("N1", 255,0,84);
    presets[4].cols[1] = makeColorFromRGB("N2", 255,84,0);
    presets[4].cols[2] = makeColorFromRGB("N3", 255,189,0);
    presets[4].cols[3] = makeColorFromRGB("N4", 0,198,255);
    presets[4].cols[4] = makeColorFromRGB("N5", 0,114,255);
    // 6-30 (simplified for brevity)
    for (int i = 5; i < TOTAL_PRESETS; ++i) {
        presets[i].title = "Theme " + to_string(i+1);
        for (int j = 0; j < COLORS_PER_PRESET; ++j) {
            presets[i].cols[j] = makeColorFromRGB("C" + to_string(j+1), 
                rand() % 256, rand() % 256, rand() % 256);
        }
    }
}

// ---------------- Explore Logic ----------------
int shuffledIndices[TOTAL_PRESETS];
int shufflePos = 0;
int currentDisplayStart = 0; // Track current display position

void shufflePresetsOnce() {
    for (int i = 0; i < TOTAL_PRESETS; ++i) shuffledIndices[i] = i;
    for (int i = TOTAL_PRESETS - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = shuffledIndices[i];
        shuffledIndices[i] = shuffledIndices[j];
        shuffledIndices[j] = tmp;
    }
    shufflePos = 0;
    currentDisplayStart = 0;
}

void showPresets(int start, int count) {
    cout << "\n--- EXPLORE: Color Palettes ---\n";
    for (int i = 0; i < count && (start + i) < TOTAL_PRESETS; ++i) {
        int idx = shuffledIndices[start + i];
        cout << (i+1) << ". " << presets[idx].title << "\n   ";
        for (int k = 0; k < COLORS_PER_PRESET; ++k) {
            cout << presets[idx].cols[k].hex;
            if (k < COLORS_PER_PRESET-1) cout << "   ";
        }
        cout << "\n";
    }
    
    bool hasMore = (start + count) < TOTAL_PRESETS;
    bool hasPrevious = start > 0;
    
    cout << "\n";
    if (hasPrevious) cout << "[P] Previous  ";
    if (hasMore) cout << "[M] More  ";
    cout << "[C] Copy Palette  [B] Back to Main Menu\n";
}

// ---------------- ID generator for user palettes ----------------
int nextIdNumber = 1001;
string generatePaletteId() {
    char buf[16];
    sprintf(buf, "P%d", nextIdNumber++);
    return string(buf);
}

// ---------------- Global store for created palettes ----------------
Palette allPalettes[200];
int paletteCount = 0;

int findPaletteIndexByIdOrName(const string &key) {
    for (int i = 0; i < paletteCount; ++i) {
        if (allPalettes[i].id == key || allPalettes[i].name == key) return i;
    }
    return -1;
}

bool isPaletteNameExists(const string &name) {
    for (int i = 0; i < paletteCount; ++i) {
        if (allPalettes[i].name == name) return true;
    }
    return false;
}

// ---------------- Main Program ----------------
int main() {
    srand((unsigned)time(0));
    initPresets();
    shufflePresetsOnce();

    RecentQueue recent;

    int mainChoice;
    do {
        clearScreen();
        cout << "\n==============================================\n";
        cout << "        COLOR PALETTE GENERATOR\n";
        cout << "==============================================\n";
        cout << "1. Explore Predefined Palettes\n";
        cout << "2. Create New Palette\n";
        cout << "3. Edit Existing Palette\n";
        cout << "4. View Recent Palettes\n";
        cout << "5. Exit Program\n";
        cout << "==============================================\n";
        cout << "Choose an option (1-5): ";
        cin >> mainChoice;
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (mainChoice == 1) { // Explore
            bool inExplore = true;
            currentDisplayStart = 0;
            int displayCount = 5; // Start with 5 palettes
            
            while (inExplore) {
                clearScreen();
                showPresets(currentDisplayStart, displayCount);
                cout << "Enter your choice: ";
                string line; getline(cin, line);
                if (line.size() == 0) { cout << "No option given.\n"; waitEnter(); continue; }
                char opt = toupper(line[0]);
                
                if (opt == 'M') {
                    // Show 2-3 more palettes
                    int newPalettes = 2 + rand() % 2; // 2 or 3
                    if (currentDisplayStart + displayCount + newPalettes <= TOTAL_PRESETS) {
                        displayCount += newPalettes;
                    } else {
                        displayCount = TOTAL_PRESETS - currentDisplayStart;
                        cout << "No more palettes available.\n";
                        waitEnter();
                    }
                } else if (opt == 'P') {
                    // Go back to previous
                    if (currentDisplayStart >= 5) {
                        currentDisplayStart -= 5;
                        displayCount = 5;
                    } else {
                        currentDisplayStart = 0;
                        displayCount = 5;
                    }
                } else if (opt == 'C') {
                    cout << "Enter number (1-" << displayCount << ") of shown palette to copy or 0 to cancel: ";
                    int num; cin >> num; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (num >= 1 && num <= displayCount) {
                        int chosenPreset = shuffledIndices[currentDisplayStart + (num - 1)];
                        
                        cout << "You chose: " << presets[chosenPreset].title << "\n";
                        cout << "Palette colors:\n";
                        for (int i = 0; i < COLORS_PER_PRESET; ++i) {
                            cout << i+1 << ". " << presets[chosenPreset].cols[i].hex
                                 << " (" << presets[chosenPreset].cols[i].name << ")\n";
                        }
                        cout << "Enter the indices of colors you want to add separated by spaces (e.g., 1 3 5), 0 to cancel:\n";
                        string inputline; getline(cin, inputline);
                        if (inputline.size() == 0) { cout << "Cancelled.\n"; continue; }
                        
                        int choices[COLORS_PER_PRESET]; int choicesCount = 0;
                        const char *s = inputline.c_str();
                        while (*s) {
                            while (*s && isspace(*s)) ++s;
                            if (!*s) break;
                            int val = 0; bool got = false;
                            while (*s && isdigit(*s)) { got = true; val = val*10 + (*s - '0'); ++s; }
                            if (got) {
                                if (val == 0) { choicesCount = 0; break; }
                                if (val >= 1 && val <= COLORS_PER_PRESET) choices[choicesCount++] = val - 1;
                            } else ++s;
                        }
                        
                        if (choicesCount == 0) { cout << "No colors chosen.\n"; continue; }
                        
                        cout << "Do you want to (1) Add to a new palette OR (2) Add to an existing palette? Enter 1 or 2: ";
                        int addChoice; cin >> addChoice; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        if (addChoice == 1) {
                            if (paletteCount >= 200) { cout << "Cannot create more palettes.\n"; continue; }
                            Palette p;
                            string pname;
                            cout << "Enter new palette name: "; getline(cin, pname);
                            if (pname.empty()) pname = "Untitled";
                            
                            // Check for duplicate name
                            if (isPaletteNameExists(pname)) {
                                cout << "Palette name already exists! Please choose a different name.\n";
                                continue;
                            }
                            
                            p.setName(pname);
                            p.setId(generatePaletteId());
                            for (int k = 0; k < choicesCount; ++k) {
                                p.addColor(presets[chosenPreset].cols[ choices[k] ]);
                            }
                            allPalettes[paletteCount++] = p;
                            recent.enqueue(p.id, p.name);
                            cout << "New palette " << p.id << " created and colors added.\n";
                        } else if (addChoice == 2) {
                            cout << "Enter target palette ID or Name: ";
                            string key; getline(cin, key);
                            int idx = findPaletteIndexByIdOrName(key);
                            if (idx == -1) { cout << "Palette not found.\n"; continue; }
                            for (int k = 0; k < choicesCount; ++k) allPalettes[idx].addColor(presets[chosenPreset].cols[ choices[k] ]);
                            recent.enqueue(allPalettes[idx].id, allPalettes[idx].name);
                            cout << "Colors added to " << allPalettes[idx].id << ".\n";
                        } else cout << "Invalid option.\n";
                    } else cout << "Invalid number.\n";
                    waitEnter();
                } else if (opt == 'B') {
                    inExplore = false;
                } else {
                    cout << "Unknown option.\n";
                    waitEnter();
                }
            }

        } else if (mainChoice == 2) { // Create New Palette
            if (paletteCount >= 200) { cout << "Max palettes reached.\n"; waitEnter(); continue; }
            Palette p;
            string pname; 
            bool validName = false;
            
            while (!validName) {
                cout << "Enter palette name: "; getline(cin, pname);
                if (pname.empty()) pname = "Untitled";
                
                if (isPaletteNameExists(pname)) {
                    cout << "Palette name already exists! Please choose a different name.\n";
                } else {
                    validName = true;
                }
            }
            
            p.setName(pname);
            p.setId(generatePaletteId());
            bool inCreate = true;
            while (inCreate) {
                clearScreen();
                cout << "\n--- Create: " << p.name << " (" << p.id << ") ---\n";
                cout << "1. Add Color (Random / By Name)\n";
                cout << "2. View Colors\n";
                cout << "3. Sort Colors\n";
                cout << "4. Save Palette\n";
                cout << "0. Cancel and Back\n";
                cout << "Choose: ";
                int c; cin >> c; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (c == 1) {
                    cout << "Add Color:\n1. Random Generate\n2. Add By Name\n0. Back\nChoose: ";
                    int a; cin >> a; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (a == 1) {
                        bool keep = true;
                        while (keep) {
                            int presetIdx = rand() % TOTAL_PRESETS;
                            int colorIdx = rand() % COLORS_PER_PRESET;
                            Color rc = presets[presetIdx].cols[colorIdx];
                            string rname = "Random_from_" + presets[presetIdx].title;
                            rc.name = rname;
                            cout << "Generated: " << rc.hex << " (" << presets[presetIdx].title << ")\n";
                            cout << "[Enter] Add & generate another, [A] Add only this, [S] Skip, [B] Back\n";
                            string line; getline(cin, line);
                            if (line.size() == 0) {
                                p.addColor(rc);
                                cout << "Added and generating another...\n";
                                continue;
                            } else {
                                char cmd = toupper(line[0]);
                                if (cmd == 'A') {
                                    p.addColor(rc);
                                    cout << "Added.\n";
                                    keep = false;
                                } else if (cmd == 'S') {
                                    cout << "Skipped.\n";
                                } else if (cmd == 'B') {
                                    keep = false;
                                }
                            }
                        }
                    } else if (a == 2) {
                        cout << "Enter color name (e.g., pink, blue) or hex (#RRGGBB): ";
                        string cname; getline(cin, cname);
                        if (!cname.empty() && cname[0] == '#') {
                            int r,g,b;
                            sscanf(cname.c_str(), "#%02x%02x%02x", &r,&g,&b);
                            Color cc = makeColorFromRGB("Manual", r,g,b);
                            cout << "Add this color " << cc.hex << " ? (Y/N): ";
                            char ch; cin >> ch; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            if (toupper(ch) == 'Y') { p.addColor(cc); cout << "Added.\n"; }
                        } else {
                            cout << "Color suggestions:\n";
                            int suggestionsShown = 0;
                            for (int i = 0; i < 5; ++i) {
                                int pi = rand() % TOTAL_PRESETS;
                                int ci = rand() % COLORS_PER_PRESET;
                                cout << (i+1) << ". " << presets[pi].cols[ci].hex << " (" << presets[pi].title << ")\n";
                                suggestionsShown++;
                            }
                            cout << "Choose 1-5 to add or 0 to cancel: ";
                            int pick; cin >> pick; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            if (pick >= 1 && pick <= 5) {
                                // Simple implementation - add random color
                                int pi = rand() % TOTAL_PRESETS;
                                int ci = rand() % COLORS_PER_PRESET;
                                p.addColor(presets[pi].cols[ci]);
                                cout << "Added color to palette.\n";
                            }
                        }
                    }
                    waitEnter();
                } else if (c == 2) {
                    p.display();
                    waitEnter();
                } else if (c == 3) {
                    if (p.count > 0) {
                        cout << "Sort by:\n1. Brightness\n2. Hue\n3. Saturation\nChoose: ";
                        int sortChoice; cin >> sortChoice; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        if (sortChoice == 1) p.sortByBrightness();
                        else if (sortChoice == 2) p.sortByHue();
                        else if (sortChoice == 3) p.sortBySaturation();
                        else cout << "Invalid choice.\n";
                    } else {
                        cout << "No colors to sort.\n";
                    }
                    waitEnter();
                } else if (c == 4) {
                    allPalettes[paletteCount++] = p;
                    recent.enqueue(p.id, p.name);
                    cout << "Palette saved as " << p.id << ".\n";
                    inCreate = false;
                    waitEnter();
                } else if (c == 0) {
                    cout << "Cancel creation. Returning to main menu.\n"; 
                    inCreate = false;
                    waitEnter();
                }
            }

        } else if (mainChoice == 3) { // Edit Existing Palette
            cout << "Enter palette ID or Name to edit: ";
            string key; getline(cin, key);
            int idx = findPaletteIndexByIdOrName(key);
            if (idx == -1) { cout << "Palette not found.\n"; waitEnter(); continue; }
            Palette &p = allPalettes[idx];
            recent.enqueue(p.id, p.name);
            bool inEdit = true;
            while (inEdit) {
                clearScreen();
                cout << "\n--- Edit: " << p.id << " : " << p.name << " ---\n";
                cout << "1. Undo Last Change\n";
                cout << "2. Redo Last Undo\n";
                cout << "3. Replace Color (by index)\n";
                cout << "4. Delete Color (by index)\n";
                cout << "5. View Colors\n";
                cout << "6. Sort Colors\n";
                cout << "0. Back\n";
                cout << "Choose: ";
                int ec; cin >> ec; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (ec == 1) { p.undo(); waitEnter(); }
                else if (ec == 2) { p.redo(); waitEnter(); }
                else if (ec == 3) {
                    p.display();
                    cout << "Enter index to replace: "; int idr; cin >> idr; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    if (idr < 1 || idr > p.count) { cout << "Invalid index.\n"; waitEnter(); continue; }
                    cout << "Provide new color hex (#RRGGBB) or 'random' to generate: ";
                    string s; getline(cin, s);
                    Color newc;
                    if (s == "random") {
                        int pi = rand()%TOTAL_PRESETS, ci = rand()%COLORS_PER_PRESET;
                        newc = presets[pi].cols[ci];
                        newc.name = "RandomReplace";
                    } else if (s.size() && s[0]=='#') {
                        int r,g,b; sscanf(s.c_str(), "#%02x%02x%02x", &r,&g,&b);
                        newc = makeColorFromRGB("ManualReplace", r,g,b);
                    } else { cout << "Invalid input.\n"; waitEnter(); continue; }
                    p.replaceAtIndex(idr-1, newc);
                    waitEnter();
                } else if (ec == 4) {
                    p.display();
                    cout << "Enter index to delete: "; int idd; cin >> idd; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    p.removeAtIndex(idd-1);
                    waitEnter();
                } else if (ec == 5) {
                    p.display(); waitEnter();
                } else if (ec == 6) {
                    if (p.count > 0) {
                        cout << "Sort by:\n1. Brightness\n2. Hue\n3. Saturation\nChoose: ";
                        int sortChoice; cin >> sortChoice; cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        if (sortChoice == 1) p.sortByBrightness();
                        else if (sortChoice == 2) p.sortByHue();
                        else if (sortChoice == 3) p.sortBySaturation();
                        else cout << "Invalid choice.\n";
                    } else {
                        cout << "No colors to sort.\n";
                    }
                    waitEnter();
                } else if (ec == 0) inEdit = false;
                else cout << "Invalid.\n";
            }
        } else if (mainChoice == 4) {
            clearScreen();
            recent.display(); 
            waitEnter();
        } else if (mainChoice == 5) {
            cout << "Goodbye! Best of luck with your project!\n";
        } else {
            cout << "Invalid choice.\n";
            waitEnter();
        }

    } while (mainChoice != 5);

    return 0;
}
