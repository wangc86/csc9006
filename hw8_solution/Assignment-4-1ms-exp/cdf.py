import numpy as np
import matplotlib.pyplot as plt

s = [[],[],[],[],[]]

for i in range(5):
    s[i] = np.loadtxt('./s'+str(i)+'.out')

fig1, ax1 = plt.subplots()

# make dataset size identical, for statistics
size = min(len(s[0]),len(s[1]),len(s[2]),len(s[3]),len(s[4]))
for i in range(5):
    s[i] = (s[i])[:size]
 
c = ['r', 'g', 'b', 'k', 'y']

for i in range(5):
    n = np.arange(1,len(s[i])+1) / np.float(len(s[i])) * 100
    ax1.step(np.sort(s[i]), n, color=c[i], label='source '+str(i))

ax1.set_xlabel('End-to-end latency (ms)')
ax1.set_ylabel('Percentage (%)')
plt.xlim(xmin=-0.5)

plt.legend(loc='best')
plt.tight_layout()
plt.show()
