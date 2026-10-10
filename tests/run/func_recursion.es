to factorial of n:
    if n is at most 1, give back 1
    give back n times factorial of (n minus 1)
say factorial of 5
say factorial of 10
to fib n:
    if n is less than 2, give back n
    give back fib of (n minus 1) plus fib of (n minus 2)
say fib of 10
let line be ""
count from 0 to 10 as k:
    set line to line followed by (fib of k) followed by " "
say line
