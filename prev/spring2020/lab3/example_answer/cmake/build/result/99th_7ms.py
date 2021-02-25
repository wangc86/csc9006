import seaborn as sns
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.lines as lines

import scipy as sp
import scipy.stats

# this function is from https://stackoverflow.com/questions/15033511/compute-a-confidence-interval-from-sample-data
def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t.ppf((1+confidence)/2., n-1)
    return m, h

# References for appropriate color schemes:
# https://davidmathlogic.com/colorblind/
# https://colorbrewer2.org
majorColor='#dc267f'
minorColor='#648fff'
minorColor2='#ffb000'

#import matplotlib as mpl
#print mpl.__version__

sns.set_style('ticks')
sns.set_context('paper',rc={'font.size':7,
                            'axes.titlesize':7,
                            'axes.labelsize':7,
                            'xtick.labelsize':7,
                            'ytick.labelsize':7,
                            'legend.fontsize':7})#,
#                            'lines.linewidth':2})

fig1, ax1 = plt.subplots()

exp_path = './'

s = ['FIFO', 'RM', 'EDF']
m = ['d', 'x', 'o']
c = [minorColor2, minorColor, majorColor]
l = [':', '--', '-']
for x in range(0,3,1):
    nOut = []
    nOut_err = []
    for i in range(20,101,20):
        nAvg = []
        for j in range(1,11,1):
            meta = np.loadtxt(exp_path + s[x] + '/' +
                    s[x] + '-config' + str(i) + '-' +
                    str(j) + '/i1') # i0:5ms; i1:7ms; i2:9ms
            a = np.sort(meta)
            nAvg.append(a[int(len(a)*0.99)])
        y, yerr = mean_confidence_interval(nAvg)
        nOut.append(y)
        nOut_err.append(yerr)
    ax1.errorbar([1.5, 4.5, 7.5, 10.5, 13.5], nOut,
      yerr=nOut_err, marker=m[x], color=c[x], lw=1.7, linestyle=l[x], capthick=0.7, capsize=2, mec=c[x], ms=4, mew=1, mfc='none')

sns.despine()
plt.xlim(0,15)
ax1.set_xticks([1.5, 4.5, 7.5, 10.5, 13.5])
ax1.set_xticklabels(['3', '22', '42', '62', '80'])
ax1.set_ylim(0,10000)

ax1.set_xlabel('Number of middle-rate topics')
ax1.set_ylabel('99th percentile of the\nend-to-end response time (us)')
ax1.set_title('Topics of 7 ms period')

h_EDF = lines.Line2D([],[],linewidth=1.7,linestyle='-',color=majorColor,marker='o', lw=0.7, mec=majorColor, ms=4, mew=1, mfc='none')
h_RM = lines.Line2D([],[],linewidth=1.7,linestyle='--',color=minorColor,marker='x', lw=0.7, mec=minorColor, ms=4, mew=1, mfc='none')
h_FIFO = lines.Line2D([],[],linewidth=1.7,linestyle=':',color=minorColor2,marker='d', lw=0.7, mec=minorColor2, ms=4, mew=1, mfc='none')

plt.legend((h_EDF,h_RM,h_FIFO),('EDF','RM','FIFO'),loc=2)#,
#            borderaxespad=0.1)
#plt.tight_layout()
plt.show()
