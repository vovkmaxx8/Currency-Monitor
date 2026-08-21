// currency_favorites.go
package main

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

type Favorites struct {
	Pairs [][2]string `json:"pairs"`
}

var favoritesFile = "favorites.json"
var apiBase = "https://api.frankfurter.app/latest?from="

func loadFavorites() [][2]string {
	data, err := os.ReadFile(favoritesFile)
	if err != nil {
		// default pairs
		return [][2]string{{"USD", "EUR"}, {"USD", "JPY"}}
	}
	var f Favorites
	if err := json.Unmarshal(data, &f); err != nil {
		return [][2]string{{"USD", "EUR"}, {"USD", "JPY"}}
	}
	return f.Pairs
}

func saveFavorites(pairs [][2]string) {
	f := Favorites{Pairs: pairs}
	data, _ := json.MarshalIndent(f, "", "  ")
	os.WriteFile(favoritesFile, data, 0644)
}

func getRates(base string) (map[string]float64, error) {
	url := apiBase + base
	resp, err := http.Get(url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}
	var result map[string]interface{}
	if err := json.Unmarshal(body, &result); err != nil {
		return nil, err
	}
	rates, ok := result["rates"].(map[string]interface{})
	if !ok {
		return nil, fmt.Errorf("invalid response")
	}
	ratesMap := make(map[string]float64)
	for k, v := range rates {
		ratesMap[k] = v.(float64)
	}
	return ratesMap, nil
}

func listRates(pairs [][2]string) {
	if len(pairs) == 0 {
		fmt.Println("No favorite pairs.")
		return
	}
	// Group by base
	baseMap := make(map[string][][2]string)
	for _, p := range pairs {
		base := p[0]
		baseMap[base] = append(baseMap[base], p)
	}
	fmt.Println("\n💱 Favorite Currency Pairs\n")
	for base, pairList := range baseMap {
		rates, err := getRates(base)
		if err != nil {
			fmt.Printf("Error fetching rates for %s: %v\n", base, err)
			continue
		}
		for _, p := range pairList {
			target := p[1]
			if rate, ok := rates[target]; ok {
				fmt.Printf("%s/%s: %.4f\n", base, target, rate)
			} else {
				fmt.Printf("%s/%s: N/A\n", base, target)
			}
		}
	}
	fmt.Printf("\nUpdated: %s\n", time.Now().Format("2006-01-02 15:04:05"))
}

func main() {
	if len(os.Args) < 2 {
		pairs := loadFavorites()
		listRates(pairs)
		return
	}
	cmd := os.Args[1]
	switch cmd {
	case "list":
		pairs := loadFavorites()
		listRates(pairs)
	case "add":
		if len(os.Args) != 4 {
			fmt.Println("Usage: add <base> <target>")
			return
		}
		base := strings.ToUpper(os.Args[2])
		target := strings.ToUpper(os.Args[3])
		pairs := loadFavorites()
		newPair := [2]string{base, target}
		exists := false
		for _, p := range pairs {
			if p == newPair {
				exists = true
				break
			}
		}
		if !exists {
			pairs = append(pairs, newPair)
			saveFavorites(pairs)
			fmt.Printf("✅ Added %s/%s\n", base, target)
		} else {
			fmt.Printf("Pair %s/%s already in favorites.\n", base, target)
		}
	case "remove":
		if len(os.Args) != 4 {
			fmt.Println("Usage: remove <base> <target>")
			return
		}
		base := strings.ToUpper(os.Args[2])
		target := strings.ToUpper(os.Args[3])
		pairs := loadFavorites()
		newPairs := [][2]string{}
		found := false
		for _, p := range pairs {
			if p[0] == base && p[1] == target {
				found = true
			} else {
				newPairs = append(newPairs, p)
			}
		}
		if found {
			saveFavorites(newPairs)
			fmt.Printf("✅ Removed %s/%s\n", base, target)
		} else {
			fmt.Printf("Pair %s/%s not found.\n", base, target)
		}
	case "watch":
		interval := 60
		if len(os.Args) >= 4 && os.Args[2] == "--interval" {
			interval, _ = strconv.Atoi(os.Args[3])
		}
		fmt.Printf("Watching every %ds. Press Ctrl+C to stop.\n", interval)
		pairs := loadFavorites()
		ticker := time.NewTicker(time.Duration(interval) * time.Second)
		defer ticker.Stop()
		for {
			listRates(pairs)
			<-ticker.C
		}
	default:
		fmt.Println("Usage: currency_favorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]")
	}
}
