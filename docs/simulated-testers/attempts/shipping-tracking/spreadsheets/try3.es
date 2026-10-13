read file "tracking.txt" and call it numbers
for each number in numbers:
    let link be "https://track.example-carrier.com/?id=" plus number
    let status be page from link
    if status is "Delivered", say number
