# This example is derived from the following webpage:
# https://matplotlib.org/3.1.1/gallery/statistics/histogram_cumulative.html

# Note that we do not need to always use the seaborn library
import numpy as np
import matplotlib.pyplot as plt

fig1, ax = plt.subplots()
exp_path = '../makeup_exp/'

fromPtoB, withinB, fromBtoS = \
  np.loadtxt(exp_path + 'latency/out2',
    delimiter=' ', usecols=(0,1,2), unpack=True)

## We may also do some pre-processing for each input data,
## for example, divide the data value by 1000.0,
## which may be useful to transform time scale..
#fromPtoB, withinB, fromBtoS = \
#  np.loadtxt(exp_path + 'out',
#    delimiter=' ',usecols=(1,2,3),unpack=True)/1000.0

e2e = fromPtoB + withinB + fromBtoS

# Now, plot the cumulative distributioin function (CDF)
n_bins = len(fromPtoB)-1
n, bins, patches = ax.hist(fromPtoB, n_bins, density=True, histtype='step',
          cumulative=True, label='from Publisher to Broker', color='g')
# the following line is used to remove the last point
patches[0].set_xy(patches[0].get_xy()[:-1])

n2, bins2, patches2 = ax.hist(e2e, n_bins, density=True, histtype='step',
          cumulative=True, label='from Publisher to Subscriber', color='b')
patches2[0].set_xy(patches2[0].get_xy()[:-1])

ax.set_xlabel('Latency (ms)')
ax.set_ylabel('Probability')
ax.set_title('Cumulative Distribution Function')
plt.legend(loc=4)

plt.show()
#plt.savefig('./cdf.pdf')
