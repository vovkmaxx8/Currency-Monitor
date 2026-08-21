// CurrencyFavorites.java
import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.time.*;
import java.util.*;
import com.google.gson.*;

public class CurrencyFavorites {
    private static final String FAVORITES_FILE = "favorites.json";
    private static final String API_URL = "https://api.frankfurter.app/latest?from=";
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    static class Pair {
        String base;
        String target;
        Pair(String base, String target) { this.base = base; this.target = target; }
    }

    static class Favorites {
        List<Pair> pairs = new ArrayList<>();
    }

    static List<Pair> loadFavorites() throws IOException {
        Path path = Paths.get(FAVORITES_FILE);
        if (Files.exists(path)) {
            String json = new String(Files.readAllBytes(path));
            Favorites fav = gson.fromJson(json, Favorites.class);
            return fav.pairs;
        }
        // default
        return Arrays.asList(new Pair("USD", "EUR"), new Pair("USD", "JPY"));
    }

    static void saveFavorites(List<Pair> pairs) throws IOException {
        Favorites fav = new Favorites();
        fav.pairs = pairs;
        Files.write(Paths.get(FAVORITES_FILE), gson.toJson(fav).getBytes());
    }

    static Map<String, Double> getRates(String base) throws Exception {
        URL url = new URL(API_URL + base);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();
        conn.setRequestMethod("GET");
        BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
        StringBuilder sb = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) sb.append(line);
        reader.close();
        JsonObject obj = gson.fromJson(sb.toString(), JsonObject.class);
        JsonObject rates = obj.getAsJsonObject("rates");
        Map<String, Double> map = new HashMap<>();
        for (String key : rates.keySet()) {
            map.put(key, rates.get(key).getAsDouble());
        }
        return map;
    }

    static void listRates(List<Pair> pairs) {
        if (pairs.isEmpty()) {
            System.out.println("No favorite pairs.");
            return;
        }
        // group by base
        Map<String, List<Pair>> grouped = new HashMap<>();
        for (Pair p : pairs) {
            grouped.computeIfAbsent(p.base, k -> new ArrayList<>()).add(p);
        }
        System.out.println("\n💱 Favorite Currency Pairs\n");
        for (Map.Entry<String, List<Pair>> entry : grouped.entrySet()) {
            String base = entry.getKey();
            try {
                Map<String, Double> rates = getRates(base);
                for (Pair p : entry.getValue()) {
                    if (rates.containsKey(p.target)) {
                        System.out.printf("%s/%s: %.4f%n", p.base, p.target, rates.get(p.target));
                    } else {
                        System.out.printf("%s/%s: N/A%n", p.base, p.target);
                    }
                }
            } catch (Exception e) {
                System.err.println("Error fetching rates for " + base + ": " + e.getMessage());
            }
        }
        System.out.printf("%nUpdated: %s%n", LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")));
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            listRates(loadFavorites());
            return;
        }
        String cmd = args[0].toLowerCase();
        switch (cmd) {
            case "list":
                listRates(loadFavorites());
                break;
            case "add":
                if (args.length != 3) {
                    System.out.println("Usage: add BASE TARGET");
                    return;
                }
                String base = args[1].toUpperCase();
                String target = args[2].toUpperCase();
                List<Pair> pairs = loadFavorites();
                boolean exists = pairs.stream().anyMatch(p -> p.base.equals(base) && p.target.equals(target));
                if (!exists) {
                    pairs.add(new Pair(base, target));
                    saveFavorites(pairs);
                    System.out.printf("✅ Added %s/%s%n", base, target);
                } else {
                    System.out.printf("Pair %s/%s already in favorites.%n", base, target);
                }
                break;
            case "remove":
                if (args.length != 3) {
                    System.out.println("Usage: remove BASE TARGET");
                    return;
                }
                base = args[1].toUpperCase();
                target = args[2].toUpperCase();
                pairs = loadFavorites();
                boolean removed = pairs.removeIf(p -> p.base.equals(base) && p.target.equals(target));
                if (removed) {
                    saveFavorites(pairs);
                    System.out.printf("✅ Removed %s/%s%n", base, target);
                } else {
                    System.out.printf("Pair %s/%s not found.%n", base, target);
                }
                break;
            case "watch":
                int interval = 60;
                for (int i = 1; i < args.length; i++) {
                    if (args[i].equals("--interval") && i+1 < args.length) {
                        interval = Integer.parseInt(args[i+1]);
                        i++;
                    }
                }
                System.out.printf("Watching every %ds. Press Ctrl+C to stop.%n", interval);
                List<Pair> watchPairs = loadFavorites();
                while (true) {
                    listRates(watchPairs);
                    Thread.sleep(interval * 1000L);
                }
            default:
                System.out.println("Usage: java CurrencyFavorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]");
        }
    }
}
