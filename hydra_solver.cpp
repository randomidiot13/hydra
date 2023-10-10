#include <algorithm>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

const int PTR_LENGTH = 3;
const int HASH_LENGTH = 5;
const int PIECE_SHAPES = 7;
const int NUM_FIELDS = 15185706;
const unsigned long long MAX_HASH = 0xFFFFFFFFFFull;
const std::string PIECE_ORDER = "IJLOSTZ";

struct Node {
    unsigned long long hash;
    std::vector<int> edges[PIECE_SHAPES];

    friend std::istream& operator>>(std::istream& is, Node& n) {
        char buffer[HASH_LENGTH];

        is.read(buffer, HASH_LENGTH);
        n.hash = 0ull;
        for (int i = 0; i < HASH_LENGTH; i++) {
            n.hash <<= 8;
            n.hash += (unsigned char) buffer[i];
        }

        for (int shape = 0; shape < PIECE_SHAPES; shape++) {
            is.read(buffer, 1);
            n.edges[shape] = std::vector<int>(buffer[0]);
            for (int i = 0; i < n.edges[shape].size(); i++) {
                is.read(buffer, PTR_LENGTH);
                n.edges[shape][i] = 0;
                for (int j = PTR_LENGTH - 1; j >= 0; j--) {
                    n.edges[shape][i] <<= 8;
                    n.edges[shape][i] += (unsigned char) buffer[j];
                }
            }
        }
        return is;
    }
};

std::vector<Node> fields(NUM_FIELDS);

bool can_pc_hold(int field, int hold, std::deque<int>& pieces) {
    if (field == NUM_FIELDS - 1) return true;
    if (pieces.empty()) return false;

    bool ret = false;
    int front = pieces.front();
    pieces.pop_front();
    for (int f: fields[field].edges[front]) {
        if (can_pc_hold(f, hold, pieces)) {
            ret = true;
            goto end;
        }
    }

    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            if (can_pc_hold(f, front, pieces)) {
                ret = true;
                goto end;
            }
        }
    }

    end:
    pieces.push_front(front);
    return ret;
}

int solve(
    int field, int placed, int hold, std::deque<int>& q,
    std::set<int>& bag, std::vector<int>& cutoffs, int neg
) {
    if (placed >= cutoffs.size() - 1) return !can_pc_hold(field, hold, q);

    if (bag.empty()) {
        for (int i = 0; i < PIECE_SHAPES; i++) bag.insert(i);
    }

    int front = q.front();
    q.pop_front();
    std::set<int> wbag;
    neg = std::min(neg, cutoffs[placed]);
    
    for (int f: fields[field].edges[front]) {
        int total = 0;
        for (int p: bag) {
            wbag = bag;
            wbag.erase(p);
            q.push_back(p);
            total += solve(f, placed+1, hold, q, wbag, cutoffs, neg - total);
            q.pop_back();
            if (total >= neg) goto give_up_front;
        }
        neg = std::min(total, neg);
        if (!neg) goto success;
        
        give_up_front:
        ; // end of loop
    }
    
    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            int total = 0;
            for (int p: bag) {
                wbag = bag;
                wbag.erase(p);
                q.push_back(p);
                total += solve(f, placed+1, front, q, wbag, cutoffs, neg - total);
                q.pop_back();
                if (total >= neg) goto give_up_hold;
            }
            neg = std::min(total, neg);
            if (!neg) goto success;
            
            give_up_hold:
            ; // end of loop
        }
    }
    
    success:
    q.push_front(front);
    return neg;
}

struct Decision {
    int prob;
    int field;
    int piece;
    int cap;
    Decision* children[PIECE_SHAPES];
    std::vector<Decision*> solve;

    Decision(int neg, int cutoff) {
        prob = neg;
        field = -1;
        piece = -1;
        cap = cutoff;
        solve = {};
        for (int i = 0; i < PIECE_SHAPES; i++) children[i] = nullptr;
    }

    ~Decision() {
        for (int i = 0; i < PIECE_SHAPES; i++) {
            if (children[i]) delete children[i];
        }
        for (Decision* d: solve) {
            if (d) delete d;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Decision* d) {
        if (!d) {
            os << "null";
            return os;
        }
        if (d->solve.empty()) {
            if (d->prob < d->cap) {
                os << '[' << fields[d->field].hash << ',' << d->piece << ',' << d->prob << ",[";
                for (int i = 0; i < PIECE_SHAPES; i++) {
                    if (i) os << ',';
                    os << d->children[i];
                }
                os << "]]";
            }
            else {
                os << "[-1,-1," << d->cap << ']';
            }
        }
        else {
            os << '[';
            for (int i = 0; i < d->solve.size(); i++) {
                if (i) os << ',';
                os << '[' << fields[d->solve[i]->field].hash << ',' << d->solve[i]->piece << ']';
            }
            os << ']';
        }
        return os;
    }
};

bool generate_pc_hold_inner(
    int field, int hold, std::deque<int>& pieces, std::vector<Decision*>& hist
) {
    if (field == NUM_FIELDS - 1) return true;
    if (pieces.empty()) return false;

    bool ret = false;
    int front = pieces.front();
    pieces.pop_front();
    for (int f: fields[field].edges[front]) {
        Decision* d = new Decision(1, 1);
        d->field = f;
        d->piece = front;
        hist.push_back(d);
        if (generate_pc_hold_inner(f, hold, pieces, hist)) {
            ret = true;
            d->prob = 0;
            goto end;
        }
        else {
            hist.pop_back();
            delete d;
        }
    }

    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            Decision* d = new Decision(1, 1);
            d->field = f;
            d->piece = hold;
            hist.push_back(d);
            if (generate_pc_hold_inner(f, front, pieces, hist)) {
                ret = true;
                d->prob = 0;
                goto end;
            }
            else {
                hist.pop_back();
                delete d;
            }
        }
    }

    end:
    pieces.push_front(front);
    return ret;
}

Decision* generate_pc_hold(int field, int hold, std::deque<int>& pieces) {
    std::vector<Decision*> hist;
    bool b = generate_pc_hold_inner(field, hold, pieces, hist);
    Decision* d = new Decision(!b, 1);
    d->solve = hist;
    return d;
}

Decision* generate_tree(
    int field, int placed, int hold, std::deque<int>& q,
    std::set<int>& bag, std::vector<int>& cutoffs, int neg
) {
    if (placed >= cutoffs.size() - 1) return generate_pc_hold(field, hold, q);

    if (bag.empty()) {
        for (int i = 0; i < PIECE_SHAPES; i++) bag.insert(i);
    }

    int front = q.front();
    q.pop_front();
    Decision* min = new Decision(std::min(neg, cutoffs[placed]), cutoffs[placed]);
    std::set<int> wbag;

    for (int f: fields[field].edges[front]) {
        Decision* d = new Decision(0, cutoffs[placed]);
        d->field = f;
        d->piece = front;
        for (int p: bag) {
            wbag = bag;
            wbag.erase(p);
            q.push_back(p);
            Decision* d2 = generate_tree(f, placed+1, hold, q, wbag, cutoffs, min->prob - d->prob);
            d->children[p] = d2;
            d->prob += d2->prob;
            q.pop_back();
            if (d->prob >= min->prob) goto give_up_front;
        }
        if (d->prob < min->prob) {
            delete min;
            min = d;
            if (!(d->prob)) goto success;
        }
        else {
            give_up_front:
            delete d;
        }
    }
    
    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            Decision* d = new Decision(0, cutoffs[placed]);
            d->field = f;
            d->piece = hold;
            for (int p: bag) {
                wbag = bag;
                wbag.erase(p);
                q.push_back(p);
                Decision* d2 = generate_tree(f, placed+1, front, q, wbag, cutoffs, min->prob - d->prob);
                d->children[p] = d2;
                d->prob += d2->prob;
                q.pop_back();
                if (d->prob >= min->prob) goto give_up_hold;
            }
            if (d->prob < min->prob) {
                delete min;
                min = d;
                if (!(d->prob)) goto success;
            }
            else {
                give_up_hold:
                delete d;
            }
        }
    }
    
    success:
    q.push_front(front);
    return min;
}

int hash_lookup(unsigned long long h) {
    int lo = 0, hi = NUM_FIELDS;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (fields[mid].hash < h) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

char* option_value(char** begin, char** end, const std::string& option) {
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) return *itr;
    return nullptr;
}

bool option_exists(char** begin, char**end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char** argv) {
    std::ifstream graph("graph.bin", std::ios::binary);
    char* p;

    unsigned long long requested_hash = -1ull;
    if ((p = option_value(argv, argv + argc, "-f"))) {
        try {
            requested_hash = std::stoull(p);
            if (requested_hash >= MAX_HASH) throw 0;
        }
        catch (...) {
            std::cerr << "Invalid hash\n";
            return 1;
        }
    }

    int see = 7;
    if ((p = option_value(argv, argv + argc, "-s"))) {
        try {
            see = std::stoi(p);
            if (see < 2 || see > 11) throw 0;
        }
        catch (...) {
            std::cerr << "Invalid see\n";
            return 1;
        }
    }

    bool boolean_mode = option_exists(argv, argv + argc, "-b");
    bool decision_mode = option_exists(argv, argv + argc, "-d");
    bool out_mode = option_exists(argv, argv + argc, "-o");
    if (boolean_mode && decision_mode) {
        std::cerr << "Cannot combine -b and -d modes\n";
        return 1;
    }

    std::cerr << "Loading graph\n";
    auto start_time = std::chrono::steady_clock::now();
    for (int i = 0; i < NUM_FIELDS; i++) {
        graph >> fields[i];
    }
    auto end_time = std::chrono::steady_clock::now();

    std::cerr << "Loaded " << NUM_FIELDS << " fields in ";
    std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms\n";

    int field_index = 0;
    if (requested_hash != -1ull) {
        field_index = hash_lookup(requested_hash);
        if (field_index >= NUM_FIELDS || fields[field_index].hash != requested_hash) {
            std::cerr << "Invalid hash\n";
            return 1;
        }
    }
    std::cerr << "Starting from field " << field_index << " (hash " << fields[field_index].hash << ")\n";

    // can't use __builtin_popcount() for portability :(
    int popcount = 0;
    for (long long i = 0; i < HASH_LENGTH * 8; i++) {
        popcount += (fields[field_index].hash >> i) & 1;
    }
    if (popcount & 3) {
        std::cerr << "Somehow not a multiple of 4 minos\n";
        return 1;
    }
    int placed = popcount / 4;

    std::cerr << "Running see " << see << "\n";
    if (boolean_mode) std::cerr << "Running boolean mode\n";
    if (decision_mode) std::cerr << "Running decision mode\n";
    if (out_mode) std::cerr << "Running stdout mode\n";
    std::cerr << '\n';

    int hold;
    std::deque<int> q;
    std::set<int> bag;
    std::vector<int> cutoffs(std::max(1, 12 - placed - see));

    for (int count = 0; ; count++) {
        std::cerr << "> ";
        std::string s;
        std::cin >> s;

        if (s.size() != see) break;
        hold = PIECE_ORDER.find(s[0]);
        if (hold == std::string::npos) {
            std::cerr << "Invalid piece\n";
            return 1;
        }

        q.clear();
        for (int i = 1; i < see; i++) {
            int x = PIECE_ORDER.find(s[i]);
            if (x == std::string::npos) {
                std::cerr << "Invalid piece\n";
                return 1;
            }
            q.push_back(x);
        }

        std::string t;
        std::cin >> t;
        bag.clear();
        for (const char c: t) {
            int x = PIECE_ORDER.find(c);
            if (x == std::string::npos) {
                std::cerr << "Invalid piece\n";
                return 1;
            }
            bag.insert(x);
        }

        cutoffs.back() = 1;
        for (int i = cutoffs.size() - 2; i >= 0; i--) {
            int num = (bag.size() + PIECE_SHAPES - i) % PIECE_SHAPES;
            cutoffs[i] = cutoffs[i + 1] * (num ? num : PIECE_SHAPES);
        }

        std::cerr << '[' << count << "] Testing queue " << s << " with bag ";
        for (int x: bag) std::cerr << PIECE_ORDER[x];
        std::cerr << '\n';

        if (decision_mode) {
            start_time = std::chrono::steady_clock::now();
            Decision* d = generate_tree(field_index, 0, hold, q, bag, cutoffs, cutoffs[0]);
            end_time = std::chrono::steady_clock::now();

            std::ofstream fout("tree_data.js");
            fout << "init_hash=" << fields[field_index].hash << '\n';
            fout << "data=" << d;
            std::cerr << "Result: " << (cutoffs[0] - d->prob) << '/' << cutoffs.front() << "\nTime: ";
            std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms\n\n";
            if (out_mode) std::cout << (cutoffs[0] - d->prob) << '\n';
            delete d;
        }
        else {
            start_time = std::chrono::steady_clock::now();
            int x = solve(field_index, 0, hold, q, bag, cutoffs, boolean_mode ? 1 : cutoffs[0]);
            end_time = std::chrono::steady_clock::now();
            if (boolean_mode) cutoffs[0] = 1;

            std::cerr << "Result: " << (cutoffs[0] - x) << '/' << cutoffs.front() << "\nTime: ";
            std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms\n\n";
            if (out_mode) std::cout << (cutoffs[0] - x) << '\n';
        }
    }

    return 0;
}
