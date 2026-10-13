rows = read csv "appointments.csv"

clinics = unique(rows.clinic)
for clinic in clinics:
    clinic_rows = rows where clinic == clinic
    missed = count(clinic_rows where status == "missed")
    total = count(clinic_rows)
    pct = missed / total * 100
    print(clinic + ": " + missed + " missed (" + round(pct, 1) + "%)")

day_counts = count rows grouped by weekday
busiest = max(day_counts)
print("Busiest day: " + busiest.weekday + " with " + busiest.count + " appointments")
