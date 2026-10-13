# Missed appointments per clinic

From a list of appointments, show how many each clinic missed and what percent that is, and which day was busiest.

## The program

`missed-appointments.es`:

```
read the table in "appointments.csv" and call it appointments.
let all per clinic be the number of appointments per clinic.
let missed per clinic be the number of appointments whose status is "missed" per clinic.
for each clinic and its count in all per clinic:
    let missed be the value for clinic in missed per clinic, or 0.
    let percent be missed times 100 divided by count, rounded.
    say clinic, "missed", missed, "of", count, "([percent]%)".
say "Busiest day:", the weekday with the highest number of appointments.
```

## The data

`appointments.csv`, in the same folder:

```
date,weekday,clinic,status
2026-10-05,Monday,North,kept
2026-10-05,Monday,North,missed
2026-10-05,Monday,South,kept
2026-10-06,Tuesday,North,kept
2026-10-06,Tuesday,South,missed
2026-10-06,Tuesday,South,missed
2026-10-07,Wednesday,North,cancelled
2026-10-07,Wednesday,South,kept
2026-10-08,Thursday,North,kept
2026-10-08,Thursday,North,kept
2026-10-08,Thursday,North,missed
2026-10-09,Friday,South,kept
```

## Running it

`easyscript run missed-appointments.es` shows:

```
North missed 2 of 7 (29%)
South missed 2 of 5 (40%)
Busiest day: Monday
```
