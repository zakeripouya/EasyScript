# Functions with text inputs, called 100,000 times: their frames keep the
# text they were given and let go of it when they end, however they end.
to shout with word:
    give back word followed by "!"
to pair with left and right:
    if left is "", give back right
    give back left followed by right
to tally with names (a list of text) and label:
    for each n in names:
        if n is "stop", give back label
    give back "done"
repeat 100000 times:
    let w be shout with ("hi" followed by it as text)
    let b be pair with w and "x"
    let c be pair with "" and b
    let d be tally with (a list of "a", "stop") and c
say shout with "end"
