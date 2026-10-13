# Arithmetic on a table's columns waits for the file to say they're money
# (a program that reads a table can hold amounts).
put the text below in file "sales.csv":
    product,quantity,price
    Mug,2,$8.00
    Beans,4,$12.50

read the table in "sales.csv" and call it sales.
a sale's money is its price times its quantity.
say the total money of sales.
for each class and its total in the total money of sales per product:
    say class, total.
for each class and its total in the total quantity of sales per product:
    say class, total.
