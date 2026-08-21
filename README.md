💱 Currency Monitor — Favorite Exchange Rate Tracker
8 languages, one simple currency tracker – watch your favorite currency pairs, get live exchange rates, and manage your watchlist from the terminal.

✨ Features
📊 View rates for your favorite currency pairs (e.g., USD/EUR, GBP/JPY)

➕ Add pairs to your watchlist

➖ Remove pairs from your watchlist

🔄 Update rates on demand (or automatically on each run)

⏱️ Continuous monitoring mode with custom interval (optional)

💾 Persistent storage – your favorites are saved in a local file

🚀 Quick Start
All implementations follow the same CLI pattern:

bash
# Show rates for all favorite pairs
<command>

# Show rates for all favorite pairs (explicit)
<command> list

# Add a new pair
<command> add USD EUR

# Remove a pair
<command> remove USD EUR

# Start monitoring with refresh every 30 seconds (optional)
<command> watch --interval 30
Configuration:
All programs store your favorite pairs in favorites.json (or favorites.txt) in the current directory.

📸 Example Output
text
💱 Favorite Currency Pairs

USD/EUR: 0.8501
GBP/USD: 1.3214
EUR/JPY: 158.23
USD/CHF: 0.9125

Updated: 2026-08-21 14:30:15
📁 Repository Structure
text
.
├── README.md
├── python/
│   └── currency_favorites.py
├── go/
│   └── currency_favorites.go
├── javascript/
│   └── currency_favorites.js
├── ruby/
│   └── currency_favorites.rb
├── php/
│   └── currency_favorites.php
├── java/
│   └── CurrencyFavorites.java
├── csharp/
│   └── CurrencyFavorites.cs
└── cpp/
    └── currency_favorites.cpp
