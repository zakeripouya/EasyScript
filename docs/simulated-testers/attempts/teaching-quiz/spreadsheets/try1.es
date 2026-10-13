Create a table with columns Country and Capital:
France, Paris
Japan, Tokyo
Kenya, Nairobi
Canada, Ottawa
Brazil, Brasilia

Set Score to 0
For each row in the table:
  Ask "What is the capital of [Country]?"
  If LOWER(answer) = LOWER(Capital) then show "Right!" and add 1 to Score
  Otherwise show "Wrong, the answer is [Capital]"

Show "Your score: [Score] out of 5"
If Score >= 4 then show "Well done!"
