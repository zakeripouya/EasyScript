bill = float(input("Bill amount: "))
people = int(input("Number of people: "))
service = input("Service (good/okay/bad): ").lower()

rates = {"good": 0.20, "okay": 0.15, "bad": 0.10}
tip = bill * rates[service]
total = bill + tip
per_person = total / people

print("Tip: $" + format(tip, ".2f"))
print("Total: $" + format(total, ".2f"))
print("Each person pays: $" + format(per_person, ".2f"))
