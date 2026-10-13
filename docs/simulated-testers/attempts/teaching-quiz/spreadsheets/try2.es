Set Score to 0

Ask "What is the capital of France?" and call the answer A1
If LOWER A1 = "paris" then show "Right!" and add 1 to Score
Otherwise:
  show "Wrong, the answer is Paris"

Ask "What is the capital of Japan?" and call the answer A2
If LOWER A2 = "tokyo" then show "Right!" and add 1 to Score
Otherwise:
  show "Wrong, the answer is Tokyo"

Ask "What is the capital of Kenya?" and call the answer A3
If LOWER A3 = "nairobi" then show "Right!" and add 1 to Score
Otherwise:
  show "Wrong, the answer is Nairobi"

Ask "What is the capital of Canada?" and call the answer A4
If LOWER A4 = "ottawa" then show "Right!" and add 1 to Score
Otherwise:
  show "Wrong, the answer is Ottawa"

Ask "What is the capital of Brazil?" and call the answer A5
If LOWER A5 = "brasilia" then show "Right!" and add 1 to Score
Otherwise:
  show "Wrong, the answer is Brasilia"

Show "Your score: " & Score & " out of 5"
If Score >= 4 then show "Well done!"
