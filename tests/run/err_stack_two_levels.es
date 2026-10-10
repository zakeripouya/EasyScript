note: calls that repeat only twice are both shown; calls between two functions aren't collapsed
to is_even with n:
    if n is 0, give back 10 divided by n
    give back is_odd of (n minus 1)
to is_odd with n:
    give back is_even of (n minus 1)
to twice with n:
    if n is 2, give back is_even with 4
    give back twice with (n plus 1)
say twice with 0
