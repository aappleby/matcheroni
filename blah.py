import os

reps = 1000000

for i in range(reps):
  #print(os.path.getmtime("blah.py"))
  os.path.getmtime("blah.py")
  pass

