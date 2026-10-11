# Writes sales.csv (100,000 rows, some fields quoted with commas inside) into
# the folder the benchmark runs in. Run by benchmarks/run.sh before timing.
awk 'BEGIN {
    print "id,region,product,amount,note"
    split("North South East West", regions, " ")
    seed = 1
    for (i = 1; i <= 100000; i++) {
        seed = (seed * 16807) % 2147483647
        region = regions[seed % 4 + 1]
        amount = seed % 1000
        note = (i % 10 == 0) ? "\"late, but paid\"" : "ok"
        printf "%d,%s,item %d,%d,%s\n", i, region, seed % 50, amount, note
    }
}' > sales.csv
