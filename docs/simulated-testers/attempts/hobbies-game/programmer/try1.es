secret = random(1, 10)
count = 0
while true {
    guess = int(input("Guess: "))
    count = count + 1
    if guess > secret {
        print("Too high")
    } else if guess < secret {
        print("Too low")
    } else {
        print("You got it in " + count + " guesses!")
        break
    }
}
