

import sys, subprocess

listFileName = sys.argv[1]
outPath = sys.argv[2]
maxEvents = int(sys.argv[3])
verb = int(sys.argv[4])
listFile = open(listFileName)

#	Each line is a fileName to process
for line in listFile:
	if line=='':
		continue
	v = line.split()

	inPathName = v[0]
	vv = inPathName.split('/')
	fileName2Process = vv[len(vv)-1]
	print "fileName to process: %s" % fileName2Process

	outFileName = "%s.root" % fileName2Process
	outPathName = "%s/%s" % (outPath, outFileName)

	cmd = './raw2ntuple %s %s %d %d' % (inPathName, outPathName,
			maxEvents, verb)
	subprocess.call(cmd, shell=True)






