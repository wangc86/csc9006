import numpy as np
import matplotlib.pyplot as plt

rt1 = np.loadtxt('./resp_t1.out')
rt2 = np.loadtxt('./resp_t2.out')

fig1, ax1 = plt.subplots()

n = np.arange(1,len(rt1)+1) / np.float(len(rt1)) * 100
rtSorted = np.sort(rt1)
ax1.step(rtSorted, n, color='k', label='t1')

n = np.arange(1,len(rt2)+1) / np.float(len(rt2)) * 100
rtSorted = np.sort(rt2)
ax1.step(rtSorted, n, color='r', lw=3, label='t2')

ax1.set_xlabel('Response Time (milliseconds)')
ax1.set_ylabel('Percentage (%)')
plt.xlim(xmin=0)

plt.legend(loc='best')
plt.tight_layout()
plt.show()
