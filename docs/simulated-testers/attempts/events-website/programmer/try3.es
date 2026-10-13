ask "Your name" and call the answer name
ask "Meal (fish, veggie, chicken)" and call the answer meal

when "Sign up" is clicked:
    append name + "," + meal to file "signups.csv"
    say "Thanks, " + name + "! You're signed up."
