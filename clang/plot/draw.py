import matplotlib.pyplot as plt
import numpy as np
import data as dt

def cdf_xy(data):
  x, y = sorted(data[1:]), np.arange(len(data[1:])) / (len(data[1:]))
  return x, y

def plot_cdf(ax, data, label):
  ax.plot(data, linewidth=1.5, label=label)
  return 0

def plot_box(ax, data):
  ax.boxplot(data)
  ax.set_xlabel('The Number of Users')
  ax.set_ylabel('Frame Latency (ms)')
  return 0


naive = dt.read_csv('naive.csv')
source = dt.read_csv('source.csv')
user = dt.read_csv('user.csv')

draw_data = user

fig1, ax1 = plt.subplots(figsize=(5, 4))
ax1.boxplot(draw_data)
ax1.set_ylim(15, 60)
ax1.grid(True)

'''
fig1, (ax1, ax2) = plt.subplots(figsize=(5, 4), nrows=2, ncols=1, sharex=True)
ax = fig1.add_subplot(111, frameon=False)
ax.spines['top'].set_color('none')
ax.spines['bottom'].set_color('none')
ax.spines['left'].set_color('none')
ax.spines['right'].set_color('none')
ax.tick_params(
    axis='both',          # changes apply to the x-axis
    which='both',      # both major and minor ticks are affected
    bottom=False,      # ticks along the bottom edge are off
    top=False,         # ticks along the top edge are off
    labelbottom=False,
    labelleft=False) # labels along the bottom edge are off

ax1.boxplot(draw_data)
ax1.set_ylim(50, 300)
ax1.spines['bottom'].set_visible(False)
#ax1.tick_params(labeltop='off')  # don't put tick labels at the top
ax1.set_xticklabels([])

ax2.boxplot(draw_data)
ax2.set_ylim(15, 40)
ax2.spines['top'].set_visible(False)
ax2.xaxis.tick_bottom()
ax2.set_xticklabels(['1', '3', '5', '7', '9'])

d = .005
kwargs = dict(transform=ax1.transAxes, color='k', clip_on=False)
ax1.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
ax1.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
ax2.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
ax2.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal
ax1.grid(True)
ax2.grid(True)
plt.xlabel('The Number of Users', labelpad=20)
plt.ylabel('Frame Latency (ms)', labelpad=30)
'''
plt.xlabel('The Number of Users')
plt.ylabel('Frame Latency (ms)')
plt.xticks([1, 2, 3, 4, 5], ['1', '3', '5', '7', '9'])
plt.show()

