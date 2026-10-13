while true:
    name = input("Object name: ")
    mass = float(input("Mass (g): "))
    volume = float(input("Volume (cm^3): "))
    density = round(mass / volume, 2)
    if density < 1:
        result = "floats"
    else:
        result = "sinks"
    print(f"{name}: density = {density} g/cm^3, it {result}")
    again = input("Another object? ")
    if again.lower() == "no":
        break
