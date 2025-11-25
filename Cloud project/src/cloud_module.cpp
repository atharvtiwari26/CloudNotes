#include "headers.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <stack>
#include <queue>
#include <ctime>
using namespace std;
namespace fs = std::filesystem;

/* ================================
   MODULE 3 — CLOUD SYNC SYSTEM
   Uses Queue for Uploads & Stack for Downloads
   ================================ */

struct UploadRequest {
    string filename;
    string timestamp;
};

struct DownloadHistory {
    string filename;
    string timestamp;
};

/* ---------- Helper Functions ---------- */
static string currentTimestamp() {
    time_t now = time(0);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

/* ---------- Data Structures ---------- */
queue<UploadRequest> uploadQueue;
stack<DownloadHistory> downloadStack;

/* ---------- Upload Handling ---------- */
void enqueueUpload(const string &file) {
    UploadRequest ur = { file, currentTimestamp() };
    uploadQueue.push(ur);
}

void processUploads() {
    if (uploadQueue.empty()) {
        cout << "No pending uploads.\n";
        return;
    }

    while (!uploadQueue.empty()) {
        UploadRequest ur = uploadQueue.front();
        uploadQueue.pop();
        string dest = "cloud/" + ur.filename;

        try {
            fs::create_directories("cloud");
            fs::copy_file("data/" + ur.filename, dest, fs::copy_options::overwrite_existing);
            cout << "☁️  Uploaded: " << ur.filename << " at " << ur.timestamp << "\n";
        } catch (...) {
            cout << "⚠️  Error uploading file: " << ur.filename << "\n";
        }
    }
}

/* ---------- Download Handling ---------- */
void pushDownload(const string &file) {
    DownloadHistory dh = { file, currentTimestamp() };
    downloadStack.push(dh);
}

void downloadFromCloud(const string &filename) {
    string source = "cloud/" + filename;
    string dest = "data/" + filename;

    if (!fs::exists(source)) {
        cout << "⚠️ No such file in cloud.\n";
        return;
    }

    try {
        fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
        pushDownload(filename);
        cout << "⬇️  Downloaded: " << filename << " from cloud.\n";
    } catch (...) {
        cout << "⚠️  Failed to download " << filename << ".\n";
    }
}

/* Undo last download */
void undoLastDownload() {
    if (downloadStack.empty()) {
        cout << "Nothing to undo.\n";
        return;
    }
    DownloadHistory dh = downloadStack.top();
    downloadStack.pop();
    try {
        fs::remove("data/" + dh.filename);
        cout << "⏪ Removed last downloaded file: " << dh.filename << "\n";
    } catch (...) {
        cout << "⚠️  Failed to remove local copy.\n";
    }
}

/* ---------- Logs ---------- */
void viewSyncLogs() {
    cout << "\n--- CLOUD SYNC LOGS ---\n";

    cout << "Pending Uploads:\n";
    if (uploadQueue.empty()) cout << "  None\n";
    else {
        queue<UploadRequest> temp = uploadQueue;
        while (!temp.empty()) {
            cout << "  " << temp.front().filename << " (" << temp.front().timestamp << ")\n";
            temp.pop();
        }
    }

    cout << "\nDownload History:\n";
    if (downloadStack.empty()) cout << "  None\n";
    else {
        stack<DownloadHistory> temp = downloadStack;
        while (!temp.empty()) {
            cout << "  " << temp.top().filename << " (" << temp.top().timestamp << ")\n";
            temp.pop();
        }
    }
}

/* ---------- Main Cloud Menu ---------- */
void cloudMenu(const string &userID) {
    int ch;
    string filename = "notes_" + userID + ".txt";

    do {
        cout << "\n=== CLOUD SYNC MENU ===\n";
        cout << "1. Queue Upload\n2. Process Uploads\n3. Download Notes\n4. Undo Last Download\n5. View Logs\n6. Exit\nChoice: ";
        cin >> ch;
        switch (ch) {
            case 1: enqueueUpload(filename); cout << "Queued " << filename << " for upload.\n"; break;
            case 2: processUploads(); break;
            case 3: downloadFromCloud(filename); break;
            case 4: undoLastDownload(); break;
            case 5: viewSyncLogs(); break;
            case 6: cout << "Returning to dashboard...\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (ch != 6);
}
