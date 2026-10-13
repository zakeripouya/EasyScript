numbers = read_lines("tracking.txt")
delivered = []

for num in numbers:
    page = fetch("https://track.example-carrier.com/?id=" + num)
    if "delivered" in page.text.lower():
        delivered.append(num)

print("Delivered parcels:")
for num in delivered:
    print(num)
