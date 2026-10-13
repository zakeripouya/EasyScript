# Convert lengths and temperatures

Ask for a length in centimeters and a temperature in Celsius, and show them in inches, meters and Fahrenheit.

## The program

`unit-converter.es`:

```
ask "A length in centimeters? " and call it centimeters.
let inches be centimeters divided by 2.54, rounded to 2 decimals.
say centimeters, "cm is", inches, "inches, or", centimeters divided by 100, "meters."
ask "A temperature in Celsius? " and call it celsius.
say celsius, "°C is", celsius times 9 divided by 5 plus 32, "°F."
```

## Running it

When it's run with `easyscript run unit-converter.es` and these answers are typed (`180`, `37`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
A length in centimeters? 180 cm is 70.87 inches, or 1.8 meters.
A temperature in Celsius? 37 °C is 98.6 °F.
```
