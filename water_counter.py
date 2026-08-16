# water_counter.py
#!/usr/bin/env python3
"""
💧 Water Counter – Daily Hydration Tracker (Python Edition)
Advanced features: colored output, progress bar, data analytics, export/import
"""

import json
import os
import sys
from datetime import datetime, timedelta
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import hashlib
import shutil

try:
    from rich.console import Console
    from rich.table import Table
    from rich.progress import Progress, BarColumn, TextColumn, TimeRemainingColumn
    from rich.panel import Panel
    from rich.prompt import Prompt, IntPrompt, FloatPrompt, Confirm
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("⚠️  Install 'rich' for enhanced UI: pip install rich")


class WaterCounter:
    """Main application class for tracking daily water intake."""
    
    DATA_DIR = Path.home() / ".water_counter"
    DATA_FILE = DATA_DIR / "data.json"
    DEFAULT_GOAL = 2000  # ml per day
    
    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
        self.data: Dict = self._load_data()
        self.goal = self.data.get("goal", self.DEFAULT_GOAL)
        self.entries: List[Dict] = self.data.get("entries", [])
        
    def _load_data(self) -> Dict:
        """Load data from JSON file, create if missing."""
        if self.DATA_FILE.exists():
            try:
                with open(self.DATA_FILE, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except (json.JSONDecodeError, OSError) as e:
                print(f"⚠️  Error loading data: {e}. Starting fresh.")
                return {"goal": self.DEFAULT_GOAL, "entries": []}
        return {"goal": self.DEFAULT_GOAL, "entries": []}
    
    def _save_data(self) -> None:
        """Save data to JSON file with atomic write."""
        self.DATA_DIR.mkdir(parents=True, exist_ok=True)
        temp_file = self.DATA_FILE.with_suffix(".tmp")
        try:
            with open(temp_file, 'w', encoding='utf-8') as f:
                json.dump({"goal": self.goal, "entries": self.entries}, f, 
                          indent=2, ensure_ascii=False)
            temp_file.replace(self.DATA_FILE)
        except OSError as e:
            print(f"❌ Failed to save data: {e}")
            sys.exit(1)
    
    def _today(self) -> str:
        """Return today's date as string."""
        return datetime.now().strftime("%Y-%m-%d")
    
    def _get_today_entries(self) -> List[Dict]:
        """Return entries for today."""
        today = self._today()
        return [e for e in self.entries if e.get("date") == today]
    
    def _get_today_total(self) -> int:
        """Return total water consumed today in ml."""
        return sum(e.get("amount", 0) for e in self._get_today_entries())
    
    def _get_progress_bar(self, current: int, goal: int, width: int = 30) -> str:
        """Generate a text progress bar."""
        if goal <= 0:
            return "⚠️  Goal not set"
        ratio = min(current / goal, 1.0)
        filled = int(ratio * width)
        bar = "█" * filled + "░" * (width - filled)
        pct = ratio * 100
        return f"[{bar}] {pct:.1f}%"
    
    def add_entry(self, amount: int, date: Optional[str] = None) -> None:
        """Add a water intake entry."""
        if amount <= 0:
            print("❌ Amount must be positive!")
            return
        if amount > 10000:
            print("⚠️  That's a LOT of water! Are you sure? (max 10000ml)")
            if not Confirm.ask("Continue?", default=False) if self.console else False:
                return
        
        entry_date = date or self._today()
        timestamp = datetime.now().isoformat()
        self.entries.append({
            "date": entry_date,
            "amount": amount,
            "timestamp": timestamp
        })
        self._save_data()
        
        today_total = self._get_today_total()
        if self.console and RICH_AVAILABLE:
            self.console.print(f"✅ [green]Added {amount}ml[/green] (Total today: {today_total}ml)")
            if today_total >= self.goal:
                self.console.print("🎉 [bold cyan]Goal achieved! Stay hydrated! 💪[/bold cyan]")
        else:
            print(f"✅ Added {amount}ml (Total today: {today_total}ml)")
            if today_total >= self.goal:
                print("🎉 Goal achieved! Stay hydrated! 💪")
    
    def show_today(self) -> None:
        """Display today's progress."""
        today_total = self._get_today_total()
        entries = self._get_today_entries()
        
        if self.console and RICH_AVAILABLE:
            panel = Panel(
                f"[bold]💧 Today's Hydration[/bold]\n"
                f"  Goal: {self.goal}ml\n"
                f"  Consumed: {today_total}ml\n"
                f"  Remaining: {max(self.goal - today_total, 0)}ml\n"
                f"  {self._get_progress_bar(today_total, self.goal)}",
                title="📊 Daily Progress",
                border_style="cyan"
            )
            self.console.print(panel)
            
            if entries:
                table = Table(title="Today's Entries", box=box.ROUNDED)
                table.add_column("#", style="dim")
                table.add_column("Time", style="cyan")
                table.add_column("Amount", style="green", justify="right")
                for i, e in enumerate(entries, 1):
                    ts = e.get("timestamp", e.get("date", ""))[11:16] if "T" in e.get("timestamp", "") else "—"
                    table.add_row(str(i), ts, f"{e['amount']}ml")
                self.console.print(table)
            else:
                self.console.print("[dim]No entries yet today. Drink up! 💧[/dim]")
        else:
            print("\n" + "=" * 50)
            print("💧 TODAY'S HYDRATION")
            print("=" * 50)
            print(f"  Goal:      {self.goal}ml")
            print(f"  Consumed:  {today_total}ml")
            print(f"  Remaining: {max(self.goal - today_total, 0)}ml")
            print(f"  Progress:  {self._get_progress_bar(today_total, self.goal)}")
            print("=" * 50)
            if entries:
                print("  Entries:")
                for i, e in enumerate(entries, 1):
                    ts = e.get("timestamp", e.get("date", ""))[11:16] if "T" in e.get("timestamp", "") else "—"
                    print(f"    {i}. {ts} → {e['amount']}ml")
            else:
                print("  No entries yet today.")
            print("=" * 50)
    
    def show_history(self, days: int = 7) -> None:
        """Display history for the last N days."""
        today = datetime.now().date()
        history = []
        
        for i in range(days):
            date_str = (today - timedelta(days=i)).strftime("%Y-%m-%d")
            daily_total = sum(e.get("amount", 0) for e in self.entries if e.get("date") == date_str)
            history.append((date_str, daily_total))
        
        if self.console and RICH_AVAILABLE:
            table = Table(title=f"📅 Last {days} Days", box=box.ROUNDED)
            table.add_column("Date", style="cyan")
            table.add_column("Amount", style="green", justify="right")
            table.add_column("Status", justify="center")
            
            for date_str, total in history:
                status = "✅" if total >= self.goal else ("⏳" if total > 0 else "❌")
                table.add_row(date_str, f"{total}ml", status)
            self.console.print(table)
        else:
            print(f"\n📅 LAST {days} DAYS")
            print("-" * 40)
            for date_str, total in history:
                status = "✅" if total >= self.goal else ("⏳" if total > 0 else "❌")
                print(f"  {date_str}: {total:>5}ml  {status}")
            print("-" * 40)
    
    def show_stats(self) -> None:
        """Display detailed statistics."""
        if not self.entries:
            print("📭 No data yet. Start tracking your water intake!")
            return
        
        total = sum(e.get("amount", 0) for e in self.entries)
        count = len(self.entries)
        avg = total / count if count > 0 else 0
        max_entry = max((e.get("amount", 0) for e in self.entries), default=0)
        min_entry = min((e.get("amount", 0) for e in self.entries), default=0)
        
        # Unique days
        unique_days = len(set(e.get("date") for e in self.entries))
        
        # Best day
        day_totals = {}
        for e in self.entries:
            date = e.get("date")
            day_totals[date] = day_totals.get(date, 0) + e.get("amount", 0)
        best_day = max(day_totals.items(), key=lambda x: x[1]) if day_totals else (None, 0)
        
        if self.console and RICH_AVAILABLE:
            panel = Panel(
                f"[bold]📊 Water Consumption Statistics[/bold]\n\n"
                f"  Total consumed:  [cyan]{total}ml[/cyan]\n"
                f"  Total entries:   [cyan]{count}[/cyan]\n"
                f"  Days tracked:    [cyan]{unique_days}[/cyan]\n"
                f"  Average per day: [cyan]{avg:.1f}ml[/cyan]\n"
                f"  Average per entry: {avg:.1f}ml\n"
                f"  Max entry:       [green]{max_entry}ml[/green]\n"
                f"  Min entry:       [yellow]{min_entry}ml[/yellow]\n"
                f"  Best day:        [bold green]{best_day[0] if best_day[0] else '—'} ({best_day[1]}ml)[/bold green]\n"
                f"  Daily goal:      {self.goal}ml",
                title="📈 Statistics",
                border_style="magenta"
            )
            self.console.print(panel)
        else:
            print("\n" + "=" * 50)
            print("📊 STATISTICS")
            print("=" * 50)
            print(f"  Total consumed:  {total}ml")
            print(f"  Total entries:   {count}")
            print(f"  Days tracked:    {unique_days}")
            print(f"  Average per day: {avg:.1f}ml")
            print(f"  Max entry:       {max_entry}ml")
            print(f"  Min entry:       {min_entry}ml")
            print(f"  Best day:        {best_day[0] if best_day[0] else '—'} ({best_day[1]}ml)")
            print(f"  Daily goal:      {self.goal}ml")
            print("=" * 50)
    
    def set_goal(self, goal: int) -> None:
        """Set the daily hydration goal."""
        if goal <= 0:
            print("❌ Goal must be positive!")
            return
        if goal > 10000:
            print("⚠️  That's an extreme goal! Max 10000ml")
            if not (self.console and RICH_AVAILABLE and Confirm.ask("Set anyway?", default=False)):
                return
        self.goal = goal
        self._save_data()
        print(f"✅ Daily goal set to {goal}ml")
    
    def export_data(self, filepath: Optional[Path] = None) -> None:
        """Export data to a JSON file."""
        if not filepath:
            filepath = Path.cwd() / f"water_counter_export_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump({"goal": self.goal, "entries": self.entries}, f, 
                          indent=2, ensure_ascii=False)
            print(f"✅ Data exported to: {filepath}")
        except OSError as e:
            print(f"❌ Export failed: {e}")
    
    def import_data(self, filepath: Path) -> None:
        """Import data from a JSON file."""
        if not filepath.exists():
            print(f"❌ File not found: {filepath}")
            return
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                data = json.load(f)
            if "goal" in data and "entries" in data:
                if self.console and RICH_AVAILABLE and not Confirm.ask(
                    f"⚠️  This will overwrite current data! Continue?", default=False
                ):
                    return
                self.goal = data["goal"]
                self.entries = data["entries"]
                self._save_data()
                print(f"✅ Imported {len(self.entries)} entries from: {filepath}")
            else:
                print("❌ Invalid data format!")
        except (json.JSONDecodeError, OSError) as e:
            print(f"❌ Import failed: {e}")
    
    def clear_data(self) -> None:
        """Clear all data with confirmation."""
        if not self.entries and self.goal == self.DEFAULT_GOAL:
            print("📭 Already empty.")
            return
        
        if self.console and RICH_AVAILABLE:
            if not Confirm.ask("⚠️  Delete ALL data? This cannot be undone!", default=False):
                return
        else:
            resp = input("⚠️  Delete ALL data? (yes/no): ").strip().lower()
            if resp != "yes":
                return
        
        self.entries = []
        self.goal = self.DEFAULT_GOAL
        self._save_data()
        print("🗑️  All data cleared.")
    
    def run(self) -> None:
        """Main interactive loop."""
        if self.console and RICH_AVAILABLE:
            self.console.print(Panel.fit(
                "[bold cyan]💧 Water Counter – Daily Hydration Tracker[/bold cyan]\n"
                "[dim]Stay hydrated, stay healthy![/dim]",
                border_style="cyan"
            ))
        else:
            print("\n" + "=" * 50)
            print("💧 WATER COUNTER – Daily Hydration Tracker")
            print("=" * 50 + "\n")
        
        while True:
            self._show_menu()
            choice = self._get_choice()
            
            if choice == "1":
                amount = self._get_amount()
                if amount:
                    self.add_entry(amount)
            elif choice == "2":
                self.show_today()
            elif choice == "3":
                self.show_history()
            elif choice == "4":
                self.show_stats()
            elif choice == "5":
                goal = self._get_goal()
                if goal:
                    self.set_goal(goal)
            elif choice == "6":
                self._handle_export_import()
            elif choice == "7":
                self.clear_data()
            elif choice == "0":
                print("👋 Stay hydrated! Goodbye!")
                break
            else:
                print("❌ Invalid choice. Please try again.")
            
            if self.console and RICH_AVAILABLE:
                self.console.print("\n[dim]Press Enter to continue...[/dim]")
                input()
    
    def _show_menu(self) -> None:
        """Display the main menu."""
        today_total = self._get_today_total()
        progress = self._get_progress_bar(today_total, self.goal)
        
        if self.console and RICH_AVAILABLE:
            menu = f"""
[bold cyan]💧 Main Menu[/bold cyan]
  Today: {today_total}ml / {self.goal}ml  {progress}

  [1] 💧 Add water intake
  [2] 📊 Show today's progress
  [3] 📅 Show history (7 days)
  [4] 📈 Show statistics
  [5] 🎯 Set daily goal (current: {self.goal}ml)
  [6] 📤 Import / Export data
  [7] 🗑️  Clear all data
  [0] 🚪 Exit
"""
            self.console.print(Panel(menu, border_style="blue"))
        else:
            print("\n" + "-" * 50)
            print(f"💧 Today: {today_total}ml / {self.goal}ml  {progress}")
            print("-" * 50)
            print("  1. 💧 Add water intake")
            print("  2. 📊 Show today's progress")
            print("  3. 📅 Show history (7 days)")
            print("  4. 📈 Show statistics")
            print(f"  5. 🎯 Set daily goal (current: {self.goal}ml)")
            print("  6. 📤 Import / Export data")
            print("  7. 🗑️  Clear all data")
            print("  0. 🚪 Exit")
            print("-" * 50)
    
    def _get_choice(self) -> str:
        """Get user menu choice."""
        if self.console and RICH_AVAILABLE:
            return Prompt.ask("[bold]Your choice[/bold]", choices=["0","1","2","3","4","5","6","7"])
        return input("Your choice: ").strip()
    
    def _get_amount(self) -> Optional[int]:
        """Get water amount from user."""
        if self.console and RICH_AVAILABLE:
            return IntPrompt.ask("[bold cyan]Amount in ml[/bold cyan]")
        try:
            return int(input("Amount in ml: ").strip())
        except ValueError:
            print("❌ Please enter a number.")
            return None
    
    def _get_goal(self) -> Optional[int]:
        """Get goal amount from user."""
        if self.console and RICH_AVAILABLE:
            return IntPrompt.ask("[bold yellow]New daily goal in ml[/bold yellow]")
        try:
            return int(input("New daily goal (ml): ").strip())
        except ValueError:
            print("❌ Please enter a number.")
            return None
    
    def _handle_export_import(self) -> None:
        """Handle export/import submenu."""
        if self.console and RICH_AVAILABLE:
            self.console.print("[bold]📤 Import / Export[/bold]")
            self.console.print("  [1] Export data")
            self.console.print("  [2] Import data")
            self.console.print("  [0] Back")
            choice = Prompt.ask("Your choice", choices=["0","1","2"])
        else:
            print("\n📤 IMPORT / EXPORT")
            print("  1. Export data")
            print("  2. Import data")
            print("  0. Back")
            choice = input("Your choice: ").strip()
        
        if choice == "1":
            self.export_data()
        elif choice == "2":
            if self.console and RICH_AVAILABLE:
                path = Prompt.ask("Path to JSON file")
            else:
                path = input("Path to JSON file: ").strip()
            if path:
                self.import_data(Path(path))
        elif choice == "0":
            return


def main():
    """Entry point."""
    try:
        app = WaterCounter()
        app.run()
    except KeyboardInterrupt:
        print("\n👋 Goodbye! Stay hydrated!")
        sys.exit(0)
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
