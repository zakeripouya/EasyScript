let answer be "yes"
repeat while answer is "yes":
    ask "Name of object?" and call the answer name
    ask "Mass in gram?" and call the answer mass
    ask "Volume in cubic centimeter?" and call the answer volume
    let density be mass / volume rounded to 2 decimal places
    say density
    if density is less than 1:
        say "it float"
    else:
        say "it sink"
    ask "Have another object?" and call the answer answer
