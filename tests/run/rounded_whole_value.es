# Rounding after arithmetic says which part it rounds (proposal 0022).
let loan be 280000.
let r be 0.005.
let n be 300.
let payment be loan * r / (1 - (1 + r) to the power of -n), rounded to 2 decimal places.
say payment.
let rough payment be loan * r / ((1 - (1 + r) to the power of -n) rounded to 2 decimal places).
say rough payment rounded to 2 decimals.
say (4.8 / 20) rounded to 2 decimals.
say 2 times (2.5 rounded).
