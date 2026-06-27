import csv

results = {}
with open("benchmark_results.csv") as f:
    for row in csv.DictReader(f):
        lang = row["lang"]
        np   = int(row["np"])
        t    = float(row["time_s"])
        results[(lang, np)] = t

print(f"{'lang':>6} {'np':>4} {'time':>8} {'speedup':>9} {'efficiency':>12}")
for (lang, np), t in sorted(results.items()):
    t1 = results[(lang, 1)]
    sp = t1 / t
    eff = sp / np * 100
    print(f"{lang:>6} {np:>4} {t:>8.3f} {sp:>9.2f} {eff:>11.1f}%")