guests = read_csv("guests.csv")
coming = guests.filter(g => g.rsvp == "yes")

print("Yes RSVPs:", coming.length)

headcount = 0
meals = {}
for g in coming:
    people = 1 + int(g["plus ones"])
    headcount += people
    meals[g.meal] = meals.get(g.meal, 0) + people

print("Total headcount:", headcount)
for meal, count in meals:
    print(meal + ":", count)

chart = pie_chart(meals, title="Meals")
chart.save("meals.svg")
