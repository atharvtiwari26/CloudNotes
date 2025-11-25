// src/analytics_module.cpp
// Full AI-style analytics (heuristic, offline, C++ only)
// Requires: include/nlohmann/json.hpp
#include "headers.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <set>
#include <map>
#include <iomanip>
#include <cmath>
#include <queue>

using json = nlohmann::json;
using namespace std;

// ---------- Helpers ----------
static vector<string> splitLines(const string &s) {
    vector<string> out;
    std::istringstream iss(s);
    string line;
    while (getline(iss, line)) out.push_back(line);
    return out;
}

static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static string stripPunct(const string &w) {
    string r;
    for (char c : w) {
        if (isalnum((unsigned char)c) || c == '#') r.push_back(c);
        else r.push_back(' ');
    }
    // collapse spaces
    string out;
    bool prevSpace = false;
    for (char c : r) {
        if (isspace((unsigned char)c)) {
            if (!prevSpace) { out.push_back(' '); prevSpace = true; }
        } else { out.push_back(c); prevSpace = false; }
    }
    // trim
    if (!out.empty() && out.front() == ' ') out.erase(out.begin());
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

static vector<string> tokenizeWords(const string &text) {
    string cleaned = stripPunct(text);
    cleaned = toLower(cleaned);
    vector<string> res;
    istringstream iss(cleaned);
    string w;
    while (iss >> w) {
        if (w.size() <= 1) continue; // skip tiny tokens
        // skip common stopwords (small list)
        static const set<string> stop = {
            "the","and","for","with","that","this","from","have","your","are","was","but","not","you","study","studies","notes"
        };
        if (stop.count(w)) continue;
        res.push_back(w);
    }
    return res;
}

static set<string> wordSet(const string &text) {
    set<string> s;
    for (auto &w : tokenizeWords(text)) s.insert(w);
    return s;
}

static double jaccardSimilarity(const set<string> &a, const set<string> &b) {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;
    size_t inter = 0;
    for (auto &x : a) if (b.count(x)) ++inter;
    size_t uni = a.size() + b.size() - inter;
    return uni ? (double)inter / (double)uni : 0.0;
}

static bool looksLikeEquation(const string &s) {
    // crude: presence of '=' or sequences of digits and operators
    static regex eqRegex(R"(([0-9]+|\b(x|y|z)\b)[\s]*[+\-*/^][\s]*([0-9]+|\b(x|y|z)\b)|=)");
    return regex_search(s, eqRegex);
}

static string summarizeText(const string &text, size_t maxLen = 120) {
    // simple: take first sentence (split on .!?), else first maxLen chars
    size_t pos = text.find_first_of(".!?");
    if (pos != string::npos && pos < 200) {
        string s = text.substr(0, pos+1);
        if (s.size() > maxLen) s = s.substr(0, maxLen) + "...";
        return s;
    }
    if (text.size() <= maxLen) return text;
    return text.substr(0, maxLen) + "...";
}

// ---------- Read notes ----------
struct NoteEntry {
    string id;
    string title;
    string timestamp; // as string
    string content;
    set<string> words;
    vector<string> tags;
    bool hasEquation = false;
    string summary;
};

static vector<NoteEntry> loadNotesAdvanced(const string &userID) {
    vector<NoteEntry> notes;
    string path = "data/notes_" + userID + ".txt";
    ifstream fin(path);
    if (!fin.is_open()) return notes;

    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        // expected format: id|title|timestamp|content
        vector<string> parts;
        size_t start = 0;
        for (int i = 0; i < 3; ++i) {
            size_t pos = line.find('|', start);
            if (pos == string::npos) { parts.push_back(line.substr(start)); start = string::npos; break; }
            parts.push_back(line.substr(start, pos - start));
            start = pos + 1;
        }
        string content;
        if (start != string::npos) content = line.substr(start);
        // If parsing produced fewer pieces, guard
        string id = parts.size() > 0 ? parts[0] : "N?";
        string title = parts.size() > 1 ? parts[1] : "Untitled";
        string ts = parts.size() > 2 ? parts[2] : "";
        NoteEntry n;
        n.id = id; n.title = title; n.timestamp = ts; n.content = content;
        n.summary = summarizeText(content);
        n.words = wordSet(title + " " + content);
        // tags: explicit hashtags (#tag) or words prefixed by #
        {
            static regex tagRe(R"((?:#)([A-Za-z0-9_]+))");
            auto begin = sregex_iterator(content.begin(), content.end(), tagRe);
            auto end = sregex_iterator();
            for (auto it = begin; it != end; ++it)
                n.tags.push_back(it->str(1));
            // also in title
            begin = sregex_iterator(title.begin(), title.end(), tagRe);
            for (auto it = begin; it != end; ++it)
                n.tags.push_back(it->str(1));
        }
        n.hasEquation = looksLikeEquation(content) || looksLikeEquation(title);
        notes.push_back(n);
    }
    fin.close();
    return notes;
}

// ---------- Analytics calculations ----------
static json buildAdvancedReport(const string &userID, const vector<NoteEntry> &notes) {
    json report;
    report["user"] = userID;
    report["generated_at"] = currentTime();
    report["note_count"] = (int)notes.size();

    // term frequencies
    map<string,int> termFreq;
    map<string,int> tagFreq;
    int equationCount = 0;
    vector<int> lengths;
    map<string,int> hourFreq;

    for (auto &n : notes) {
        if (n.hasEquation) equationCount++;
        for (auto &t : n.tags) {
            tagFreq[t]++;
        }
        for (auto &w : n.words) termFreq[w]++; // word set (unique per note)
        lengths.push_back((int)n.content.size());
        // parse hour from timestamp if possible "YYYY-MM-DD hh:mm:ss"
        if (!n.timestamp.empty()) {
            try {
                size_t sp = n.timestamp.find(' ');
                if (sp != string::npos && n.timestamp.size() >= sp + 3) {
                    string hh = n.timestamp.substr(sp+1,2);
                    int hour = stoi(hh);
                    hourFreq[to_string(hour)]++;
                }
            } catch(...) {}
        }
    }

    // Top terms
    vector<pair<string,int>> terms(termFreq.begin(), termFreq.end());
    sort(terms.begin(), terms.end(), [](auto &a, auto &b){ return a.second > b.second; });
    report["top_terms"] = json::array();
    for (int i=0; i< (int)min((size_t)15, terms.size()); ++i) {
        report["top_terms"].push_back({ {"term", terms[i].first}, {"count", terms[i].second} });
    }

    // Top tags
    vector<pair<string,int>> tags(tagFreq.begin(), tagFreq.end());
    sort(tags.begin(), tags.end(), [](auto &a, auto &b){ return a.second > b.second; });
    report["top_tags"] = json::array();
    for (int i=0; i< (int)min((size_t)10, tags.size()); ++i) {
        report["top_tags"].push_back({ {"tag", tags[i].first}, {"count", tags[i].second} });
    }

    // equation / math stats
    report["equation_count"] = equationCount;
    if (!lengths.empty()) {
        double sum=0; for (int L: lengths) sum+=L;
        double avg = sum/lengths.size();
        report["avg_note_length"] = avg;
        report["min_note_length"] = *min_element(lengths.begin(), lengths.end());
        report["max_note_length"] = *max_element(lengths.begin(), lengths.end());
    } else {
        report["avg_note_length"] = 0;
    }

    // activity hours (sorted)
    vector<pair<int,string>> hoursVec;
    for (auto &p : hourFreq) hoursVec.push_back({p.second, p.first});
    sort(hoursVec.begin(), hoursVec.end(), greater<>());
    report["active_hours"] = json::array();
    for (auto &h : hoursVec) report["active_hours"].push_back({ {"hour", h.second}, {"count", h.first} });

    // similarity / clusters: simple pairwise Jaccard
    int n = notes.size();
    json sim = json::array();
    for (int i=0;i<n;i++) {
        for (int j=i+1;j<n;j++) {
            double score = jaccardSimilarity(notes[i].words, notes[j].words);
            if (score > 0.15) { // threshold for related notes
                sim.push_back({
                    {"a_id", notes[i].id},
                    {"b_id", notes[j].id},
                    {"score", round(score*1000)/1000.0}
                });
            }
        }
    }
    report["note_similarity"] = sim;

    // per-note summaries
    report["notes"] = json::array();
    for (auto &nentry : notes) {
        json ne;
        ne["id"] = nentry.id;
        ne["title"] = nentry.title;
        ne["timestamp"] = nentry.timestamp;
        ne["summary"] = nentry.summary;
        ne["has_equation"] = nentry.hasEquation;
        ne["tag_count"] = (int)nentry.tags.size();
        ne["tags"] = nentry.tags;
        report["notes"].push_back(ne);
    }

    // recommendations: heuristics
    json recs = json::array();
    // if many equations, recommend practice problems + dedicated review notes
    if (equationCount >= max(1, (int)notes.size()/3)) {
        recs.push_back("You frequently write equations. Add worked-step notes and problem-solving sessions.");
    }
    // if few tags, recommend tagging habit
    if (tags.empty() || tags.size() < 3) {
        recs.push_back("Consider using #tags in your notes (e.g. #math, #cloud) to improve search and analytics.");
    }
    // low variety (top term dominates)
    if (!terms.empty() && terms.front().second > max(2, (int)notes.size()/2)) {
        recs.push_back(string("You're heavily focused on '") + terms.front().first + "'. Consider diversifying study topics.");
    }
    // if many short notes
    if (!lengths.empty() && report["avg_note_length"].get<double>() < 50.0) {
        recs.push_back("Your notes are very short. Try adding short reflections or summaries to increase retention.");
    }
    // active hours insight
    if (!hourFreq.empty()) {
        auto peak = *max_element(hourFreq.begin(), hourFreq.end(), [](auto &a, auto &b){ return a.second < b.second; });
        recs.push_back(string("You are most active around hour ") + peak.first + ". Consider scheduling focused sessions then.");
    }
    // weak topics: detect common academic words that are missing
    vector<string> academicSeeds = {"math","physics","chemistry","cloud","aws","algorithms","datastructures","network","security","sql"};
    for (auto &seed : academicSeeds) {
        if (termFreq.count(seed) == 0) {
            // suggest only if user has at least a couple notes (otherwise it's obvious)
            if ((int)notes.size() >= 3) {
                recs.push_back(string("You have few or no notes on '") + seed + "'. Consider adding a short note on this topic.");
            }
        }
    }
    report["recommendations"] = recs;

    return report;
}

// ---------- ASCII chart print ----------
static void printHorizontalBar(const string &label, int count, int maxCount) {
    int width = 30;
    int len = maxCount ? (int)round((double)count / maxCount * width) : 0;
    cout << setw(12) << left << label << " | ";
    for (int i=0;i<len;i++) cout << '#';
    cout << " (" << count << ")\n";
}

// ---------- Public menu ----------
void analyticsMenu(const string &userID) {
    auto notes = loadNotesAdvanced(userID);
    if (notes.empty()) {
        cout << "⚠️ No notes found to analyze.\n";
        return;
    }

    json report = buildAdvancedReport(userID, notes);
    // build simple maps for printing
    vector<pair<string,int>> topTerms;
    for (auto &t : report["top_terms"]) topTerms.push_back({ t["term"].get<string>(), t["count"].get<int>() });
    vector<pair<string,int>> topTags;
    for (auto &t : report["top_tags"]) topTags.push_back({ t["tag"].get<string>(), t["count"].get<int>() });

    int ch;
    do {
        cout << "\n=== ANALYTICS & AI INSIGHTS ===\n";
        cout << "1. View Summary Report\n";
        cout << "2. View Top Terms & Tags\n";
        cout << "3. View Note Summaries\n";
        cout << "4. View Similar Notes (clusters)\n";
        cout << "5. View Recommendations\n";
        cout << "6. Export JSON Report\n";
        cout << "7. Back\n";
        cout << "Choice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }

        switch (ch) {
            case 1: {
                cout << "\n=== Summary for " << userID << " ===\n";
                cout << "Notes Count: " << report["note_count"] << "\n";
                cout << "Detected Equations: " << report["equation_count"] << "\n";
                cout << "Avg Note Length: " << report["avg_note_length"] << "\n";
                cout << "\nTop Tags:\n";
                int mcount = 0;
                for (auto &p : topTags) if (p.second > mcount) mcount = p.second;
                for (auto &p : topTags) printHorizontalBar(p.first, p.second, mcount);
                cout << "\nActive Hours (top):\n";
                for (auto &h : report["active_hours"]) cout << "  " << h["hour"].get<string>() << " : " << h["count"].get<int>() << "\n";
                break;
            }
            case 2: {
                cout << "\n=== Top Terms ===\n";
                int maxc = 0;
                for (auto &p : topTerms) if (p.second > maxc) maxc = p.second;
                for (int i=0;i<(int)topTerms.size();++i) printHorizontalBar(topTerms[i].first, topTerms[i].second, maxc);
                cout << "\n=== Top Tags ===\n";
                for (int i=0;i<(int)topTags.size();++i) printHorizontalBar(topTags[i].first, topTags[i].second, maxc);
                break;
            }
            case 3: {
                cout << "\n=== Note Summaries ===\n";
                for (auto &n : report["notes"]) {
                    cout << n["id"].get<string>() << " | " << n["title"].get<string>() << " | " << n["timestamp"].get<string>() << "\n";
                    cout << "  Summary: " << n["summary"].get<string>() << "\n";
                    cout << "  Tags: ";
                    for (auto &t : n["tags"]) cout << "#" << t.get<string>() << " ";
                    cout << "\n\n";
                }
                break;
            }
            case 4: {
                cout << "\n=== NOTE CLUSTERS (Balanced Similarity) ===\n";

                     // BALANCED threshold
                     double threshold = 0.18;

                     // Build adjacency-like relation (graph edges)
                      int n = notes.size();
                     vector<vector<int>> adj(n);
                     for (int i = 0; i < n; i++) {
                        for (int j = i + 1; j < n; j++) {
                             double sim = jaccardSimilarity(notes[i].words, notes[j].words);
                              if (sim >= threshold) {
                             adj[i].push_back(j);
                              adj[j].push_back(i);
            }
        }
    }

    // Find connected components (clusters)
    vector<int> visited(n, 0);
    int clusterID = 1;

    for (int i = 0; i < n; i++) {
        if (visited[i]) continue;

        // BFS/DFS for cluster
        vector<int> cluster;
        queue<int> q;
        q.push(i);
        visited[i] = 1;

        while (!q.empty()) {
            int u = q.front(); q.pop();
            cluster.push_back(u);
            for (int v : adj[u]) {
                if (!visited[v]) {
                    visited[v] = 1;
                    q.push(v);
                }
            }
        }

        // Print cluster only if more than 1 note
        if (cluster.size() > 1) {
            cout << "\n=== Cluster " << clusterID++ << " (size: " << cluster.size() << ") ===\n";

            // Collect major keywords in this cluster
            map<string,int> kw;
            for (int idx : cluster) {
                for (auto &w : notes[idx].words) {
                    kw[w]++;
                }
            }

            // Sort keywords
            vector<pair<string,int>> vec(kw.begin(), kw.end());
            sort(vec.begin(), vec.end(),
                 [](auto &a, auto &b){ return a.second > b.second; });

            // Show cluster topic guess (top 3 terms)
            cout << "Topic Guess: ";
            int limit = min(3, (int)vec.size());
            for (int k = 0; k < limit; k++) {
                cout << vec[k].first;
                if (k + 1 < limit) cout << ", ";
            }
            cout << "\n";

            // List notes inside cluster
            for (int idx : cluster) {
                cout << " - " << notes[idx].id << " | "
                     << notes[idx].title << "\n";
            }
        }
    }

    cout << "\n(Notes not matching any group are standalone.)\n";
    break;
}

            case 5: {
                cout << "\n=== Recommendations ===\n";
                for (auto &r : report["recommendations"]) cout << " - " << r.get<string>() << "\n";
                break;
            }
            case 6: {
                fs::create_directories("analytics");
                string file = "analytics/advanced_report_" + userID + ".json";
                ofstream fout(file, ios::trunc);
                fout << report.dump(4) << endl;
                fout.close();
                cout << "✅ Advanced report exported to " << file << "\n";
                break;
            }
            case 7: cout << "Returning...\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (ch != 7);
}
