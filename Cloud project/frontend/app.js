// app.js — FINAL STABLE VERSION
const API_ROOT = "http://localhost:5000";

const App = {

  // -----------------------------
  // USER STORAGE
  // -----------------------------
  getUser() {
    return localStorage.getItem("cloud_user");
  },

  setUser(u) {
    if (u) localStorage.setItem("cloud_user", u);
    else localStorage.removeItem("cloud_user");
    this.updateUserBadge();
  },

  updateUserBadge() {
    const u = this.getUser();
    document.querySelectorAll("#userBadge").forEach(el => {
      el.textContent = u ? `Signed in as ${u}` : "Not signed in";
    });
  },

  // -----------------------------
  // HTTP HELPERS
  // -----------------------------
  async post(path, body) {
    const res = await fetch(API_ROOT + path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body)
    });
    return await res.text();
  },

  async get(path) {
    const res = await fetch(API_ROOT + path);
    return await res.json().catch(() => ({}));
  },

  // -----------------------------
  // AUTH
  // -----------------------------
  async login(username, password) {
    const r = await this.post("/api/login", { userID: username, password });
    if (r === "OK") {
      this.setUser(username);
      location.href = "dashboard.html";
    } else {
      alert("Invalid username or password");
    }
  },

  async signup(username, password) {
    const r = await this.post("/api/signup", { userID: username, password });
    if (r === "Signup OK") {
      this.setUser(username);
      location.href = "dashboard.html";
    } else {
      alert("Signup failed: " + r);
    }
  },

  async ensureAuth() {
    if (!this.getUser()) {
      location.href = "index.html";
      return false;
    }
    this.updateUserBadge();
    return true;
  },

  signout() {
    this.setUser(null);
    location.href = "index.html";
  },

  // -----------------------------
  // NOTES
  // -----------------------------
  async addNote(text, title = "") {
    const user = this.getUser();
    const r = await this.post("/api/addNote", {
      userID: user,
      title,
      body: text
    });
    if (r === "OK") return true;
    alert("Error saving note: " + r);
    return false;
  },

  async deleteNote(noteID) {
    const user = this.getUser();
    const r = await this.post("/api/deleteNote", { userID: user, noteID });
    if (r === "OK") {
      this.loadNotesPage();
    } else {
      alert("Delete failed");
    }
  },

  async editNote(noteID, title, body) {
    const user = this.getUser();
    const r = await this.post("/api/editNote", {
      userID: user,
      noteID,
      title,
      body
    });

    if (r === "OK") {
      this.loadNotesPage();
    } else {
      alert("Edit failed");
    }
  },

  encode(b) {
    return btoa(unescape(encodeURIComponent(b)));
  },
  decode(b) {
    return decodeURIComponent(escape(atob(b)));
  },

  async loadNotesPage() {
    const user = this.getUser();
    const notes = await this.get(`/api/notes?user=${user}`);
    const list = document.getElementById("notesList");
    const search = document.getElementById("searchBox")?.value.toLowerCase() || "";

    const filtered = search ? this.searchNotes(search, notes) : notes;

    list.innerHTML = "";

    filtered.slice().reverse().forEach(n => {
      const div = document.createElement("div");
      div.className = "note-row";
      div.innerHTML = `
        <div class="note-title">${n.title}
          <small>${n.timestamp}</small>
        </div>

        <div class="note-body">${n.body.replace(/\n/g,"<br>")}</div>

        <button class="btn" onclick="App.showEdit('${n.id}', '${n.title.replace(/'/g,"&#39;")}', \`${n.body.replace(/`/g,"\\`")}\`)">Edit</button>
        <button class="btn" onclick="App.deleteNote('${n.id}')">Delete</button>
      `;
      list.appendChild(div);
    });
  },

  searchNotes(query, notes) {
    query = query.toLowerCase();
    return notes.filter(n =>
      n.title.toLowerCase().includes(query) ||
      n.body.toLowerCase().includes(query)
    );
  },
// -----------------------------
// FULL NOTE POPUP (VIEW OTHER PEOPLE'S NOTES)
// -----------------------------
openFullNote(owner, noteID) {
  this.currentPopupOwner = owner;
  this.currentPopupNoteID = noteID;

  this.get(`/api/notes?user=${owner}`).then(notes => {
    const n = notes.find(x => x.id === noteID);
    if (!n) return;

    document.getElementById("popupTitle").textContent = n.title;
    document.getElementById("popupBody").innerHTML = n.body.replace(/\n/g, "<br>");
    document.getElementById("fullNotePopup").style.display = "block";
  });
},

closePopup() {
  document.getElementById("fullNotePopup").style.display = "none";
},

async savePopupNote() {
  const user = this.getUser();
  const owner = this.currentPopupOwner;
  const id = this.currentPopupNoteID;

  const notes = await this.get(`/api/notes?user=${owner}`);
  const n = notes.find(x => x.id === id);
  if (!n) return alert("Error retrieving note");

  const ok = await this.addNote(n.body, n.title);
  if (ok) alert("Saved to your notes!");
},

  // -----------------------------
  // EDIT POPUP
  // -----------------------------
  showEdit(id, title, body) {
  this.editingID = id;

  document.getElementById("editTitle").value = title;
  document.getElementById("editBody").value  = body;

  document.getElementById("editPopup").style.display = "block";
},

  async saveEdit() {
    const id = this.editingID;
    const title = document.getElementById("editTitle").value;
    const body  = document.getElementById("editBody").value;

    await this.editNote(id, title, body);
    document.getElementById("editPopup").style.display = "none";
  },

  // -----------------------------
  // DASHBOARD
  // -----------------------------
  async loadDashboard() {
    const user = this.getUser();
    const notes = await this.get(`/api/notes?user=${user}`);

    document.getElementById("totalNotes").textContent = notes.length;

    const recent = notes.length ? notes[notes.length - 1].body : "—";
    document.getElementById("recentNote").textContent = recent;

    const list = document.getElementById("notesList");
    list.innerHTML = "";

    notes.slice().reverse().slice(0, 5).forEach(n => {
      const div = document.createElement("div");
      div.className = "note-row";
      div.innerHTML = `
        <div class="note-title">${n.title} <small>${n.timestamp}</small></div>
        <div class="note-body">${n.body}</div>
      `;
      list.appendChild(div);
    });

    // -----------------------------
    // LOAD RECOMMENDATIONS
    // -----------------------------
    const recDiv = document.getElementById("recommendBox");
    try {
      const rec = await this.get(`/api/recommend?user=${user}`);
      recDiv.innerHTML = rec && rec.length
        ? rec.map(r => `<div>• ${r}</div>`).join("")
        : "<div>No recommendations yet</div>";
    } catch {
      recDiv.innerHTML = "<div class='muted'>Error loading recommendations</div>";
    }
  },

  // -----------------------------
  // ANALYTICS
  // -----------------------------
  async loadAnalyticsPage() {
    const user = this.getUser();
    const a = await this.get(`/api/analytics?user=${user}`);

    const kw = Array.isArray(a.keywords) ? a.keywords.join(", ") : "—";
    document.getElementById("keywordList").textContent = kw;
    document.getElementById("analyticsJson").textContent =
      JSON.stringify(a, null, 2);
  },

  // -----------------------------
  // GLOBAL SEARCH
  // -----------------------------
  async searchAll(query) {
    if (!query.trim()) return [];

    // await fetch(`${API_ROOT}/api/addSearchTerm`, {
    //   method: "POST",
    //   headers: { "Content-Type": "application/json" },
    //   body: JSON.stringify({ user: this.getUser(), term: query })
    // });

    const q = encodeURIComponent(query.trim());
    const res = await fetch(`${API_ROOT}/api/search?q=${q}`);
    if (!res.ok) return [];

    try {
      const j = await res.json();
      return Array.isArray(j) ? j : [];
    } catch(e) {
      return [];
    }
  },

  // -----------------------------
  // EXPORT PDF
  // -----------------------------
  async exportPdf() {
    const user = this.getUser();
    if (!user) return alert("Not logged in!");

    const r = await fetch(`${API_ROOT}/api/exportPdf?user=${user}`);
    const data = await r.json().catch(() => null);

    if (!data || data.status !== "exported") {
      alert("PDF export failed");
      return;
    }

    const pdfRes = await fetch(`${API_ROOT}/exported_notes.pdf`);
    const blob = await pdfRes.blob();
    const url = URL.createObjectURL(blob);

    window.open(url, "_blank");
  }
};

window.App = App;
