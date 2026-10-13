let games be read "games.csv"

let wins be count games by "winner"
print "Most wins:", max wins

for each game in unique games "game":
    let rows be filter games where game is game
    print game, average rows "score"

let top3 be first 3 of sort games by "score" descending
for each row in top3:
    print row.winner, row.game, row.score
