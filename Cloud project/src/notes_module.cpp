#include "headers.h"

#include <cstdlib>

using namespace std;

// Simple linked-list notes implementation

static string generateNoteID() {
    string id = "N";
    for (int i = 0; i < 5; ++i) id += char('0' + rand() % 10);
    return id;
}

Note* loadNotes(const string &userID) {
    string path = "data/notes_" + userID + ".txt";
    ifstream fin(path);
    if (!fin.is_open()) return nullptr;

    Note *head = nullptr, *tail = nullptr;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        Note *n = new Note();
        // saved format: id|title|timestamp|content (we'll parse with '|' to allow spaces)
        getline(iss, n->id, '|');
        getline(iss, n->title, '|');
        getline(iss, n->timestamp, '|');
        getline(iss, n->content, '\0'); // rest (single-line entry)
        n->next = nullptr;
        if (!head) head = tail = n;
        else { tail->next = n; tail = n; }
    }
    fin.close();
    return head;
}

void saveNotes(Note *head, const string &userID) {
    string path = "data/notes_" + userID + ".txt";
    ofstream fout(path, ios::trunc);
    for (Note *p = head; p; p = p->next) {
        // use '|' as safe delimiter (no newlines inside notes in current format)
        fout << p->id << "|" << p->title << "|" << p->timestamp << "|" << p->content << "\n";
    }
    fout.close();
}

void displayNotes(Note *head) {
    if (!head) {
        cout << "No notes found.\n";
        return;
    }
    cout << "\n--- Your Notes ---\n";
    for (Note *p = head; p; p = p->next) {
        cout << "ID: " << p->id << "\nTitle: " << p->title << "\nCreated: " << p->timestamp << "\nContent: " << p->content << "\n-------------------------\n";
    }
}

void createNote(Note* &head, const string &userID) {
    Note *n = new Note();
    n->id = generateNoteID();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter note title: ";
    getline(cin, n->title);
    cout << "Enter content: ";
    getline(cin, n->content);
    n->timestamp = currentTime();
    n->next = head;
    head = n;
    saveNotes(head, userID);
    cout << "✅ Note created successfully.\n";
}

void deleteNote(Note* &head, const string &userID) {
    if (!head) { cout << "No notes to delete.\n"; return; }
    string id;
    cout << "Enter note ID to delete: ";
    cin >> id;
    Note *curr = head, *prev = nullptr;
    while (curr && curr->id != id) { prev = curr; curr = curr->next; }
    if (!curr) { cout << "Note not found.\n"; return; }
    if (!prev) head = curr->next;
    else prev->next = curr->next;
    delete curr;
    saveNotes(head, userID);
    cout << "🗑️ Note deleted successfully.\n";
}

void editNote(Note *head, const string &userID) {
    if (!head) { cout << "No notes to edit.\n"; return; }
    string id;
    cout << "Enter note ID to edit: ";
    cin >> id;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    Note *curr = head;
    while (curr && curr->id != id) curr = curr->next;
    if (!curr) { cout << "Note not found.\n"; return; }
    cout << "Editing Note: " << curr->title << "\n";
    cout << "New title: ";
    getline(cin, curr->title);
    cout << "New content: ";
    getline(cin, curr->content);
    curr->timestamp = currentTime();
    saveNotes(head, userID);
    cout << "✏️ Note updated.\n";
}

void notesMenu(const string &userID) {
    Note *head = loadNotes(userID);
    int ch;
    do {
        cout << "\n=== NOTES MENU ===\n";
        cout << "1. View Notes\n2. Create Note\n3. Edit Note\n4. Delete Note\n5. Export Notes as PDF\n6. Exit\nChoice: ";
        if (!(cin >> ch)) { cin.clear(); cin.ignore(1<<20,'\n'); continue; }
        switch (ch) {
            case 1: displayNotes(head); break;
            case 2: createNote(head, userID); break;
            case 3: editNote(head, userID); break;
            case 4: deleteNote(head, userID); break;
            case 5: exportNotesToPDF(userID); break;
            case 6: cout << "Returning to Dashboard...\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (ch != 6);
    saveNotes(head, userID);
    // free list
    while (head) { Note *tmp = head; head = head->next; delete tmp; }
}


#include "pico_pdf.hpp"

void exportNotesToPDF(const std::string &userID) {
    std::string notesFile = "data/notes_" + userID + ".txt";
    std::ifstream in(notesFile);
    if (!in.is_open()) {
        std::cout << "No notes to export.\n";
        return;
    }

    std::string all;
    std::string line;
    while (std::getline(in, line))
        all += line + "\\n";

    pico::pdf pdf;
    pdf.add_page();
    pdf.add_text_object(all);
    pdf.save("exported_notes.pdf");

    std::cout << "PDF exported as exported_notes.pdf\n";
}
