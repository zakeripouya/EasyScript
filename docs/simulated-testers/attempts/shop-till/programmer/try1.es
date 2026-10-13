prices = {"coffee beans": 12.50, "mug": 8, "filter papers": 3.20}
total = 0

while true:
    item = input("Item: ")
    if item == "done":
        break
    if item not in prices:
        print("Sorry, " + item + " isn't on the list.")
        continue
    qty = number(input("How many: "))
    total = total + prices[item] * qty

print("Total: $" + total)
if total > 50:
    discounted = total * 0.9
    print("Discounted total: $" + discounted)
