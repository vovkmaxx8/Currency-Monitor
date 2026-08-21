# currency_favorites.py
import sys
import json
import os
import requests
from datetime import datetime

FAVORITES_FILE = "favorites.json"
API_URL = "https://api.frankfurter.app/latest?from="

class CurrencyMonitor:
    def __init__(self):
        self.favorites = []
        self.load_favorites()

    def load_favorites(self):
        if os.path.exists(FAVORITES_FILE):
            with open(FAVORITES_FILE, "r") as f:
                self.favorites = json.load(f)
        else:
            self.favorites = [["USD", "EUR"], ["USD", "JPY"]]  # default

    def save_favorites(self):
        with open(FAVORITES_FILE, "w") as f:
            json.dump(self.favorites, f, indent=2)

    def add_pair(self, base, target):
        pair = [base.upper(), target.upper()]
        if pair not in self.favorites:
            self.favorites.append(pair)
            self.save_favorites()
            print(f"✅ Added {base}/{target}")
        else:
            print(f"Pair {base}/{target} already in favorites.")

    def remove_pair(self, base, target):
        pair = [base.upper(), target.upper()]
        if pair in self.favorites:
            self.favorites.remove(pair)
            self.save_favorites()
            print(f"✅ Removed {base}/{target}")
        else:
            print(f"Pair {base}/{target} not found.")

    def get_rates(self):
        # Group by base currency to reduce API calls
        rates = {}
        for base, target in self.favorites:
            if base not in rates:
                # Fetch all rates from this base
                url = API_URL + base
                try:
                    resp = requests.get(url)
                    resp.raise_for_status()
                    data = resp.json()
                    rates[base] = data["rates"]
                except Exception as e:
                    print(f"Error fetching rates for {base}: {e}", file=sys.stderr)
                    rates[base] = {}
        return rates

    def list_rates(self):
        if not self.favorites:
            print("No favorite pairs.")
            return
        rates = self.get_rates()
        print("\n💱 Favorite Currency Pairs\n")
        for base, target in self.favorites:
            if target in rates.get(base, {}):
                rate = rates[base][target]
                print(f"{base}/{target}: {rate:.4f}")
            else:
                print(f"{base}/{target}: N/A")
        print(f"\nUpdated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

def main():
    app = CurrencyMonitor()
    if len(sys.argv) < 2:
        app.list_rates()
    else:
        cmd = sys.argv[1].lower()
        if cmd == "list":
            app.list_rates()
        elif cmd == "add" and len(sys.argv) == 4:
            app.add_pair(sys.argv[2], sys.argv[3])
        elif cmd == "remove" and len(sys.argv) == 4:
            app.remove_pair(sys.argv[2], sys.argv[3])
        elif cmd == "watch":
            import time
            interval = 60
            if len(sys.argv) >= 4 and sys.argv[2] == "--interval":
                interval = int(sys.argv[3])
            print(f"Watching every {interval}s. Press Ctrl+C to stop.")
            try:
                while True:
                    app.list_rates()
                    time.sleep(interval)
            except KeyboardInterrupt:
                print("\nStopped.")
        else:
            print("Usage: currency_favorites.py [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]")

if __name__ == "__main__":
    main()
