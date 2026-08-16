# WaterCounter.java
/**
 * 💧 Water Counter – Daily Hydration Tracker (Java Edition)
 * Advanced: colorful console, JSON persistence, analytics, export/import
 * Requires: Java 17+
 */

import java.io.*;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.stream.Collectors;

// ─── Data Classes ──────────────────────────────────────────────────────────

class WaterEntry {
    private String date;
    private int amount;
    private String timestamp;

    public WaterEntry() {}

    public WaterEntry(String date, int amount, String timestamp) {
        this.date = date;
        this.amount = amount;
        this.timestamp = timestamp;
    }

    public String getDate() { return date; }
    public int getAmount() { return amount; }
    public String getTimestamp() { return timestamp; }

    public void setDate(String date) { this.date = date; }
    public void setAmount(int amount) { this.amount = amount; }
    public void setTimestamp(String timestamp) { this.timestamp = timestamp; }
}

class WaterData {
    private int goal;
    private List<WaterEntry> entries;

    public WaterData() {
        this.goal = 2000;
        this.entries = new ArrayList<>();
    }

    public WaterData(int goal, List<WaterEntry> entries) {
        this.goal = goal;
        this.entries = entries;
    }

    public int getGoal() { return goal; }
    public List<WaterEntry> getEntries() { return entries; }
    public void setGoal(int goal) { this.goal = goal; }
    public void setEntries(List<WaterEntry> entries) { this.entries = entries; }
}

// ─── Main App ──────────────────────────────────────────────────────────────

public class WaterCounter {
    private static final int DEFAULT_GOAL = 2000;
    private static final int MAX_AMOUNT = 10000;
    private static final int HISTORY_DAYS = 7;
    private static final String DATA_DIR = System.getProperty("user.home") + "/.water_counter";
    private static final String DATA_FILE = DATA_DIR + "/data.json";

    // ─── Colors ──────────────────────────────────────────────────────────────

    private static final String RESET = "\u001B[0m";
    private static final String BRIGHT = "\u001B[1m";
    private static final String DIM = "\u001B[2m";
    private static final String RED = "\u001B[31m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";
    private static final String CYAN = "\u001B[36m";

    private static String c(String text, String color) {
        return color + text + RESET;
    }

    // ─── State ───────────────────────────────────────────────────────────────

    private int goal;
    private List<WaterEntry> entries;
    private final Scanner scanner;

    public WaterCounter() {
        this.scanner = new Scanner(System.in);
        ensureDataDir();
        loadData();
    }

    // ─── Data Persistence ──────────────────────────────────────────────────

    private void ensureDataDir() {
        try {
            Files.createDirectories(Paths.get(DATA_DIR));
        } catch (IOException e) {
            System.err.println(c("⚠️  Could not create data directory.", YELLOW));
        }
    }

    private void loadData() {
        Path path = Paths.get(DATA_FILE);
        if (!Files.exists(path)) {
            this.goal = DEFAULT_GOAL;
            this.entries = new ArrayList<>();
            return;
        }
        try {
            String json = new String(Files.readAllBytes(path));
            WaterData data = parseJson(json);
            this.goal = data.getGoal() > 0 ? data.getGoal() : DEFAULT_GOAL;
            this.entries = data.getEntries();
            if (this.entries == null) this.entries = new ArrayList<>();
        } catch (Exception e) {
            System.err.println(c("⚠️  Error loading data. Starting fresh.", YELLOW));
            this.goal = DEFAULT_GOAL;
            this.entries = new ArrayList<>();
        }
    }

    private void saveData() {
        String json = toJson();
        Path tempPath = Paths.get(DATA_FILE + ".tmp");
        try {
            Files.write(tempPath, json.getBytes());
            Files.move(tempPath, Paths.get(DATA_FILE), StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            System.err.println(c("❌ Failed to save data: " + e.getMessage(), RED));
            System.exit(1);
        }
    }

    // ─── Simple JSON (manual) ──────────────────────────────────────────────

    private String toJson() {
        StringBuilder sb = new StringBuilder();
        sb.append("{\n  \"goal\": ").append(goal).append(",\n  \"entries\": [\n");
        for (int i = 0; i < entries.size(); i++) {
            WaterEntry e = entries.get(i);
            sb.append("    {\n");
            sb.append("      \"date\": \"").append(escapeJson(e.getDate())).append("\",\n");
            sb.append("      \"amount\": ").append(e.getAmount()).append(",\n");
            sb.append("      \"timestamp\": \"").append(escapeJson(e.getTimestamp())).append("\"\n");
            sb.append("    }");
            if (i < entries.size() - 1) sb.append(",");
            sb.append("\n");
        }
        sb.append("  ]\n}");
        return sb.toString();
    }

    private WaterData parseJson(String json) {
        WaterData data = new WaterData();
        data.setGoal(DEFAULT_GOAL);
        data.setEntries(new ArrayList<>());

        // Parse goal
        int goalIdx = json.indexOf("\"goal\":");
        if (goalIdx >= 0) {
            int start = json.indexOf(":", goalIdx) + 1;
            int end = json.indexOf(",", start);
            if (end < 0) end = json.indexOf("}", start);
            if (end > start) {
                try {
                    data.setGoal(Integer.parseInt(json.substring(start, end).trim()));
                } catch (NumberFormatException ignored) {}
            }
        }

        // Parse entries
        int entriesIdx = json.indexOf("\"entries\":");
        if (entriesIdx < 0) return data;
        int arrayStart = json.indexOf("[", entriesIdx);
        if (arrayStart < 0) return data;
        int arrayEnd = json.lastIndexOf("]");
        if (arrayEnd <= arrayStart) return data;

        String entriesStr = json.substring(arrayStart + 1, arrayEnd);
        List<String> entryStrings = new ArrayList<>();
        int braceCount = 0;
        StringBuilder current = new StringBuilder();
        for (char ch : entriesStr.toCharArray()) {
            if (ch == '{') braceCount++;
            else if (ch == '}') braceCount--;
            current.append(ch);
            if (braceCount == 0 && ch == '}') {
                entryStrings.add(current.toString());
                current = new StringBuilder();
            }
        }

        for (String es : entryStrings) {
            if (es.trim().isEmpty()) continue;
            WaterEntry entry = new WaterEntry();
            String dateVal = findValue(es, "date");
            if (!dateVal.isEmpty()) entry.setDate(dateVal.trim());
            String amtVal = findValue(es, "amount");
            if (!amtVal.isEmpty()) {
                try {
                    entry.setAmount(Integer.parseInt(amtVal.trim()));
                } catch (NumberFormatException ignored) {}
            }
            String tsVal = findValue(es, "timestamp");
            if (!tsVal.isEmpty()) entry.setTimestamp(tsVal.trim());
            if (!entry.getDate().isEmpty() && entry.getAmount() > 0) {
                data.getEntries().add(entry);
            }
        }
        return data;
    }

    private String findValue(String s, String key) {
        int pos = s.indexOf("\"" + key + "\":");
        if (pos < 0) return "";
        pos = s.indexOf(":", pos) + 1;
        while (pos < s.length() && (s.charAt(pos) == ' ' || s.charAt(pos) == '\n' || s.charAt(pos) == '\r')) pos++;
        if (s.charAt(pos) == '"') {
            pos++;
            int end = s.indexOf("\"", pos);
            if (end < 0) return "";
            return s.substring(pos, end);
        } else {
            int end = s.indexOf(",", pos);
            if (end < 0) end = s.indexOf("}", pos);
            if (end < 0) return s.substring(pos).trim();
            return s.substring(pos, end).trim();
        }
    }

    private String escapeJson(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    // ─── Helpers ────────────────────────────────────────────────────────────

    private String getToday() {
        return LocalDate.now().format(DateTimeFormatter.ISO_LOCAL_DATE);
    }

    private String getTimestamp() {
        return LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME);
    }

    private List<WaterEntry> getTodayEntries() {
        String today = getToday();
        return entries.stream()
                .filter(e -> e.getDate().equals(today))
                .collect(Collectors.toList());
    }

    private int getTodayTotal() {
        return getTodayEntries().stream().mapToInt(WaterEntry::getAmount).sum();
    }

    private String getProgressBar(int current, int goal, int width) {
        if (goal <= 0) return "⚠️  Goal not set";
        double ratio = Math.min((double) current / goal, 1.0);
        int filled = (int) (ratio * width);
        String bar = "█".repeat(filled) + "░".repeat(width - filled);
        return String.format("%s %.1f%%", bar, ratio * 100.0);
    }

    private String ask(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private int askInt(String prompt) {
        while (true) {
            try {
                return Integer.parseInt(ask(prompt));
            } catch (NumberFormatException e) {
                System.out.println(c("❌ Please enter a number.", RED));
            }
        }
    }

    private boolean askConfirm(String prompt) {
        String ans = ask(prompt + " (yes/no): ").toLowerCase();
        return ans.equals("yes") || ans.equals("y");
    }

    // ─── Core Actions ──────────────────────────────────────────────────────

    private void addEntry(int amount) {
        if (amount <= 0) {
            System.out.println(c("❌ Amount must be positive!", RED));
            return;
        }
        if (amount > MAX_AMOUNT) {
            System.out.println(c("⚠️  That's a LOT of water! Max " + MAX_AMOUNT + "ml", YELLOW));
            if (!askConfirm("Continue?")) return;
        }

        WaterEntry entry = new WaterEntry(getToday(), amount, getTimestamp());
        entries.add(entry);
        saveData();

        int todayTotal = getTodayTotal();
        System.out.println(c("✅ Added " + amount + "ml (Total today: " + todayTotal + "ml)", GREEN));
        if (todayTotal >= goal) {
            System.out.println(c("🎉 Goal achieved! Stay hydrated! 💪", CYAN));
        }
    }

    private void showToday() {
        int todayTotal = getTodayTotal();
        List<WaterEntry> todayEntries = getTodayEntries();

        System.out.println("\n" + c("═".repeat(50), DIM));
        System.out.println(c("💧 TODAY'S HYDRATION", BRIGHT + CYAN));
        System.out.println(c("═".repeat(50), DIM));
        System.out.println("  Goal:      " + c(goal + "ml", CYAN));
        System.out.println("  Consumed:  " + c(todayTotal + "ml", GREEN));
        System.out.println("  Remaining: " + c(Math.max(goal - todayTotal, 0) + "ml", YELLOW));
        System.out.println("  Progress:  " + getProgressBar(todayTotal, goal, 30));
        System.out.println(c("═".repeat(50), DIM));

        if (todayEntries.isEmpty()) {
            System.out.println(c("  No entries yet today. Drink up! 💧", DIM));
        } else {
            System.out.println("  Entries:");
            for (int i = 0; i < todayEntries.size(); i++) {
                WaterEntry e = todayEntries.get(i);
                String ts = e.getTimestamp().length() >= 16 ? e.getTimestamp().substring(11, 16) : "—";
                System.out.println("    " + (i + 1) + ". " + ts + " → " + c(e.getAmount() + "ml", GREEN));
            }
        }
        System.out.println(c("═".repeat(50), DIM));
    }

    private void showHistory(int days) {
        LocalDate now = LocalDate.now();
        List<AbstractMap.SimpleEntry<String, Integer>> history = new ArrayList<>();

        for (int i = 0; i < days; i++) {
            LocalDate d = now.minusDays(i);
            String dateStr = d.format(DateTimeFormatter.ISO_LOCAL_DATE);
            int total = entries.stream()
                    .filter(e -> e.getDate().equals(dateStr))
                    .mapToInt(WaterEntry::getAmount)
                    .sum();
            history.add(new AbstractMap.SimpleEntry<>(dateStr, total));
        }

        System.out.println("\n📅 LAST " + days + " DAYS");
        System.out.println(c("─".repeat(40), DIM));
        for (var h : history) {
            String status = h.getValue() >= goal ? "✅" : (h.getValue() > 0 ? "⏳" : "❌");
            System.out.printf("  %s: %5dml  %s\n", h.getKey(), h.getValue(), status);
        }
        System.out.println(c("─".repeat(40), DIM));
    }

    private void showStats() {
        if (entries.isEmpty()) {
            System.out.println(c("📭 No data yet. Start tracking your water intake!", YELLOW));
            return;
        }

        int total = entries.stream().mapToInt(WaterEntry::getAmount).sum();
        int count = entries.size();
        double avg = (double) total / count;
        int maxEntry = entries.stream().mapToInt(WaterEntry::getAmount).max().orElse(0);
        int minEntry = entries.stream().mapToInt(WaterEntry::getAmount).min().orElse(0);

        Set<String> uniqueDays = entries.stream().map(WaterEntry::getDate).collect(Collectors.toSet());
        int dayCount = uniqueDays.size();

        // Best day
        Map<String, Integer> dayTot
