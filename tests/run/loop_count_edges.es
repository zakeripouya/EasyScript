note: counts whose last numbers are equal to the end (within the 1e-12 tolerance)
count from 0 to 1 in steps of 0.1:
    say it
count down from 1 to 0 by 0.3:
    say it
note: near 10 trillion the tolerance is 10, so every number within 10 of the end is the end
let rounds be 0
count from 10000000000000 to 10000000000005:
    increase rounds by 1
    if it is 10000000000005, say "landed on the end"
say rounds
