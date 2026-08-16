💧 Water Counter – Daily Hydration Tracker
"Track every drop, stay on top – because hydration is health."

📋 Table of Contents
✨ Features

📁 Repository Structure

🚀 Quick Start

💻 Language Implementations

📊 Data Format

🤝 Contributing

📄 License

✨ Features
Feature	Description
💧 Add Intake	Log water consumption in ml with automatic timestamp
🎯 Daily Goal	Set and track personalized daily hydration targets (default: 2000ml)
📈 Statistics	View total, average, min, max, and days tracked
📅 History	Review last 7 days of consumption at a glance
💾 Persistence	All data saved locally in JSON format
📤 Export / Import	Backup or transfer your data across devices
🎨 Colorful CLI	Beautiful terminal output with progress bars and colors
⚡ Cross-Platform	Works on Windows, macOS, and Linux
📁 Repository Structure
text
water-counter/
├── README.md
├── python/
│   └── water_counter.py
├── javascript/
│   └── water_counter.js
├── typescript/
│   └── water_counter.ts
├── go/
│   └── water_counter.go
├── rust/
│   └── water_counter.rs
├── cpp/
│   └── water_counter.cpp
├── java/
│   └── WaterCounter.java
└── csharp/
    └── WaterCounter.cs
🚀 Quick Start
Prerequisites
Each language requires its respective runtime/compiler (see individual sections)

Clone & Run
bash
git clone https://github.com/yourusername/water-counter.git
cd water-counter
# Navigate to your language folder and run
💻 Language Implementations
1. 🐍 Python
bash
cd python
python water_counter.py
Requires: Python 3.8+

2. 🟨 JavaScript (Node.js)
bash
cd javascript
node water_counter.js
Requires: Node.js 14+

3. 🟦 TypeScript
bash
cd typescript
npm install -g ts-node
ts-node water_counter.ts
Requires: Node.js 14+, TypeScript

4. 🟩 Go
bash
cd go
go run water_counter.go
Requires: Go 1.18+

5. 🦀 Rust
bash
cd rust
cargo run
Requires: Rust 1.70+

6. ⚙️ C++
bash
cd cpp
g++ -std=c++17 water_counter.cpp -o water_counter
./water_counter
Requires: C++17 compatible compiler

7. ☕ Java
bash
cd java
javac WaterCounter.java
java WaterCounter
Requires: JDK 17+

8. 🔷 C#
bash
cd csharp
dotnet run
Requires: .NET 6.0+

📊 Data Format
All implementations use a unified JSON schema:

json
{
  "goal": 2000,
  "entries": [
    {
      "date": "2026-01-15",
      "amount": 250,
      "timestamp": "2026-01-15T08:30:00"
    }
  ]
}
Data is stored in the user's home directory under .water_counter/data.json.

🤝 Contributing
Contributions are welcome! Please:

Fork the repository

Create a feature branch

Commit your changes

Open a Pull Request

📄 License
MIT © 2026 Water Counter Team
