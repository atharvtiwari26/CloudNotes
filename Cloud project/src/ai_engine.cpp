// src/ai_engine.cpp
// Uses analytics reports to provide human-friendly insights
#include "headers.h"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <fstream>

using json = nlohmann::json;
using namespace std;

static json loadAdvancedReport(const string &userID) {
    string path = "analytics/advanced_report_" + userID + ".json";
    if (!fs::exists(path)) return json();
    ifstream fin(path);
    if (!fin.is_open()) return json();
    try {
        json j; fin >> j; fin.close(); return j;
    } catch(...) { return json(); }
}

static void printOneLine(const string &s) {
    cout << s << "\n";
}

static void shortActionPlan(const json &report) {
    if (report.is_null()) { cout << "No report available. Run Analytics -> Export JSON first.\n"; return; }
    cout << "\n=== Action Plan ===\n";
    // rule-based action generation
    int noteCount = report.value("note_count", 0);
    int eqCount = report.value("equation_count", 0);
    double avgLen = report.value("avg_note_length", 0.0);

    if (noteCount < 3) printOneLine("• Add at least 3 notes this week to get better analytics.");
    if (eqCount > max(1, noteCount/3)) printOneLine("• You write many equations — create 1 worked-example note per topic.");
    if (avgLen < 80.0) printOneLine("• Try writing slightly longer summaries (2-4 sentences).");

    // suggest top terms to focus on
    if (report.contains("top_terms") && report["top_terms"].size() > 0) {
        auto top = report["top_terms"][0]["term"].get<string>();
        cout << "• Focus topic this week: " << top << "\n";
    }
    // suggest weak topic from recommendations if any mention 'few or no notes'
    for (auto &r : report["recommendations"]) {
        string s = r.get<string>();
        if (s.find("few or no notes on") != string::npos) {
            cout << "• Add a quick note on: " << s.substr(s.find("'")+1, s.rfind("'") - s.find("'") - 1) << "\n";
        }
    }
}

static void fullNarrativeSummary(const json &report) {
    if (report.is_null()) { cout << "No advanced report found. Export one from Analytics first.\n"; return; }
    cout << "\n=== Narrative Summary ===\n";
    cout << "User: " << report.value("user", string("unknown")) << "\n";
    cout << "Generated at: " << report.value("generated_at", string("")) << "\n";
    cout << "You have " << report.value("note_count",0) << " notes. ";
    if (report.value("note_count",0) < 5) cout << "Try to add more notes to capture your learning pattern.\n";
    else cout << "\n";

    int eq = report.value("equation_count",0);
    if (eq > 0) cout << "You record equations in notes (" << eq << " occurrences). Consider saving step-by-step solutions for long-term recall.\n";

    if (report.contains("top_terms") && report["top_terms"].size() > 0) {
        cout << "Your top focus topics are: ";
        int n = min(5, (int)report["top_terms"].size());
        for (int i=0;i<n;i++) {
            cout << report["top_terms"][i]["term"].get<string>();
            if (i+1<n) cout << ", ";
            else cout << ".\n";
        }
    }

    cout << "Recommendations:\n";
    for (auto &r : report["recommendations"]) cout << " - " << r.get<string>() << "\n";
}

void aiEngineMenu(const string &userID) {
    int ch;
    do {
        cout << "\n=== AI INSIGHTS MENU ===\n";
        cout << "1. Generate/Export Advanced Analytics (run from Analytics menu)\n";
        cout << "2. Quick Action Plan\n";
        cout << "3. Narrative Summary\n";
        cout << "4. Show exported advanced JSON (if exists)\n";
        cout << "5. Back\n";
        cout << "Choice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }

        switch (ch) {
            case 1:
                cout << "Please run Analytics -> Export JSON to create the advanced report first.\n";
                break;
            case 2: {
                auto rep = loadAdvancedReport(userID);
                shortActionPlan(rep);
                break;
            }
            case 3: {
                auto rep = loadAdvancedReport(userID);
                fullNarrativeSummary(rep);
                break;
            }
            case 4: {
                auto rep = loadAdvancedReport(userID);
                if (rep.is_null()) cout << "No exported report found (analytics/advanced_report_" << userID << ".json)\n";
                else cout << rep.dump(4) << "\n";
                break;
            }
            case 5: cout << "Returning...\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (ch != 5);
}
