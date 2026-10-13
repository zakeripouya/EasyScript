let games be read("games.csv")

let wins be count(games, "winner")
print("Most wins:", max(wins))

for each game in unique(games, "game"):
    let rows be filter(games, "game", game)
    print(game, average(rows, "score"))

let top3 be first(3, sort(games, "score", descending))
for each row in top3:
    print(row["winner"], row["game"], row["score"])
