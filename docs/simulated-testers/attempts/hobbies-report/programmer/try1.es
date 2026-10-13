games = load("games.csv")

wins = games.groupBy("winner").count()
print("Most wins:", wins.max())

for game in games.unique("game"):
    rows = games.filter(row => row.game == game)
    print(game, average(rows.score))

top3 = games.sortBy("score", descending).take(3)
for row in top3:
    print(row.winner, row.game, row.score)
