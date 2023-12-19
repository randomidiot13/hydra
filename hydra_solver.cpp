#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#define MAJOR 0
#define MINOR 3
#define MICRO 20231218

const int PTR_LENGTH = 3;
const int HASH_LENGTH = 5;
const int PIECE_SHAPES = 7;
const int NUM_FIELDS = 15185706;
const unsigned long long MAX_HASH = 0xFFFFFFFFFFull;
const unsigned long long TWO_LINE_HASH = 0xFFFFFull;
const std::string PIECE_ORDER = "IJLOSTZ";
const std::set<int> FULL_BAG = {0, 1, 2, 3, 4, 5, 6};
const double F = std::pow(2, -32);

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
            for (int i = 0; i < (int)n.edges[shape].size(); i++) {
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

template<class T>
T epsilon() {
    return 1 - (T) (1 - F);
}

template<class T>
struct Decision {
    T prob;
    T cap;
    int field;
    int piece;
    bool is_solve;
    Decision<T>* children[PIECE_SHAPES];

    Decision(T neg, T cutoff, bool s) {
        prob = neg;
        cap = cutoff;
        field = -1;
        piece = -1;
        is_solve = s;
        for (int i = 0; i < PIECE_SHAPES; i++) children[i] = nullptr;
    }

    ~Decision() {
        for (int i = 0; i < PIECE_SHAPES; i++) {
            if (children[i]) delete children[i];
        }
    }
};

template<class T>
std::ostream& operator<<(std::ostream& os, const Decision<T>* d) {
    if (!d) {
        os << "null";
        return os;
    }
    if (!d->is_solve) {
        if (d->prob < d->cap) {
            os << '[' << fields[d->field].hash << ',' << d->piece << ',' << (d->prob/epsilon<T>()) << ",[";
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
        os << "[[" << (d->prob/epsilon<T>()) << ']';
        const Decision<T>* d2 = d;
        while (d2) {
            os << ",[" << fields[d2->field].hash << ',' << d2->piece << ']';
            d2 = d2->children[0];
        }
        os << ']';
    }
    return os;
}

template<class T>
T score_perfect_single(
    int field, int hold, std::deque<int>& pieces, bool two_line,
    const std::array<T, PIECE_SHAPES>& weights
) {
    if (field == NUM_FIELDS - 1) return weights[hold];
    if (two_line && fields[field].hash == TWO_LINE_HASH) return 0;
    if (pieces.empty()) return 1;

    T min = 1;
    int front = pieces.front();
    pieces.pop_front();
    for (int f: fields[field].edges[front]) {
        T temp = score_perfect_single(f, hold, pieces, two_line, weights);
        if (temp < min) {
            min = temp;
            if (!min) goto end;
        }
    }

    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            T temp = score_perfect_single(f, front, pieces, two_line, weights);
            if (temp < min) {
                min = temp;
                if (!min) goto end;
            }
        }
    }

    end:
    pieces.push_front(front);
    return min;
}

template<class T>
T score_perfect_multiple(
    int field, int hold, std::deque<int>& pieces, bool two_line,
    const std::map<int, std::array<T, PIECE_SHAPES>>& weights, std::map<int, T>& options
) {
    if (two_line && fields[field].hash == TWO_LINE_HASH) options.clear();
    if (options.empty()) return 0;

    int front = pieces.front();

    if (pieces.empty()) {
        if (!fields[field].edges[hold].empty()) {
            auto itr = options.begin();
            while (itr != options.end()) {
                itr->second = std::min(itr->second, weights.at(itr->first)[itr->first]);
                if (itr->second) itr++;
                else itr = options.erase(itr);
            }
        }

        auto itr = options.begin();
        while (itr != options.end()) {
            if (!fields[field].edges[itr->first].empty()) {
                itr->second = std::min(itr->second, weights.at(itr->first)[hold]);
                if (itr->second) itr++;
                else itr = options.erase(itr);
            }
            else itr++;
        }

        goto ret;
    }

    pieces.pop_front();
    for (int f: fields[field].edges[front]) {
        if (!score_perfect_multiple(f, hold, pieces, two_line, weights, options)) goto end_push;
    }

    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            if (!score_perfect_multiple(f, front, pieces, two_line, weights, options)) goto end_push;
        }
    }

    end_push:
    pieces.push_front(front);
    ret:
    T sum = 0;
    for (const std::pair<int, T>& p: options) sum += p.second;
    return sum;
}

template<class T>
T score_imperfect(
    int field, int hold, std::deque<int>& q, bool two_line,
    int placed, T neg, std::set<int>& bag, std::vector<T>& cutoffs,
    std::map<std::string, std::array<T, PIECE_SHAPES>>& weights
) {
    if (placed >= (int)cutoffs.size() - 1) {
        std::string s = "";
        for (int p: bag) {
            s += "IJLOSTZ"[p];
        }
        auto itr = weights.find(s);
        if (itr == weights.end()) {
            itr = weights.find("default");
        }
        return score_perfect_single(field, hold, q, two_line, itr->second);
    }
    if (two_line && fields[field].hash == TWO_LINE_HASH) return 0;

    if (bag.empty()) bag = FULL_BAG;

    int front = q.front();
    q.pop_front();
    std::set<int> wbag;
    neg = std::min(neg, cutoffs[placed]);

    if (placed == (int)cutoffs.size() - 2) {
        std::map<int, std::array<T, PIECE_SHAPES>> weight_map;
        for (int p: bag) {
            wbag = bag;
            wbag.erase(p);
            std::string s = "";
            for (int piece: wbag) {
                s += "IJLOSTZ"[piece];
            }
            auto itr = weights.find(s);
            if (itr == weights.end()) {
                itr = weights.find("default");
            }
            weight_map[p] = itr->second;
        }

        std::map<int, T> options;
        for (int p: bag) options[p] = cutoffs[placed + 1];

        for (int f: fields[field].edges[front]) {
            std::map<int, T> woptions = options;
            neg = std::min(neg, score_perfect_multiple(f, hold, q, two_line, weight_map, woptions));
            if (!neg) goto success;
        }

        if (front != hold) {
            for (int f: fields[field].edges[hold]) {
                std::map<int, T> woptions = options;
                neg = std::min(neg, score_perfect_multiple(f, front, q, two_line, weight_map, woptions));
                if (!neg) goto success;
            }
        }

        goto success;
    }
    
    for (int f: fields[field].edges[front]) {
        T total = 0;
        for (int p: bag) {
            wbag = bag;
            wbag.erase(p);
            q.push_back(p);
            total += score_imperfect(f, hold, q, two_line, placed+1, neg - total, wbag, cutoffs, weights);
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
            T total = 0;
            for (int p: bag) {
                wbag = bag;
                wbag.erase(p);
                q.push_back(p);
                total += score_imperfect(f, front, q, two_line, placed+1, neg - total, wbag, cutoffs, weights);
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

template<class T>
void tree_perfect_single(
    int field, int hold, std::deque<int>& pieces,
    Decision<T>* prev, const std::array<T, PIECE_SHAPES>& weights
) {
    if (pieces.empty()) {
        prev->prob = weights[hold];
        return;
    }

    Decision<T>* min = new Decision<T>(prev->prob, 1, true);
    int front = pieces.front();
    pieces.pop_front();

    for (int f: fields[field].edges[front]) {
        Decision<T>* d = new Decision<T>(min->prob, 1, true);
        d->field = f;
        d->piece = front;
        tree_perfect_single(f, hold, pieces, d, weights);
        if (d->prob >= min->prob) {
            delete d;
            continue;
        }
        delete min;
        min = d;
        if (!(min->prob)) goto success;
    }

    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            Decision<T>* d = new Decision<T>(min->prob, 1, true);
            d->field = f;
            d->piece = hold;
            tree_perfect_single(f, front, pieces, d, weights);
            if (d->prob >= min->prob) {
                delete d;
                continue;
            }
            delete min;
            min = d;
            if (!(min->prob)) goto success;
        }
    }

    success:
    pieces.push_front(front);
    prev->prob = min->prob;
    prev->children[0] = min;
}

template<class T>
Decision<T>* tree_imperfect(
    int field, int hold, std::deque<int>& q,
    int placed, T neg, std::set<int>& bag, std::vector<T>& cutoffs,
    std::map<std::string, std::array<T, PIECE_SHAPES>>& weights
) {
    if (placed >= (int)cutoffs.size() - 1) {
        std::string s = "";
        for (int p: bag) {
            s += "IJLOSTZ"[p];
        }
        auto itr = weights.find(s);
        if (itr == weights.end()) {
            itr = weights.find("default");
        }
        Decision<T>* d = new Decision<T>(std::min((T)1, neg), 1, true);
        tree_perfect_single(field, hold, q, d, itr->second);
        Decision<T>* d2 = d->children[0];
        d->children[0] = nullptr;
        delete d;
        if (d2->field == -1) d2->is_solve = false;
        return d2;
    }

    if (bag.empty()) bag = FULL_BAG;

    int front = q.front();
    q.pop_front();
    Decision<T>* min = new Decision<T>(std::min(neg, cutoffs[placed]), cutoffs[placed], false);
    std::set<int> wbag;

    for (int f: fields[field].edges[front]) {
        Decision<T>* d = new Decision<T>(0, cutoffs[placed], false);
        d->field = f;
        d->piece = front;
        for (int p: bag) {
            wbag = bag;
            wbag.erase(p);
            q.push_back(p);
            Decision<T>* d2 = tree_imperfect(f, hold, q, placed+1, min->prob - d->prob, wbag, cutoffs, weights);
            d->children[p] = d2;
            d->prob += d2->prob;
            q.pop_back();
            if (d->prob >= min->prob) break;
        }
        if (d->prob < min->prob) {
            delete min;
            min = d;
            if (!(d->prob)) goto success;
        }
        else delete d;
    }
    
    if (front != hold) {
        for (int f: fields[field].edges[hold]) {
            Decision<T>* d = new Decision<T>(0, cutoffs[placed], false);
            d->field = f;
            d->piece = hold;
            for (int p: bag) {
                wbag = bag;
                wbag.erase(p);
                q.push_back(p);
                Decision<T>* d2 = tree_imperfect(f, front, q, placed+1, min->prob - d->prob, wbag, cutoffs, weights);
                d->children[p] = d2;
                d->prob += d2->prob;
                q.pop_back();
                if (d->prob >= min->prob) break;
            }
            if (d->prob < min->prob) {
                delete min;
                min = d;
                if (!(d->prob)) goto success;
            }
            else delete d;
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

template<class T>
int inner_main(int argc, char** argv) {
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
    bool two_line_mode = option_exists(argv, argv + argc, "-t");
    bool version_mode = option_exists(argv, argv + argc, "-v");
    bool weighted_mode = option_exists(argv, argv + argc, "-w");

    if (boolean_mode && decision_mode) {
        std::cerr << "Cannot combine -b and -d modes\n";
        return 1;
    }
    if (boolean_mode && weighted_mode) {
        std::cerr << "Cannot combine -b and -w modes\n";
        return 1;
    }
    if (decision_mode && two_line_mode) {
        std::cerr << "Cannot combine -d and -t modes\n";
        return 1;
    }

    std::cerr << "Hydra v" << MAJOR << '.' << MINOR << '.' << MICRO << '\n';
    if (version_mode) return 0;

    std::map<std::string, std::array<T, PIECE_SHAPES>> weight_map;
    weight_map["default"] = {0, 0, 0, 0, 0, 0, 0};
    std::cerr << "Loading weights... " << std::flush;
    std::ifstream weights("weights.txt");
    for (int i = 1; i < (1 << PIECE_SHAPES); i++) {
        std::string s;
        weights >> s;
        if (s == "null") s = "";
        weight_map[s] = {};
        for (int j = 0; j < PIECE_SHAPES; j++) {
            long long x;
            weights >> x;
            if (x < 0 || x > 1/F) {
                std::cerr << "\nWeights must be in the range [0, 2^32]\n";
                return 1;
            }
            weight_map[s][j] = x * epsilon<T>();
        }
    }
    if (!weighted_mode) {
        auto itr = weight_map.begin();
        while (itr != weight_map.end()) {
            if (itr->first == "default") itr++;
            else itr = weight_map.erase(itr);
        }
    }
    std::cerr << "\n";
    
    std::cerr << "Loading graph... " << std::flush;
    std::ifstream graph("graph.bin", std::ios::binary);
    auto start_time = std::chrono::steady_clock::now();
    for (int i = 0; i < NUM_FIELDS; i++) {
        graph >> fields[i];
    }
    auto end_time = std::chrono::steady_clock::now();
    std::cerr << " (took ";
    std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms)\n";

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
    if (two_line_mode) std::cerr << "Running 2L mode\n";
    if (weighted_mode) std::cerr << "Running weighted mode\n";
    std::cerr << '\n';

    int hold;
    std::deque<int> q;
    std::set<int> bag;

    std::cerr << std::setprecision(19);
    std::cout << std::setprecision(19);

    for (int count = 0; ; count++) {
        std::vector<T> cutoffs(std::max(1, 12 - placed - see));
        std::cerr << "> ";
        std::string s;
        std::cin >> s;

        if (s == "-f") {
            std::cin >> requested_hash;
            if (requested_hash >= MAX_HASH) {
                std::cerr << "Invalid hash\n";
                return 1;
            }
            field_index = hash_lookup(requested_hash);
            if (field_index >= NUM_FIELDS || fields[field_index].hash != requested_hash) {
                std::cerr << "Invalid hash\n";
                return 1;
            }
            // can't use __builtin_popcount() for portability :(
            popcount = 0;
            for (long long i = 0; i < HASH_LENGTH * 8; i++) {
                popcount += (fields[field_index].hash >> i) & 1;
            }
            if (popcount & 3) {
                std::cerr << "Somehow not a multiple of 4 minos\n";
                return 1;
            }
            placed = popcount / 4;
            std::cerr << "Changed starting field to " << field_index << " (hash " << fields[field_index].hash << ")\n\n";
            count--;
            continue;
        }

        if (s == "-s") {
            std::cin >> see;
            if (see < 2 || see > 11) {
                std::cerr << "Invalid see\n";
                return 1;
            }
            std::cerr << "Changed see to " << see << "\n\n";
            count--;
            continue;
        }

        if ((int)s.size() != see) break;
        hold = PIECE_ORDER.find(s[0]);
        if (hold == (int)std::string::npos) {
            std::cerr << "Invalid piece\n";
            return 1;
        }

        q.clear();
        for (int i = 1; i < see; i++) {
            int x = PIECE_ORDER.find(s[i]);
            if (x == (int)std::string::npos) {
                std::cerr << "Invalid piece\n";
                return 1;
            }
            q.push_back(x);
        }

        std::string t;
        std::cin >> t;
        if (t.size() == 1 && '0' < t[0] && t[0] <= '7') {
            int size = t[0] - '0';
            if (size + see < PIECE_SHAPES) {
                std::cerr << "Too few pieces to infer bag\n";
                return 1;
            }
            bag = FULL_BAG;
            for (int i = 0; i < PIECE_SHAPES - size; i++) {
                int x = PIECE_ORDER.find(s[see + ~i]);
                if (!bag.count(x)) {
                    std::cerr << "Cannot infer bag from duplicate pieces\n";
                    return 1;
                }
                bag.erase(x);
            }
        }
        else {
            bag.clear();
            for (const char c: t) {
                int x = PIECE_ORDER.find(c);
                if (x == (int)std::string::npos) {
                    std::cerr << "Invalid piece\n";
                    return 1;
                }
                bag.insert(x);
            }
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
            Decision<T>* d = tree_imperfect(field_index, hold, q, 0, cutoffs[0], bag, cutoffs, weight_map);
            end_time = std::chrono::steady_clock::now();

            std::ofstream fout("tree_data.js");
            fout << std::setprecision(19);
            fout << "init_hash=" << fields[field_index].hash << '\n';
            fout << "data=" << d;
            T result;
            if (weighted_mode) {
                result = d->prob / epsilon<T>();
                std::cerr << "Result: " << result;
            }
            else {
                result = cutoffs[0] - d->prob;
                std::cerr << "Result: " << result << '/' << cutoffs[0];
            }
            std::cerr << "\nTime: ";
            std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms\n\n";
            if (out_mode) std::cout << result << '\n';
            delete d;
        }
        else {
            start_time = std::chrono::steady_clock::now();
            T result = score_imperfect(field_index, hold, q, two_line_mode, 0, boolean_mode ? 1 : cutoffs[0], bag, cutoffs, weight_map);
            end_time = std::chrono::steady_clock::now();
            
            if (boolean_mode) cutoffs[0] = 1;
            if (weighted_mode) {
                result /= epsilon<T>();
                std::cerr << "Result: " << result;
            }
            else {
                result = cutoffs[0] - result;
                std::cerr << "Result: " << result << '/' << cutoffs[0];
            }
            std::cerr << "\nTime: ";
            std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms\n\n";
            if (out_mode) std::cout << result << '\n';
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    bool weighted_mode = option_exists(argv, argv + argc, "-w");
    if (weighted_mode) return (int)inner_main<double>(argc, argv);
    else return inner_main<int>(argc, argv);
}
