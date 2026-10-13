# Split the bill, with a tip for the service

Ask for the bill, how many people, and how the service was. Work out the tip (20%, 15% or 10%), the total and each person's share.

## The program

`split-the-bill.es`:

```
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
```

## Running it

When it's run with `easyscript run split-the-bill.es` and these answers are typed (`86.40`, `3`, `good`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
What's the bill? How many people? How was the service (good, okay or bad)? Tip: $17.28
Total: $103.68
Each person pays: $34.56
```

## Under the hood

The tip rate is a table: `the tip rate depends on service:` checks its lines from the top, and the `otherwise` line covers every other answer, so there's always a rate.
