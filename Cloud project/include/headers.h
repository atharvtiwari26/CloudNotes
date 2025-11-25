#ifndef HEADERS_H
#define HEADERS_H

// Project-wide basic includes and helpers
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>   // *** REQUIRED ***
#include <filesystem>
#include <regex>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>

#include "headers.h"
#include "httplib.h"       // *** REQUIRED ***
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;


// ---------------- Basic types ----------------
struct User {
    string id;
    string password; // stored encoded
    string role; // "student", "faculty", "admin"
};

struct Note {
    string id;
    string title;
    string content;
    vector<string> tags;
    string timestamp;
    Note *next; // simple linked-list for notes module
};

// ---------------- Helpers ----------------
inline string currentTime() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

inline string encode(const string &s) {
    string res = s;
    for (char &c : res) c = c + 2;
    return res;
}

inline string decode(const string &s) {
    string res = s;
    for (char &c : res) c = c - 2;
    return res;
}

// ---------------- Directory helpers ----------------
inline void ensureBaseDirectories() {
    fs::create_directories("data");
    fs::create_directories("cloud_mock");
    fs::create_directories("analytics");
    fs::create_directories("ai_reports");
    fs::create_directories("data/interests");
}

// ---------------- Module function declarations ----------------
// User / auth
void runUserSystem();                  // starts the auth loop
void signupUser();                     // NEW: creates user via JSON

// Notes
void notesMenu(const string &userID);
Note* loadNotes(const string &userID);
void saveNotes(Note *head, const string &userID);
void createNote(Note* &head, const string &userID);
void displayNotes(Note *head);
void deleteNote(Note* &head, const string &userID);
void editNote(Note *head, const string &userID);

// Cloud sync (canonical single cloud menu)
void cloudSyncMenu(const string &userID);

// Analytics & AI
void analyticsMenu(const string &userID);
void aiEngineMenu(const string &userID); // AI / recommendation menu

void exportNotesToPDF(const string &userID);

#endif // HEADERS_H
