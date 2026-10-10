numbers = []
x = 42
for _ in range(1000000):
    x = x * 16807 % 2147483647
    numbers.append(x)
numbers.sort()
print(numbers[0], numbers[499999], numbers[999999])
