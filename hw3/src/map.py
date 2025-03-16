import sys
encoding ='big5-hkscs'
f = open(sys.argv[1],'r', encoding=encoding)  
mapping = {}
line = f.readline()
time = 0
while line:
    arr = line.split(' ')
    Chinese = arr[0]
    ZhuYin = arr[1]
    zhuyins = ZhuYin.split('/')
    for z in zhuyins:
        if z[0] not in mapping:
            mapping[z[0]] = []
            if Chinese not in mapping[z[0]]:
                mapping[z[0]].append(Chinese)
        else:
            if Chinese not in mapping[z[0]]:
                mapping[z[0]].append(Chinese)
    if Chinese not in mapping:
        mapping[Chinese] = []
        mapping[Chinese].append(Chinese)
    line = f.readline()

fw = open(sys.argv[2],'w', encoding=encoding) 
num0 = len(mapping)
#print("num=")
#print(num0)
for key in mapping:
    line = key
    line += "\t"
    num = len(mapping[key])
    for i in range(num):
        if i != num-1:
            line += mapping[key][i]
            line += " "
        else:
            line += mapping[key][i]
    line += "\n"
    fw.write(line)

f.close()
fw.close()
