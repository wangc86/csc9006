# This example is derived from the following webpage:
# https://matplotlib.org/3.1.1/gallery/statistics/histogram_cumulative.html

# Note that we do not need to always use the seaborn library
import numpy as np
import matplotlib.pyplot as plt

majorColor='#dc267f'
minorColor='#648fff'
minorColor2='#ffb000'

fig1, ax1 = plt.subplots()
#exp_path = '../makeup_exp/'
exp_path = './'

s = ['FIFO', 'RM', 'EDF']
m = ['d', 'x', 'o']
c = [minorColor2, minorColor, majorColor]
l = [':', '--', '-']
for x in range(0,3,1):
    nOut = []
    nOut_err = []
    meta = np.loadtxt(exp_path + s[x] + '/' + s[x] +
              '-config80-1/i1')
    nOut.append(meta)
    n_bins = len(meta)-1
    n, bins, patches = ax1.hist(meta, n_bins, density=True, histtype='step',
          cumulative=True, label=s[x], color=c[x], lw=1.7, linestyle=l[x])
    # the following line is used to remove the last point
    patches[0].set_xy(patches[0].get_xy()[:-1])

#sns.despine()
#plt.xlim(0,15)
#ax.set_xticklabels(['3', '22', '42', '62', '80'])
#ax.set_xticks([1.5, 4.5, 7.5, 10.5, 13.5])
#ax1.set_ylim(0,105)

ax1.set_xlabel('End-to-end response time (us)')
ax1.set_ylabel('Probability')
ax1.set_title('Topics of 7 ms period\n(around 80% CPU utilization)')

#h_EDF = lines.Line2D([],[],linewidth=1.7,linestyle='-',color=majorColor,marker='o', lw=0.7, mec=majorColor, ms=4, mew=1, mfc='none')
#h_RM = lines.Line2D([],[],linewidth=1.7,linestyle='--',color=minorColor,marker='x', lw=0.7, mec=minorColor, ms=4, mew=1, mfc='none')
#h_FIFO = lines.Line2D([],[],linewidth=1.7,linestyle=':',color=minorColor2,marker='d', lw=0.7, mec=minorColor2, ms=4, mew=1, mfc='none')

#plt.legend((h_EDF,h_RM,h_FIFO),('EDF','RM','FIFO'),loc=2)#,
#            borderaxespad=0.1)
plt.legend(loc=4)#,

plt.show()
#plt.savefig('./cdf.pdf')
