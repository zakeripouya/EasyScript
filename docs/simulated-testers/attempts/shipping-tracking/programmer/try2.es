read file "tracking.txt" and call it numbers
let delivered be a new list

for each num in numbers:
    let page be fetch "https://track.example-carrier.com/?id=" + num
    if page contains "delivered":
        add num to delivered

say "Delivered parcels:"
for each num in delivered:
    say num
