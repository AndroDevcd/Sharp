# Online Python compiler (interpreter) to run Python online.
# Write Python 3 code in this online editor and run it.
import time

t0 = time.time()

for x in range(10000000):
    y = 2

t1 = time.time()

total = t1-t0
print("done")
print(total)   