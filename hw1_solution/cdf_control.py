import numpy as np
import matplotlib.pyplot as plt

rt = np.loadtxt('./resp.out.100')

fig1, ax1 = plt.subplots()

n = np.arange(1,len(rt)+1) / np.float(len(rt)) * 100
rtSorted = np.sort(rt)
ax1.step(rtSorted, n, color='k', label='timer period = 100 ms')

ax1.set_xlabel('Response Time (milliseconds)')
ax1.set_ylabel('Percentage (%)')

plt.legend(loc=4)
plt.tight_layout()
plt.show()
