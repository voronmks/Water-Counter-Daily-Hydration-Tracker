# water_counter.rs
/**
 * 💧 Water Counter – Daily Hydration Tracker (Rust Edition)
 * Advanced: colorful terminal, JSON persistence, analytics, export/import
 */

use chrono::{DateTime, Local, NaiveDate};
use serde::{Deserialize, Serialize};
use std::fs;
use std::io::{self, Write, BufRead};
use std::path::PathBuf;

// ─── Types ──────────────────────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
struct WaterEntry {
    date: String,
    amount: u32,
    timestamp: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct WaterData {
    goal: u32,
    entries: Vec<WaterEntry>,
}

// ─── Constants ─────────────────────────────────────────────────────────────

const DEFAULT_GOAL: u32 = 2000;
const MAX_AMOUNT: u32 = 10000;
const HISTORY_DAYS: usize = 7;

// ─── Colors ──────────────────────────────────────────────────────────────────

fn c(text: &str, color: &str) -> String {
    format!("{}{}{}", color, text, "\x1b[0m")
}

const RESET: &str = "\x1b[0m";
const BRIGHT: &str = "\x1b[1m";
const DIM: &str = "\x1b[2m";
const RED: &str = "\x1b[31m";
const GREEN: &str = "\x1b[32m";
const YELLOW: &str = "\x1b[33m";
const BLUE: &str = "\x1b[34m";
const MAGENTA: &str = "\x1b[35m";
const CYAN: &str = "\x1b[36m";

// ─── Main App ──────────────────────────────────────────────────────────────

struct WaterCounter {
    goal: u32,
    entries: Vec<WaterEntry>,
    data_path: PathBuf,
}

impl WaterCounter {
    fn new() -> Result<Self, Box<dyn std::error::Error>> {
        let home = std::env::var("HOME").or_else(|_| std::env::var("USERPROFILE"))?;
        let mut data_dir = PathBuf::from(home);
        data_dir.push(".water_counter");
        fs::create_dir_all(&data_dir)?;
        let data_path = data_dir.join("data.json");

        let mut app = WaterCounter {
            goal: DEFAULT_GOAL,
            entries: Vec::new(),
            data_path,
        };
        app.load_data()?;
        Ok(app)
    }

    // ─── Data Persistence ──────────────────────────────────────────────────

    fn load_data(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        if !self.data_path.exists() {
            self.goal = DEFAULT_GOAL;
            self.entries = Vec::new();
            return Ok(());
        }

        let raw = fs::read_to_string(&self.data_path)?;
        match serde_json::from_str::<WaterData>(&raw) {
            Ok(data) => {
                self.goal = if data.goal > 0 { data.goal } else { DEFAULT_GOAL };
                self.entries = data.entries;
                Ok(())
            }
            Err(e) => {
                eprintln!("{}", c("⚠️  Error parsing data. Starting fresh.", YELLOW));
                self.goal = DEFAULT_GOAL;
                self.entries = Vec::new();
                Ok(())
            }
        }
    }

    fn save_data(&self) -> Result<(), Box<dyn std::error::Error>> {
        let data = WaterData {
            goal: self.goal,
            entries: self.entries.clone(),
        };
        let raw = serde_json::to_string_pretty(&data)?;
        let temp_path = self.data_path.with_extension("tmp");
        fs::write(&temp_path, raw)?;
        fs::rename(&temp_path, &self.data_path)?;
        Ok(())
    }

    // ─── Helpers ──────────────────────────────────────────────────────────

    fn today(&self) -> String {
        Local::now().format("%Y-%m-%d").to_string()
    }

    fn get_today_entries(&self) -> Vec<WaterEntry> {
        let today = self.today();
        self.entries
            .iter()
            .filter(|e| e.date == today)
            .cloned()
            .collect()
    }

    fn get_today_total(&self) -> u32 {
        self.get_today_entries().iter().map(|e| e.amount).sum()
    }

    fn get_progress_bar(&self, current: u32, goal: u32, width: usize) -> String {
        if goal == 0 {
            return "⚠️  Goal not set".to_string();
        }
        let ratio = (current as f64 / goal as f64).min(1.0);
        let filled = (ratio * width as f64) as usize;
        let bar = "█".repeat(filled) + &"░".repeat(width - filled);
        format!("{} {:.1}%", bar, ratio * 100.0)
    }

    fn ask(prompt: &str) -> String {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let stdin = io::stdin();
        let mut line = String::new();
        stdin.lock().read_line(&mut line).unwrap();
        line.trim().to_string()
    }

    fn ask_int(prompt: &str) -> Result<u32, Box<dyn std::error::Error>> {
        let answer = Self::ask(prompt);
        Ok(answer.parse()?)
    }

    fn ask_confirm(prompt: &str) -> bool {
        let answer = Self::ask(&format!("{} (yes/no): ", prompt));
        let a = answer.to_lowercase();
        a == "yes" || a == "y"
    }

    // ─── Core Actions ──────────────────────────────────────────────────────

    fn add_entry(&mut self, amount: u32) -> Result<(), Box<dyn std::error::Error>> {
        if amount == 0 {
            return Err("Amount must be positive!".into());
        }
        if amount > MAX_AMOUNT {
            eprintln!("{}", c(&format!("⚠️  That's a LOT of water! Max {}ml", MAX_AMOUNT), YELLOW));
            if !Self::ask_confirm("Continue?") {
                return Ok(());
            }
        }

        let date = self.today();
        let timestamp = Local::now().to_rfc3339();

        self.entries.push(WaterEntry {
            date,
            amount,
            timestamp,
        });
        self.save_data()?;

        let today_total = self.get_today_total();
        println!("{}", c(&format!("✅ Added {}ml (Total today: {}ml)", amount, today_total), GREEN));
        if today_total >= self.goal {
            println!("{}", c("🎉 Goal achieved! Stay hydrated! 💪", CYAN));
        }
        Ok(())
    }

    fn show_today(&self) {
        let today_total = self.get_today_total();
        let entries = self.get_today_entries();

        println!("\n{}", c(&"═".repeat(50), DIM));
        println!("{}", c("💧 TODAY'S HYDRATION", &format!("{}{}", BRIGHT, CYAN)));
        println!("{}", c(&"═".repeat(50), DIM));
        println!("  Goal:      {}", c(&format!("{}ml", self.goal), CYAN));
        println!("  Consumed:  {}", c(&format!("{}ml", today_total), GREEN));
        println!("  Remaining: {}", c(&format!("{}ml", self.goal.saturating_sub(today_total)), YELLOW));
        println!("  Progress:  {}", self.get_progress_bar(today_total, self.goal, 30));
        println!("{}", c(&"═".repeat(50), DIM));

        if entries.is_empty() {
            println!("{}", c("  No entries yet today. Drink up! 💧", DIM));
        } else {
            println!("  Entries:");
            for (i, e) in entries.iter().enumerate() {
                let ts = if e.timestamp.len() >= 16 {
                    &e.timestamp[11..16]
                } else {
                    "—"
                };
                println!("    {}. {} → {}", i + 1, ts, c(&format!("{}ml", e.amount), GREEN));
            }
        }
        println!("{}", c(&"═".repeat(50), DIM));
    }

    fn show_history(&self, days: usize) {
        let now = Local::now();
        let mut history = Vec::new();

        for i in 0..days {
            let d = now - chrono::Duration::days(i as i64);
            let date_str = d.format("%Y-%m-%d").to_string();
            let total: u32 = self.entries
                .iter()
                .filter(|e| e.date == date_str)
                .map(|e| e.amount)
                .sum();
            history.push((date_str, total));
        }

        println!("\n📅 LAST {} DAYS", days);
        println!("{}", c(&"─".repeat(40), DIM));
        for (date, total) in history {
            let status = if total >= self.goal {
                "✅"
            } else if total > 0 {
                "⏳"
            } else {
                "❌"
            };
            println!("  {}: {:>5}ml  {}", date, total, status);
        }
        println!("{}", c(&"─".repeat(40), DIM));
    }

    fn show_stats(&self) {
        if self.entries.is_empty() {
            println!("{}", c("📭 No data yet. Start tracking your water intake!", YELLOW));
            return;
        }

        let total: u32 = self.entries.iter().map(|e| e.amount).sum();
        let count = self.entries.len();
        let avg = total as f64 / count as f64;

        let max_entry = self.entries.iter().map(|e| e.amount).max().unwrap_or(0);
        let min_entry = self.entries.iter().map(|e| e.amount).min().unwrap_or(0);

        let mut unique_days = std::collections::HashSet::new();
        for e in &self.entries {
            unique_days.insert(&e.date);
        }
        let day_count = unique_days.len();

        // Best day
        let mut day_totals = std::collections::HashMap::new();
        for e in &self.entries {
            *day_totals.entry(&e.date).or_insert(0) += e.amount;
        }
        let mut best_day = "";
        let mut best_amount = 0;
        for (date, amt) in &day_totals {
            if *amt > best_amount {
                best_amount = *amt;
                best_day = date;
            }
        }

        println!("\n{}", c(&"═".repeat(50), DIM));
        println!("{}", c("📊 STATISTICS", &format!("{}{}", BRIGHT, MAGENTA)));
        println!("{}", c(&"═".repeat(50), DIM));
        println!("  Total consumed:  {}", c(&format!("{}ml", total), CYAN));
        println!("  Total entries:   {}", c(&format!("{}", count), CYAN));
        println!("  Days tracked:    {}", c(&format!("{}", day_count), CYAN));
        println!("  Average per day: {}", c(&format!("{:.1}ml", avg), CYAN));
        println!("  Max entry:       {}", c(&format!("{}ml", max_entry), GREEN));
        println!("  Min entry:       {}", c(&format!("{}ml", min_entry), YELLOW));
        let best_str = if best_day.is_empty() {
            "—".to_string()
        } else {
            format!("{} ({}ml)", best_day, best_amount)
        };
        println!("  Best day:        {}", c(&best_str, GREEN));
        println!("  Daily goal:      {}", c(&format!("{}ml", self.goal), CYAN));
        println!("{}", c(&"═".repeat(50), DIM));
    }

    fn set_goal(&mut self, goal: u32) -> Result<(), Box<dyn std::error::Error>> {
        if goal == 0 {
            return Err("Goal must be positive!".into());
        }
        if goal > MAX_AMOUNT {
            eprintln!("{}", c(&format!("⚠️  That's an extreme goal! Max {}ml", MAX_AMOUNT), YELLOW));
            if !Self::ask_confirm("Set anyway?") {
                return Ok(());
            }
        }
        self.goal = goal;
        self.save_data()?;
        println!("{}", c(&format!("✅ Daily goal set to {}ml", goal), GREEN));
        Ok(())
    }

    fn export_data(&self, filepath: Option<&str>) -> Result<(), Box<dyn std::error::Error>> {
        let fp = if let Some(p) = filepath {
            p.to_string()
        } else {
            let ts = Local::now().format("%Y%m%d_%H%M%S").to_string();
            format!("water_counter_export_{}.json", ts)
        };

        let data = WaterData {
            goal: self.goal,
            entries: self.entries.clone(),
        };
        let raw = serde_json::to_string_pretty(&data)?;
        fs::write(&fp, raw)?;
        println!("{}", c(&format!("✅ Data exported to: {}", fp), GREEN));
        Ok(())
    }

    fn import_data(&mut self, filepath: &str) -> Result<(), Box<dyn std::error::Error>> {
        if !std::path::Path::new(filepath).exists() {
            return Err(format!("File not found: {}", filepath).into());
        }
        let raw = fs::read_to_string(filepath)?;
        let data: WaterData = serde_json::from_str(&raw)?;
        if data.goal == 0 || data.entries.is_empty() {
            return Err("Invalid data format!".into());
        }
        if !Self::ask_confirm("⚠️  This will overwrite current data! Continue?") {
            return Ok(());
        }
        self.goal = data.goal;
        self.entries = data.entries;
        self.save_data()?;
        println!("{}", c(&format!("✅ Imported {} entries from: {}", self.entries.len(), filepath), GREEN));
        Ok(())
    }

    fn clear_data(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        if self.entries.is_empty() && self.goal == DEFAULT_GOAL {
            println!("{}", c("📭 Already empty.", DIM));
            return Ok(());
        }
        if !Self::ask_confirm("⚠️  Delete ALL data? This cannot be undone!") {
            return Ok(());
        }
        self.entries.clear();
        self.goal = DEFAULT_GOAL;
        self.save_data()?;
        println!("{}", c("🗑️  All data cleared.", YELLOW));
        Ok(())
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    fn show_menu(&self) {
        let today_total = self.get_today_total();
        let progress = self.get_progress_bar(today_total, self.goal, 30);

        println!("\n{}", c(&"═".repeat(50), CYAN));
        println!("{}", c("💧 WATER COUNTER – Daily Hydration Tracker", &format!("{}{}", BRIGHT, CYAN)));
        println!("{}", c(&"═".repeat(50), CYAN));
        println!("  Today: {}ml / {}ml  {}", today_total, self.goal, progress);
        println!("{}", c(&"─".repeat(50), DIM));
        println!("  1. 💧 Add water intake");
        println!("  2. 📊 Show today's progress");
        println!("  3. 📅 Show history ({} days)", HISTORY_DAYS);
        println!("  4. 📈 Show statistics");
        println!("  5. 🎯 Set daily goal (current: {}ml)", self.goal);
        println!("  6. 📤 Import / Export data");
        println!("  7. 🗑️  Clear all data");
        println!("  0. 🚪 Exit");
        println!("{}", c(&"═".repeat(50), CYAN));
    }

    fn handle_import_export(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        println!("\n📤 IMPORT / EXPORT");
        println!("  1. Export data");
        println!("  2. Import data");
        println!("  0. Back");
        let choice = Self::ask("Your choice: ");

        match choice.as_str() {
            "1" => self.export_data(None)?,
            "2" => {
                let fp = Self::ask("Path to JSON file: ");
                if !fp.is_empty() {
                    self.import_data(&fp)?;
                }
            }
            _ => {}
        }
        Ok(())
    }

    fn run(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        println!("{}", c("\n💧 Water Counter – Daily Hydration Tracker", &format!("{}{}", BRIGHT, CYAN)));
        println!("{}", c("Stay hydrated, stay healthy!", DIM));

        loop {
            self.show_menu();
            let choice = Self::ask("Your choice: ");

            match choice.as_str() {
                "1" => {
                    match Self::ask_int("Amount in ml: ") {
                        Ok(amount) => {
                            if let Err(e) = self.add_entry(amount) {
                                eprintln!("{}", c(&format!("❌ {}", e), RED));
                            }
                        }
                        Err(_) => eprintln!("{}", c("❌ Please enter a number.", RED)),
                    }
                }
                "2" => self.show_today(),
                "3" => self.show_history(HISTORY_DAYS),
                "4" => self.show_stats(),
                "5" => {
                    match Self::ask_int("New daily goal (ml): ") {
                        Ok(goal) => {
                            if let Err(e) = self.set_goal(goal) {
                                eprintln!("{}", c(&format!("❌ {}", e), RED));
                            }
                        }
                        Err(_) => eprintln!("{}", c("❌ Please enter a number.", RED)),
                    }
                }
                "6" => {
                    if let Err(e) = self.handle_import_export() {
                        eprintln!("{}", c(&format!("❌ {}", e), RED));
                    }
                }
                "7" => {
                    if let Err(e) = self.clear_data() {
                        eprintln!("{}", c(&format!("❌ {}", e), RED));
                    }
                }
                "0" => {
                    println!("{}", c("👋 Stay hydrated! Goodbye!", CYAN));
                    break;
                }
                _ => eprintln!("{}", c("❌ Invalid choice.", RED)),
            }

            if choice != "0" {
                print!("\nPress Enter to continue...");
                io::stdout().flush().unwrap();
                let mut _dummy = String::new();
                io::stdin().read_line(&mut _dummy).unwrap();
            }
        }

        Ok(())
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut app = WaterCounter::new()?;
    app.run()?;
    Ok(())
}
