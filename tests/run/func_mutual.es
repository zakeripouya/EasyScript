to is_even n:
    if n is 0, give back yes
    give back is_odd of (n minus 1)
to is_odd n:
    if n is 0, give back no
    give back is_even of (n minus 1)
say is_even of 10
say is_odd of 7
say is_even of 3
