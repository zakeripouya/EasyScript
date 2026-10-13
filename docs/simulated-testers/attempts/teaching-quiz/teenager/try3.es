let score be 0
ask "capital of france?" and call the answer france
if france is "paris" or france is "Paris", say "right" and set score to score + 1
if france is not "paris" and france is not "Paris", say "wrong its paris"
ask "capital of japan?" and call the answer japan
if japan is "tokyo" or japan is "Tokyo", say "right" and set score to score + 1
if japan is not "tokyo" and japan is not "Tokyo", say "wrong its tokyo"
ask "capital of kenya?" and call the answer kenya
if kenya is "nairobi" or kenya is "Nairobi", say "right" and set score to score + 1
if kenya is not "nairobi" and kenya is not "Nairobi", say "wrong its nairobi"
ask "capital of canada?" and call the answer canada
if canada is "ottawa" or canada is "Ottawa", say "right" and set score to score + 1
if canada is not "ottawa" and canada is not "Ottawa", say "wrong its ottawa"
ask "capital of brazil?" and call the answer brazil
if brazil is "brasilia" or brazil is "Brasilia", say "right" and set score to score + 1
if brazil is not "brasilia" and brazil is not "Brasilia", say "wrong its brasilia"
say score plus "/5"
if score is 4 or more, say "Well done!"
