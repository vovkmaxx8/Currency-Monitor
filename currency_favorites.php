# currency_favorites.php
#!/usr/bin/env php
<?php

define('FAVORITES_FILE', 'favorites.json');
define('API_URL', 'https://api.frankfurter.app/latest?from=');

function loadFavorites() {
    if (file_exists(FAVORITES_FILE)) {
        $data = json_decode(file_get_contents(FAVORITES_FILE), true);
        return $data['pairs'] ?? [['USD', 'EUR'], ['USD', 'JPY']];
    }
    return [['USD', 'EUR'], ['USD', 'JPY']];
}

function saveFavorites($pairs) {
    file_put_contents(FAVORITES_FILE, json_encode(['pairs' => $pairs], JSON_PRETTY_PRINT));
}

function getRates($base) {
    $url = API_URL . $base;
    $resp = file_get_contents($url);
    if ($resp === false) {
        return [];
    }
    $data = json_decode($resp, true);
    return $data['rates'] ?? [];
}

function listRates($pairs) {
    if (empty($pairs)) {
        echo "No favorite pairs.\n";
        return;
    }
    $grouped = [];
    foreach ($pairs as $pair) {
        $base = $pair[0];
        $target = $pair[1];
        if (!isset($grouped[$base])) $grouped[$base] = [];
        $grouped[$base][] = $target;
    }
    echo "\n💱 Favorite Currency Pairs\n\n";
    foreach ($grouped as $base => $targets) {
        $rates = getRates($base);
        foreach ($targets as $target) {
            if (isset($rates[$target])) {
                printf("%s/%s: %.4f\n", $base, $target, $rates[$target]);
            } else {
                echo "$base/$target: N/A\n";
            }
        }
    }
    echo "\nUpdated: " . date('Y-m-d H:i:s') . "\n";
}

if ($argc < 2) {
    $pairs = loadFavorites();
    listRates($pairs);
    exit(0);
}
$cmd = strtolower($argv[1]);
switch ($cmd) {
    case 'list':
        $pairs = loadFavorites();
        listRates($pairs);
        break;
    case 'add':
        if ($argc != 4) {
            echo "Usage: add BASE TARGET\n";
            exit(1);
        }
        $base = strtoupper($argv[2]);
        $target = strtoupper($argv[3]);
        $pairs = loadFavorites();
        $pair = [$base, $target];
        $found = false;
        foreach ($pairs as $p) {
            if ($p[0] == $base && $p[1] == $target) {
                $found = true;
                break;
            }
        }
        if (!$found) {
            $pairs[] = $pair;
            saveFavorites($pairs);
            echo "✅ Added $base/$target\n";
        } else {
            echo "Pair $base/$target already in favorites.\n";
        }
        break;
    case 'remove':
        if ($argc != 4) {
            echo "Usage: remove BASE TARGET\n";
            exit(1);
        }
        $base = strtoupper($argv[2]);
        $target = strtoupper($argv[3]);
        $pairs = loadFavorites();
        $newPairs = [];
        $removed = false;
        foreach ($pairs as $p) {
            if ($p[0] == $base && $p[1] == $target) {
                $removed = true;
            } else {
                $newPairs[] = $p;
            }
        }
        if ($removed) {
            saveFavorites($newPairs);
            echo "✅ Removed $base/$target\n";
        } else {
            echo "Pair $base/$target not found.\n";
        }
        break;
    case 'watch':
        $interval = 60;
        for ($i=2; $i<$argc; $i++) {
            if ($argv[$i] == '--interval' && isset($argv[$i+1])) {
                $interval = (int)$argv[$i+1];
                $i++;
            }
        }
        echo "Watching every {$interval}s. Press Ctrl+C to stop.\n";
        $pairs = loadFavorites();
        while (true) {
            listRates($pairs);
            sleep($interval);
        }
        break;
    default:
        echo "Usage: currency_favorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]\n";
        exit(1);
}
?>
