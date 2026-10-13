read file "tracking.txt" and call it A
for each number in A:
    let link be "https://track.example-carrier.com/?id=" plus number
    let status be the page at link
    if status is "Delivered", say number
