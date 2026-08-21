// currency_favorites.js
#!/usr/bin/env node
const fs = require('fs');
const path = require('path');

const FAVORITES_FILE = 'favorites.json';
const API_URL = 'https://api.frankfurter.app/latest?from=';

function loadFavorites() {
    if (fs.existsSync(FAVORITES_FILE)) {
        try {
            const data = JSON.parse(fs.readFileSync(FAVORITES_FILE));
            return data.pairs || [['USD', 'EUR'], ['USD', 'JPY']];
        } catch (e) {
            return [['USD', 'EUR'], ['USD', 'JPY']];
        }
    }
    return [['USD', 'EUR'], ['USD', 'JPY']];
}

function saveFavorites(pairs) {
    fs.writeFileSync(FAVORITES_FILE, JSON.stringify({ pairs }, null, 2));
}

async function getRates(base) {
    const url = API_URL + base;
    const resp = await fetch(url);
    if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
    const data = await resp.json();
    return data.rates;
}

function listRates(pairs) {
    if (pairs.length === 0) {
        console.log('No favorite pairs.');
        return;
    }
    // Group by base
    const grouped = {};
    for (const [base, target] of pairs) {
        if (!grouped[base]) grouped[base] = [];
        grouped[base].push(target);
    }
    console.log('\n💱 Favorite Currency Pairs\n');
    Promise.all(Object.entries(grouped).map(async ([base, targets]) => {
        try {
            const rates = await getRates(base);
            for (const target of targets) {
                if (rates[target] !== undefined) {
                    console.log(`${base}/${target}: ${rates[target].toFixed(4)}`);
                } else {
                    console.log(`${base}/${target}: N/A`);
                }
            }
        } catch (e) {
            console.error(`Error fetching rates for ${base}: ${e.message}`);
        }
    })).then(() => {
        console.log(`\nUpdated: ${new Date().toISOString().slice(0,19).replace('T',' ')}`);
    }).catch(console.error);
}

function main() {
    const args = process.argv.slice(2);
    if (args.length === 0) {
        const pairs = loadFavorites();
        listRates(pairs);
        return;
    }
    const cmd = args[0].toLowerCase();
    if (cmd === 'list') {
        const pairs = loadFavorites();
        listRates(pairs);
    } else if (cmd === 'add' && args.length === 3) {
        const base = args[1].toUpperCase();
        const target = args[2].toUpperCase();
        const pairs = loadFavorites();
        const newPair = [base, target];
        if (!pairs.some(p => p[0] === base && p[1] === target)) {
            pairs.push(newPair);
            saveFavorites(pairs);
            console.log(`✅ Added ${base}/${target}`);
        } else {
            console.log(`Pair ${base}/${target} already in favorites.`);
        }
    } else if (cmd === 'remove' && args.length === 3) {
        const base = args[1].toUpperCase();
        const target = args[2].toUpperCase();
        let pairs = loadFavorites();
        const filtered = pairs.filter(p => !(p[0] === base && p[1] === target));
        if (filtered.length < pairs.length) {
            saveFavorites(filtered);
            console.log(`✅ Removed ${base}/${target}`);
        } else {
            console.log(`Pair ${base}/${target} not found.`);
        }
    } else if (cmd === 'watch') {
        let interval = 60;
        const idx = args.indexOf('--interval');
        if (idx !== -1 && args.length > idx + 1) {
            interval = parseInt(args[idx + 1]);
        }
        console.log(`Watching every ${interval}s. Press Ctrl+C to stop.`);
        const pairs = loadFavorites();
        setInterval(() => listRates(pairs), interval * 1000);
        listRates(pairs);
    } else {
        console.log('Usage: currency_favorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]');
    }
}

main().catch(console.error);
