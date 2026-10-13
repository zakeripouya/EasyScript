read file "tracking.txt" and call it numbers
for each number in numbers:
    let page be website "https://track.example-carrier.com/?id=" plus number
    if page is delivered, say number
