#include "headers.h"

using namespace std;

/* Slightly improved tag BST for interest extraction (keeps simple logic) */

struct TagNode {
    string tag;
    int freq;
    TagNode *left;
    TagNode *right;
    TagNode(string t): tag(t), freq(1), left(nullptr), right(nullptr) {}
};

static TagNode* insertTag(TagNode* root, const string &tag) {
    if (!root) return new TagNode(tag);
    if (tag == root->tag) root->freq++;
    else if (tag < root->tag) root->left = insertTag(root->left, tag);
    else root->right = insertTag(root->right, tag);
    return root;
}

static void inorder(TagNode *root) {
    if (!root) return;
    inorder(root->left);
    cout << "  " << root->tag << " (" << root->freq << ")\n";
    inorder(root->right);
}

static void topTags(TagNode *root, vector<pair<string,int>> &tags) {
    if (!root) return;
    topTags(root->left, tags);
    tags.push_back({ root->tag, root->freq });
    topTags(root->right, tags);
}

static TagNode* loadUserTags(const string &userID) {
    string path = "data/notes_" + userID + ".txt";
    ifstream fin(path);
    if (!fin.is_open()) {
        cout << "⚠️ No notes found for user.\n";
        return nullptr;
    }
    TagNode *root = nullptr;
    string line;
    while (getline(fin, line)) {
        size_t pos = line.find("Tags:");
        if (pos != string::npos) {
            string tagsPart = line.substr(pos + 5);
            stringstream ss(tagsPart);
            string tag;
            while (getline(ss, tag, ',')) {
                if (!tag.empty()) {
                    tag.erase(remove_if(tag.begin(), tag.end(), ::isspace), tag.end());
                    root = insertTag(root, tag);
                }
            }
        } else {
            // Also extract inline #tags
            stringstream ss(line);
            string token;
            while (ss >> token) {
                if (!token.empty() && token.front() == '#') {
                    string t = token;
                    while (!t.empty() && ispunct(t.back())) t.pop_back();
                    t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end());
                    root = insertTag(root, t);
                }
            }
        }
    }
    fin.close();
    return root;
}

static vector<string> generateTagSuggestions(TagNode *root, const string &noteTitle) {
    vector<pair<string,int>> allTags;
    topTags(root, allTags);
    sort(allTags.begin(), allTags.end(), [](auto &a, auto &b){ return a.second > b.second; });
    vector<string> suggestions;
    string lowerTitle = noteTitle;
    transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
    for (auto &t : allTags) {
        string lowTag = t.first;
        transform(lowTag.begin(), lowTag.end(), lowTag.begin(), ::tolower);
        if (lowerTitle.find(lowTag) != string::npos || t.second > 2) suggestions.push_back(t.first);
        if (suggestions.size() >= 5) break;
    }
    if (suggestions.empty()) suggestions.push_back("general");
    return suggestions;
}

static void saveInterestProfile(const string &userID, TagNode *root) {
    vector<pair<string,int>> tags;
    topTags(root, tags);
    fs::create_directories("data/interests");
    ofstream fout("data/interests/" + userID + "_interests.txt", ios::trunc);
    fout << "=== User Interest Summary (" << userID << ") ===\n";
    for (auto &t : tags) fout << t.first << " : " << t.second << "\n";
    fout.close();
    cout << "✅ Interest summary saved for " << userID << "\n";
}

void aiSuggestTags(const string &userID) {
    TagNode *root = loadUserTags(userID);
    if (!root) return;
    cout << "\n🧠 Enter title of your new note: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    string title; getline(cin, title);
    auto suggestions = generateTagSuggestions(root, title);
    cout << "\n🔖 AI Suggested Tags:\n";
    for (auto &s : suggestions) cout << "  #" << s << "\n";
    saveInterestProfile(userID, root);
}

void viewInterestTree(const string &userID) {
    TagNode *root = loadUserTags(userID);
    if (!root) return;
    cout << "\n🌳 Your Tag Frequency Tree:\n";
    inorder(root);
    saveInterestProfile(userID, root);
}

// menu for this module
void aiModuleMenu(const string &userID) {
    int ch;
    do {
        cout << "\n=== AI / ML INTERESTS MENU ===\n";
        cout << "1. View Interest Tree\n2. Get AI Tag Suggestions\n3. Exit\nChoice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }
        switch (ch) {
            case 1: viewInterestTree(userID); break;
            case 2: aiSuggestTags(userID); break;
            case 3: cout << "Returning to dashboard...\n"; break;
            default: cout << "Invalid option.\n";
        }
    } while (ch != 3);
}
