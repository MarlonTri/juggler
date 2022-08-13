import pyperclip
import matplotlib.pyplot as plt
from pprint import pprint

s=pyperclip.paste()

s = [x for x in s.split("\n") if "Progress" in x]
s=s[1:]
t = [x.split() for x in s]
t = [(int(x),float(y[1:])) for _,x,_,y,_ in t]

x = [z[0] for z in t]
y = [z[1] for z in t]
plt.scatter(x,y)
plt.show()



x = [z[0] for z in t]
y = [y[i]-z for i,z in enumerate([0] + y[:-1])]
plt.scatter(x,y)
plt.show()
