# Functions given to built-ins: map, filter and reduce over a million numbers.
from functools import reduce


def main():
    numbers = []
    x = 42
    for _ in range(1000000):
        x = x * 16807 % 2147483647
        numbers.append(x % 1000)
    total = 0
    for _ in range(10):
        doubled = list(map(lambda n: n * 2, numbers))
        kept = list(filter(lambda n: n % 3 == 0, doubled))
        total += reduce(lambda a, b: a + b, kept, 0)
        numbers = list(map(lambda n: n + 1, numbers))
    print(total)


main()
