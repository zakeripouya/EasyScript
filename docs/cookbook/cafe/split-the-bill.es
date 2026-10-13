ask "What's the bill? " and call it bill.
ask "How many people? " and call it people.
ask "How was the service (good, okay or bad)? " and call it service.
the tip rate depends on service:
    20 when it's "good".
    15 when it's "okay".
    10 otherwise.
let tip be tip rate percent of bill.
say "Tip:", tip as money.
let total be bill plus tip.
say "Total:", total as money.
say "Each person pays:", total divided by people, as money.
