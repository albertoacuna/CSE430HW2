 #!/usr/bin/python

import random
 
size = 10
file = open('matrix1_10.txt','w')
for i in range(size):
    for j in range(size):
	num = random.randint(1,10);
	file.write('%d ' % num);
    file.write('\n');
file.close()
