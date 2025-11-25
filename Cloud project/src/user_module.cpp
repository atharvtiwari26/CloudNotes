#include "headers.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

static const string USERS_JSON = "data/users.json";

// ----------------- JSON helpers -----------------
static json loadUsersJson() {
    ensureBaseDirectories();
    if (!fs::exists(USERS_JSON)) {
        return json::object();
    }
    std::ifstream fin(USERS_JSON);
    if (!fin.is_open()) return json::object();
    try {
        json j; fin >> j;
        return j;
    } catch (...) {
        return json::object();
    }
}

static bool saveUsersJson(const json &j) {
    ensureBaseDirectories();
    std::ofstream fout(USERS_JSON, ios::trunc);
    if (!fout.is_open()) return false;
    fout << j.dump(4) << std::endl; // pretty print
    fout.close();
    return true;
}

// ----------------- Signup -----------------
void signupUser() {
    json users = loadUsersJson();

    string id, pw, role;
    cout << "\n=== SIGN UP ===\n";
    cout << "Choose a User ID: ";
    cin >> id;

    // basic validation
    if (id.empty()) { cout << "Invalid ID.\n"; return; }
    if (users.contains(id)) {
        cout << "This username already exists. Try logging in.\n";
        return;
    }

    cout << "Choose Password: ";
    cin >> pw;
    cout << "Role (admin/faculty/student): ";
    cin >> role;
    if (role != "admin" && role != "faculty" && role != "student") {
        cout << "Invalid role. Defaulting to 'student'.\n";
        role = "student";
    }

    // create user object (Option C structure)
    json userObj;
    userObj["password"] = encode(pw);
    userObj["role"] = role;
    userObj["notes"] = json::array();
    userObj["interests"] = json::array();

    users[id] = userObj;

    if (saveUsersJson(users)) {
        // create any per-user folders/files if desired
        fs::create_directories("data");
        cout << "✅ Signup successful. You can now login with your credentials.\n";
    } else {
        cout << "⚠️ Failed to save user. Check permissions.\n";
    }
}

// ----------------- Login -----------------
bool loginUser(string &userID, string &role) {
    json users = loadUsersJson();

    cout << "\n=== LOGIN ===\n";
    string id, pw;
    cout << "Enter ID: ";
    cin >> id;
    cout << "Enter Password: ";
    cin >> pw;

    if (!users.contains(id)) {
        cout << "\n❌ Invalid credentials.\n";
        return false;
    }

    try {
        string stored = users[id].value("password", string());
        if (decode(stored) == pw) {
            userID = id;
            role = users[id].value("role", string("student"));
            cout << "\n✅ Login successful. Welcome " << userID << " (" << role << ")\n";

            // ensure personal data file exists
            fs::create_directories("data");
            // Ensure notes file exists (empty) to avoid "no notes" later
            string notesPath = "data/notes_" + userID + ".txt";
            if (!fs::exists(notesPath)) {
                ofstream fout(notesPath); fout.close();
            }

            return true;
        } else {
            cout << "\n❌ Invalid credentials.\n";
            return false;
        }
    } catch (...) {
        cout << "\n❌ Login error.\n";
        return false;
    }
}

// ----------------- Dashboard & run loop -----------------
// Forward declarations of other modules
extern void notesMenu(const string &userID);
extern void cloudSyncMenu(const string &userID);
extern void analyticsMenu(const string &userID);
extern void aiEngineMenu(const string &userID);

void dashboard(const string &userID, const string &role) {
    int ch;
    while (true) {
        cout << "\n============================\n";
        cout << "   Welcome, " << userID << " (" << role << ")\n";
        cout << "============================\n";
        cout << "1. Notes Management\n";
        cout << "2. Cloud Sync\n";
        cout << "3. AI/ML Analytics\n";
        if (role == "admin") cout << "4. Manage Users (Signup)\n";
        cout << "0. Logout\n";
        cout << "Choice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20, '\n'); continue; }

        switch (ch) {
            case 1: notesMenu(userID); break;
            case 2: cloudSyncMenu(userID); break;
            case 3: analyticsMenu(userID); break;
            case 4:
                if (role == "admin") signupUser();
                else cout << "Access Denied.\n";
                break;
            case 0:
                cout << "👋 Logged out successfully.\n";
                return;
            default: cout << "Invalid choice.\n";
        }
    }
}

void runUserSystem() {
    ensureBaseDirectories();

    int ch;
    string userID, role;
    while (true) {
        cout << "\n============================\n";
        cout << "       CLOUD NOTES LOGIN\n";
        cout << "============================\n";
        cout << "1. Login\n";
        cout << "2. Signup\n";
        cout << "0. Exit\n";
        cout << "Choice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }

        switch (ch) {
            case 1:
                if (loginUser(userID, role)) dashboard(userID, role);
                break;
            case 2:
                signupUser();
                break;
            case 0:
                cout << "Exiting CloudNotes. Bye!\n";
                return;
            default:
                cout << "Invalid choice.\n";
        }
    }
}
