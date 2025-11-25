#include "headers.h"

#ifdef USE_AWS
// AWS SDK integration would go here.
#endif

using namespace std;

// Simple mock-cloud implementation and optional AWS stubs

static string ts_filename() {
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", localtime(&now));
    return string(buf);
}

static void ensureFolders() {
    fs::create_directories("data");
    fs::create_directories("cloud_mock");
}

// mock upload
static bool mockUpload(const string &userID) {
    ensureFolders();
    string local = "data/notes_" + userID + ".txt";
    string cloud = "cloud_mock/notes_" + userID + ".txt";
    if (!fs::exists(local)) {
        cout << "No local notes found: " << local << "\n";
        return false;
    }
    try {
        fs::copy_file(local, cloud, fs::copy_options::overwrite_existing);
        ofstream log("data/sync_log_" + userID + ".txt", ios::app);
        log << "[" << ts_filename() << "] UPLOAD -> " << cloud << "\n";
        cout << "✓ Uploaded to mock cloud: " << cloud << "\n";
        return true;
    } catch (...) {
        cout << "⚠️ Error during mock upload.\n";
        return false;
    }
}

// mock download
static bool mockDownload(const string &userID) {
    ensureFolders();
    string cloud = "cloud_mock/notes_" + userID + ".txt";
    string local = "data/notes_" + userID + ".txt";
    if (!fs::exists(cloud)) {
        cout << "No mock cloud copy yet: " << cloud << "\n";
        return false;
    }
    try {
        fs::copy_file(cloud, local, fs::copy_options::overwrite_existing);
        ofstream log("data/sync_log_" + userID + ".txt", ios::app);
        log << "[" << ts_filename() << "] DOWNLOAD -> " << local << "\n";
        cout << "✓ Downloaded to local: " << local << "\n";
        return true;
    } catch (...) {
        cout << "⚠️ Error during mock download.\n";
        return false;
    }
}

static void showSyncLog(const string &userID) {
    string path = "data/sync_log_" + userID + ".txt";
    if (!fs::exists(path)) { cout << "No sync log yet.\n"; return; }
    ifstream fin(path);
    cout << "\n--- Sync Log (" << userID << ") ---\n";
    string line;
    while (getline(fin, line)) cout << line << "\n";
    fin.close();
}

#ifdef USE_AWS
static bool awsUpload(const string &userID) { /* TODO implement with SDK */ return false; }
static bool awsDownload(const string &userID) { /* TODO implement */ return false; }
#else
static bool awsUpload(const string &userID) { cout << "AWS mode is OFF. Compile with -DUSE_AWS after SDK setup.\n"; return false; }
static bool awsDownload(const string &userID) { cout << "AWS mode is OFF. Compile with -DUSE_AWS after SDK setup.\n"; return false; }
#endif

void cloudSyncMenu(const string &userID) {
    ensureFolders();
    int ch;
    do {
        cout << "\n=== CLOUD SYNC MENU (" << userID << ") ===\n";
        cout << "1) Upload notes to cloud (mock)\n";
        cout << "2) Download notes from cloud (mock)\n";
        cout << "3) Upload notes to AWS (optional)\n";
        cout << "4) Download notes from AWS (optional)\n";
        cout << "5) View sync log\n";
        cout << "6) Back\n";
        cout << "Choice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }
        switch (ch) {
            case 1: mockUpload(userID); break;
            case 2: mockDownload(userID); break;
            case 3: awsUpload(userID); break;
            case 4: awsDownload(userID); break;
            case 5: showSyncLog(userID); break;
            case 6: cout << "Returning...\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (ch != 6);
}
