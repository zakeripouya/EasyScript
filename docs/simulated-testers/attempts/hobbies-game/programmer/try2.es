let secret be random(1, 10)
let tries be 0
while true:
    let guess be int(input("Guess: "))
    set tries to tries + 1
    if guess > secret:
        print("Too high")
    else if guess < secret:
        print("Too low")
    else:
        print("You got it in " + tries + " guesses!")
        break
