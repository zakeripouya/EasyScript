read file "tracking.txt" and call it numbers
let delivered be ""

for each num in numbers:
    let url be "https://track.example-carrier.com/?id=" plus num
    let page be fetch(url)
    if page contains "delivered":
        set delivered to delivered plus num plus " "

say "Delivered parcels:"
say delivered
