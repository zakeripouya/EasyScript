let bill be ask("Bill amount: ")
let people be ask("Number of people: ")
let service be ask("Service (good/okay/bad): ")

if service == "good":
    let rate be 0.20
elif service == "okay":
    let rate be 0.15
else:
    let rate be 0.10

let tip be bill * rate
let total be bill + tip
let per_person be total / people

say "Tip: $" + round(tip, 2)
say "Total: $" + round(total, 2)
say "Each person pays: $" + round(per_person, 2)
