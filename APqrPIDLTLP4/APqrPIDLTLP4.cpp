/*
			Copyright (C) 2011 Georgia Institute of Technology

			This program is free software: you can redistribute it and/or modify
			it under the terms of the GNU General Public License as published by
			the Free Software Foundation, either version 3 of the License, or
			(at your option) any later version.

			This program is distributed in the hope that it will be useful,
			but WITHOUT ANY WARRANTY; without even the implied warranty of
			MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
			GNU General Public License for more details.

			You should have received a copy of the GNU General Public License
			along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


/*
 Version history.

	APqrPIDLTLP: a module based on APqrPID that now reads a iAP from a file. The code was written (Tim) using APqrPID as the core into which the necesary functions were copied from an APqrLTLP version to obtain file reading funcitonality.
	APqrPIDLTLP2: same as above but now programmed (by Tim) the other way around, using an APqrLTLP version as a template and replacing the AP correction algorithm part by the one used in APqrPID.
	APqrPIDLTLP3 (Tim): this one starts with pacing the cell using the blue channel and starts correction once an AP is successfully evoked.
	APqrLTLP4: so far containes fixes (Balazs), including:
		- the variable 'length' and 'dlength' were used inconsistently in v3, espceially in the sumy, sumx and sumx2 funcitons -> this is just esthetics, not a bug
		- PID, PID_diff and Int get zeroed on 'Modify'. In case error accumulates too much during a bad recording.
		- P, I and D terms are available as state variables
		- Int is only calculated when Vm<blue_Vrev and VLED<5V		*/

#include "APqrPIDLTLP4.h"
#include <math.h>
#include <time.h>
#include <main_window.h>

extern "C" Plugin::Object *createRTXIPlugin(void)
{
	return new APqrPIDLTLP4(); // Change the name of the plug-in here
}

static DefaultGUIModel::variable_t vars[] = {
	{ "Loops", "Number of Times to Loop Data From File", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::UINTEGER, },
	{ "Length (ms)", "Length of Trial is Computed From the Real-Time Period", DefaultGUIModel::STATE, },
	{ "Gain", "Factor to amplify iAP", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Offset", "Factor to offset iAP (mV)", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Pulse_strength (V)", "Blue LED driver voltage (V) for pacing", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "File Name", "ASCII Input File", DefaultGUIModel::COMMENT, },
	{ "Vm (mV)", "Membrane potential (mV)", DefaultGUIModel::INPUT, },
	{ "VLED_blue", "Output for LED driver", DefaultGUIModel::OUTPUT, },
	{ "VLED_red", "Output for LED driver", DefaultGUIModel::OUTPUT, },
	{ "iAP", "ideal AP", DefaultGUIModel::STATE, },
	{ "V_light_on (mV)", "Threshold potential for when the pulse can be given to the cells", 
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "V_cutoff (mV)", "Threshold potential for the detection of the beginning of an AP, together with Slope_thresh", 
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Slope_thresh (mV/ms)", "SLope threshold that defines the beginning of the AP (mV/ms)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Rm_blue (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_red (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Correction start", "iAP count (index+1) when correction starts",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Blue_Vrev", "Apparent reversal potential of the 'blue' ChR current",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "K_p", "Scale factor for the proportional part of the PID",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "K_i", "Scale factor for the integral part of the PID",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "K_d", "Scale factor for the derivative part of the PID",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "dlength", "Amount of points that need to be taken into account to find the derivative (slope of the linear trend line of these points)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "PID_tresh", "treshold value under which the same output as before gets repeated",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "min_PID", "value under which the lights get switched off",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Vm2 (mV)", "Membrane potential (mV)", DefaultGUIModel::STATE, },
	{ "P", "P term", DefaultGUIModel::STATE, },
	{ "I", "I term", DefaultGUIModel::STATE, },
	{ "D", "D term", DefaultGUIModel::STATE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, 
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, },
	{ "PID", "PID", DefaultGUIModel::STATE, },
	{ "act", "act", DefaultGUIModel::STATE, },
	{ "idx", "idx", DefaultGUIModel::STATE, },	
	{ "idx2", "idx2", DefaultGUIModel::STATE, },
	{ "modulo_state", "number", DefaultGUIModel::STATE, },
	{ "Vm_V", "Vm_V", DefaultGUIModel::STATE, },
	{ "iAP_V", "iAP_V", DefaultGUIModel::STATE, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

APqrPIDLTLP4::APqrPIDLTLP4(void) : DefaultGUIModel("APqr PIDLTLP4", ::vars, ::num_vars)
{
	setWhatsThis("This module loads data from an ASCII formatted file. It samples one value from the the file on every time step and creates and generates an output signal that powers an LED. This LED activates light-gated ion channels in cardiomyocytes with the aim of regulating their memrane potentials. The module computes the time length of the waveform based on the current real-time period.");

	initParameters();
	DefaultGUIModel::createGUI(vars, num_vars);
	customizeGUI();
	update(INIT);
	refresh();
	QTimer::singleShot(0, this, SLOT(resizeMe()));
}

APqrPIDLTLP4::~APqrPIDLTLP4(void) {}

void APqrPIDLTLP4::cleanup()
{
	int i;
	for(i=0;i<10000;i++){
		Vm_log[i]=0;
		Vm_diff_log[i]=0;
	}
}

double APqrPIDLTLP4::sumy(double arr[], int n, double dlength, double modulo)
{
    double sumy = 0; // initialize sum
 
    // Iterate through all elements
    // and add them to sum
    for (int i = n-dlength+1 ; i < n+1; i++)
    sumy += arr[i % (int)modulo];
 
    return sumy;
}

double APqrPIDLTLP4::sumxy(double arr[], int n, double dlength, double period, double modulo)
{
    double sumxy = 0; // initialize sum
	int j = 0;

    // Iterate through all elements
    // and add them to sum
    for (int i = n-dlength+1 ; i < n+1; i++)
    {
	sumxy += arr[i % (int)modulo] * (j*period);
	j++;
    }
 
    return sumxy;
}

double APqrPIDLTLP4::sumx(double period, double dlength)
{
	double sumx = 0;

	for (int i = 0; i<dlength; i++)
	sumx += i*period;

	return sumx;
}

double APqrPIDLTLP4::sumx2(double period, double dlength)
{
	double sumx2 = 0;

	for (int i = 0; i<dlength; i++)
	sumx2 += i*period*i*period;

	return sumx2;
}

void APqrPIDLTLP4::execute(void)
{
	systime = idx * dt; // time in milli-seconds
	Vm = input(0) * 1e2; // convert 10V to mV. Divided by 10 because the amplifier produces 10-fold amplified voltages. Multiplied by 1000 to vonvert V to mV.

	Vm_log[idx2 % wave.size()] = Vm;

	if ((nloops && loop >= nloops) || !wave.size()) {
		pauseButton->setChecked(true);
		return;
	}

	if (act == 0 && Vm < V_light_on){
		output(0) = pulse_strength;
		output(1) = 0;
	}

	// Part of the code that resets the counter once a new AP is detected	
	if (act == 0 && (Vm - Vm_log[(idx2-(int)(1/dt)) % wave.size()]) >= slope_thresh && Vm > V_cutoff)
	{
		idx = 0;		
		act = 1;
	}

	if (act == 1){
		iAP = wave[idx] * gain + offset;
		Vm_diff_log[idx] = Vm - iAP;
		if (VLED < 5 && (Vm < blue_Vrev || Vm_diff_log[idx] > 0)) //this line was modified so that Int is only calculated when the output has not reached its maximum (5V) and either hyperpolarization is needed (red ch) or depolarization is needed (blue ch) and Vm is more negative than blue_Vrev.
		{Int = Int + Vm_diff_log[idx];}
		num = dlength *(sumxy(Vm_diff_log, idx, dlength, dt, modulo)) - sumx(dt, dlength)*sumy(Vm_diff_log, idx, dlength, modulo);
		denom = dlength*sumx2(dt, dlength) - sumx(dt, dlength)*sumx(dt, dlength);
		if (abs(denom) < 0.001)
			{slope = 10000;}
		else
			{slope = num/denom;} // Slope is measured in mV/ms
	
	
	
		P = K_p * Vm_diff_log[idx]; // Term that is proportional to the instantaneous difference in voltage.
		I = K_i * Int; // Term that speeds up or slows down the rate of change based on the history of voltage differences.
		D = K_d * slope; // Term that predicts the bahviour that is about to happen and helps in stabilizing.

		PID_diff = PID;
		PID = P + I + D;
		PID_diff = PID_diff - PID;

		if (idx >= corr_start-1 && abs(PID_diff) > PID_tresh){
			// PID_tresh gives a value that bounds the actions of the output (applies to PID_diff).
			// When smaller than this value, the previous light-ouput will be repeated.
			// This explains the lack of an else case.
			if (PID < 0 && abs(PID) > min_PID && Vm < blue_Vrev)
			{
				// min_PID gives a value where you don't consider it necessary to correct anything (applies to PID).
				// So when the absolute value is smaller than this value, the output will be set to 0.
				// This is the else case.
				VLED = -PID * (1/Rm_blue);			
				if (VLED > 5){VLED = 5;}
				output(0) = VLED;
				output(1) = 0;
			}
			else if (PID > 0 && abs(PID) > min_PID)
			{
				VLED = PID * (1/Rm_red);
				// Rm_red scales the amount of applied light for the red channel.
				// By lowering this value compared to Rm_blue, it is possible to counteract the smaller effect of repolarizing currents than depolarizing currents.
				if (VLED > 5){VLED = 5;}	
				output(1) = VLED;
				output(0) = 0;		
			}
			else
			{
				output(0) = 0;
				output(1) = 0;
			}			
		}
		
	}

	idx++;
	idx2++;

	PID_copy = (double)PID;
	act_copy = (double)act;
	idx_copy = (double)idx;
	idx2_copy = (double)idx2;
	iAP_V = (double)iAP/1000;
	Vm_V = (double)Vm/1000;

	if (idx2 >= wave.size()){
		idx2 = 0;
		act = 0;
		output(0) = 0;
		output(1) = 0;
		if (nloops) ++loop;
	}
}

void APqrPIDLTLP4::customizeGUI(void)
{
	QGridLayout *customlayout = DefaultGUIModel::getLayout();

	QGroupBox *fileBox = new QGroupBox("File");
	QHBoxLayout *fileBoxLayout = new QHBoxLayout;
	fileBox->setLayout(fileBoxLayout);
	QPushButton *loadBttn = new QPushButton("Load File");
	fileBoxLayout->addWidget(loadBttn);
	QPushButton *previewBttn = new QPushButton("Preview File");
	fileBoxLayout->addWidget(previewBttn);
	QObject::connect(loadBttn, SIGNAL(clicked()), this, SLOT(loadFile()));
	QObject::connect(previewBttn, SIGNAL(clicked()), this, SLOT(previewFile()));

	customlayout->addWidget(fileBox, 0, 0);
	setLayout(customlayout);
}

void APqrPIDLTLP4::update(DefaultGUIModel::update_flags_t flag)
{
	switch (flag) {
		case INIT:
			setParameter("Loops", QString::number(nloops));
			setParameter("Gain", QString::number(gain));
			setParameter("Offset", QString::number(offset));
			setParameter("Pulse_strength (V)", pulse_strength);
			setComment("File Name", filename);
			setState("Length (ms)", length);
			setParameter("Slope_thresh (mV/ms)", slope_thresh);
          		setParameter("Blue_Vrev", blue_Vrev);
			setParameter("Rm_blue (MOhm)", Rm_blue);
			setParameter("Rm_red (MOhm)", Rm_red);
			setParameter("Slope_thresh (mV/ms)", slope_thresh);
			setParameter("Correction start", corr_start);
			setParameter("Blue_Vrev", blue_Vrev);
			setParameter("K_p", K_p);
			setParameter("K_i", K_i);
			setParameter("K_d", K_d);
			setParameter("V_light_on (mV)", V_light_on);
			setParameter("V_cutoff (mV)", V_cutoff);
			setParameter("dlength", dlength);
			setParameter("PID_tresh", PID_tresh);
			setParameter("min_PID", min_PID);
			setState("Vm2 (mV)", Vm);
			setState("Time (ms)", systime);
			setState("Period (ms)", dt);
			setState("PID", PID_copy);			
			setState("act", act_copy);			
			setState("idx", idx_copy);			
			setState("idx2", idx2_copy);
			setState("modulo_state", modulo);
			setState("iAP", iAP);
			setState("Vm_V", Vm_V);
			setState("iAP_V", iAP_V);
			setState("P", P);
			setState("I", I);
			setState("D", D);
			break;

		case MODIFY:
			nloops = getParameter("Loops").toUInt();
			gain = getParameter("Gain").toDouble();
			offset = getParameter("Offset").toDouble();
			pulse_strength = getParameter("Pulse_strength (V)").toDouble();
			filename = getComment("File Name");
			Rm_blue = getParameter("Rm_blue (MOhm)").toDouble();
			Rm_red = getParameter("Rm_red (MOhm)").toDouble();
			slope_thresh = getParameter("Slope_thresh (mV/ms)").toDouble();
			V_light_on = getParameter("V_light_on (mV)").toDouble();
			V_cutoff = getParameter("V_cutoff (mV)").toDouble();
			corr_start = getParameter("Correction start").toDouble();
			blue_Vrev = getParameter("Blue_Vrev").toDouble();
			K_p = getParameter("K_p").toDouble();
			K_i = getParameter("K_i").toDouble();
			K_d = getParameter("K_d").toDouble();
			dlength = getParameter("dlength").toDouble();
			PID_tresh = getParameter("PID_tresh").toDouble();
			min_PID = getParameter("min_PID").toDouble();
			systime = 0;
			idx = 0;
			idx2 = 0;
			PID = 0;
			PID_diff = 0;
			Int = 0;
			cleanup();
			break;

		case PAUSE:
			output(0) = 0;
			output(1) = 0;
			act = 0;
			idx = 0;
			loop = 0;
			systime = 0;
			idx2 = 0;
			break;

		case UNPAUSE:
			break;

		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-6; // time in milli-seconds
			modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
			loadFile(filename);

		default:
			break;
	}
}

void APqrPIDLTLP4::initParameters()
{
	dt = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	gain = 1;
	offset = 0;
	filename = "No file loaded.";
	idx = 0;
	idx2 = 0;
	loop = 0;
	nloops = 100;
	length = 0;
	pulse_strength = 3; //V
	slope_thresh = 5.0; // mV
	Vm = -80; // mV
	Rm_blue = 150; // MOhm
	Rm_red = 50; // MOhm
	corr_start = 0;
    	blue_Vrev = -20;
	VLED = 0;	
	output(0) = 0;
	output(1) = 0;
	systime = 0;
	act = 0;
	V_light_on = -60; // mV
	V_cutoff = -40; // mV
	iAP=-80;
	PID = 0;
	dlength = 10;
	Int = 0;
	num = 0;
	denom = 1;
	slope = 0;
	P = 0;
	I = 0;
	D = 0;
	K_p = 1;
	K_i = 0.1;
	K_d = 0.1;
	PID_diff = 0;
	PID = 0;
	PID_tresh = 0.1;
	min_PID = 0.2;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
}

void APqrPIDLTLP4::loadFile()
{
	QFileDialog* fd = new QFileDialog(this, "Wave Maker Input File");//, TRUE);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setViewMode(QFileDialog::Detail);
	QString fileName;
	if (fd->exec() == QDialog::Accepted) {
		QStringList files = fd->selectedFiles();
		if (!files.isEmpty()) fileName = files.takeFirst();
		setComment("File Name", fileName);
		wave.clear();
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			double value;
			while (!stream.atEnd()) {
				stream >> value;
				wave.push_back(value);
			}
			filename = fileName;
		}
		length = wave.size() * dt;
		setState("Length (ms)", length); // initialized in ms, display in ms
	} else setComment("File Name", "No file loaded.");
}

void APqrPIDLTLP4::loadFile(QString fileName)
{
	if (fileName == "No file loaded.") {
		return;
	} else {
		wave.clear();
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			double value;
			while (!stream.atEnd()) {
				stream >> value;
				wave.push_back(value);
			}
		}
		length = wave.size() * dt;
		setState("Length (ms)", length); // initialized in ms, display in ms
	}
}

void APqrPIDLTLP4::previewFile()
{
	double* time = new double[static_cast<int> (wave.size())];
	double* yData = new double[static_cast<int> (wave.size())];
	for (int i = 0; i < wave.size(); i++) {
		time[i] = dt * i;
		yData[i] = wave[i];
	}
	PlotDialog *preview = new PlotDialog(this, "Wave Maker Waveform", time, yData, wave.size());

	preview->show();
}
