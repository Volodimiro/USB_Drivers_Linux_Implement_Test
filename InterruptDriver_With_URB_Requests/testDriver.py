#!/usr/bin/python3
import sys

#L = "\x02" + sys.argv[1]
#L = "\x02\x00"
#print(L)
 #["Geeks\n", "for\n", "Geeks\n"]
 
# writing to file
#file1 = open('/dev/usbTest0', 'w')
#file1.writelines(L)
#file1.close()
 
# Using readlines()
file1 = open('/dev/usbTest0', 'r')

#Lines = file1.readlines()
print(file1.readlines()) 
count = 0
# Strips the newline character
#for line in Lines:
#    count += 1
#    print("Line{}: {}".format(count, line.strip()))