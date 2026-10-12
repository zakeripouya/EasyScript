unless ready:
    say "waiting".
unless ready, say "still waiting".
to sign of x:
    give back -1 if x is below 0.
    give back 1 unless x is 0.
    give back 0.
to warn with amount:
    stop here unless amount is above 100.
    return if amount is 1000.
    say amount.
