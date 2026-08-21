// currency_favorites.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

const string FAVORITES_FILE = "favorites.json";
const string API_URL = "https://api.frankfurter.app/latest?from=";

struct Pair {
    string base;
    string target;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string fetchUrl(const string& url) {
    CURL *curl = curl_easy_init();
    string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) return "";
    }
    return response;
}

vector<Pair> loadFavorites() {
    vector<Pair> pairs;
    ifstream f(FAVORITES_FILE);
    if (f.is_open()) {
        json j;
        f >> j;
        if (j.contains("pairs")) {
            for (auto& p : j["pairs"]) {
                pairs.push_back({p["base"], p["target"]});
            }
        }
        f.close();
    }
    if (pairs.empty()) {
        pairs = {{"USD", "EUR"}, {"USD", "JPY"}};
    }
    return pairs;
}

void saveFavorites(const vector<Pair>& pairs) {
    json j;
    j["pairs"] = json::array();
    for (auto& p : pairs) {
        j["pairs"].push_back({{"base", p.base}, {"target", p.target}});
    }
    ofstream f(FAVORITES_FILE);
    f << setw(2) << j << endl;
}

map<string, double> getRates(const string& base) {
    string url = API_URL + base;
    string resp = fetchUrl(url);
    if (resp.empty()) return {};
    auto data = json::parse(resp);
    if (!data.contains("rates")) return {};
    map<string, double> rates;
    for (auto& [key, val] : data["rates"].items()) {
        rates[key] = val.get<double>();
    }
    return rates;
}

void listRates(const vector<Pair>& pairs) {
    if (pairs.empty()) {
        cout << "No favorite pairs.\n";
        return;
    }
    // group by base
    map<string, vector<Pair>> grouped;
    for (auto& p : pairs) {
        grouped[p.base].push_back(p);
    }
    cout << "\n💱 Favorite Currency Pairs\n\n";
    for (auto& [base, basePairs] : grouped) {
        auto rates = getRates(base);
        for (auto& p : basePairs) {
            if (rates.count(p.target)) {
                cout << base << "/" << p.target << ": " << fixed << setprecision(4) << rates[p.target] << "\n";
            } else {
                cout << base << "/" << p.target << ": N/A\n";
            }
        }
    }
    time_t t = time(nullptr);
    cout << "\nUpdated: " << put_time(localtime(&t), "%Y-%m-%d %H:%M:%S") << "\n";
}

int main(int argc, char* argv[]) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (argc < 2) {
        auto pairs = loadFavorites();
        listRates(pairs);
        curl_global_cleanup();
        return 0;
    }
    string cmd = argv[1];
    if (cmd == "list") {
        auto pairs = loadFavorites();
        listRates(pairs);
    } else if (cmd == "add") {
        if (argc != 4) {
            cerr << "Usage: add BASE TARGET\n";
            curl_global_cleanup();
            return 1;
        }
        string base = argv[2];
        string target = argv[3];
        transform(base.begin(), base.end(), base.begin(), ::toupper);
        transform(target.begin(), target.end(), target.begin(), ::toupper);
        auto pairs = loadFavorites();
        bool exists = false;
        for (auto& p : pairs) {
            if (p.base == base && p.target == target) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            pairs.push_back({base, target});
            saveFavorites(pairs);
            cout << "✅ Added " << base << "/" << target << "\n";
        } else {
            cout << "Pair " << base << "/" << target << " already in favorites.\n";
        }
    } else if (cmd == "remove") {
        if (argc != 4) {
            cerr << "Usage: remove BASE TARGET\n";
            curl_global_cleanup();
            return 1;
        }
        string base = argv[2];
        string target = argv[3];
        transform(base.begin(), base.end(), base.begin(), ::toupper);
        transform(target.begin(), target.end(), target.begin(), ::toupper);
        auto pairs = loadFavorites();
        vector<Pair> newPairs;
        bool removed = false;
        for (auto& p : pairs) {
            if (p.base == base && p.target == target) {
                removed = true;
            } else {
                newPairs.push_back(p);
            }
        }
        if (removed) {
            saveFavorites(newPairs);
            cout << "✅ Removed " << base << "/" << target << "\n";
        } else {
            cout << "Pair " << base << "/" << target << " not found.\n";
        }
    } else if (cmd == "watch") {
        int interval = 60;
        for (int i = 2; i < argc; i++) {
            if (string(argv[i]) == "--interval" && i+1 < argc) {
                interval = stoi(argv[i+1]);
                i++;
            }
        }
        cout << "Watching every " << interval << "s. Press Ctrl+C to stop.\n";
        auto pairs = loadFavorites();
        while (true) {
            listRates(pairs);
            this_thread::sleep_for(chrono::seconds(interval));
        }
    } else {
        cerr << "Usage: currency_favorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]\n";
        curl_global_cleanup();
        return 1;
    }
    curl_global_cleanup();
    return 0;
}
