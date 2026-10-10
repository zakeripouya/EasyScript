total = 0
for row in range(1, 10001):
    for column in range(1, 10001):
        if (row + column) % 3 == 0:
            total += 1
print(total)
