
import sys, subprocess

listFileName = sys.argv[1]
outPath = sys.argv[2]
verbosity = 0

inPath = "/afs/cern.ch/work/v/vkhriste/Projects/DRS4/data_out/IRRAD_12122014_1/"
listFile = open(listFileName)
for line in listFile:
	v = line.split()
	fileName1 = v[0]
	fileName2 = v[1]
	folderName = v[2]

	inPathName1 = "%s%s" % (inPath, fileName1)
	inPathName2 = "%s%s" % (inPath, fileName2)
	outPathName = "%s/Compare_%s_%s.root" % (outPath, fileName1, fileName2)
	fullFolderName = "%s/%s" % (outPath, folderName)

	cmd = "mkdir %s" % fullFolderName
	subprocess.call(cmd, shell=True)

	print "Comparing %s %s" % (fileName1, fileName2)
	print "Saving Plots to Folder: %s" % fullFolderName
	cmd = "./analyzeNtuples %s %s %s %s %d" % (inPathName1, inPathName2,
			outPathName, fullFolderName, verbosity)
	subprocess.call(cmd, shell=True)
	












