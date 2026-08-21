// CurrencyFavorites.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

class Pair
{
    [JsonPropertyName("base")] public string Base { get; set; }
    [JsonPropertyName("target")] public string Target { get; set; }
}

class Favorites
{
    [JsonPropertyName("pairs")] public List<Pair> Pairs { get; set; } = new List<Pair>();
}

class CurrencyFavorites
{
    private static readonly string FavoritesFile = "favorites.json";
    private static readonly string ApiUrl = "https://api.frankfurter.app/latest?from=";
    private static readonly HttpClient client = new HttpClient();
    private static readonly JsonSerializerOptions options = new JsonSerializerOptions { WriteIndented = true };

    static async Task<List<Pair>> LoadFavoritesAsync()
    {
        if (File.Exists(FavoritesFile))
        {
            string json = await File.ReadAllTextAsync(FavoritesFile);
            var fav = JsonSerializer.Deserialize<Favorites>(json);
            return fav?.Pairs ?? new List<Pair>();
        }
        return new List<Pair> { new Pair { Base = "USD", Target = "EUR" }, new Pair { Base = "USD", Target = "JPY" } };
    }

    static async Task SaveFavoritesAsync(List<Pair> pairs)
    {
        var fav = new Favorites { Pairs = pairs };
        string json = JsonSerializer.Serialize(fav, options);
        await File.WriteAllTextAsync(FavoritesFile, json);
    }

    static async Task<Dictionary<string, double>> GetRatesAsync(string baseCurrency)
    {
        string url = ApiUrl + baseCurrency;
        var response = await client.GetAsync(url);
        response.EnsureSuccessStatusCode();
        string json = await response.Content.ReadAsStringAsync();
        using var doc = JsonDocument.Parse(json);
        var ratesElement = doc.RootElement.GetProperty("rates");
        var dict = new Dictionary<string, double>();
        foreach (var prop in ratesElement.EnumerateObject())
        {
            dict[prop.Name] = prop.Value.GetDouble();
        }
        return dict;
    }

    static async Task ListRatesAsync(List<Pair> pairs)
    {
        if (pairs.Count == 0)
        {
            Console.WriteLine("No favorite pairs.");
            return;
        }
        var grouped = pairs.GroupBy(p => p.Base);
        Console.WriteLine("\n💱 Favorite Currency Pairs\n");
        foreach (var group in grouped)
        {
            string baseCurrency = group.Key;
            try
            {
                var rates = await GetRatesAsync(baseCurrency);
                foreach (var p in group)
                {
                    if (rates.TryGetValue(p.Target, out double rate))
                    {
                        Console.WriteLine($"{p.Base}/{p.Target}: {rate:F4}");
                    }
                    else
                    {
                        Console.WriteLine($"{p.Base}/{p.Target}: N/A");
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error fetching rates for {baseCurrency}: {e.Message}");
            }
        }
        Console.WriteLine($"\nUpdated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
    }

    static async Task Main(string[] args)
    {
        if (args.Length == 0)
        {
            var pairs = await LoadFavoritesAsync();
            await ListRatesAsync(pairs);
            return;
        }
        string cmd = args[0].ToLower();
        switch (cmd)
        {
            case "list":
                pairs = await LoadFavoritesAsync();
                await ListRatesAsync(pairs);
                break;
            case "add":
                if (args.Length != 3)
                {
                    Console.WriteLine("Usage: add BASE TARGET");
                    return;
                }
                string baseCur = args[1].ToUpper();
                string targetCur = args[2].ToUpper();
                pairs = await LoadFavoritesAsync();
                if (!pairs.Any(p => p.Base == baseCur && p.Target == targetCur))
                {
                    pairs.Add(new Pair { Base = baseCur, Target = targetCur });
                    await SaveFavoritesAsync(pairs);
                    Console.WriteLine($"✅ Added {baseCur}/{targetCur}");
                }
                else
                {
                    Console.WriteLine($"Pair {baseCur}/{targetCur} already in favorites.");
                }
                break;
            case "remove":
                if (args.Length != 3)
                {
                    Console.WriteLine("Usage: remove BASE TARGET");
                    return;
                }
                baseCur = args[1].ToUpper();
                targetCur = args[2].ToUpper();
                pairs = await LoadFavoritesAsync();
                bool removed = pairs.RemoveAll(p => p.Base == baseCur && p.Target == targetCur) > 0;
                if (removed)
                {
                    await SaveFavoritesAsync(pairs);
                    Console.WriteLine($"✅ Removed {baseCur}/{targetCur}");
                }
                else
                {
                    Console.WriteLine($"Pair {baseCur}/{targetCur} not found.");
                }
                break;
            case "watch":
                int interval = 60;
                for (int i = 1; i < args.Length; i++)
                {
                    if (args[i] == "--interval" && i + 1 < args.Length)
                    {
                        interval = int.Parse(args[i + 1]);
                        i++;
                    }
                }
                Console.WriteLine($"Watching every {interval}s. Press Ctrl+C to stop.");
                var watchPairs = await LoadFavoritesAsync();
                while (true)
                {
                    await ListRatesAsync(watchPairs);
                    await Task.Delay(interval * 1000);
                }
            default:
                Console.WriteLine("Usage: CurrencyFavorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]");
                break;
        }
    }
}
