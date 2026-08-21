# currency_favorites.rb
#!/usr/bin/env ruby
require 'json'
require 'net/http'
require 'uri'
require 'time'

FAVORITES_FILE = 'favorites.json'
API_URL = 'https://api.frankfurter.app/latest?from='

def load_favorites
  if File.exist?(FAVORITES_FILE)
    data = JSON.parse(File.read(FAVORITES_FILE))
    data['pairs'] || [['USD', 'EUR'], ['USD', 'JPY']]
  else
    [['USD', 'EUR'], ['USD', 'JPY']]
  end
end

def save_favorites(pairs)
  File.write(FAVORITES_FILE, JSON.pretty_generate({ 'pairs' => pairs }))
end

def get_rates(base)
  uri = URI(API_URL + base)
  response = Net::HTTP.get(uri)
  data = JSON.parse(response)
  data['rates']
rescue => e
  puts "Error fetching rates for #{base}: #{e.message}"
  {}
end

def list_rates(pairs)
  if pairs.empty?
    puts "No favorite pairs."
    return
  end
  grouped = {}
  pairs.each do |base, target|
    grouped[base] ||= []
    grouped[base] << target
  end
  puts "\n💱 Favorite Currency Pairs\n"
  grouped.each do |base, targets|
    rates = get_rates(base)
    targets.each do |target|
      if rates[target]
        puts "#{base}/#{target}: #{'%.4f' % rates[target]}"
      else
        puts "#{base}/#{target}: N/A"
      end
    end
  end
  puts "\nUpdated: #{Time.now.strftime('%Y-%m-%d %H:%M:%S')}"
end

cmd = ARGV[0]
case cmd
when nil, 'list'
  pairs = load_favorites
  list_rates(pairs)
when 'add'
  if ARGV.length != 3
    puts "Usage: add BASE TARGET"
    exit 1
  end
  base = ARGV[1].upcase
  target = ARGV[2].upcase
  pairs = load_favorites
  if pairs.include?([base, target])
    puts "Pair #{base}/#{target} already in favorites."
  else
    pairs << [base, target]
    save_favorites(pairs)
    puts "✅ Added #{base}/#{target}"
  end
when 'remove'
  if ARGV.length != 3
    puts "Usage: remove BASE TARGET"
    exit 1
  end
  base = ARGV[1].upcase
  target = ARGV[2].upcase
  pairs = load_favorites
  new_pairs = pairs.reject { |b, t| b == base && t == target }
  if new_pairs.length < pairs.length
    save_favorites(new_pairs)
    puts "✅ Removed #{base}/#{target}"
  else
    puts "Pair #{base}/#{target} not found."
  end
when 'watch'
  interval = 60
  if ARGV.include?('--interval')
    idx = ARGV.index('--interval')
    interval = ARGV[idx+1].to_i if idx && ARGV[idx+1]
  end
  puts "Watching every #{interval}s. Press Ctrl+C to stop."
  pairs = load_favorites
  loop do
    list_rates(pairs)
    sleep(interval)
  end
else
  puts "Usage: currency_favorites [list|add BASE TARGET|remove BASE TARGET|watch [--interval N]]"
end
