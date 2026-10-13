let score be 0
ask "capital of france?" and call the answer a
if lowercase a is "paris", say "right" and set score to score + 1
if lowercase a is not "paris", say "wrong its paris"
ask "capital of japan?" and call the answer b
if lowercase b is "tokyo", say "right" and set score to score + 1
if lowercase b is not "tokyo", say "wrong its tokyo"
ask "capital of kenya?" and call the answer c
if lowercase c is "nairobi", say "right" and set score to score + 1
if lowercase c is not "nairobi", say "wrong its nairobi"
ask "capital of canada?" and call the answer d
if lowercase d is "ottawa", say "right" and set score to score + 1
if lowercase d is not "ottawa", say "wrong its ottawa"
ask "capital of brazil?" and call the answer e
if lowercase e is "brasilia", say "right" and set score to score + 1
if lowercase e is not "brasilia", say "wrong its brasilia"
say score "/5"
if score is 4 or more, say "Well done!"
