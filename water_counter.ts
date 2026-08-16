# water_counter.ts
/**
 * 💧 Water Counter – Daily Hydration Tracker (TypeScript Edition)
 * Fully typed, with advanced features: analytics, export/import, colorful CLI
 */

import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';
import * as readline from 'readline';

// ─── Types ────────────────────────────────────────────────────────────────────

interface WaterEntry {
  date: string;
  amount: number;
  timestamp: string;
}

interface WaterData {
  goal: number;
  entries: WaterEntry[];
}

interface Config {
  dataDir: string;
  dataFile: string;
  defaultGoal: number;
  maxAmount: number;
  historyDays: number;
}

// ─── Constants ──────────────────────────────────────────────────────────────

const CONFIG: Config = {
  dataDir: path.join(os.homedir(), '.water_counter'),
  dataFile: 'data.json',
  defaultGoal: 2000,
  maxAmount: 10000,
  historyDays: 7,
};

// ─── Colors ──────────────────────────────────────────────────────────────────

const colors = {
  reset: '\x1b[0m',
  bright: '\x1b[1m',
  dim: '\x1b[2m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  magenta: '\x1b[35m',
  cyan: '\x1b[36m',
  white: '\x1b[37m',
};

const c = (str: string, color: string): string => `${color}${str}${colors.reset}`;

// ─── Main Class ─────────────────────────────────────────────────────────────

class WaterCounter {
  private rl: readline.Interface;
  private goal: number;
  private entries: WaterEntry[];
  private dataPath: string;

  constructor() {
    this.rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
    });
    this.dataPath = path.join(CONFIG.dataDir, CONFIG.dataFile);
    const data = this._loadData();
    this.goal = data.goal || CONFIG.defaultGoal;
    this.entries = data.entries || [];
  }

  // ─── Data Persistence ─────────────────────────────────────────────────────

  private _ensureDataDir(): void {
    if (!fs.existsSync(CONFIG.dataDir)) {
      fs.mkdirSync(CONFIG.dataDir, { recursive: true });
    }
  }

  private _loadData(): WaterData {
    this._ensureDataDir();
    if (fs.existsSync(this.dataPath)) {
      try {
        const raw = fs.readFileSync(this.dataPath, 'utf8');
        return JSON.parse(raw) as WaterData;
      } catch (_) {
        console.log(c('⚠️  Error loading data. Starting fresh.', colors.yellow));
      }
    }
    return { goal: CONFIG.defaultGoal, entries: [] };
  }

  private _saveData(): void {
    this._ensureDataDir();
    const tempPath = this.dataPath + '.tmp';
    try {
      fs.writeFileSync(
        tempPath,
        JSON.stringify({ goal: this.goal, entries: this.entries }, null, 2)
      );
      fs.renameSync(tempPath, this.dataPath);
    } catch (err: any) {
      console.log(c(`❌ Failed to save: ${err.message}`, colors.red));
      process.exit(1);
    }
  }

  // ─── Helpers ──────────────────────────────────────────────────────────────

  private _today(): string {
    return new Date().toISOString().split('T')[0];
  }

  private _getTodayEntries(): WaterEntry[] {
    const today = this._today();
    return this.entries.filter(e => e.date === today);
  }

  private _getTodayTotal(): number {
    return this._getTodayEntries().reduce((sum, e) => sum + e.amount, 0);
  }

  private _getProgressBar(current: number, goal: number, width: number = 30): string {
    if (goal <= 0) return '⚠️  Goal not set';
    const ratio = Math.min(current / goal, 1.0);
    const filled = Math.floor(ratio * width);
    const bar = '█'.repeat(filled) + '░'.repeat(width - filled);
    return `${bar} ${(ratio * 100).toFixed(1)}%`;
  }

  private _ask(question: string): Promise<string> {
    return new Promise(resolve => {
      this.rl.question(question, resolve);
    });
  }

  private async _askInt(question: string): Promise<number | null> {
    const answer = await this._ask(question);
    const num = parseInt(answer.trim());
    return isNaN(num) ? null : num;
  }

  private async _askConfirm(question: string): Promise<boolean> {
    const answer = await this._ask(`${question} (yes/no): `);
    const t = answer.trim().toLowerCase();
    return t === 'yes' || t === 'y';
  }

  // ─── Core Actions ─────────────────────────────────────────────────────────

  async addEntry(amount: number, date?: string): Promise<boolean> {
    if (amount <= 0) {
      console.log(c('❌ Amount must be positive!', colors.red));
      return false;
    }
    if (amount > CONFIG.maxAmount) {
      console.log(c(`⚠️  That's a LOT of water! Max ${CONFIG.maxAmount}ml`, colors.yellow));
      const ok = await this._askConfirm('Continue?');
      if (!ok) return false;
    }

    const entryDate = date || this._today();
    const timestamp = new Date().toISOString();
    this.entries.push({ date: entryDate, amount, timestamp });
    this._saveData();

    const todayTotal = this._getTodayTotal();
    console.log(c(`✅ Added ${amount}ml (Total today: ${todayTotal}ml)`, colors.green));
    if (todayTotal >= this.goal) {
      console.log(c('🎉 Goal achieved! Stay hydrated! 💪', colors.cyan));
    }
    return true;
  }

  showToday(): void {
    const todayTotal = this._getTodayTotal();
    const entries = this._getTodayEntries();

    console.log('\n' + c('═'.repeat(50), colors.dim));
    console.log(c('💧 TODAY\'S HYDRATION', colors.bright + colors.cyan));
    console.log(c('═'.repeat(50), colors.dim));
    console.log(`  Goal:      ${c(this.goal + 'ml', colors.cyan)}`);
    console.log(`  Consumed:  ${c(todayTotal + 'ml', colors.green)}`);
    console.log(`  Remaining: ${c(Math.max(this.goal - todayTotal, 0) + 'ml', colors.yellow)}`);
    console.log(`  Progress:  ${this._getProgressBar(todayTotal, this.goal)}`);
    console.log(c('═'.repeat(50), colors.dim));

    if (entries.length > 0) {
      console.log('  Entries:');
      entries.forEach((e, i) => {
        const ts = e.timestamp ? e.timestamp.slice(11, 16) : '—';
        console.log(`    ${i + 1}. ${ts} → ${c(e.amount + 'ml', colors.green)}`);
      });
    } else {
      console.log(c('  No entries yet today. Drink up! 💧', colors.dim));
    }
    console.log(c('═'.repeat(50), colors.dim));
  }

  showHistory(days: number = CONFIG.historyDays): void {
    const today = new Date();
    const history: { date: string; total: number }[] = [];

    for (let i = 0; i < days; i++) {
      const d = new Date(today);
      d.setDate(d.getDate() - i);
      const dateStr = d.toISOString().split('T')[0];
      const total = this.entries
        .filter(e => e.date === dateStr)
        .reduce((sum, e) => sum + e.amount, 0);
      history.push({ date: dateStr, total });
    }

    console.log(`\n📅 LAST ${days} DAYS`);
    console.log(c('─'.repeat(40), colors.dim));
    history.forEach(({ date, total }) => {
      const status = total >= this.goal ? '✅' : total > 0 ? '⏳' : '❌';
      console.log(`  ${date}: ${String(total).padStart(5)}ml  ${status}`);
    });
    console.log(c('─'.repeat(40), colors.dim));
  }

  showStats(): void {
    if (this.entries.length === 0) {
      console.log(c('📭 No data yet. Start tracking your water intake!', colors.yellow));
      return;
    }

    const total = this.entries.reduce((s, e) => s + e.amount, 0);
    const count = this.entries.length;
    const avg = total / count;
    const maxEntry = Math.max(...this.entries.map(e => e.amount));
    const minEntry = Math.min(...this.entries.map(e => e.amount));

    const uniqueDays = new Set(this.entries.map(e => e.date)).size;

    const dayTotals: Record<string, number> = {};
    this.entries.forEach(e => {
      dayTotals[e.date] = (dayTotals[e.date] || 0) + e.amount;
    });
    let bestDay: string | null = null;
    let bestAmount = 0;
    for (const [date, amt] of Object.entries(dayTotals)) {
      if (amt > bestAmount) {
        bestAmount = amt;
        bestDay = date;
      }
    }

    console.log('\n' + c('═'.repeat(50), colors.dim));
    console.log(c('📊 STATISTICS', colors.bright + colors.magenta));
    console.log(c('═'.repeat(50), colors.dim));
    console.log(`  Total consumed:  ${c(total + 'ml', colors.cyan)}`);
    console.log(`  Total entries:   ${c(count, colors.cyan)}`);
    console.log(`  Days tracked:    ${c(uniqueDays, colors.cyan)}`);
    console.log(`  Average per day: ${c(avg.toFixed(1) + 'ml', colors.cyan)}`);
    console.log(`  Max entry:       ${c(maxEntry + 'ml', colors.green)}`);
    console.log(`  Min entry:       ${c(minEntry + 'ml', colors.yellow)}`);
    console.log(`  Best day:        ${c(bestDay ? `${bestDay} (${bestAmount}ml)` : '—', colors.green)}`);
    console.log(`  Daily goal:      ${c(this.goal + 'ml', colors.cyan)}`);
    console.log(c('═'.repeat(50), colors.dim));
  }

  async setGoal(goal: number): Promise<void> {
    if (goal <= 0) {
      console.log(c('❌ Goal must be positive!', colors.red));
      return;
    }
    if (goal > CONFIG.maxAmount) {
      console.log(c(`⚠️  That's an extreme goal! Max ${CONFIG.maxAmount}ml`, colors.yellow));
      const ok = await this._askConfirm('Set anyway?');
      if (!ok) return;
    }
    this.goal = goal;
    this._saveData();
    console.log(c(`✅ Daily goal set to ${goal}ml`, colors.green));
  }

  async exportData(filepath?: string): Promise<void> {
    if (!filepath) {
      const ts = new Date().toISOString().replace(/[:.]/g, '').slice(0, 14);
      filepath = path.join(process.cwd(), `water_counter_export_${ts}.json`);
    }
    try {
      fs.writeFileSync(filepath, JSON.stringify(
        { goal: this.goal, entries: this.entries },
        null,
        2
      ));
      console.log(c(`✅ Data exported to: ${filepath}`, colors.green));
    } catch (err: any) {
      console.log(c(`❌ Export failed: ${err.message}`, colors.red));
    }
  }

  async importData(filepath: string): Promise<void> {
    if (!fs.existsSync(filepath)) {
      console.log(c(`❌ File not found: ${filepath}`, colors.red));
      return;
    }
    try {
      const raw = fs.readFileSync(filepath, 'utf8');
      const data = JSON.parse(raw) as WaterData;
      if (data.goal === undefined || !data.entries) {
        console.log(c('❌ Invalid data format!', colors.red));
        return;
      }
      const ok = await this._askConfirm('⚠️  This will overwrite current data! Continue?');
      if (!ok) return;
      this.goal = data.goal;
      this.entries = data.entries;
      this._saveData();
      console.log(c(`✅ Imported ${this.entries.length} entries from: ${filepath}`, colors.green));
    } catch (err: any) {
      console.log(c(`❌ Import failed: ${err.message}`, colors.red));
    }
  }

  async clearData(): Promise<void> {
    if (this.entries.length === 0 && this.goal === CONFIG.defaultGoal) {
      console.log(c('📭 Already empty.', colors.dim));
      return;
    }
    const ok = await this._askConfirm('⚠️  Delete ALL data? This cannot be undone!');
    if (!ok) return;
    this.entries = [];
    this.goal = CONFIG.defaultGoal;
    this._saveData();
    console.log(c('🗑️  All data cleared.', colors.yellow));
  }

  // ─── Menu ──────────────────────────────────────────────────────────────────

  private async _showMenu(): Promise<void> {
    const todayTotal = this._getTodayTotal();
    const progress = this._getProgressBar(todayTotal, this.goal);

    console.log('\n' + c('═'.repeat(50), colors.cyan));
    console.log(c('💧 WATER COUNTER – Daily Hydration Tracker', colors.bright + colors.cyan));
    console.log(c('═'.repeat(50), colors.cyan));
    console.log(`  Today: ${todayTotal}ml / ${this.goal}ml  ${progress}`);
    console.log(c('─'.repeat(50), colors.dim));
    console.log('  1. 💧 Add water intake');
    console.log('  2. 📊 Show today\'s progress');
    console.log(`  3. 📅 Show history (${CONFIG.historyDays} days)`);
    console.log('  4. 📈 Show statistics');
    console.log(`  5. 🎯 Set daily goal (current: ${this.goal}ml)`);
    console.log('  6. 📤 Import / Export data');
    console.log('  7. 🗑️  Clear all data');
    console.log('  0. 🚪 Exit');
    console.log(c('═'.repeat(50), colors.cyan));
  }

  private async _handleImportExport(): Promise<void> {
    console.log('\n📤 IMPORT / EXPORT');
    console.log('  1. Export data');
    console.log('  2. Import data');
    console.log('  0. Back');
    const choice = await this._ask('Your choice: ');

    if (choice.trim() === '1') {
      await this.exportData();
    } else if (choice.trim() === '2') {
      const filepath = await this._ask('Path to JSON file: ');
      if (filepath.trim()) {
        await this.importData(filepath.trim());
      }
    }
  }

  async run(): Promise<void> {
    console.clear();
    console.log(c('\n💧 Water Counter – Daily Hydration Tracker', colors.bright + colors.cyan));
    console.log(c('Stay hydrated, stay healthy!', colors.dim));

    while (true) {
      await this._showMenu();
      const choice = (await this._ask('Your choice: ')).trim();

      switch (choice) {
        case '1': {
          const amount = await this._askInt('Amount in ml: ');
          if (amount !== null) await this.addEntry(amount);
          break;
        }
        case '2': this.showToday(); break;
        case '3': this.showHistory(); break;
        case '4': this.showStats(); break;
        case '5': {
          const goal = await this._askInt('New daily goal (ml): ');
          if (goal !== null) await this.setGoal(goal);
          break;
        }
        case '6': await this._handleImportExport(); break;
        case '7': await this.clearData(); break;
        case '0':
          console.log(c('👋 Stay hydrated! Goodbye!', colors.cyan));
          this.rl.close();
          return;
        default:
          console.log(c('❌ Invalid choice.', colors.red));
      }

      await this._ask('\nPress Enter to continue...');
    }
  }
}

// ─── Main ─────────────────────────────────────────────────────────────────────

const main = async (): Promise<void> => {
  try {
    const app = new WaterCounter();
    await app.run();
  } catch (err: any) {
    console.log(c(`❌ Unexpected error: ${err.message}`, colors.red));
    process.exit(1);
  }
};

main();
