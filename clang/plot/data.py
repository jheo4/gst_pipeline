import pandas as pd
import numpy as np

def read_csv(file_name):
  colnames = ['one', 'three', 'five', 'seven', 'nine']
  data = pd.read_csv(file_name, names=colnames)

  one = data.one.tolist()
  three = data.three.tolist()
  five = data.five.tolist()
  seven = data.seven.tolist()
  nine = data.nine.tolist()

  colums = ['1', '3', '5', '7', '9']

  one = [x for x in one[1:] if str(x) != 'nan' ]
  three = [x for x in three[1:] if str(x) != 'nan' ]
  five = [x for x in five[1:] if str(x) != 'nan' ]
  seven = [x for x in seven[1:] if str(x) != 'nan' ]
  nine = [x for x in nine[1:] if str(x) != 'nan' ]

  one = np.array(one).astype(np.float)
  three = np.array(three).astype(np.float)
  five = np.array(five).astype(np.float)
  seven = np.array(seven).astype(np.float)
  nine = np.array(nine).astype(np.float)
  return [one, three, five, seven, nine]
