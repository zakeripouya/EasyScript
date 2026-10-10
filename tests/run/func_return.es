to first_even_up_to n:
    count from 1 to n:
        if it mod 2 is 0, give back it
    return
say first_even_up_to of 5
say first_even_up_to of 1
to nothing_back:
    say "falls off the end"
say nothing_back
to early x:
    if x is 1:
        give back "one"
    give back "other"
say early of 1
say early of 2
to loop_out:
    forever:
        give back "left the loop and the function"
say loop_out
