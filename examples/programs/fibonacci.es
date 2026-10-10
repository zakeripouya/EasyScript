note: The Fibonacci numbers: each one is the sum of the two before it.
to fibonacci of n:
    if n is less than 2, give back n
    give back fibonacci of (n minus 1) plus fibonacci of (n minus 2)
let line be ""
count from 0 to 10 as n:
    set line to line followed by (fibonacci of n) followed by " "
say line
