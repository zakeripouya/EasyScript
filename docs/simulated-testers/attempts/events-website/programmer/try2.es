let page be a web page called "Event Sign-Up"

ask "Your name" and call the answer name
ask "Meal (fish, veggie, chicken)" and call the answer meal

when the "Sign up" button is clicked:
    append name + "," + meal to file "signups.csv"
    say "Thanks, " + name + "! You're signed up."
