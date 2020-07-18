# Read the documentation:
# https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.boxplot.html

# Note that we do not need to always use the seaborn library
import numpy as np
import matplotlib.pyplot as plt

fig1, ax = plt.subplots()
exp_path = '../makeup_exp/'

fromPtoB, withinB, fromBtoS = \
  np.loadtxt(exp_path + 'latency/out2',
    delimiter=' ', usecols=(0,1,2), unpack=True)/1000.0

e2e = fromPtoB + withinB + fromBtoS

# Now, plot the box and wisker plot 
ax.boxplot([fromPtoB,e2e], labels=['from Publisher to Broker','from Publisher to Subscriber'], showfliers=False)

# It is necessary to adjust the tick range of the y axis
# so that
#   (1) the highest tick is higher than the largest data value, and
#   (2) the smallest data value is visible
ax.set_ylim(-3,50)

ax.set_xlabel('Data Type')
ax.set_ylabel('Latency (seconds)')
ax.set_title('The Box and Whisker Plot')

plt.show()
#plt.savefig('./boxplot.pdf')
