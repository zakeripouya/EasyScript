ask "income?" and call the answer income
ask "rent?" and call the answer rent
ask "utilities?" and call the answer utilities
ask "insurance?" and call the answer insurance
ask "phone?" and call the answer phone
set income to number income
set rent to number rent
set utilities to number utilities
set insurance to number insurance
set phone to number phone
let left be income - rent - utilities - insurance - phone
show left
if left < 500, say "warning ur running low"
ask "how much to save?" and call the answer save
set save to number save
show 5000 / save
