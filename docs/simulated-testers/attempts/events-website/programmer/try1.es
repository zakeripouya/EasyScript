page = new WebPage("Event Sign-Up")

form = page.add_form()
form.add_input("name", label="Your name", required=true)
form.add_select("meal", label="Meal", options=["fish", "veggie", "chicken"])
form.add_button("Sign up")

form.on_submit(data => {
    append_csv("signups.csv", { name: data.name, meal: data.meal })
    page.show("Thanks, " + data.name + "! You're signed up.")
})

page.serve(port=8080)
