# WaterCounter.cs
/**
 * 💧 Water Counter – Daily Hydration Tracker (C# Edition)
 * Advanced: colorful console, JSON persistence, analytics, export/import
 * Requires: .NET 6.0+
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

// ─── Data Classes ──────────────────────────────────────────────────────────

public class WaterEntry
{
    [JsonPropertyName("date")]
    public string Date { get; set; } = "";
    
    [JsonPropertyName("amount")]
    public int Amount { get; set; }
    
    [JsonPropertyName("timestamp")]
    public string Timestamp { get; set; } = "";
}

public class WaterData
{
    [JsonPropertyName("goal")]
    public int Goal { get; set; } = 2000;
    
    [JsonPropertyName("entries")]
    public List<WaterEntry> Entries { get; set; } = new();
}

// ─── Main App ──────────────────────────────────────────────────────────────

public class WaterCounter
{
    private const int DefaultGoal = 2000;
    private const int MaxAmount = 10000;
    private const int HistoryDays = 7;
    
    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
        ".water_counter"
    );
    private static readonly string DataFile = Path.Combine(DataDir, "data.json");

    // ─── Colors ──────────────────────────────────────────────────────────────

    private static readonly string Reset = "\u001B[0m";
    private static readonly string Bright = "\u001B[1m";
    private static readonly string Dim = "\u001B[2m";
    private static readonly string Red = "\u001B[31m";
    private static readonly string Green = "\u001B[32m";
    private static readonly string Yellow = "\u001B[33m";
    private static readonly string Blue = "\u001B[34m";
    private static readonly string Magenta = "\u001B[35m";
    private static readonly string Cyan = "\u001B[36m";

    private static string C(string text, string color) => color + text + Reset;

    // ─── State ───────────────────────────────────────────────────────────────

    private int goal;
    private List<WaterEntry> entries;

    public WaterCounter()
    {
        Directory.CreateDirectory(DataDir);
        LoadData();
    }

    // ─── Data Persistence ──────────────────────────────────────────────────

    private void LoadData()
    {
        if (!File.Exists(DataFile))
        {
            goal = DefaultGoal;
            entries = new List<WaterEntry>();
            return;
        }

        try
        {
            string json = File.ReadAllText(DataFile);
            var data = JsonSerializer.Deserialize<WaterData>(json);
            if (data != null)
            {
                goal = data.Goal > 0 ? data.Goal : DefaultGoal;
                entries = data.Entries ?? new List<WaterEntry>();
            }
            else
            {
                goal = DefaultGoal;
                entries = new List<WaterEntry>();
            }
        }
        catch
        {
            Console.WriteLine(C("⚠️  Error loading data. Starting fresh.", Yellow));
            goal = DefaultGoal;
            entries = new List<WaterEntry>();
        }
    }

    private void SaveData()
    {
        var data = new WaterData { Goal = goal, Entries = entries };
        string json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
        string tempPath = DataFile + ".tmp";
        File.WriteAllText(tempPath, json);
        File.Move(tempPath, DataFile, true);
    }

    // ─── Helpers ────────────────────────────────────────────────────────────

    private string GetToday() => DateTime.Now.ToString("yyyy-MM-dd");

    private string GetTimestamp() => DateTime.Now.ToString("yyyy-MM-ddTHH:mm:ss");

    private List<WaterEntry> GetTodayEntries()
    {
        string today = GetToday();
        return entries.Where(e => e.Date == today).ToList();
    }

    private int GetTodayTotal() => GetTodayEntries().Sum(e => e.Amount);

    private string GetProgressBar(int current, int goal, int width = 30)
    {
        if (goal <= 0) return "⚠️  Goal not set";
        double ratio = Math.Min((double)current / goal, 1.0);
        int filled = (int)(ratio * width);
        string bar = new string('█', filled) + new string('░', width - filled);
        return $"{bar} {ratio * 100.0:F1}%";
    }

    private string Ask(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private int AskInt(string prompt)
    {
        while (true)
        {
            if (int.TryParse(Ask(prompt), out int result))
                return result;
            Console.WriteLine(C("❌ Please enter a number.", Red));
        }
    }

    private bool AskConfirm(string prompt)
    {
        string ans = Ask(prompt + " (yes/no): ").ToLower();
        return ans == "yes" || ans == "y";
    }

    // ─── Core Actions ──────────────────────────────────────────────────────

    private void AddEntry(int amount)
    {
        if (amount <= 0)
        {
            Console.WriteLine(C("❌ Amount must be positive!", Red));
            return;
        }
        if (amount > MaxAmount)
        {
            Console.WriteLine(C($"⚠️  That's a LOT of water! Max {MaxAmount}ml", Yellow));
            if (!AskConfirm("Continue?")) return;
        }

        var entry = new WaterEntry
        {
            Date = GetToday(),
            Amount = amount,
            Timestamp = GetTimestamp()
        };
        entries.Add(entry);
        SaveData();

        int todayTotal = GetTodayTotal();
        Console.WriteLine(C($"✅ Added {amount}ml (Total today: {todayTotal}ml)", Green));
        if (todayTotal >= goal)
        {
            Console.WriteLine(C("🎉 Goal achieved! Stay hydrated! 💪", Cyan));
        }
    }

    private void ShowToday()
    {
        int todayTotal = GetTodayTotal();
        var todayEntries = GetTodayEntries();

        Console.WriteLine("\n" + C(new string('═', 50), Dim));
        Console.WriteLine(C("💧 TODAY'S HYDRATION", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Dim));
        Console.WriteLine($"  Goal:      {C($"{goal}ml", Cyan)}");
        Console.WriteLine($"  Consumed:  {C($"{todayTotal}ml", Green)}");
        Console.WriteLine($"  Remaining: {C($"{Math.Max(goal - todayTotal, 0)}ml", Yellow)}");
        Console.WriteLine($"  Progress:  {GetProgressBar(todayTotal, goal)}");
        Console.WriteLine(C(new string('═', 50), Dim));

        if (todayEntries.Count == 0)
        {
            Console.WriteLine(C("  No entries yet today. Drink up! 💧", Dim));
        }
        else
        {
            Console.WriteLine("  Entries:");
            for (int i = 0; i < todayEntries.Count; i++)
            {
                var e = todayEntries[i];
                string ts = e.Timestamp.Length >= 16 ? e.Timestamp[11..16] : "—";
                Console.WriteLine($"    {i + 1}. {ts} → {C($"{e.Amount}ml", Green)}");
            }
        }
        Console.WriteLine(C(new string('═', 50), Dim));
    }

    private void ShowHistory(int days)
    {
        var history = new List<(string Date, int Total)>();
        var now = DateTime.Now;

        for (int i = 0; i < days; i++)
        {
            var d = now.AddDays(-i);
            string dateStr = d.ToString("yyyy-MM-dd");
            int total = entries.Where(e => e.Date == dateStr).Sum(e => e.Amount);
            history.Add((dateStr, total));
        }

        Console.WriteLine($"\n📅 LAST {days} DAYS");
        Console.WriteLine(C(new string('─', 40), Dim));
        foreach (var h in history)
        {
            string status = h.Total >= goal ? "✅" : (h.Total > 0 ? "⏳" : "❌");
            Console.WriteLine($"  {h.Date}: {h.Total,5}ml  {status}");
        }
        Console.WriteLine(C(new string('─', 40), Dim));
    }

    private void ShowStats()
    {
        if (entries.Count == 0)
        {
            Console.WriteLine(C("📭 No data yet. Start tracking your water intake!", Yellow));
            return;
        }

        int total = entries.Sum(e => e.Amount);
        int count = entries.Count;
        double avg = (double)total / count;
        int maxEntry = entries.Max(e => e.Amount);
        int minEntry = entries.Min(e => e.Amount);

        var uniqueDays = entries.Select(e => e.Date).Distinct().Count();

        // Best day
        var dayTotals = new Dictionary<string, int>();
        foreach (var e in entries)
        {
            dayTotals[e.Date] = dayTotals.GetValueOrDefault(e.Date) + e.Amount;
        }
        var bestDay = dayTotals.OrderByDescending(kv => kv.Value).FirstOrDefault();

        Console.WriteLine("\n" + C(new string('═', 50), Dim));
        Console.WriteLine(C("📊 STATISTICS", Bright + Magenta));
        Console.WriteLine(C(new string('═', 50), Dim));
        Console.WriteLine($"  Total consumed:  {C($"{total}ml", Cyan)}");
        Console.WriteLine($"  Total entries:   {C($"{count}", Cyan)}");
        Console.WriteLine($"  Days tracked:    {C($"{uniqueDays}", Cyan)}");
        Console.WriteLine($"  Average per day: {C($"{avg:F1}ml", Cyan)}");
        Console.WriteLine($"  Max entry:       {C($"{maxEntry}ml", Green)}");
        Console.WriteLine($"  Min entry:       {C($"{minEntry}ml", Yellow)}");
        Console.WriteLine($"  Best day:        {C((bestDay.Key != null ? $"{bestDay.Key} ({bestDay.Value}ml)" : "—"), Green)}");
        Console.WriteLine($"  Daily goal:      {C($"{goal}ml", Cyan)}");
        Console.WriteLine(C(new string('═', 50), Dim));
    }

    private void SetGoal(int newGoal)
    {
        if (newGoal <= 0)
        {
            Console.WriteLine(C("❌ Goal must be positive!", Red));
            return;
        }
        if (newGoal > MaxAmount)
        {
            Console.WriteLine(C($"⚠️  That's an extreme goal! Max {MaxAmount}ml", Yellow));
            if (!AskConfirm("Set anyway?")) return;
        }
        goal = newGoal;
        SaveData();
        Console.WriteLine(C($"✅ Daily goal set to {goal}ml", Green));
    }

    private void ExportData(string? filepath = null)
    {
        if (string.IsNullOrEmpty(filepath))
        {
            string ts = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            filepath = $"water_counter_export_{ts}.json";
        }
        try
        {
            var data = new WaterData { Goal = goal, Entries = entries };
            string json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(filepath, json);
            Console.WriteLine(C($"✅ Data exported to: {filepath}", Green));
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Export failed: {ex.Message}", Red));
        }
    }

    private void ImportData(string filepath)
    {
        try
        {
            if (!File.Exists(filepath))
            {
                Console.WriteLine(C($"❌ File not found: {filepath}", Red));
                return;
            }
            string json = File.ReadAllText(filepath);
            var data = JsonSerializer.Deserialize<WaterData>(json);
            if (data == null || data.Goal <= 0 || data.Entries == null)
            {
                Console.WriteLine(C("❌ Invalid data format!", Red));
                return;
            }
            if (!AskConfirm("⚠️  This will overwrite current data! Continue?")) return;
            goal = data.Goal;
            entries = data.Entries;
            SaveData();
            Console.WriteLine(C($"✅ Imported {entries.Count} entries from: {filepath}", Green));
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Import failed: {ex.Message}", Red));
        }
    }

    private void ClearData()
    {
        if (entries.Count == 0 && goal == DefaultGoal)
        {
            Console.WriteLine(C("📭 Already empty.", Dim));
            return;
        }
        if (!AskConfirm("⚠️  Delete ALL data? This cannot be undone!")) return;
        entries.Clear();
        goal = DefaultGoal;
        SaveData();
        Console.WriteLine(C("🗑️  All data cleared.", Yellow));
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    private void ShowMenu()
    {
        int todayTotal = GetTodayTotal();
        string progress = GetProgressBar(todayTotal, goal);

        Console.WriteLine("\n" + C(new string('═', 50), Cyan));
        Console.WriteLine(C("💧 WATER COUNTER – Daily Hydration Tracker", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine($"  Today: {todayTotal}ml / {goal}ml  {progress}");
        Console.WriteLine(C(new string('─', 50), Dim));
        Console.WriteLine("  1. 💧 Add water intake");
        Console.WriteLine("  2. 📊 Show today's progress");
        Console.WriteLine($"  3. 📅 Show history ({HistoryDays} days)");
        Console.WriteLine("  4. 📈 Show statistics");
        Console.WriteLine($"  5. 🎯 Set daily goal (current: {goal}ml)");
        Console.WriteLine("  6. 📤 Import / Export data");
        Console.WriteLine("  7. 🗑️  Clear all data");
        Console.WriteLine("  0. 🚪 Exit");
        Console.WriteLine(C(new string('═', 50), Cyan));
    }

    private void HandleImportExport()
    {
        Console.WriteLine("\n📤 IMPORT / EXPORT");
        Console.WriteLine("  1. Export data");
        Console.WriteLine("  2. Import data");
        Console.WriteLine("  0. Back");
        string choice = Ask("Your choice: ");

        if (choice == "1")
        {
            ExportData();
        }
        else if (choice == "2")
        {
            string fp = Ask("Path to JSON file: ");
            if (!string.IsNullOrEmpty(fp)) ImportData(fp);
        }
    }

    public void Run()
    {
        Console.Clear();
        Console.WriteLine(C("\n💧 Water Counter – Daily Hydration Tracker", Bright + Cyan));
        Console.WriteLine(C("Stay hydrated, stay healthy!", Dim));

        while (true)
        {
            ShowMenu();
            string choice = Ask("Your choice: ");

            switch (choice)
            {
                case "1":
                    int amount = AskInt("Amount in ml: ");
                    AddEntry(amount);
                    break;
                case "2":
                    ShowToday();
                    break;
                case "3":
                    ShowHistory(HistoryDays);
                    break;
                case "4":
                    ShowStats();
                    break;
                case "5":
                    int newGoal = AskInt("New daily goal (ml): ");
                    SetGoal(newGoal);
                    break;
                case "6":
                    HandleImportExport();
                    break;
                case "7":
                    ClearData();
                    break;
                case "0":
                    Console.WriteLine(C("👋 Stay hydrated! Goodbye!", Cyan));
                    return;
                default:
                    Console.WriteLine(C("❌ Invalid choice.", Red));
                    break;
            }

            if (choice != "0")
            {
                Console.Write("\nPress Enter to continue...");
                Console.ReadLine();
            }
        }
    }

    // ─── Main ──────────────────────────────────────────────────────────────

    public static void Main(string[] args)
    {
        try
        {
            var app = new WaterCounter();
            app.Run();
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Unexpected error: {ex.Message}", Red));
            Environment.Exit(1);
        }
    }
}
