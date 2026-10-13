Ask "House price?" and call the answer price.
Ask "Deposit?" and call the answer deposit.
Ask "Interest rate for year in percent?" and call the answer interest rate.
Ask "Number of year?" and call the answer years.
Let loan be price minus deposit.
Let monthly rate be interest rate divided by 100 divided by 12.
Let month number be years multiplied by 12.
Let payment be loan multiplied by monthly rate divided by (1 minus (1 plus monthly rate) power minus month number).
Let payment be payment round to 2 decimal.
Say loan.
Say payment.
Let total be payment multiplied by month number.
Say total.
