


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
#include "TCanvas.h"

using namespace std;
using namespace ROOT;

bool exitSignal = false;

#define NUMCHS 2

struct TIn
{
	string fileName;
	TFile *rootFile;
	TTree *tEvents[NUMCHS];
	double charge[NUMCHS];
};

struct TOut
{
	string fileName;
	string folderName;
	TFile *rootFile;

	TH1D *hRatioQRef;
	TH1D *hRatioQData_NoRef;
	TH1D *hRatioQData_wRef;
};

struct TService
{
	int verbosity;
	int maxEvents;
	ofstream logFile;
	bool activeChs[NUMCHS];
};

void sigHandler(int sig);
int init(TOut&);
int init(TIn&);
int compare(TIn&, TIn&, TOut&, TService&);
int savePlots(TOut&);

//
//	Main Entry Point
//
int main(int argc, char **argv)
{
	signal(SIGABRT, &sigHandler);
	signal(SIGTERM, &sigHandler);
	signal(SIGINT, &sigHandler);

	string inFileName1 = argv[1];
	string inFileName2 = argv[2];
	string outFileName = argv[3];
	string folderName = argv[4];
	int verbosity = atoi(argv[5]);


	TIn in1;
	TIn in2;
	TOut out;
	TService service;

	in1.fileName = inFileName1;
	in2.fileName = inFileName2;
	out.fileName = outFileName;
	out.folderName = folderName;
	service.logFile.open("log.log");
	service.verbosity = verbosity;

	init(in1);
	init(in2);
	init(out);
	compare(in1, in2, out, service);

	savePlots(out);
	out.rootFile->Write();
	out.rootFile->Close();
	
	return 0;
}

int savePlots(TOut &out)
{
	cout << "### Saving in folder: " << out.folderName << endl;
	TCanvas *c = new TCanvas("Ratios", "Ratios", 200, 10, 1000, 800);
	c->Divide(1,2);
	c->cd(1);
	char fullPathName[200];

	out.hRatioQRef->Draw();
	c->cd(2);
	out.hRatioQData_wRef->Draw();

	sprintf(fullPathName, "%s/plots.png", out.folderName.c_str());
	c->SaveAs(fullPathName);

	return 0;
}

int init(TOut& out)
{
	out.rootFile = new TFile(out.fileName.c_str(), "recreate");
	out.hRatioQRef = new TH1D("RatioQRef", "Ratio for QRef", 1000, 0, 2);
	out.hRatioQData_NoRef = new TH1D("RatioQData_NoRef", 
			"Ratio for QData w/o Ref", 1000, 0, 2);
	out.hRatioQData_wRef = new TH1D("RatioQData_wRef", 
			"Ratio for QData w/ Ref", 1000, 0, 2);

	return 0;
}

int init(TIn &in)
{
	in.rootFile = new TFile(in.fileName.c_str());
	
	char dirName[200];
	for (int i=0; i<NUMCHS; i++)
	{
		sprintf(dirName, "Ch%d/EventsInfo", i+1);
		in.rootFile->cd(dirName);
		in.tEvents[i] = (TTree*)gDirectory->Get("Events");
		in.tEvents[i]->SetBranchAddress("charge", &(in.charge[i]));
	}

	return 0;
}

int compare(TIn &in1, TIn &in2, TOut &out, TService &service)
{
	for (int iE=0; iE<in1.tEvents[0]->GetEntries(); iE++)
	{
		if (iE%1000==0)
			cout << "### Processing Event " << iE << endl;

		in1.tEvents[0]->GetEntry(iE);
		in1.tEvents[1]->GetEntry(iE);
		in2.tEvents[0]->GetEntry(iE);
		in2.tEvents[1]->GetEntry(iE);

		double q1_Ref = in1.charge[0];
		double q1_Data = in1.charge[1];
		double q2_Ref = in2.charge[0];
		double q2_Data = in2.charge[1];

		double ratio_QRef = q1_Ref/q2_Ref;
		out.hRatioQRef->Fill(ratio_QRef);
		double ratio_QData_NoRef = q1_Data/q2_Data;
		out.hRatioQData_NoRef->Fill(ratio_QData_NoRef);
		double ratio_QData_wRef = ratio_QData_NoRef/ratio_QRef;
		out.hRatioQData_wRef->Fill(ratio_QData_wRef);
	}

	return 0;
}

void sigHandler(int sig)
{
	cout << "### Signal: " << sig << " caught. Exiting..." << endl;
	exitSignal = true;
}









