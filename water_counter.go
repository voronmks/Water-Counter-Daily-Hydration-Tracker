# water_counter.go
/**
 * 💧 Water Counter – Daily Hydration Tracker (Go Edition)
 * Advanced: colored output, JSON persistence, analytics, export/import
 */

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"time"
)

// ─── Types ────────────────────────────────────────────────────────────────────

type WaterEntry struct {
	Date      string `json:"date"`
	Amount    int    `json:"amount"`
	Timestamp string `json:"timestamp"`
}

type WaterData struct {
	Goal    int           `json:"goal"`
	Entries []WaterEntry `json:"entries"`
}

// ─── Constants ────────────────────────────────────────────────────────────────

const (
	defaultGoal = 2000
	maxAmount   = 10000
	historyDays = 7
)

// ─── Colors ──────────────────────────────────────────────────────────────────

const (
	reset  = "\x1b[0m"
	bright = "\x1b[1m"
	dim    = "\x1b[2m"
	red    = "\x1b[31m"
	green  = "\x1b[32m"
	yellow = "\x1b[33m"
	blue   = "\x1b[34m"
	magenta = "\x1b[35m"
	cyan   = "\x1b[36m"
)

func c(str, color string) string {
	return color + str + reset
}

// ─── Main App ────────────────────────────────────────────────────────────────

type WaterCounter struct {
	goal      int
	entries   []WaterEntry
	dataPath  string
	reader    *bufio.Reader
}

func NewWaterCounter() *WaterCounter {
	home, err := os.UserHomeDir()
	if err != nil {
		panic(err)
	}
	dataDir := filepath.Join(home, ".water_counter")
	if err := os.MkdirAll(dataDir, 0755); err != nil {
		panic(err)
	}
	dataPath := filepath.Join(dataDir, "data.json")

	wc := &WaterCounter{
		dataPath: dataPath,
		reader:   bufio.NewReader(os.Stdin),
	}
	wc.loadData()
	return wc
}

// ─── Data Persistence ──────────────────────────────────────────────────────

func (w *WaterCounter) loadData() {
	if _, err := os.Stat(w.dataPath); os.IsNotExist(err) {
		w.goal = defaultGoal
		w.entries = []WaterEntry{}
		return
	}

	raw, err := ioutil.ReadFile(w.dataPath)
	if err != nil {
		fmt.Println(c("⚠️  Error loading data. Starting fresh.", yellow))
		w.goal = defaultGoal
		w.entries = []WaterEntry{}
		return
	}

	var data WaterData
	if err := json.Unmarshal(raw, &data); err != nil {
		fmt.Println(c("⚠️  Error parsing data. Starting fresh.", yellow))
		w.goal = defaultGoal
		w.entries = []WaterEntry{}
		return
	}

	w.goal = data.Goal
	if w.goal <= 0 {
		w.goal = defaultGoal
	}
	w.entries = data.Entries
	if w.entries == nil {
		w.entries = []WaterEntry{}
	}
}

func (w *WaterCounter) saveData() error {
	data := WaterData{
		Goal:    w.goal,
		Entries: w.entries,
	}
	raw, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return err
	}
	tempPath := w.dataPath + ".tmp"
	if err := ioutil.WriteFile(tempPath, raw, 0644); err != nil {
		return err
	}
	return os.Rename(tempPath, w.dataPath)
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

func (w *WaterCounter) today() string {
	return time.Now().Format("2006-01-02")
}

func (w *WaterCounter) getTodayEntries() []WaterEntry {
	today := w.today()
	var result []WaterEntry
	for _, e := range w.entries {
		if e.Date == today {
			result = append(result, e)
		}
	}
	return result
}

func (w *WaterCounter) getTodayTotal() int {
	total := 0
	for _, e := range w.getTodayEntries() {
		total += e.Amount
	}
	return total
}

func (w *WaterCounter) getProgressBar(current, goal, width int) string {
	if goal <= 0 {
		return "⚠️  Goal not set"
	}
	ratio := float64(current) / float64(goal)
	if ratio > 1.0 {
		ratio = 1.0
	}
	filled := int(ratio * float64(width))
	bar := strings.Repeat("█", filled) + strings.Repeat("░", width-filled)
	return fmt.Sprintf("%s %.1f%%", bar, ratio*100)
}

func (w *WaterCounter) ask(prompt string) string {
	fmt.Print(prompt)
	line, _ := w.reader.ReadString('\n')
	return strings.TrimSpace(line)
}

func (w *WaterCounter) askInt(prompt string) (int, error) {
	answer := w.ask(prompt)
	return strconv.Atoi(answer)
}

func (w *WaterCounter) askConfirm(prompt string) bool {
	answer := w.ask(prompt + " (yes/no): ")
	answer = strings.ToLower(answer)
	return answer == "yes" || answer == "y"
}

// ─── Core Actions ──────────────────────────────────────────────────────────

func (w *WaterCounter) AddEntry(amount int, date ...string) error {
	if amount <= 0 {
		return fmt.Errorf("amount must be positive")
	}
	if amount > maxAmount {
		fmt.Println(c(fmt.Sprintf("⚠️  That's a LOT of water! Max %dml", maxAmount), yellow))
		if !w.askConfirm("Continue?") {
			return nil
		}
	}

	entryDate := w.today()
	if len(date) > 0 && date[0] != "" {
		entryDate = date[0]
	}
	timestamp := time.Now().Format(time.RFC3339)

	w.entries = append(w.entries, WaterEntry{
		Date:      entryDate,
		Amount:    amount,
		Timestamp: timestamp,
	})
	if err := w.saveData(); err != nil {
		return err
	}

	todayTotal := w.getTodayTotal()
	fmt.Printf(c("✅ Added %dml (Total today: %dml)\n", green), amount, todayTotal)
	if todayTotal >= w.goal {
		fmt.Println(c("🎉 Goal achieved! Stay hydrated! 💪", cyan))
	}
	return nil
}

func (w *WaterCounter) ShowToday() {
	todayTotal := w.getTodayTotal()
	entries := w.getTodayEntries()

	fmt.Println("\n" + c(strings.Repeat("═", 50), dim))
	fmt.Println(c("💧 TODAY'S HYDRATION", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), dim))
	fmt.Printf("  Goal:      %s\n", c(fmt.Sprintf("%dml", w.goal), cyan))
	fmt.Printf("  Consumed:  %s\n", c(fmt.Sprintf("%dml", todayTotal), green))
	fmt.Printf("  Remaining: %s\n", c(fmt.Sprintf("%dml", max(w.goal-todayTotal, 0)), yellow))
	fmt.Printf("  Progress:  %s\n", w.getProgressBar(todayTotal, w.goal, 30))
	fmt.Println(c(strings.Repeat("═", 50), dim))

	if len(entries) > 0 {
		fmt.Println("  Entries:")
		for i, e := range entries {
			ts := "—"
			if len(e.Timestamp) >= 16 {
				ts = e.Timestamp[11:16]
			}
			fmt.Printf("    %d. %s → %s\n", i+1, ts, c(fmt.Sprintf("%dml", e.Amount), green))
		}
	} else {
		fmt.Println(c("  No entries yet today. Drink up! 💧", dim))
	}
	fmt.Println(c(strings.Repeat("═", 50), dim))
}

func (w *WaterCounter) ShowHistory(days ...int) {
	n := historyDays
	if len(days) > 0 && days[0] > 0 {
		n = days[0]
	}

	now := time.Now()
	history := make([]struct {
		Date  string
		Total int
	}, n)

	for i := 0; i < n; i++ {
		d := now.AddDate(0, 0, -i)
		dateStr := d.Format("2006-01-02")
		total := 0
		for _, e := range w.entries {
			if e.Date == dateStr {
				total += e.Amount
			}
		}
		history[i] = struct {
			Date  string
			Total int
		}{Date: dateStr, Total: total}
	}

	fmt.Printf("\n📅 LAST %d DAYS\n", n)
	fmt.Println(c(strings.Repeat("─", 40), dim))
	for _, h := range history {
		status := "❌"
		if h.Total >= w.goal {
			status = "✅"
		} else if h.Total > 0 {
			status = "⏳"
		}
		fmt.Printf("  %s: %5dml  %s\n", h.Date, h.Total, status)
	}
	fmt.Println(c(strings.Repeat("─", 40), dim))
}

func (w *WaterCounter) ShowStats() {
	if len(w.entries) == 0 {
		fmt.Println(c("📭 No data yet. Start tracking your water intake!", yellow))
		return
	}

	total := 0
	minEntry := int(^uint(0) >> 1)
	maxEntry := 0
	for _, e := range w.entries {
		total += e.Amount
		if e.Amount < minEntry {
			minEntry = e.Amount
		}
		if e.Amount > maxEntry {
			maxEntry = e.Amount
		}
	}
	count := len(w.entries)
	avg := float64(total) / float64(count)

	uniqueDays := make(map[string]bool)
	for _, e := range w.entries {
		uniqueDays[e.Date] = true
	}
	dayCount := len(uniqueDays)

	// Best day
	dayTotals := make(map[string]int)
	for _, e := range w.entries {
		dayTotals[e.Date] += e.Amount
	}
	bestDay := ""
	bestAmount := 0
	for date, amt := range dayTotals {
		if amt > bestAmount {
			bestAmount = amt
			bestDay = date
		}
	}

	fmt.Println("\n" + c(strings.Repeat("═", 50), dim))
	fmt.Println(c("📊 STATISTICS", bright+magenta))
	fmt.Println(c(strings.Repeat("═", 50), dim))
	fmt.Printf("  Total consumed:  %s\n", c(fmt.Sprintf("%dml", total), cyan))
	fmt.Printf("  Total entries:   %s\n", c(fmt.Sprintf("%d", count), cyan))
	fmt.Printf("  Days tracked:    %s\n", c(fmt.Sprintf("%d", dayCount), cyan))
	fmt.Printf("  Average per day: %s\n", c(fmt.Sprintf("%.1fml", avg), cyan))
	fmt.Printf("  Max entry:       %s\n", c(fmt.Sprintf("%dml", maxEntry), green))
	fmt.Printf("  Min entry:       %s\n", c(fmt.Sprintf("%dml", minEntry), yellow))
	bestStr := "—"
	if bestDay != "" {
		bestStr = fmt.Sprintf("%s (%dml)", bestDay, bestAmount)
	}
	fmt.Printf("  Best day:        %s\n", c(bestStr, green))
	fmt.Printf("  Daily goal:      %s\n", c(fmt.Sprintf("%dml", w.goal), cyan))
	fmt.Println(c(strings.Repeat("═", 50), dim))
}

func (w *WaterCounter) SetGoal(goal int) error {
	if goal <= 0 {
		return fmt.Errorf("goal must be positive")
	}
	if goal > maxAmount {
		fmt.Println(c(fmt.Sprintf("⚠️  That's an extreme goal! Max %dml", maxAmount), yellow))
		if !w.askConfirm("Set anyway?") {
			return nil
		}
	}
	w.goal = goal
	if err := w.saveData(); err != nil {
		return err
	}
	fmt.Printf(c("✅ Daily goal set to %dml\n", green), goal)
	return nil
}

func (w *WaterCounter) ExportData(filepath ...string) error {
	fp := ""
	if len(filepath) > 0 && filepath[0] != "" {
		fp = filepath[0]
	} else {
		ts := time.Now().Format("20060102_150405")
		fp = fmt.Sprintf("water_counter_export_%s.json", ts)
	}

	data := WaterData{Goal: w.goal, Entries: w.entries}
	raw, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return err
	}
	if err := ioutil.WriteFile(fp, raw, 0644); err != nil {
		return err
	}
	fmt.Printf(c("✅ Data exported to: %s\n", green), fp)
	return nil
}

func (w *WaterCounter) ImportData(filepath string) error {
	if _, err := os.Stat(filepath); os.IsNotExist(err) {
		return fmt.Errorf("file not found: %s", filepath)
	}
	raw, err := ioutil.ReadFile(filepath)
	if err != nil {
		return err
	}
	var data WaterData
	if err := json.Unmarshal(raw, &data); err != nil {
		return fmt.Errorf("invalid data format: %v", err)
	}
	if data.Goal <= 0 || data.Entries == nil {
		return fmt.Errorf("invalid data format")
	}
	if !w.askConfirm("⚠️  This will overwrite current data! Continue?") {
		return nil
	}
	w.goal = data.Goal
	w.entries = data.Entries
	if err := w.saveData(); err != nil {
		return err
	}
	fmt.Printf(c("✅ Imported %d entries from: %s\n", green), len(w.entries), filepath)
	return nil
}

func (w *WaterCounter) ClearData() error {
	if len(w.entries) == 0 && w.goal == defaultGoal {
		fmt.Println(c("📭 Already empty.", dim))
		return nil
	}
	if !w.askConfirm("⚠️  Delete ALL data? This cannot be undone!") {
		return nil
	}
	w.entries = []WaterEntry{}
	w.goal = defaultGoal
	if err := w.saveData(); err != nil {
		return err
	}
	fmt.Println(c("🗑️  All data cleared.", yellow))
	return nil
}

// ─── Menu ────────────────────────────────────────────────────────────────────

func (w *WaterCounter) showMenu() {
	todayTotal := w.getTodayTotal()
	progress := w.getProgressBar(todayTotal, w.goal, 30)

	fmt.Println("\n" + c(strings.Repeat("═", 50), cyan))
	fmt.Println(c("💧 WATER COUNTER – Daily Hydration Tracker", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Printf("  Today: %dml / %dml  %s\n", todayTotal, w.goal, progress)
	fmt.Println(c(strings.Repeat("─", 50), dim))
	fmt.Println("  1. 💧 Add water intake")
	fmt.Println("  2. 📊 Show today's progress")
	fmt.Printf("  3. 📅 Show history (%d days)\n", historyDays)
	fmt.Println("  4. 📈 Show statistics")
	fmt.Printf("  5. 🎯 Set daily goal (current: %dml)\n", w.goal)
	fmt.Println("  6. 📤 Import / Export data")
	fmt.Println("  7. 🗑️  Clear all data")
	fmt.Println("  0. 🚪 Exit")
	fmt.Println(c(strings.Repeat("═", 50), cyan))
}

func (w *WaterCounter) handleImportExport() {
	fmt.Println("\n📤 IMPORT / EXPORT")
	fmt.Println("  1. Export data")
	fmt.Println("  2. Import data")
	fmt.Println("  0. Back")
	choice := w.ask("Your choice: ")

	switch choice {
	case "1":
		if err := w.ExportData(); err != nil {
			fmt.Println(c("❌ Export failed: "+err.Error(), red))
		}
	case "2":
		fp := w.ask("Path to JSON file: ")
		if fp != "" {
			if err := w.ImportData(fp); err != nil {
				fmt.Println(c("❌ Import failed: "+err.Error(), red))
			}
		}
	}
}

func (w *WaterCounter) Run() {
	fmt.Print("\033[H\033[2J")
	fmt.Println(c("\n💧 Water Counter – Daily Hydration Tracker", bright+cyan))
	fmt.Println(c("Stay hydrated, stay healthy!", dim))

	for {
		w.showMenu()
		choice := w.ask("Your choice: ")

		switch choice {
		case "1":
			amount, err := w.askInt("Amount in ml: ")
			if err == nil {
				if err := w.AddEntry(amount); err != nil {
					fmt.Println(c("❌ "+err.Error(), red))
				}
			} else {
				fmt.Println(c("❌ Please enter a number.", red))
			}
		case "2":
			w.ShowToday()
		case "3":
			w.ShowHistory()
		case "4":
			w.ShowStats()
		case "5":
			goal, err := w.askInt("New daily goal (ml): ")
			if err == nil {
				if err := w.SetGoal(goal); err != nil {
					fmt.Println(c("❌ "+err.Error(), red))
				}
			} else {
				fmt.Println(c("❌ Please enter a number.", red))
			}
		case "6":
			w.handleImportExport()
		case "7":
			if err := w.ClearData(); err != nil {
				fmt.Println(c("❌ "+err.Error(), red))
			}
		case "0":
			fmt.Println(c("👋 Stay hydrated! Goodbye!", cyan))
			return
		default:
			fmt.Println(c("❌ Invalid choice.", red))
		}

		if choice != "0" {
			fmt.Print("\nPress Enter to continue...")
			w.reader.ReadString('\n')
		}
	}
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

// ─── Main ─────────────────────────────────────────────────────────────────────

func main() {
	app := NewWaterCounter()
	app.Run()
}
