 #!/usr/bin/python

import random
import sys

size = int(sys.argv[1])

fileName= 'input/matrix1_' + str(size) + '.txt'

file = open(fileName,'w')
for i in range(size):
    for j in range(size):
	num = random.randint(1,10);
	file.write('%d ' % num);
    file.write('\n');
file.close()

fileName= 'matrix2_' + str(size) + '.txt'
file = open(fileName,'w')
for i in range(size):
    for j in range(size):
	num = random.randint(1,10);
	file.write('%d ' % num);
    file.write('\n');
file.close()
