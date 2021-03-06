




#include <signal.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <iomanip>
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
#define RESCONVFACTOR 20

struct TDataContainer
{
	int ipointMax[NUMCHS];
	double eventTime;
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
	TTree *tEvents[NUMCHS];
	double charge[NUMCHS];

	TH1D *hSignal[NUMCHS];
	TH1D *hSignal_g50[NUMCHS];
	TH1D *hSignal_g40[NUMCHS];
	TH2D *hSignal_2D[NUMCHS];
	TH2D *hSignal_2D_g50[NUMCHS];
	TH2D *hSignal_2D_g40[NUMCHS];

	TH1D *hSigInt[NUMCHS];
	TH1D *hSigInt_g40[NUMCHS];
	TH1D *hSigInt_g50[NUMCHS];

	TH1D *hdT[NUMCHS];
	TH1D *hSigWidth[NUMCHS];
	TH1D *hCharge[NUMCHS];
	TH1D *hiCellTMin[NUMCHS];
	TH1D *hiCellTMax[NUMCHS];
	TH1D *hiCellMaxAmp[NUMCHS];

	TH1D *hEventTime;
	
};

struct TService
{
	int verbosity;
	int maxEvents;
	ofstream logFile;
	ofstream txtFile;
	bool activeChs[NUMCHS];
};

int processFile(TIn&, TOut&, TService&);
int convert2Root(TOut&, TDataContainer, TService&);
int computeQ(TOut&, TDataContainer, TService&);
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
	service.txtFile.open("log.txt");
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
	out.hEventTime = new TH1D("Event Time", "Event Time",
			10000, 56500, 60000);

	char dirName[200];
	for (int i=0; i<NUMCHS; i++)
	{
		sprintf(dirName, "Ch%d", i+1);
		out.rootFile->mkdir(dirName);
		out.rootFile->cd(dirName);
		gDirectory->mkdir("EventsInfo");
	}

	char histName[200];
	for (int i=0; i<NUMCHS; i++)
	{
		sprintf(dirName, "Ch%d", i+1);
		out.rootFile->cd(dirName);
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

		sprintf(histName, "SigIntegral_Ch%d", i+1);
		out.hSigInt[i] = new TH1D(histName, histName, 1000, 0, 5000);
		sprintf(histName, "SigIntegral_g40_Ch%d", i+1);
		out.hSigInt_g40[i] = new TH1D(histName, histName, 1000, 0, 5000);
		sprintf(histName, "SigIntegral_g50_Ch%d", i+1);
		out.hSigInt_g50[i] = new TH1D(histName, histName, 1000, 0, 5000);

		sprintf(histName, "dT");
		out.hdT[i] = new TH1D(histName, histName, 10000,0, 100);

		sprintf(histName, "SigWidth_Ch%d", i+1);
		out.hSigWidth[i] = new TH1D(histName, histName, 1000, 0, 100);
		sprintf(histName, "Charge_Ch%d", i+1);
		out.hCharge[i] = new TH1D(histName, histName, 10000, 0, 200000);

		sprintf(histName, "iCellTMin_Ch%d", i+1);
		out.hiCellTMin[i] = new TH1D(histName, histName, 1024, 0, 1024);
		sprintf(histName, "iCellTMax_Ch%d", i+1);
		out.hiCellTMax[i] = new TH1D(histName, histName, 1024, 0, 1024);
		sprintf(histName, "iCellMaxAmp_Ch%d", i+1);
		out.hiCellMaxAmp[i] = new TH1D(histName, histName, 1024, 0, 1024);

		gDirectory->cd("EventsInfo");
		out.tEvents[i] = new TTree("Events", "Events");
		out.tEvents[i]->Branch("charge", &(out.charge[i]), "charge/D");
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
//	Compute Charge for enabled channels
//	Consider subtracting the background
//
int computeQ(TOut &out, TDataContainer data, TService &service)
{
	for (int ich=0; ich<NUMCHS; ich++)
	{
		if (service.activeChs[ich]==false)
			continue;

		//
		//	Calculate the Pedestal here
		//
		double sumVdt(0), deltaT(0);
		for (int i=0; i<300; i++)
		{
			double v1 = data.data[ich][i][1];
			double v2 = data.data[ich][i+1][1];
			double t1 = data.data[ich][i][0];
			double t2 = data.data[ich][i+1][0];

			double dt = t2-t1; deltaT+=dt;
			double avgV = 0.5*(v2+v1);
			sumVdt += avgV*dt;
		}
		//	Here is the pedestal in VOltage units
		double pedV = sumVdt/deltaT;

		int ipointMax = data.ipointMax[ich];
		out.hiCellMaxAmp[ich]->Fill(ipointMax);
		double maxAmp = data.data[ich][ipointMax][1] - pedV;

		double ratio(1.);
		int i(ipointMax);
		while(ratio>0.05)
		{
			i--;
			double cA = data.data[ich][i][1]-pedV;
			ratio = cA/maxAmp;
		}
		//	Now, i will the point where the current voltage
		//	amplitude is about 5% of the max
		int itmin = i;
		out.hiCellTMin[ich]->Fill(itmin);


		i=ipointMax; ratio = 1.;
		while(ratio>0.05)
		{
			i++;
			double cA = data.data[ich][i][1] - pedV;
			ratio = cA/maxAmp;
		}
		//	Now, we have the point where the current voltage 
		//	amplitude is about 5% of the max
		int itmax = i;		
		out.hiCellTMax[ich]->Fill(itmax);

		//
		//	Now, let's compute the Charge
		//	or Integral really
		//
		double charge(0);
		sumVdt = 0;
		double width(0);
		for (int j=itmin; j<itmax; j++)
		{
			double v1 = data.data[ich][j][1] - pedV;
			double v2 = data.data[ich][j+1][1] - pedV;
			double t1 = data.data[ich][j][0];
			double t2 = data.data[ich][j+1][0];

			double dt = t2-t1;
			double avgV = 0.5*(v2+v1);
			sumVdt += avgV*dt;
			width += dt;
		}
		//	Now, charge = Ohmfact*sumVdt
		charge = RESCONVFACTOR*sumVdt;
		out.charge[ich] = charge;

		out.hSigWidth[ich]->Fill(width);
		out.hCharge[ich]->Fill(charge);
		out.tEvents[ich]->Fill();
	}

	return 0;
}
//	Convert Current Event Data to Root
//
int convert2Root(TOut &out, TDataContainer data, TService &service)
{

	//
	//	Separate Charge Computation from filling some auxillary histos
	//
	computeQ(out, data, service);

	//
	//	Here, I have only sample histos
	//	Actual charge integration is done separately.
	//
	for (int ich=0; ich<NUMCHS; ich++)
	{
		if (service.activeChs[ich]==false)
			continue;

		double sum(0), sum_g40(0), sum_g50(0);
		for (int i=0; i<NUMPOINTSPEREVENT; i++)
		{
			double time = data.data[ich][i][0];
			double x = data.data[ich][i][1];
			int ipointMax = data.ipointMax[ich];
			double maxTime = data.data[ich][ipointMax][0];
			double maxX = data.data[ich][ipointMax][1];

			if (i<(NUMPOINTSPEREVENT-1))
			{
				double t1 = data.data[ich][i][0];
				double t2 = data.data[ich][i+1][0];
				double deltaT = t2-t1;
				out.hdT[ich]->Fill(deltaT);

				if (service.verbosity>1)
					service.logFile << deltaT << endl;
			}

			if (service.verbosity>1)
				service.logFile << ich+1 << "  " << time << "  " << x << endl;
			out.hSignal[ich]->Fill(x);
			out.hSignal_2D[ich]->Fill(i, x);

			if (i>=(ipointMax-50) && i<=(ipointMax+50))
				sum+=x;

			if (maxX>40.)
			{
				out.hSignal_g40[ich]->Fill(x);
				out.hSignal_2D_g40[ich]->Fill(i, x);
				if (i>=(ipointMax-50) && i<=(ipointMax+50))
					sum_g40+=x;
			}
			if (maxX>50.)
			{
				out.hSignal_g50[ich]->Fill(x);
				out.hSignal_2D_g50[ich]->Fill(i, x);
				if (i>=(ipointMax-50) && i<=(ipointMax+50))
					sum_g50+=x;
			}
		}
		out.hSigInt[ich]->Fill(sum);
		out.hSigInt_g40[ich]->Fill(sum_g40);
		out.hSigInt_g50[ich]->Fill(sum_g50);
	}

	//	Accessing time of the event information
	out.hEventTime->Fill(data.eventTime);

	return 0;
}

//
//	Main Processing
//
int processFile(TIn &in, TOut &out, TService &service)
{
	int lineCounter = 0;
	string str;

	cout << "### Start Processing..." << endl;

	lineCounter=0;
	bool collectData = false;
	TDataContainer data;
	int iEvent(0), currentCh(0), ipoint(0);
	double maxAmp[NUMCHS]; 
	while (in.xmlFile>>str && exitSignal==false)
	{
//		size_t pos_bra = str.find('<');
//		str = str.erase(0, pos_bra);
		if (service.verbosity>5)
			service.txtFile << str << endl;

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
				service.logFile << stag << "  " << pos_bra << "  " 
					<< pos_ket << "  "	<< str << endl;

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
				in.xmlFile>>str;
				size_t pos_bra = str.find('<');
				string stime = str.substr(0, pos_bra);
				int secs, msecs;
				int hours, mins;
				sscanf(stime.c_str(), "%d:%d:%d.%d", &hours, &mins, &secs, 
						&msecs);
				data.eventTime = 3600.*hours + 60.*mins + secs + 0.001*msecs;
				if (service.verbosity>1)
					service.logFile << stime << endl
						<< "Event Time: " << setprecision(10)
						<< data.eventTime << endl
						<< "hours:" << hours << " mins:" << mins << " secs:"
						<< secs << " msecs:" << msecs << endl;

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
				ipoint = 0;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN2>")
			{
				currentCh = 2;
				ipoint = 0;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN3>")
			{
				currentCh = 3;
				ipoint = 0;
				service.activeChs[currentCh-1] = true;
			}
			else if (stag=="<CHN4>")
			{
				currentCh = 4;
				ipoint = 0;
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

















