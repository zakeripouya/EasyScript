vocabulary = ["w" + str(n) for n in range(20000)]
counts = {}
x = 42
for _ in range(2000000):
    x = x * 16807 % 2147483647
    word = vocabulary[x % 20000]
    counts[word] = counts.get(word, 0) + 1
print(len(counts), counts["w0"], counts["w7"], counts["w19999"])
