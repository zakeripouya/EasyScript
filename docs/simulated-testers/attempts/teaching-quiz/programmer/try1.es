capitals = {
    "France": "Paris",
    "Japan": "Tokyo",
    "Kenya": "Nairobi",
    "Canada": "Ottawa",
    "Brazil": "Brasilia"
}

score = 0
for country, capital in capitals.items():
    answer = input("What is the capital of " + country + "? ")
    if answer.strip().lower() == capital.lower():
        print("Right!")
        score += 1
    else:
        print("Wrong! The answer is " + capital + ".")

print("Your score: " + score + " out of 5")
if score >= 4:
    print("Well done!")
