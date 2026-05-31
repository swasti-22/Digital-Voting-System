#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────
//  Candidate
// ─────────────────────────────────────────────
class Candidate {
public:
    int    id;
    string name;
    int    votes;

    Candidate(int id, const string& name)
        : id(id), name(name), votes(0) {}

    // Serialize to a single line: "id|name|votes"
    string serialize() const {
        return to_string(id) + "|" + name + "|" + to_string(votes);
    }

    // Deserialize from a line produced by serialize()
    static bool deserialize(const string& line, Candidate& out) {
        size_t p1 = line.find('|');
        size_t p2 = line.rfind('|');
        if (p1 == string::npos || p1 == p2) return false;
        try {
            int    id    = stoi(line.substr(0, p1));
            string name  = line.substr(p1 + 1, p2 - p1 - 1);
            int    votes = stoi(line.substr(p2 + 1));
            out = Candidate(id, name);
            out.votes = votes;
            return true;
        } catch (...) {
            return false;
        }
    }
};

// ─────────────────────────────────────────────
//  Election  —  owns all state + logic
// ─────────────────────────────────────────────
class Election {
    vector<Candidate> candidates;
    vector<int>       votedVoters;

    const string DATA_FILE   = "election_data.txt";
    const string RESULT_FILE = "results.txt";

    // ── helpers ──────────────────────────────

    static void clearInput() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    static int readInt(const string& prompt) {
        int v;
        while (true) {
            cout << prompt;
            if (cin >> v) { clearInput(); return v; }
            cout << "  Invalid input — please enter a number.\n";
            clearInput();
        }
    }

    Candidate* findCandidate(int id) {
        for (auto& c : candidates)
            if (c.id == id) return &c;
        return nullptr;
    }

    bool idTaken(int id) const {
        for (const auto& c : candidates)
            if (c.id == id) return true;
        return false;
    }

    bool hasVoted(int voterID) const {
        for (int v : votedVoters)
            if (v == voterID) return true;
        return false;
    }

    // ── persistence ──────────────────────────

    void save() const {
        ofstream f(DATA_FILE);
        if (!f) { cout << "  Warning: could not save data.\n"; return; }

        // section 1 — candidates
        f << "[candidates]\n";
        for (const auto& c : candidates)
            f << c.serialize() << "\n";

        // section 2 — voted voter IDs
        f << "[voters]\n";
        for (int v : votedVoters)
            f << v << "\n";
    }

    void load() {
        ifstream f(DATA_FILE);
        if (!f) return;   // first run — nothing to load

        candidates.clear();
        votedVoters.clear();

        string line, section;
        while (getline(f, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') { section = line; continue; }

            if (section == "[candidates]") {
                Candidate c(0, "");
                if (Candidate::deserialize(line, c))
                    candidates.push_back(c);
            } else if (section == "[voters]") {
                try { votedVoters.push_back(stoi(line)); }
                catch (...) {}
            }
        }
    }

    // ── display helpers ───────────────────────

    static void printDivider(char ch = '-', int w = 40) {
        cout << string(w, ch) << "\n";
    }

    static void printHeader(const string& title) {
        int w = 40;
        printDivider('=', w);
        int pad = (w - (int)title.size()) / 2;
        cout << string(pad, ' ') << title << "\n";
        printDivider('=', w);
    }

    void printCandidates() const {
        if (candidates.empty()) {
            cout << "  No candidates registered yet.\n";
            return;
        }
        cout << "\n"
             << left << setw(6) << "ID"
             << setw(24) << "Name"
             << right << setw(8) << "Votes" << "\n";
        printDivider('-', 38);
        for (const auto& c : candidates) {
            cout << left << setw(6) << c.id
                 << setw(24) << c.name
                 << right << setw(8) << c.votes << "\n";
        }
    }

    // ── feature functions ─────────────────────

    void addCandidate() {
        cout << "\n";
        int id = readInt("  Enter Candidate ID : ");
        if (idTaken(id)) {
            cout << "  ID " << id << " is already taken. Choose another.\n";
            return;
        }
        clearInput();   // in case readInt already consumed the newline
        cout << "  Enter Candidate Name: ";
        string name;
        getline(cin, name);
        if (name.empty()) { cout << "  Name cannot be empty.\n"; return; }

        candidates.emplace_back(id, name);
        save();
        cout << "  Candidate \"" << name << "\" added.\n";
    }

    void removeCandidate() {
        if (candidates.empty()) { cout << "  No candidates to remove.\n"; return; }
        printCandidates();
        int id = readInt("\n  Enter Candidate ID to remove: ");
        auto it = remove_if(candidates.begin(), candidates.end(),
                            [id](const Candidate& c){ return c.id == id; });
        if (it == candidates.end()) {
            cout << "  Candidate ID " << id << " not found.\n";
        } else {
            cout << "  Removed candidate \"" << it->name << "\".\n";
            candidates.erase(it, candidates.end());
            save();
        }
    }

    void castVote() {
        int voterID = readInt("\n  Enter your Voter ID: ");

        if (hasVoted(voterID)) {
            cout << "  Voter " << voterID << " has already cast a vote.\n";
            return;
        }

        if (candidates.empty()) {
            cout << "  No candidates available to vote for.\n";
            return;
        }

        printCandidates();
        int candidateID = readInt("\n  Enter Candidate ID to vote for: ");

        Candidate* target = findCandidate(candidateID);
        if (!target) {
            cout << "  Candidate ID " << candidateID << " not found.\n";
            return;
        }

        target->votes++;
        votedVoters.push_back(voterID);
        save();
        cout << "  Vote cast for \"" << target->name << "\". Thank you!\n";
    }

    void showResults() const {
        if (candidates.empty()) {
            cout << "  No candidates found.\n";
            return;
        }

        printCandidates();

        // find max votes
        int maxVotes = 0;
        for (const auto& c : candidates)
            maxVotes = max(maxVotes, c.votes);

        // collect all candidates with max votes
        vector<string> winners;
        for (const auto& c : candidates)
            if (c.votes == maxVotes)
                winners.push_back(c.name);

        cout << "\n";
        printDivider('-', 38);
        if (maxVotes == 0) {
            cout << "  No votes have been cast yet.\n";
        } else if (winners.size() == 1) {
            cout << "  Winner : " << winners[0]
                 << " (" << maxVotes << " votes)\n";
        } else {
            cout << "  Tie between " << winners.size() << " candidates"
                 << " (" << maxVotes << " votes each):\n";
            for (const auto& w : winners)
                cout << "    * " << w << "\n";
        }
        printDivider('-', 38);

        // persist results
        ofstream f(RESULT_FILE);
        if (f) {
            f << "=== Election Results ===\n";
            for (const auto& c : candidates)
                f << c.name << " : " << c.votes << " votes\n";
            f << "\n";
            if (maxVotes == 0) {
                f << "No votes cast.\n";
            } else if (winners.size() == 1) {
                f << "Winner: " << winners[0] << " (" << maxVotes << " votes)\n";
            } else {
                f << "Tie between: ";
                for (size_t i = 0; i < winners.size(); ++i) {
                    f << winners[i];
                    if (i + 1 < winners.size()) f << ", ";
                }
                f << " (" << maxVotes << " votes each)\n";
            }
            cout << "  Results saved to \"" << RESULT_FILE << "\".\n";
        }
    }

    void resetElection() {
        int confirm = readInt("  Type 1 to confirm reset, 0 to cancel: ");
        if (confirm != 1) { cout << "  Reset cancelled.\n"; return; }
        candidates.clear();
        votedVoters.clear();
        save();
        cout << "  Election data cleared.\n";
    }

    // ── menus ─────────────────────────────────

    void adminMenu() {
        int choice;
        do {
            cout << "\n";
            printHeader("ADMIN PANEL");
            cout << "  1. Add candidate\n"
                 << "  2. Remove candidate\n"
                 << "  3. View candidates\n"
                 << "  4. View results\n"
                 << "  5. Reset election\n"
                 << "  6. Back\n";
            printDivider();
            choice = readInt("  Choice: ");

            switch (choice) {
                case 1: addCandidate();  break;
                case 2: removeCandidate(); break;
                case 3: cout << "\n"; printCandidates(); break;
                case 4: cout << "\n"; showResults(); break;
                case 5: resetElection(); break;
                case 6: cout << "  Returning to main menu.\n"; break;
                default: cout << "  Invalid choice.\n";
            }
        } while (choice != 6);
    }

    void voterMenu() {
        int choice;
        do {
            cout << "\n";
            printHeader("VOTER PANEL");
            cout << "  1. View candidates\n"
                 << "  2. Cast vote\n"
                 << "  3. Back\n";
            printDivider();
            choice = readInt("  Choice: ");

            switch (choice) {
                case 1: cout << "\n"; printCandidates(); break;
                case 2: castVote(); break;
                case 3: cout << "  Returning to main menu.\n"; break;
                default: cout << "  Invalid choice.\n";
            }
        } while (choice != 3);
    }

public:
    // ── entry point ───────────────────────────

    void run() {
        load();   // restore previous session if any

        int choice;
        do {
            cout << "\n";
            printHeader("DIGITAL VOTING SYSTEM");
            cout << "  1. Admin panel\n"
                 << "  2. Voter panel\n"
                 << "  3. Exit\n";
            printDivider();
            choice = readInt("  Choice: ");

            switch (choice) {
                case 1: adminMenu(); break;
                case 2: voterMenu(); break;
                case 3: cout << "  Goodbye!\n"; break;
                default: cout << "  Invalid choice.\n";
            }
        } while (choice != 3);
    }
};

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    Election election;
    election.run();
    return 0;
}
