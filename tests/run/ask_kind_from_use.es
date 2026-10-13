# A plain "ask" asks for what its answer is used as (proposal 0017).
ask "What's the bill? " and call it bill.
ask "How many people? " and call it people.
say "Each pays", bill divided by people.
ask "Your name? " and call it name.
say "Hi,", name.
ask "Happy? " and call it happy.
if happy, say "Good."
ask "The tip? " and call it tip.
say "With the tip:", tip plus $10.
let total be 10.
ask "Add how much? " and call it total.
say total times 2.
to double with x:
    give back x times 2.
ask "A number to double? " and call it n.
say double with n.
ask "Anything? " and call it anything.
say anything.
