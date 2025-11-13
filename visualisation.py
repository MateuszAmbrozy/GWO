import matplotlib.pyplot as plt

points = []
with open("best_route.txt", "r") as f:
    for line in f:
        if line.startswith("#") or not line.strip():
            continue
        parts = line.split()
        if len(parts) < 3:
            continue
        _, x, y = parts
        points.append((float(x), float(y)))

if len(points) < 2:
    print("Brak wystarczajacej liczby punktow do wizualizacji.")
    exit()

xs = [p[0] for p in points]
ys = [p[1] for p in points]
xs.append(xs[0])
ys.append(ys[0])

plt.figure(figsize=(6,6))
plt.plot(xs, ys, marker='o', linestyle='-', color='blue')
plt.title("Najlepsza trasa GWO dla TSP")
plt.xlabel("X")
plt.ylabel("Y")

for i, (x, y) in enumerate(points):
    plt.text(x + 0.1, y + 0.1, str(i), fontsize=9)

plt.grid(True)
plt.tight_layout()
plt.savefig("wykres.png")