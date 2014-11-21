




#include <signal.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sstream>
#include <vector>
#include <math.h>
#include <string>

#include "TROOT.h"
#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"

using namespace std;
using namespace ROOT;

bool exitSignal = false;

#define NUMCHS 4
#define NUMPOINTSPEREVENT 1024
#define NUMDATA 2

struct TDataContainer
{
	int ipointMax[NUMCHS];
	double data[NUMCHS][NUMPOINTSPEREVENT][NUMDATA];
};

struct TIn
{
	string fileName;
	ifstream xmlFile;
};

struct TOut
{
	string fileName;
	TFile *rootFile;
	TH1D *hSignal[NUMCHS];
	TH1D *hSignal_g50[NUMCHS];
	TH1D *hSignal_g40[NUMCHS];
	TH2D *hSignal_2D[NUMCHS];
	TH2D *hSignal_2D_g50[NUMCHS];
	TH2D *hSignal_2D_g40[NUMCHS];
};

struct TService
{
	int verbosity;
	int maxEvents;
	ofstream logFile;
	bool activeChs[NUMCHS];
};

int processFile(TIn&, TOut&, TService&);
int convert2Root(TOut&, TDataContainer, TService&);
int init(TOut&);
int init(TIn&);
void sigHandler(int sig);

//
//	Main Entry point
//
int main(int argc, char **argv)
{
	signal(SIGABRT, &sigHandler);
	signal(SIGTERM, &sigHandler);
	signal(SIGINT, &sigHandler);

	//	Input
	string inFileName = argv[1];
	string outFileName = argv[2];
	int maxEvents = atoi(argv[3]);
	int verbosity = atoi(argv[4]);

	//	Arrange input
	TIn input;
	input.fileName = inFileName;
	TOut output;
	output.fileName = outFileName;
	TService service;
	service.verbosity = verbosity;
	service.maxEvents = maxEvents;
	service.logFile.open("log.log");
	for (int i=0; i<NUMCHS; i++)
		service.activeChs[i] = false;

	//	Main processing
	init(output);
	init(input);
	processFile(input, output, service);

	output.rootFile->Write();
	output.rootFile->Close();

	return 0;
}

//
//	Initialize Output
//	and input
//
int init(TOut &out)
{
	out.rootFile = new TFile(out.fileName.c_str(), "recreate");
	char histName[200];
	for (int i=0; i<NUMCHS; i++)
	{
		sprintf(histName, "Signal_Ch%d", i+1);
		out.hSignal[i] = new TH1D(histName, histName, 1000, 0, 400);
		sprintf(histName, "SignalvsTime_Ch%d", i+1);
		out.hSignal_2D[i] = new TH2D(histName, histName, 1030, 0, 1030,
				1000, 0, 400);

		sprintf(histName, "Signal_g40_Ch%d", i+1);
		out.hSignal_g40[i] = new TH1D(histName, histName, 1000, 0, 400);

		sprintf(histName, "Signal_g50_Ch%d", i+1);
		out.hSignal_g50[i] = new TH1D(histName, histName, 1000, 0, 400);

		sprintf(histName, "SignalvsTime_g40_Ch%d", i+1);
		out.hSignal_2D_g40[i] = new TH2D(histName, histName, 1030, 0, 1030,
				1000, 0, 400);

		sprintf(histName, "SignalvsTime_g50_Ch%d", i+1);
		out.hSignal_2D_g50[i] = new TH2D(histName, histName, 1030, 0, 1030,
				1000, 0, 400);
	}

	cout << "### Initializing Ouput: " << out.fileName << endl;

	return 0;
}

int init(TIn &in)
{
	in.xmlFile.open(in.fileName.c_str());
	cout << "### Reading in XML File: " << in.fileName << endl;

	return 0;
}

//
//	Convert Current Event Data to Root
//
int convert2Root(TOut &out, TDataContainer data, TService &service)
{
	for (int ich=0; ich<NUMCHS; ich++)
	{
		if (service.activeChs[ich]==false)
			continue;
		for (int i=0; i<NUMPOINTSPEREVENT; i++)
		{
			double time = data.data[ich][i][0];
			double x = data.data[ich][i][1];
			int ipointMax = data.ipointMax[ich];
			double maxTime = data.data[ich][ipointMax][0];
			double maxX = data.data[ich][ipointMax][1];

			if (service.verbosity>1)
				service.logFile << ich+1 << "  " << time << "  " << x << endl;
			out.hSignal[ich]->Fill(x);
			out.hSignal_2D[ich]->Fill(time, x);

			if (maxX>40.)
			{
				out.hSignal_g40[ich]->Fill(x);
				out.hSignal_2D_g40[ich]->Fill(time, x);
			}
			if (maxX>50.)
			{
				out.hSignal_g50[ich]->Fill(x);
				out.hSignal_2D_g50[ich]->Fill(time, x);
			}
		}
	}

	return 0;
}

//
//	Main Processing
//
int processFile(TIn &in, TOut &out, TService &service)
{
	int lineCounter = 0;
	string str;
	//	Always skip the first 3 lines
	while(in.xmlFile>>str && lineCounter<3)
		lineCounter++;

	lineCounter=0;
	bool collectData = false;
	TDataContainer data;
	int iEvent(0), currentCh(0), ipoint(0);
	double maxAmp[NUMCHS]; 
	while (in.xmlFile>>str && exitSignal==false)
	{
		if (service.verbosity>5)
			cout << str << endl;

		//	Look for Event tags
		if (str=="<Event>")
		{
			collectData = true;
			currentCh = 0;
			ipoint = 0;
			for (int i=0; i<NUMCHS; i++)
				maxAmp[i] = 0;
			continue;
		}
		else if (str=="</Event>")
		{
			collectData = false;

			if (iEvent%1000==0)
				cout << "### Converting to Root Event " << iEvent << endl;
			convert2Root(out, data, service);
			continue;
		}

		//
		//	Main Data Parsing
		//
		if (collectData==true)
		{
			size_t pos_bra = str.find('<');
			size_t pos_ket = str.find('>');
			string stag = str.substr(pos_bra, pos_ket+1);

			if (service.verbosity>4)
				service.logFile << stag << "  " << pos_bra << "  " << pos_ket << "  "
					<< str << endl;

			if (stag=="<Serial>")
			{
				sscanf(str.c_str(), "<Serial>%d</Serial>",
						&iEvent);
				if (service.maxEvents<iEvent)
				{
					cout 
						<< "### Exceeded Max Number of Events to process. Exiting..."
						<< endl;
					break;
				}
				
				if (iEvent%1000==0)
					cout << "### Parsing Event " << iEvent << endl;
			}
			if (stag=="<Time>")
			{
				continue;
			}
			if (stag=="<HUnit>")
			{
				continue;
			}
			if (stag=="<VUnit>")
			{
				continue;
			}
			if (stag=="<Trigger_Cell>")
			{
				continue;
			}
			if (stag=="<CHN1>")
			{
				currentCh = 1;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN2>")
			{
				currentCh = 2;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN3>")
			{
				currentCh = 3;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN4>")
			{
				currentCh = 4;
				service.activeChs[currentCh-1] = true;
			}
			if (stag=="<Data>")
			{
				//	Extract the data: time,x
				size_t pos_comma = str.find(',');
				size_t pos_2bra = str.find('<', 3);
				string stime = str.substr(6, pos_comma-6);
				string sx = str.substr(pos_comma+1, 
						(pos_2bra-1)-(pos_comma+1)+1);
				float time = atof(stime.c_str());
				float x = atof(sx.c_str());

				x = fabs(x);
				if (currentCh>0)
				{
					data.data[currentCh-1][ipoint][0] = time;
					data.data[currentCh-1][ipoint][1] = x;
					if (x>maxAmp[currentCh-1])
					{
						data.ipointMax[currentCh-1] = ipoint;
						maxAmp[currentCh-1] = x;
					}
					ipoint++;
					if (service.verbosity>1)
						service.logFile << str << "  "
							<< currentCh << "  " << stime << "  " << time 
							<< "  " << sx << "  " << x
							<< endl;
				}
				else
					cout << "ERROR: currentCh is 0" << endl;
			}
		}
	}

	return 0;
}

//
//	Signal Handler
//
void sigHandler(int sig)
{
	cout << "### Signal: " << sig << "caught. Exiting..." << endl;
	exitSignal = true;
}

















