note: constants can be used anywhere, even above the line that makes them
say rate
keep rate as 0.25
keep shop at "Corner Shop"
keep label as "Shop: " followed by shop
keep doubled as rate times 2
keep negative as -rate
keep is_open as yes
keep closed as not is_open
keep big as rate is greater than 0.1
keep from_text as "42" as a number plus 1
keep as_text as 3.5 as text followed by "!"
keep letters as length of "héllo"
keep both as is_open and big
keep joined as "rate=" and rate
keep remainder as 10 mod 3
say label
say doubled
say negative
say closed
say big
say from_text
say as_text
say letters
say both
say joined
say remainder
to with_tax of amount:
    give back amount plus amount times rate
say with_tax of 100
to describe:
    if is_open, give back shop followed by " is open"
    give back shop followed by " is closed"
say describe
count from 1 to 2:
    say it times rate
