let bill be ask "Bill amount: "
let people be ask "Number of people: "
let service be ask "Service (good/okay/bad): "

let rate be 0.10
if service == "good":
    set rate to 0.20
if service == "okay":
    set rate to 0.15

let tip be bill * rate
let total be bill + tip
let per_person be total / people

say "Tip: $" + tip
say "Total: $" + total
say "Each person pays: $" + per_person
