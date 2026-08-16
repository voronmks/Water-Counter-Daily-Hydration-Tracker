# water_counter.cpp
/**
 * 💧 Water Counter – Daily Hydration Tracker (C++ Edition)
 * Advanced: colored output, JSON persistence, analytics, export/import
 * Requires: nlohmann/json library (included via single-header)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <memory>

// ─── JSON Support (Single-header nlohmann/json) ────────────────────────────
// In production, use: #include <nlohmann/json.hpp>
// For simplicity, we use a minimal JSON parser/writer
// ─────────────────────────────────────────────────────────────────────────────

// Simple JSON serialization for our data structures
struct WaterEntry {
    std::string date;
    int amount;
    std::string timestamp;
};

struct WaterData {
    int goal;
    std::vector<WaterEntry> entries;
};

// ─── Constants ──────────────────────────────────────────────────────────────

const int DEFAULT_GOAL = 2000;
const int MAX_AMOUNT = 10000;
const int HISTORY_DAYS = 7;

// ─── Colors ──────────────────────────────────────────────────────────────────

#ifdef _WIN32
    #include <windows.h>
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }
    #define RESET_COLOR setColor(7)
    #define COLOR_RED setColor(12)
    #define COLOR_GREEN setColor(10)
    #define COLOR_YELLOW setColor(14)
    #define COLOR_BLUE setColor(9)
    #define COLOR_MAGENTA setColor(13)
    #define COLOR_CYAN setColor(11)
    #define COLOR_BRIGHT setColor(15)
    #define COLOR_DIM setColor(8)
#else
    #define RESET_COLOR std::cout << "\x1b[0m"
    #define COLOR_RED std::cout << "\x1b[31m"
    #define COLOR_GREEN std::cout << "\x1b[32m"
    #define COLOR_YELLOW std::cout << "\x1b[33m"
    #define COLOR_BLUE std::cout << "\x1b[34m"
    #define COLOR_MAGENTA std::cout << "\x1b[35m"
    #define COLOR_CYAN std::cout << "\x1b[36m"
    #define COLOR_BRIGHT std::cout << "\x1b[1m"
    #define COLOR_DIM std::cout << "\x1b[2m"
#endif

#define C(str, color) color << str << RESET_COLOR

// ─── Helpers ──────────────────────────────────────────────────────────────────

std::string get_today() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

std::string get_timestamp() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string get_home_dir() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    return home ? std::string(home) : ".";
}

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) parts.push_back(item);
    }
    return parts;
}

// ─── Minimal JSON ────────────────────────────────────────────────────────────

std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string serialize_data(const WaterData& data) {
    std::ostringstream json;
    json << "{\n  \"goal\": " << data.goal << ",\n  \"entries\": [\n";
    for (size_t i = 0; i < data.entries.size(); ++i) {
        const auto& e = data.entries[i];
        json << "    {\n";
        json << "      \"date\": \"" << json_escape(e.date) << "\",\n";
        json << "      \"amount\": " << e.amount << ",\n";
        json << "      \"timestamp\": \"" << json_escape(e.timestamp) << "\"\n";
        json << "    }";
        if (i + 1 < data.entries.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n}";
    return json.str();
}

bool deserialize_data(const std::string& json_str, WaterData& data) {
    // Very simple parser for our format
    // In production, use a proper JSON library
    data.goal = DEFAULT_GOAL;
    data.entries.clear();

    auto find_value = [&](const std::string& key) -> std::string {
        size_t pos = json_str.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        pos = json_str.find(":", pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\n' || json_str[pos] == '\r')) pos++;
        if (json_str[pos] == '"') {
            pos++;
            size_t end = json_str.find("\"", pos);
            if (end == std::string::npos) return "";
            return json_str.substr(pos, end - pos);
        } else {
            size_t end = json_str.find_first_of(",}\n\r", pos);
            if (end == std::string::npos) return "";
            return json_str.substr(pos, end - pos);
        }
    };

    // Parse goal
    std::string goal_str = find_value("goal");
    if (!goal_str.empty()) {
        try { data.goal = std::stoi(trim(goal_str)); }
        catch (...) { data.goal = DEFAULT_GOAL; }
    }

    // Parse entries
    size_t entries_start = json_str.find("\"entries\":");
    if (entries_start == std::string::npos) return true;
    size_t array_start = json_str.find("[", entries_start);
    if (array_start == std::string::npos) return true;
    size_t array_end = json_str.rfind("]");
    if (array_end == std::string::npos || array_end <= array_start) return true;

    std::string entries_str = json_str.substr(array_start + 1, array_end - array_start - 1);
    std::vector<std::string> entry_strings;
    int brace_count = 0;
    std::string current;
    for (char c : entries_str) {
        if (c == '{') brace_count++;
        else if (c == '}') brace_count--;
        current += c;
        if (brace_count == 0 && c == '}') {
            entry_strings.push_back(current);
            current.clear();
        }
    }

    for (const auto& es : entry_strings) {
        if (trim(es).empty()) continue;
        WaterEntry entry;
        std::string date_val = find_value_in(es, "date");
        if (!date_val.empty()) entry.date = trim(date_val);
        std::string amt_val = find_value_in(es, "amount");
        if (!amt_val.empty()) {
            try { entry.amount = std::stoi(trim(amt_val)); }
            catch (...) { entry.amount = 0; }
        }
        std::string ts_val = find_value_in(es, "timestamp");
        if (!ts_val.empty()) entry.timestamp = trim(ts_val);
        if (!entry.date.empty() && entry.amount > 0) {
            data.entries.push_back(entry);
        }
    }
    return true;
}

std::string find_value_in(const std::string& s, const std::string& key) {
    size_t pos = s.find("\"" + key + "\":");
    if (pos == std::string::npos) return "";
    pos = s.find(":", pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r')) pos++;
    if (s[pos] == '"') {
        pos++;
        size_t end = s.find("\"", pos);
        if (end == std::string::npos) return "";
        return s.substr(pos, end - pos);
    } else {
        size_t end = s.find_first_of(",}\n\r", pos);
        if (end == std::string::npos) return "";
        return s.substr(pos, end - pos);
    }
}

// ─── Main App ──────────────────────────────────────────────────────────────

class WaterCounter {
private:
    int goal;
    std::vector<WaterEntry> entries;
    std::string data_path;
    std::string home_dir;

    void load_data() {
        std::ifstream file(data_path);
        if (!file.is_open()) {
            goal = DEFAULT_GOAL;
            entries.clear();
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        WaterData data;
        if (deserialize_data(buffer.str(), data)) {
            goal = data.goal > 0 ? data.goal : DEFAULT_GOAL;
            entries = data.entries;
        } else {
            goal = DEFAULT_GOAL;
            entries.clear();
        }
    }

    void save_data() {
        WaterData data{goal, entries};
        std::string json = serialize_data(data);
        // Atomic write
        std::string temp_path = data_path + ".tmp";
        std::ofstream file(temp_path);
        if (!file.is_open()) {
            std::cerr << C("❌ Failed to save data!", RED) << std::endl;
            return;
        }
        file << json;
        file.close();
        std::filesystem::rename(temp_path, data_path);
    }

    std::string get_today() const {
        return ::get_today();
    }

    std::vector<WaterEntry> get_today_entries() const {
        std::vector<WaterEntry> result;
        std::string today = get_today();
        for (const auto& e : entries) {
            if (e.date == today) result.push_back(e);
        }
        return result;
    }

    int get_today_total() const {
        int total = 0;
        for (const auto& e : get_today_entries()) {
            total += e.amount;
        }
        return total;
    }

    std::string get_progress_bar(int current, int goal, int width = 30) const {
        if (goal <= 0) return "⚠️  Goal not set";
        double ratio = std::min(static_cast<double>(current) / goal, 1.0);
        int filled = static_cast<int>(ratio * width);
        std::string bar = std::string(filled, '█') + std::string(width - filled, '░');
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %.1f%%", bar.c_str(), ratio * 100.0);
        return std::string(buf);
    }

    std::string ask(const std::string& prompt) const {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return trim(line);
    }

    int ask_int(const std::string& prompt) const {
        while (true) {
            std::string ans = ask(prompt);
            try {
                return std::stoi(ans);
            } catch (...) {
                std::cout << C("❌ Please enter a number.", RED) << std::endl;
            }
        }
    }

    bool ask_confirm(const std::string& prompt) const {
        std::string ans = ask(prompt + " (yes/no): ");
        std::string lower = ans;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "yes" || lower == "y";
    }

public:
    WaterCounter() {
        home_dir = get_home_dir();
        std::string data_dir = home_dir + "/.water_counter";
        std::filesystem::create_directories(data_dir);
        data_path = data_dir + "/data.json";
        load_data();
    }

    void add_entry(int amount) {
        if (amount <= 0) {
            std::cout << C("❌ Amount must be positive!", RED) << std::endl;
            return;
        }
        if (amount > MAX_AMOUNT) {
            std::cout << C("⚠️  That's a LOT of water! Max " + std::to_string(MAX_AMOUNT) + "ml", YELLOW) << std::endl;
            if (!ask_confirm("Continue?")) return;
        }

        WaterEntry entry;
        entry.date = get_today();
        entry.amount = amount;
        entry.timestamp = get_timestamp();
        entries.push_back(entry);
        save_data();

        int today_total = get_today_total();
        std::cout << C("✅ Added " + std::to_string(amount) + "ml (Total today: " + std::to_string(today_total) + "ml)", GREEN) << std::endl;
        if (today_total >= goal) {
            std::cout << C("🎉 Goal achieved! Stay hydrated! 💪", CYAN) << std::endl;
        }
    }

    void show_today() const {
        int today_total = get_today_total();
        auto today_entries = get_today_entries();

        std::cout << "\n" << C(std::string(50, '═'), DIM) << std::endl;
        std::cout << C("💧 TODAY'S HYDRATION", BRIGHT) << C("", CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), DIM) << std::endl;
        std::cout << "  Goal:      " << C(std::to_string(goal) + "ml", CYAN) << std::endl;
        std::cout << "  Consumed:  " << C(std::to_string(today_total) + "ml", GREEN) << std::endl;
        std::cout << "  Remaining: " << C(std::to_string(std::max(goal - today_total, 0)) + "ml", YELLOW) << std::endl;
        std::cout << "  Progress:  " << get_progress_bar(today_total, goal) << std::endl;
        std::cout << C(std::string(50, '═'), DIM) << std::endl;

        if (today_entries.empty()) {
            std::cout << C("  No entries yet today. Drink up! 💧", DIM) << std::endl;
        } else {
            std::cout << "  Entries:" << std::endl;
            for (size_t i = 0; i < today_entries.size(); ++i) {
                const auto& e = today_entries[i];
                std::string ts = e.timestamp.size() >= 16 ? e.timestamp.substr(11, 5) : "—";
                std::cout << "    " << (i + 1) << ". " << ts << " → " << C(std::to_string(e.amount) + "ml", GREEN) << std::endl;
            }
        }
        std::cout << C(std::string(50, '═'), DIM) << std::endl;
    }

    void show_history(int days = HISTORY_DAYS) const {
        auto now = std::time(nullptr);
        std::vector<std::pair<std::string, int>> history;

        for (int i = 0; i < days; ++i) {
            auto t = now - i * 86400;
            auto tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d");
            std::string date_str = oss.str();

            int total = 0;
            for (const auto& e : entries) {
                if (e.date == date_str) total += e.amount;
            }
            history.push_back({date_str, total});
        }

        std::cout << "\n📅 LAST " << days << " DAYS" << std::endl;
        std::cout << C(std::string(40, '─'), DIM) << std::endl;
        for (const auto& h : history) {
            std::string status = h.second >= goal ? "✅" : (h.second > 0 ? "⏳" : "❌");
            std::cout << "  " << h.first << ": " << std::setw(5) << h.second << "ml  " << status << std::endl;
        }
        std::cout << C(std::string(40, '─'), DIM) << std::endl;
    }

    void show_stats() const {
        if (entries.empty()) {
            std::cout << C("📭 No data yet. Start tracking your water intake!", YELLOW) << std::endl;
            return;
        }

        int total = 0;
        int min_entry = INT_MAX;
        int max_entry = 0;
        for (const auto& e : entries) {
            total += e.amount;
            if (e.amount < min_entry) min_entry = e.amount;
            if (e.amount > max_entry) max_entry = e.amount;
        }
        int count = entries.size();
        double avg = static_cast<double>(total) / count;

        std::set<std::string> unique_days;
        for (const auto& e : entries) {
            unique_days.insert(e.date);
        }
        int day_count = unique_days.size();

        // Best day
        std::map<std::string, int> day_totals;
        for (const auto& e : entries) {
            day_totals[e.date] += e.amount;
        }
        std::string best_day;
        int best_amount = 0;
        for (const auto& [date, amt] : day_totals) {
            if (amt > best_amount) {
                best_amount = amt;
                best_day = date;
            }
        }

        std::cout << "\n" << C(std::string(50, '═'), DIM) << std::endl;
        std::cout << C("📊 STATISTICS", BRIGHT) << C("", MAGENTA) << std::endl;
        std::cout << C(std::string(50, '═'), DIM) << std::endl;
        std::cout << "  Total consumed:  " << C(std::to_string(total) + "ml", CYAN) << std::endl;
        std::cout << "  Total entries:   " << C(std::to_string(count), CYAN) << std::endl;
        std::cout << "  Days tracked:    " << C(std::to_string(day_count), CYAN) << std::endl;
        std::cout << "  Average per day: " << C(std::to_string(avg).substr(0, 4) + "ml", CYAN) << std::endl;
        std::cout << "  Max entry:       " << C(std::to_string(max_entry) + "ml", GREEN) << std::endl;
        std::cout << "  Min entry:       " << C(std::to_string(min_entry) + "ml", YELLOW) << std::endl;
        std::cout << "  Best day:        " << C((best_day.empty() ? "—" : best_day + " (" + std::to_string(best_amount) + "ml)"), GREEN) << std::endl;
        std::cout << "  Daily goal:      " << C(std::to_string(goal) + "ml", CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), DIM) << std::endl;
    }

    void set_goal(int new_goal) {
        if (new_goal <= 0) {
            std::cout << C("❌ Goal must be positive!", RED) << std::endl;
            return;
        }
        if (new_goal > MAX_AMOUNT) {
            std::cout << C("⚠️  That's an extreme goal! Max " + std::to_string(MAX_AMOUNT) + "ml", YELLOW) << std::endl;
            if (!ask_confirm("Set anyway?")) return;
        }
        goal = new_goal;
        save_data();
        std::cout << C("✅ Daily goal set to " + std::to_string(goal) + "ml", GREEN) << std::endl;
    }

    void export_data(const std::string& filepath = "") {
        std::string fp = filepath;
        if (fp.empty()) {
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "water_counter_export_%Y%m%d_%H%M%S.json");
            fp = oss.str();
        }
        WaterData data{goal, entries};
        std::string json = serialize_data(data);
        std::ofstream file(fp);
        if (!file.is_open()) {
            std::cout << C("❌ Export failed!", RED) << std::endl;
            return;
        }
        file << json;
        file.close();
        std::cout << C("✅ Data exported to: " + fp, GREEN) << std::endl;
    }

    void import_data(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << C("❌ File not found: " + filepath, RED) << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        WaterData data;
        if (!deserialize_data(buffer.str(), data) || data.goal <= 0) {
            std::cout << C("❌ Invalid data format!", RED) << std::endl;
            return;
        }
        if (!ask_confirm("⚠️  This will overwrite current data! Continue?")) return;
        goal = data.goal;
        entries = data.entries;
        save_data();
        std::cout << C("✅ Imported " + std::to_string(entries.size()) + " entries from: " + filepath, GREEN) << std::endl;
    }

    void clear_data() {
        if (entries.empty() && goal == DEFAULT_GOAL) {
            std::cout << C("📭 Already empty.", DIM) << std::endl;
            return;
        }
        if (!ask_confirm("⚠️  Delete ALL data? This cannot be undone!")) return;
        entries.clear();
        goal = DEFAULT_GOAL;
        save_data();
        std::cout << C("🗑️  All data cleared.", YELLOW) << std::endl;
    }

    void show_menu() const {
        int today_total = get_today_total();
        std::string progress = get_progress_bar(today_total, goal);

        std::cout << "\n" << C(std::string(50, '═'), CYAN) << std::endl;
        std::cout << C("💧 WATER COUNTER – Daily Hydration Tracker", BRIGHT) << C("", CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), CYAN) << std::endl;
        std::cout << "  Today: " << today_total << "ml / " << goal << "ml  " << progress << std::endl;
        std::cout << C(std::string(50, '─'), DIM) << std::endl;
        std::cout << "  1. 💧 Add water intake" << std::endl;
        std::cout << "  2. 📊 Show today's progress" << std::endl;
        std::cout << "  3. 📅 Show history (" << HISTORY_DAYS << " days)" << std::endl;
        std::cout << "  4. 📈 Show statistics" << std::endl;
        std::cout << "  5. 🎯 Set daily goal (current: " << goal << "ml)" << std::endl;
        std::cout << "  6. 📤 Import / Export data" << std::endl;
        std::cout << "  7. 🗑️  Clear all data" << std::endl;
        std::cout << "  0. 🚪 Exit" << std::endl;
        std::cout << C(std::string(50, '═'), CYAN) << std::endl;
    }

    void handle_import_export() {
        std::cout << "\n📤 IMPORT / EXPORT" << std::endl;
        std::cout << "  1. Export data" << std::endl;
        std::cout << "  2. Import data" << std::endl;
        std::cout << "  0. Back" << std::endl;
        std::string choice = ask("Your choice: ");

        if (choice == "1") {
            export_data();
        } else if (choice == "2") {
            std::string fp = ask("Path to JSON file: ");
            if (!fp.empty()) import_data(fp);
        }
    }

    void run() {
        std::cout << "\033[2J\033[1;1H";
        std::cout << C("\n💧 Water Counter – Daily Hydration Tracker", BRIGHT) << C("", CYAN) << std::endl;
        std::cout << C("Stay hydrated, stay healthy!", DIM) << std::endl;

        while (true) {
            show_menu();
            std::string choice = ask("Your choice: ");

            if (choice == "1") {
                int amount = ask_int("Amount in ml: ");
                add_entry(amount);
            } else if (choice == "2") {
                show_today();
            } else if (choice == "3") {
                show_history();
            } else if (choice == "4") {
                show_stats();
            } else if (choice == "5") {
                int new_goal = ask_int("New daily goal (ml): ");
                set_goal(new_goal);
            } else if (choice == "6") {
                handle_import_export();
            } else if (choice == "7") {
                clear_data();
            } else if (choice == "0") {
                std::cout << C("👋 Stay hydrated! Goodbye!", CYAN) << std::endl;
                break;
            } else {
                std::cout << C("❌ Invalid choice.", RED) << std::endl;
            }

            if (choice != "0") {
                std::cout << "\nPress Enter to continue...";
                std::cin.get();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
    }
};

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    try {
        WaterCounter app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << C("❌ Unexpected error: ", RED) << e.what() << std::endl;
        return 1;
    }
    return 0;
}
