// src/main.cpp
// Self-contained CloudNotes server (C++17)

#include "httplib.h"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <iostream>
#include <unordered_map>
#include <regex>
#include <unordered_set>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

// ---------------- CONFIG ----------------
static const string USERS_JSON = "data/user.json";
static const fs::path NOTES_DIR = R"(C:\Users\athar\Desktop\Cloud project\data)";
static const string FRONTEND_DIR = "frontend";
static const string EXPORTED_PDF = "exported_notes.pdf";

// ---------------- TOKENIZER ----------------
static vector<string> tokenize(const string &text) {
    string s;
    for (char c : text) {
        if (isalnum((unsigned char)c) || c == '#' || c == '_')
            s.push_back((char)tolower(c));
        else
            s.push_back(' ');
    }

    static const unordered_set<string> stop = {
        "the","and","for","with","that","this","from","have",
        "your","are","was","but","not","you","a","an","in","on","to"
    };

    vector<string> out;
    istringstream iss(s);
    string w;

    while (iss >> w) {
        if (w.size() <= 1) continue;
        if (stop.count(w)) continue;
        out.push_back(w);
    }
    return out;
}

// ---------------- HELPERS ----------------
static void ensureDirectories() {
    try {
        fs::create_directories(NOTES_DIR);
        fs::create_directories(fs::path(USERS_JSON).parent_path());
    } catch (...) {}
}

static json loadUsersJson() {
    json j = json::object();
    try {
        if (!fs::exists(USERS_JSON)) return j;
        ifstream fin(USERS_JSON);
        if (!fin.is_open()) return j;
        fin >> j;
    } catch (...) {}
    if (!j.is_object()) j = json::object();
    return j;
}

static bool saveUsersJson(const json &j) {
    try {
        fs::create_directories(fs::path(USERS_JSON).parent_path());
        ofstream fout(USERS_JSON, ios::trunc);
        if (!fout.is_open()) return false;
        fout << j.dump(4);
        return true;
    } catch (...) { return false; }
}

static string currentTimestamp() {
    using namespace chrono;
    auto now = system_clock::now();
    time_t t = system_clock::to_time_t(now);
    tm tm{};

#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static string makeNoteID() {
    auto now = chrono::system_clock::now();
    auto sec = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();
    mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    uint32_t r = rng() & 0xFFFF;

    ostringstream oss;
    oss << "N" << sec << hex << setw(4) << setfill('0') << r;
    return oss.str();
}

static fs::path userNotesPath(const string &userID) {
    return NOTES_DIR / ("notes_" + userID + ".txt");
}

// ---------------- NOTE PARSER ----------------
static bool parseNoteLine(const string &line, string &id, string &title, string &ts, string &body) {
    size_t p1 = line.find('|');
    if (p1 == string::npos) return false;
    size_t p2 = line.find('|', p1 + 1);
    if (p2 == string::npos) return false;
    size_t p3 = line.find('|', p2 + 1);
    if (p3 == string::npos) return false;

    id = line.substr(0, p1);
    title = line.substr(p1 + 1, p2 - p1 - 1);
    ts = line.substr(p2 + 1, p3 - p2 - 1);
    body = line.substr(p3 + 1);

    return true;
}

static vector<json> loadNotesForUser(const string &userID) {
    vector<json> res;
    fs::path path = userNotesPath(userID);
    if (!fs::exists(path)) return res;

    ifstream fin(path);
    string line;

    while (getline(fin, line)) {
        if (line.empty()) continue;

        string id, title, ts, body;
        if (!parseNoteLine(line, id, title, ts, body)) continue;

        json n;
        n["id"] = id;
        n["title"] = title;
        n["timestamp"] = ts;
        n["body"] = body;
        res.push_back(n);
    }
    return res;
}

static bool appendNoteForUser(const string &userID, const string &title, const string &body) {
    fs::path path = userNotesPath(userID);
    fs::create_directories(path.parent_path());

    ofstream fout(path, ios::app);
    if (!fout.is_open()) return false;

    string id = makeNoteID();
    string ts = currentTimestamp();

    string t = title; replace(t.begin(), t.end(), '|', '/');
    string b = body;  replace(b.begin(), b.end(), '|', '/');

    fout << id << "|" << t << "|" << ts << "|" << b << "\n";
    return true;
}

// ---------------- ANALYTICS ----------------
static json simpleAnalytics(const string &userID) {
    auto notes = loadNotesForUser(userID);
    json out;
    out["total"] = (int)notes.size();

    vector<int> last7(7,0);
    vector<string> labels = {"6d","5d","4d","3d","2d","1d","today"};

    time_t now = time(nullptr);
    for (auto &n : notes) {
        string ts = n.value("timestamp", "");
        if (ts.size() < 10) continue;
        int y=0,m=0,d=0;
        sscanf(ts.c_str(), "%d-%d-%d", &y, &m, &d);
        tm tm_note = {};
        tm_note.tm_year = y - 1900;
        tm_note.tm_mon  = m - 1;
        tm_note.tm_mday = d;
        time_t tnote = mktime(&tm_note);
        int days = int((now - tnote) / 86400);
        if (days >= 0 && days < 7) last7[6 - days]++; // keep order as labels
    }
    out["notesByDay"] = last7;
    out["labels"] = labels;

    unordered_map<string,int> freq;
    for (auto &n : notes) {
        auto toks = tokenize(n["title"].get<string>() + " " + n["body"].get<string>());
        for (auto &t : toks) freq[t]++;
    }
    vector<pair<string,int>> vec(freq.begin(), freq.end());
    sort(vec.begin(), vec.end(), [](auto &a, auto &b){ return a.second > b.second; });
    json keywords = json::array();
    for (int i=0; i<min(10,(int)vec.size()); ++i) keywords.push_back(vec[i].first);
    out["keywords"] = keywords;

    return out;
}

// ---------------- AI RECOMMENDATIONS ----------------
// Returns the top 5 keywords from the user's notes
// ---------------- BETTER AI RECOMMENDATIONS ----------------
static json computeRecommendations(const string &userID) {
    auto notes = loadNotesForUser(userID);

    // Words to ignore
    static const unordered_set<string> useless = {
        "the","and","for","with","that","this","from","have","your","are","was",
        "but","not","you","a","an","in","on","to","of","as","it","is","be","at",
        "by","or","we","i",""
    };

    unordered_map<string,int> freq;

    for (auto &n : notes) {
        string text = n["title"].get<string>() + " " + n["body"].get<string>();

        // Lowercase
        for (char &c : text) c = tolower(c);

        // Extract WORDS + TWO-WORD PHRASES
        vector<string> tokens;
        string word;
        for (char c : text) {
            if (isalnum((unsigned char)c)) word += c;
            else if (!word.empty()) { tokens.push_back(word); word.clear(); }
        }
        if (!word.empty()) tokens.push_back(word);

        // Count single meaningful words
        for (auto &t : tokens)
            if (!useless.count(t))
                freq[t]++;

        // Count 2-word pairs (better topics)
        for (size_t i = 0; i + 1 < tokens.size(); i++) {
            string a = tokens[i], b = tokens[i + 1];
            if (useless.count(a) || useless.count(b)) continue;
            string phrase = a + " " + b;
            freq[phrase] += 3;   // weight phrases more
        }
    }

    // Convert to sorted vector
    vector<pair<string,int>> vec(freq.begin(), freq.end());
    sort(vec.begin(), vec.end(), [](auto &a, auto &b){
        return a.second > b.second;
    });

    // Build recommendations
    json rec = json::array();
    int count = 0;
    for (auto &p : vec) {
        if (count >= 5) break;
        if (p.first.size() < 3) continue; // ignore tiny junk
        rec.push_back("Learn more about: " + p.first);
        count++;
    }

    return rec;
}


// ---------------- CREATE PDF ----------------
static bool createExportedNotesPdf(const string &userID) {
    auto notes = loadNotesForUser(userID);

    ofstream fout(EXPORTED_PDF, ios::binary | ios::trunc);
    if (!fout.is_open()) return false;

    // build text body
    ostringstream content;
    content << "Notes for user: " << userID << "\n\n";

    for (auto &n : notes) {
        content << n["timestamp"] << " - " << n["title"] << "\n";
        content << n["body"] << "\n\n";
    }

    string text = content.str();

    // simple wrap
    auto wrapLine = [](const string &s, int maxLen) {
        vector<string> out;
        string cur;
        for (char c : s) {
            if (c == '\n') {
                out.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
                if ((int)cur.size() >= maxLen) {
                    out.push_back(cur);
                    cur.clear();
                }
            }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    };

    vector<string> lines = wrapLine(text, 90);
    int height = max(792, (int)lines.size() * 16 + 100);

    // naive PDF generation (works for simple text)
    fout << "%PDF-1.4\n";
    fout << "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n";
    fout << "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n";
    fout << "3 0 obj << /Type /Page /Parent 2 0 R "
         << "/MediaBox [0 0 612 " << height << "] "
         << "/Contents 4 0 R "
         << "/Resources << /Font << /F1 5 0 R >> >> >> endobj\n";

    ostringstream stream;
    stream << "BT\n/F1 12 Tf\n50 " << (height - 40) << " Td\n";
    for (auto &line : lines) {
        string safe;
        for (char c : line) {
            if (c == '(' || c == ')') safe.push_back('\\');
            safe.push_back(c);
        }
        stream << "(" << safe << ") Tj\n0 -14 Td\n";
    }
    stream << "ET";
    string streamData = stream.str();

    fout << "4 0 obj << /Length " << streamData.size() << " >> stream\n"
         << streamData << "\nendstream endobj\n";

    fout << "5 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj\n";

    // minimal xref (offsets are approximate — acceptable for simple viewers)
    fout << "xref\n0 6\n"
         << "0000000000 65535 f \n"
         << "0000000010 00000 n \n"
         << "0000000060 00000 n \n"
         << "0000000120 00000 n \n"
         << "0000000200 00000 n \n"
         << "0000000300 00000 n \n";
    fout << "trailer << /Size 6 /Root 1 0 R >>\n"
         << "startxref\n350\n%%EOF";

    return true;
}

// ---------------- SERVER ----------------
int main() {
    ensureDirectories();
    httplib::Server svr;

    // CORS
    svr.Options(".*", [](const httplib::Request &, httplib::Response &res){
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.status = 200;
    });

    svr.set_mount_point("/", FRONTEND_DIR.c_str());

    // PDF serve
    svr.Get("/exported_notes.pdf", [](const httplib::Request &, httplib::Response &res){
        ifstream fin(EXPORTED_PDF, ios::binary);
        if (!fin.is_open()) {
            res.status = 404;
            res.set_content("not found", "text/plain");
            return;
        }
        stringstream buf; buf << fin.rdbuf();
        res.set_content(buf.str(), "application/pdf");
    });

    // SIGNUP
    svr.Post("/api/signup", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            string userID = j["userID"];
            string password = j["password"];

            json users = loadUsersJson();
            if (users.contains(userID)) {
                res.set_content("User exists", "text/plain");
                return;
            }

            users[userID]["password"] = password;
            users[userID]["searchHistory"] = json::array();

            saveUsersJson(users);
            ofstream(userNotesPath(userID)).close();

            res.set_content("Signup OK", "text/plain");
        } catch (...) {
            res.set_content("Invalid JSON", "text/plain");
        }
    });

    // LOGIN
    svr.Post("/api/login", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            string userID = j["userID"];
            string password = j["password"];

            json users = loadUsersJson();
            if (users.contains(userID) && users[userID]["password"] == password)
                res.set_content("OK", "text/plain");
            else
                res.set_content("ERR", "text/plain");
        } catch (...) {
            res.set_content("ERR", "text/plain");
        }
    });

    // ADD NOTE
    svr.Post("/api/addNote", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            bool ok = appendNoteForUser(j["userID"], j["title"], j["body"]);
            res.set_content(ok ? "OK" : "ERR", "text/plain");
        } catch (...) {
            res.set_content("ERR", "text/plain");
        }
    });

    // DELETE NOTE
    svr.Post("/api/deleteNote", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            string user = j["userID"];
            string id   = j["noteID"];

            fs::path path = userNotesPath(user);
            if (!fs::exists(path)) {
                res.set_content("ERR", "text/plain");
                return;
            }

            vector<string> out;
            ifstream fin(path);
            string line;

            while (getline(fin, line)) {
                if (line.rfind(id + "|", 0) != 0)
                    out.push_back(line);
            }

            ofstream fout(path, ios::trunc);
            for (auto &l : out) fout << l << "\n";

            res.set_content("OK", "text/plain");
        } catch (...) {
            res.set_content("ERR", "text/plain");
        }
    });

    // EDIT NOTE
    svr.Post("/api/editNote", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            string user = j["userID"];
            string note = j["noteID"];
            string title = j["title"];
            string body  = j["body"];

            fs::path path = userNotesPath(user);
            if (!fs::exists(path)) {
                res.set_content("ERR", "text/plain");
                return;
            }

            vector<string> out;

            ifstream fin(path);
            string line;

            while (getline(fin, line)) {
                string id,t,ts,b;
                if (!parseNoteLine(line,id,t,ts,b)) continue;

                if (id == note) {
                    replace(title.begin(), title.end(), '|', '/');
                    replace(body.begin(),  body.end(), '|', '/');
                    out.push_back(id + "|" + title + "|" + ts + "|" + body);
                } else out.push_back(line);
            }

            ofstream fout(path, ios::trunc);
            for (auto &l : out) fout << l << "\n";

            res.set_content("OK", "text/plain");
        } catch (...) {
            res.set_content("ERR", "text/plain");
        }
    });

    // GET NOTES
    svr.Get("/api/notes", [](const httplib::Request &req, httplib::Response &res){
        auto it = req.params.find("user");
        if (it == req.params.end()) {
            res.set_content("[]", "application/json");
            return;
        }
        res.set_content(json(loadNotesForUser(it->second)).dump(), "application/json");
    });

    // GLOBAL SEARCH
    svr.Get("/api/search", [](const httplib::Request &req, httplib::Response &res){
        auto it = req.params.find("q");
        if (it == req.params.end()) {
            res.set_content("[]", "application/json");
            return;
        }

        string q = it->second;
        transform(q.begin(), q.end(), q.begin(), ::tolower);

        json users = loadUsersJson();
        vector<json> results;

        for (auto &[uid, _] : users.items()) {
            auto notes = loadNotesForUser(uid);
            for (auto &n : notes) {
                string combined = n["title"].get<string>() + " " + n["body"].get<string>();
                string lower = combined;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find(q) != string::npos) {
                    json hit;
                    hit["user"] = uid;
                    hit["id"] = n["id"];
                    hit["title"] = n["title"];
                    hit["body"] = n["body"];
                    hit["timestamp"] = n["timestamp"];
                    results.push_back(hit);
                }
            }
        }

        res.set_content(json(results).dump(), "application/json");
    });

    // AI RECOMMENDATIONS
    svr.Get("/api/recommend", [](const httplib::Request &req, httplib::Response &res){
        auto it = req.params.find("user");
        if (it == req.params.end()) {
            res.set_content("[]", "application/json");
            return;
        }

        res.set_content(computeRecommendations(it->second).dump(), "application/json");
    });

    // Analytics route
    svr.Get("/api/analytics", [](const httplib::Request &req, httplib::Response &res){
        auto it = req.params.find("user");
        if (it == req.params.end()) {
            res.set_content("{}", "application/json");
            return;
        }
        try {
            json j = simpleAnalytics(it->second);
            res.set_content(j.dump(), "application/json");
        } catch(...) {
            res.set_content("{}", "application/json");
        }
    });

    // SAVE SEARCH TERM
    svr.Post("/api/addSearchTerm", [](const httplib::Request &req, httplib::Response &res){
        try {
            auto j = json::parse(req.body);
            string user = j["user"];
            string term = j["term"];

            transform(term.begin(), term.end(), term.begin(), ::tolower);

            json users = loadUsersJson();
            if (!users.contains(user)) {
                res.set_content("ERR", "text/plain");
                return;
            }

            if (!users[user].contains("searchHistory"))
                users[user]["searchHistory"] = json::array();

            users[user]["searchHistory"].push_back(term);
            saveUsersJson(users);

            res.set_content("OK", "text/plain");
        }
        catch(...) {
            res.set_content("ERR", "text/plain");
        }
    });

    // PDF Export
    svr.Get("/api/exportPdf", [](const httplib::Request &req, httplib::Response &res){
        auto it = req.params.find("user");
        if (it == req.params.end()) {
            res.set_content(R"({"status":"missing_user"})", "application/json");
            return;
        }

        bool ok = createExportedNotesPdf(it->second);
        if (ok)
            res.set_content(R"({"status":"exported"})", "application/json");
        else
            res.set_content(R"({"status":"error"})", "application/json");
    });

    cout << "Server running at http://localhost:5000\n";
    svr.listen("0.0.0.0", 5000);

    return 0;
}
