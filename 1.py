import os

now = 0

while True:
    os.system('./a.out ' + str(now) + ' 1 7000 &')
    print(now)
    now += 1
