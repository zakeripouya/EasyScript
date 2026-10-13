let score be 0

ask "What is the capital of France?" and call the answer france
if france is "Paris", say "That is right!"
if france is not "Paris", say "Sorry, that is wrong. The right answer is Paris."
if france is "Paris", set score to score plus 1

ask "What is the capital of Japan?" and call the answer japan
if japan is "Tokyo", say "That is right!"
if japan is not "Tokyo", say "Sorry, that is wrong. The right answer is Tokyo."
if japan is "Tokyo", set score to score plus 1

ask "What is the capital of Kenya?" and call the answer kenya
if kenya is "Nairobi", say "That is right!"
if kenya is not "Nairobi", say "Sorry, that is wrong. The right answer is Nairobi."
if kenya is "Nairobi", set score to score plus 1

ask "What is the capital of Canada?" and call the answer canada
if canada is "Ottawa", say "That is right!"
if canada is not "Ottawa", say "Sorry, that is wrong. The right answer is Ottawa."
if canada is "Ottawa", set score to score plus 1

ask "What is the capital of Brazil?" and call the answer brazil
if brazil is "Brasilia", say "That is right!"
if brazil is not "Brasilia", say "Sorry, that is wrong. The right answer is Brasilia."
if brazil is "Brasilia", set score to score plus 1

say "Your score is " score " out of 5."
if score is 4 or more, say "Well done!"
