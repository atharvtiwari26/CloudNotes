AI-Powered Cloud Note Management System with Search & Recommendations

CloudNotes is a full-featured note-management platform built using a C++ backend, REST APIs, and a clean web-based frontend. It enables users to create, store, search, and organize notes with the support of an AI recommendation engine that finds similar and relevant notes automatically.

This project demonstrates real-world system design using Data Structures, Algorithms, TF-IDF vectorization, and RESTful architecture.

⭐ Features

📝 Create, edit, delete, and view notes

🔍 Full-text search using an inverted index

🤖 AI-powered recommendations using TF-IDF vectors + cosine similarity

🔐 Secure user authentication

⚙️ Local persistence with optimized indexing

💡 Fast, lightweight C++ backend

🌐 JSON-based REST API ready for cloud deployment

🎨 Clean, responsive frontend

🏗 System Architecture
1. Frontend (HTML/JS)

User dashboard

Note editor

Search interface

Recommendation panel

AJAX calls to backend API

2. Backend (C++ Server)

Handles:

Authentication

CRUD operations

Search indexing

Recommendation pipeline

Data persistence

3. Storage Layer

Local storage with ID-based indexing

Vectors, unordered_maps, file writes

Future upgrade: cloud-hosted database

🧠 Recommendation Engine (AI Module)

CloudNotes uses lightweight ML concepts:

TF-IDF Vectorizer

Converts each note into a numerical vector (vector<float>)

Cosine Similarity

Calculates closeness between notes

Priority Queue (Top-K Ranking)

Selects best similar notes for recommendation output

This is fast, efficient, and entirely implemented from scratch.

📚 Data Structures Used

unordered_map → user records, word indexes

vector → notes, TF-IDF vectors

map/unordered_map → tags, frequency tables

priority_queue → top-K recommendations

Doubly linked list + Hash Map → LRU caching (optional)

Inverted Index → high-speed search engine

📡 API Endpoints (Sample)
Method	Endpoint	Description
POST	/signup	Create new user
POST	/login	Authenticate user
POST	/addNote	Add new note
GET	/getNotes	Fetch notes
POST	/search	Search notes
GET	/recommend	Get recommendations
🌐 Future Upgrades

Cloud hosting will be added as an enhancement:

Deploy backend to AWS / Render / Azure

Add multi-device sync

Migrate local files → cloud database

Add real-time updates

This will turn CloudNotes into a production-ready cloud application.

🧪 Testing

✔ API testing (Postman)

✔ Recommendation accuracy tests

✔ Stress tests on search engine

✔ UI stability testing

✔ Data persistence validation

🎓 Project Status

✅ Core project completed
🔄 Cloud hosting scheduled as future upgrade
🚧 Optional improvements in progress

👥 Team Members

(Fill your team details here)

📂 How to Run
1. Backend
g++ server.cpp -o server
./server

2. Frontend

Open index.html in browser
OR
Serve via simple HTTP server (recommended)

📎 Repository Structure (Recommended)
CloudNotes/
│── backend/
│   ├── src/
│   ├── include/
│   ├── data/
│   └── server.cpp
│
│── frontend/
│   ├── index.html
│   ├── app.js
│   └── styles.css
│
│── docs/
│   ├── architecture.png
│   └── proposal.docx
│
└── README.md
