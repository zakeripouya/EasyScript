ask "price?" and call the answer price
ask "deposit?" and call the answer deposit
ask "rate?" and call the answer rate
ask "years?" and call the answer years
let loan be price minus deposit
let r be rate / 100 / 12
let n be years * 12
let payment be loan * r / (1 - (1 + r) to the power of -n) rounded to 2 decimal places
say loan
say payment
say payment * n
