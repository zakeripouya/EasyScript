read file "tracking.txt" and call it t
for each line in t:
    let p be "https://track.example-carrier.com/?id=" plus line
    if p is delivered, say line
