import numpy as np
import matplotlib.pyplot as plt

#import matplotlib as mpl
#print mpl.__version__

rt = np.loadtxt('./responseTime_t1rm.out')

fig1, ax1 = plt.subplots()

n = np.arange(1,len(rt)+1) / np.float(len(rt)) * 100
rtSorted = np.sort(rt)
ax1.step(rtSorted, n, color='k', label='task t1rm')

ax1.set_xlabel('Response Time (microseconds)')
ax1.set_ylabel('Percentage (%)')

plt.legend()


plt.tight_layout()
plt.show()
#plt.savefig('./responseTimeCDF.pdf',
#            format='pdf', dpi=1000, bbox_inches='tight')
