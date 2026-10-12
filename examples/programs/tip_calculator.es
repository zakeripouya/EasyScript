# Works out a tip. Asks again if an answer doesn't fit,
# and remembers the last tip in a file if it can.

ask for an amount "How much was the bill? " and call it bill.
ask yes or no "Was the service good? " and call it service was good.
let tip rate be 0.15.
if service was good, set tip rate to 0.2.
let tip be bill times tip rate.

say "Tip:", tip rate as a percent, "is", tip.
say "Total:", bill plus tip.

try to read file "last tip.txt" and call it last tip.
if that fails:
    say "This is your first tip here.".
if last tip isn't empty, say "Last time you tipped [last tip].".
write tip in file "last tip.txt".
