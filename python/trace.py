import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import multiprocessing as mp
import random

rs = 2
r = 20
u2 = 3000


def fb(theta, r, rs):
    return r * np.sin(theta) / np.sqrt(1 - rs / r)


def dydx(x, b):
    return 1 / (x * x * np.sqrt(np.power(1 / b, 2) - np.power(1 / x, 2) + rs / np.power(x, 3)))


def rk(x0, x, b):
    total_phi = 0

    y1 = 0

    while x0 > 2:
        h = x0 * x0 / 10000
        "Apply Runge Kutta Formulas to find next value of y"

        k1 = h * dydx(x0, b)
        k2 = h * dydx(x0 - 0.5 * h, b)
        k3 = h * dydx(x0 - h, b)

        # Update next value of y
        y1 = y1 + (1.0 / 6.0) * (k1 + 4 * k2 + k3)

        # Update next value of x
        x0 = x0 - h
        if (np.power(1 / b, 2) - np.power(1 / (x0 - h), 2) + rs / np.power((x0 - h), 3) < 0):
            break

    total_phi += y1

    y2 = 0
    while x0 < x:
        h = x0 * x0 / 10000
        "Apply Runge Kutta Formulas to find next value of y"
        k1 = h * dydx(x0, b)
        k2 = h * dydx(x0 + 0.5 * h, b)
        k3 = h * dydx(x0 + h, b)

        # Update next value of y
        y2 = y2 + (1.0 / 6.0) * (k1 + 4 * k2 + k3)

        # Update next value of x
        x0 = x0 + h
    total_phi += y2

    return total_phi


# Driver method
x0 = 200
y = 0

b = fb(1 / 100 * np.pi, 200, rs)

img = mpimg.imread('LMC_APOD.jpg')

fov_width = 120
fov_height = fov_width / \
             (img.size / (img.size / img[1].size) / 3) * img.size / img[1].size
type(img)


def angle(col, row):
    x_angle = (1280 - np.abs(col)) / 1280 * (fov_width / 2)
    y_angle = (1024 - np.abs(row)) / 1024 * (fov_height / 2)
    return np.radians(np.sqrt(x_angle ** 2 + y_angle ** 2))


i = 0
newrow = 8 * 4
newcol = 8 * 4
sample_gap = int(2048 / newrow)
newsize = np.array([newrow, newcol, 3])
center_coord = np.array([2048 / 2, 2560 / 2])
theta_plot = np.zeros(newsize, dtype=np.uint8)
dphi_plot = np.zeros(newsize, dtype=np.uint8)
b_plot = np.zeros(newsize, dtype=np.uint8)

sample_count = 16

total_process = 1

print("what")

def worker(index):
    newimg = np.zeros(newsize, dtype=np.uint8)

    work = int(newrow / total_process)
    start = index * work
    end = start + work

    for row in range(start, end):
        if index is 0:
            print("Progress:", row / newrow)
        for col in range(0, newcol):
            theta = angle(sample_gap * col, sample_gap * row)
            b = fb(theta, r, rs)
            if b < np.sqrt(27):
                newimg[row][col] = [0, 0, 0]
                continue
            else:
                dphi = rk(20, 300, b) % (2 * np.pi)
                phi = np.pi - dphi
                if phi > np.pi / 3 or phi < -np.pi / 3:
                    newimg[row][col] = [254, 0, 0]
                    continue
                bend_ratio = phi / theta
                newpoint = np.array(
                    center_coord + bend_ratio * ([row * sample_gap, col * sample_gap] - center_coord))
                if newpoint[0] > 2048 or newpoint[1] > 2560:
                    newimg[row][col] = [0, 0, 254]
                    continue

                color = np.zeros([3], dtype=np.float)
                for i in range(0, sample_count):
                    x = int(newpoint[0] + random.randint(-1, 1))
                    y = int(newpoint[1] + random.randint(-1, 1))
                    if x < 0:
                        x = 0
                    if x >= 2048:
                        x = 2047
                    if y < 0:
                        y = 0
                    if y >= 2560:
                        y = 2559
                    color += img[x][y] / sample_count
                newimg[row, col] = color
                # newimg[row, col] = img[int(newpoint[0])][int(newpoint[1])]
    return newimg


if __name__ == "__main__":
    p = mp.Pool(processes=total_process)
    data = p.map(worker, ([i for i in range(0, total_process)]))

    newimg = np.zeros(newsize, dtype=np.uint8)
    for i in range(0, total_process):
        newimg += data[i]

    plt.imshow(newimg)
    plt.show()
