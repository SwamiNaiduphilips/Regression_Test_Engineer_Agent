import matplotlib.pyplot as plt

def readSamplesFromFile():
  """
  Reads the "samplesList.txt" file line by line and returns a list of its contents.
  """
  with open("samplesssList.txt", "r") as file:
    count=0
    my_list = []
    for line in file:
      count=count+1
      # Assuming lines contain strings, remove trailing newline and convert if needed
      item = line.strip()
      # Convert item back to its original data type based on your data
      # (e.g., int(), float()) if necessary
      if((count % 10) == 0):
         my_list.append(item)
  return my_list

test_list =readSamplesFromFile()


# Example usage
# samples_list = readSamplesFromFile()



def new_Analysis( samples_list1):
  samples_list =samples_list1
  constant = int (60000/64)
  final_array =[]
  samp_ref= 0
  samp_ref = int((len(samples_list))/70000)
  for i in range(70000):
    element_ref = samples_list[0]
    transition =0
    for j in range(samp_ref):
      if(samples_list[j]!=element_ref):
        transition = transition+1;
        element_ref=samples_list[j]
    final_array.append(transition * constant)
    del samples_list[:samp_ref] 
  # print(final_array)
  plt.plot(final_array)
  plt.xlabel("Time in milli seconds")
  plt.ylabel("RPM")
  plt.grid(True)

  current_length = 0;
  max_lengths=[]
  count =0
  print(len(final_array))
  for i in range(len(final_array)):
    if final_array[i]>4000 :
      del final_array[i]
    else:
      break
  start_index =0
  print(len(final_array))
  for i in range(len(final_array)):
    if final_array[i] <=3900:
      current_length +=1   
    else:
      if current_length>0:
        if current_length>40:
         max_lengths.append([i,current_length])
      current_length =0
  time_periods=[]
  time_periods.append([max_lengths[0][1],0])
  for i in range(1,len(max_lengths)):
    #print(max_lengths[i-1][0],max_lengths[i][0]-max_lengths[i][1])
    subset = final_array[max_lengths[i-1][0]+100:max_lengths[i][0]-max_lengths[i][1]-100]
    time_periods.append([max_lengths[i][0]-max_lengths[i][1]-max_lengths[i-1][0],sum(subset)/len(subset)])
    time_periods.append([max_lengths[i][1],0])
  print(" hello")
  print(time_periods)
  return_string =''
  for item in time_periods:
    return_string+=str(item[0]) + "|" + str(int(item[1])) + ";"
  return (return_string)
  plt.show()

# max_lengths[i][0]-max_lengths[i][1]-100
new_Analysis(test_list)