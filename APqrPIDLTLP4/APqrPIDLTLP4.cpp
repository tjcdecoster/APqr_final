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

#include "APqrPIDLTLP4.h"
#include <math.h>
#include <time.h>
#include <main_window.h>

/*
 ****************
 * APqrPIDLTLP4 *
 ****************

This software provides an Action Potential Cure (APqr) to imprint
membrane potentials in excitable biological systems. Values to be
imprinted are read from ACSII file. The imprinting itself occurs
with the use of LED-controlled illumination on optogenetically
modified cells.

IN:
	*) Loops			Number of Times to Loop Data From File (called iAP)
	*) gain				Factor to amplify iAP
	*) offset			Factor to offset iAP (mV)
	*) Pulse_strength	Blue LED driver voltage (V) for pacing
	*) Filename			ASCII Input File-name
	*) Vm				Membrane potential (mV) coming from file
	*) V_light_on		Threshold potential for when the pulse can be given to
						the cells
	*) V_cutoff			Threshold potential for the detection of the beginning
						of an AP
	*) Slope_tresh		Slope threshold that defines the beginning of the
						AP (mV/ms)
	*) Rm_blue			Initial resistance for the blue LED channel
	*) Rm_red			Initial resistance for the red LED channel
	*) corr_start		Gives the possibility to start at a later time than
						the lognum+1-st AP with correcting the AP
	*) Blue_Vrev		Apparent reversal potential of the 'blue' ChR current
	*) K_p				Scale factor for the proportional part of the PID
	*) K_i				Scale factor for the integral part of the PID
	*) K_d				Scale factor for the derivative part of the PID
	*) dlength			Amount of points that need to be taken into account to
						find the derivative (slope of the linear trend line of
						these points)
	*) PID_tresh		treshold value under which the same output as before
						gets repeated
	*) min_PID			value under which the lights get switched off
OUT:
	*) VLED_blue		voltage that is used to power the first LED driver that
						regulates the light that is shined onto the cells
	*) VLED_red			voltage that is used to power the second LED driver that
						regulates the light that is shined onto the cells
*/

/*
createRTXIPlugin
----------------
Creation of a new RTXI Plugin

IN:
	*) None
OUT:
	*) RTXIPlugin
*/

/*
createRTXIPlugin
----------------
Creation of a new RTXI Plugin

IN:
	*) None
OUT:
	*) RTXIPlugin
*/
extern "C" Plugin::Object *createRTXIPlugin(void)
{
	return new APqrPIDLTLP4(); // Change the name of the plug-in here
}

/*
vars[]
----
This is not a function, but rather the construction of a list. This list contains
all the variables that are visible and/or modifiable in the GUI of the software
module. There is the choice between INPUT, OUTPUT, PARAMETER and STATE.

INPUT: connected to the input port
OUTPUT: connected to the output port
PARAMETER: modifiable variable in the code
STATE: non-modifiable variable in the code
*/
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
	{ "P", "P term", DefaultGUIModel::STATE, },
	{ "I", "I term", DefaultGUIModel::STATE, },
	{ "D", "D term", DefaultGUIModel::STATE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, 
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, },
	{ "PID", "PID", DefaultGUIModel::STATE, },
	{ "act", "act", DefaultGUIModel::STATE, },
	{ "idx", "idx", DefaultGUIModel::STATE, },	
	{ "idx2", "idx2", DefaultGUIModel::STATE, },
};

/*
num_vars
--------
variable denoting the amount of variables that is displayed in the GUI
*/
static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

/*
gAPqrPIDLTLP4
------
This function constructs the actual GUI by basing itself on the Default GUI Model.
It creates a module with a name, initializes the GUI, initializes the parameters,
adds a refresh, and allows you to resize.

IN:
	*) None
OUT:
	*) None
*/
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

/*
cleanup
-------
The APqr software makes use of two list structures which need cleaning after
a reset of parameters. The cleanup function takes care of this.

IN:
	*) None
OUT:
	*) None
*/
void APqrPIDLTLP4::cleanup()
{
	int i;
	for(i=0;i<10000;i++){
		Vm_log[i]=0;
		Vm_diff_log[i]=0;
	}
}

/*
sumy
----
Sum of X elements in an array.
This function is used in the calculation of the derivative term.

IN:
	*) arr[]	Array with summable elements
	*) n		index where you should end summing
	*) length	total amount X of numbers to be summed
	*) modulo	length of the array arr[]
OUT:
	*) sumy		The sum of 'length' elements in the array arr[]
*/
double APqrPIDLTLP4::sumy(double arr[], int n, double dlength, double modulo)
{
    double sumy = 0; // initialize sum
 
    // Iterate through all elements
    // and add them to sum
    for (int i = n-dlength+1 ; i < n+1; i++)
    sumy += arr[i % (int)modulo];
 
    return sumy;
}

/*
sumxy
----
Sum of X elements in an array multiplied by time/period.
This function is used in the calculation of the derivative term.

IN:
	*) arr[]	Array with summable elements
	*) n		index where you should end summing
	*) length	total amount X of numbers to be summed
	*) period	the length of a single time-step
	*) modulo	length of the array arr[]
OUT:
	*) sumxy		The sum of 'length' elements in the array arr[] multiplied
				by time-step
*/
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

/*
sumx
----
Sum of X time-step elements.
This function is used in the calculation of the derivative term.

IN:
	*) period	the length of a single time-step
	*) length	total amount X of numbers to be summed
OUT:
	*) sumx		The sum of 'length' time-steps
*/
double APqrPIDLTLP4::sumx(double period, double dlength)
{
	double sumx = 0;

	for (int i = 0; i<dlength; i++)
	sumx += i*period;

	return sumx;
}

/*
sumx2
----
Sum of X squared time-step elements.
This function is used in the calculation of the derivative term.

IN:
	*) period	the length of a single time-step
	*) length	total amount X of numbers to be summed
OUT:
	*) sumx		The sum of squared 'length' time-steps
*/
double APqrPIDLTLP4::sumx2(double period, double dlength)
{
	double sumx2 = 0;

	for (int i = 0; i<dlength; i++)
	sumx2 += i*period*i*period;

	return sumx2;
}

/*
execute
-------
This is the main funtcion of the code that is looped through real-time.
It contains the four main parts of the algorithm:
	1) Reading in the ideal AP
	2) Detecting AP upstrokes
	3) Computing AP correction and outputting this
	4) Updating the necessary variables

IN:
	*) None
OUT:
	*) VLED1	voltage that is used to power the first LED driver that
				regulates the light that is shined onto the cells
	*) VLED2 	voltage that is used to power the second LED driver that
				regulates the light that is shined onto the cells
*/
void APqrPIDLTLP4::execute(void)
{
	systime = idx * dt; // time in milli-seconds
	Vm = input(0) * 1e2; // convert 10V to mV. Divided by 10 because the amplifier produces 10-fold amplified voltages. Multiplied by 1000 to vonvert V to mV.

	Vm_log[idx2 % wave.size()] = Vm; 	// Logging the measured Vm in a list
										// where the wave.size component makes
										// sure you keep cycling when you have
										// reached the maximum number in the list.

	if ((nloops && loop >= nloops) || !wave.size()) {
		// Pause the working of this module as long as no File has been provided, or as soon
		// as the maximal number of loops through this file has been reached
		pauseButton->setChecked(true);
		return;
	}

	if (act == 0 && Vm < V_light_on){
		// This statement is entered whenever the cell is no longer in a state where an AP
		// is forced upon it
		// The if-conditions measure the following:
		// 1) That you are currently not imprinting anything any more
		// 2) That you are below the stimulation threshold

		output(0) = pulse_strength;
		output(1) = 0;
	}

	// ****************************
	// ****************************
	// ** Detecting AP upstrokes **
	// ****************************
	// ****************************
	if (act == 0 && (Vm - Vm_log[(idx2-(int)(1/dt)) % wave.size()]) >= slope_thresh && Vm > V_cutoff)
	{
		// This statement is entered whenever an upstroke is detected after the
		// ideal APs have been recorded.
		// The if conditions measure the following:
		// 1) Whether currently nothing is being done or corrected
		// 2) Whether two consecutive measuring points show a large enough slope that can
		//    be identified with an upstroke
		// 3) Whether the mesured voltage is above a voltage treshold

		idx = 0; // Reset the correction index/counter
		act = 1; // Switch the correction on
	}

	// Part of the code that implements the PID
	if (act == 1){
		// This statement is entered whenever the instruction to correct the AP has
		// been given.

		iAP = wave[idx] * gain + offset; // adjust the values from the AP-file in case necessary

		// ************************************
		// * Calculate the proportional error *
		// ************************************
		Vm_diff_log[idx] = Vm - iAP; // Log the errors
		// ***************************************
		// * Calculate the integral of the error *
		// ***************************************
		if (VLED < 5 && (Vm < blue_Vrev || Vm_diff_log[idx] > 0))
		{
			// Int is only calculated when the voltage LED output has not reached
			// its maximum (5V) and on eof two conditiosn is satisfied: depolarization
			// is needed (blue ch) and Vm is more negative than blue_Vrev, or
			// hyperpolarization is needed (red ch).
			// This step was taken such that the Int term cannot amass further when
			// the system can not react to it.

			Int = Int + Vm_diff_log[idx];
		}
		// *****************************************
		// * Calculate the derivative of the error *
		// *****************************************
		// Calculate the numerator for a linear regression between the last "length" amount of points.
		// This larger amount of points is chosen to cut out the noise that is intrinsically present
		// in a membrane potential recording.
		num = dlength *(sumxy(Vm_diff_log, idx, dlength, dt, modulo)) - sumx(dt, dlength)*sumy(Vm_diff_log, idx, dlength, modulo);
		// Calculate the denominator
		denom = dlength*sumx2(dt, dlength) - sumx(dt, dlength)*sumx(dt, dlength);
		// Calculate the derivative when the denominator is not too small
		if (abs(denom) < 0.001)
			{slope = 10000;}
		else
			{slope = num/denom;} // Slope is measured in mV/ms
	
		// ************************************
		// * Calculate the separate PID terms *
		// ************************************	
		P = K_p * Vm_diff_log[idx]; // Term that is proportional to the instantaneous difference in voltage.
		I = K_i * Int; // Term that speeds up or slows down the rate of change based on the history of voltage differences.
		D = K_d * slope; // Term that predicts the bahviour that is about to happen and helps in stabilizing.

		PID_diff = PID; // Update the PID difference term
		PID = P + I + D; // Calculate the sum of all the individual terms
		PID_diff = PID_diff - PID; // Calculate the PID difference term

		if (idx >= corr_start-1 && abs(PID_diff) > PID_tresh){
			// PID_tresh gives a value that bounds the actions of the output (applies to PID_diff).
			// When smaller than this value, the previous light-ouput will be repeated.
			// This explains the lack of an else case.
			if (PID < 0 && abs(PID) > min_PID && Vm < blue_Vrev)
			{
				// If PID is smaller than 0, a depolarizing current is needed. Therefore here the output for
				// the blue LED driver is calculated. However, blu elight has no influence when the membrane
				// potential is larger than the reversal potential of the light-gated channel. Hence, an extra limit
				// was imposed to limit this.
				// min_PID gives a value where you don't consider it necessary to correct anything (applies to PID).
				// So when the absolute value is smaller than this value, the output will be set to 0 (in the else case later on).

				VLED = -PID * (1/Rm_blue); // Calculate VLED by applying a LED-specific factor
				if (VLED > 5){VLED = 5;} // Limit the LED driver output to its maximum value
				output(0) = VLED; // Send output to the blue LED driver
				output(1) = 0; // Make sure the red LED driver does not receive any output
			}
			else if (PID > 0 && abs(PID) > min_PID)
			{
				// If PID is larger than 0, a repolarizing current is needed. Therefore here the output for
				// the red LED driver is calculated.

				VLED = PID * (1/Rm_red); // Calculate VLED by applying a LED-specific factor
				// Rm_red scales the amount of applied light for the red channel.
				// By lowering this value compared to Rm_blue, it is possible to counteract the smaller effect of repolarizing currents than depolarizing currents.
				if (VLED > 5){VLED = 5;} // Limit the LED driver output to its maximum value
				output(1) = VLED; // Send output to the red LED driver
				output(0) = 0; // Make sure the blue LED driver does not receive any output
			}
			else
			{
				// In all other cases, don't shine any light
				output(0) = 0; // Make sure the blue LED driver does not receive any output
				output(1) = 0; // Make sure the red LED driver does not receive any output
			}			
		}
		
	}

	idx++; // This is the total counter and does not get reset
	idx2++; // This is the counter within one AP. WIll be reset after every upstroke

	PID_copy = (double)PID;
	act_copy = (double)act;
	idx_copy = (double)idx;
	idx2_copy = (double)idx2;

	// This part of the code makes sure no output is produced in the last part of the action potential to let the cell come to rest.
	// It also makes sure that the necessary variables are reset when the ASCII file came to an end.
	if (idx2 >= wave.size()){
		idx2 = 0; // Reset the AP counter
		act = 0; // Stop imprinting after the end of the file has been reached
		output(0) = 0; // Send a 0 output since the last output is otherwise kept
		output(1) = 0; // Send a 0 output since the last output is otherwise kept
		if (nloops) ++loop; // Increase the loop counter for the amount of times we go through the ASCII file
	}
}

/*
customizeGUI
------------
Function that adds more buttons and functionality to the standard GUI.
The added functions are there to support the reading of an ASCII file.

IN:
	*) None
OUT:
	*) None
*/
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

/*
Update
------
This function updates the parameters of the code depending on the flag that is 
given to it, where each flag is associated to a button.
INIT: associated to the loading of the module
MODIFY: associated to the Modify button
PERIOD: associated to the period linker with the "system control panel" module
PAUSE: associate to the pause button when pressing on it
UNPAUSE: associated to the pause button when unpressing it

IN:
	*) flag				Indicating the state of the update:
						INIT, MODIFY, PERIOD, PAUSE, UNPAUSE
OUT:
	*) None
*/
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
			setState("Time (ms)", systime);
			setState("Period (ms)", dt);
			setState("PID", PID_copy);			
			setState("act", act_copy);			
			setState("idx", idx_copy);			
			setState("idx2", idx2_copy);
			setState("iAP", iAP);
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

/*
initParameters
--------------
This function sets all values to their defaults when no external parameters are provided
through the GUI interface.

IN:
	*) None
OUT:
	*) None
*/
void APqrPIDLTLP4::initParameters()
{
	// system related parameters
	systime = 0;
	dt = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	// cell related parameters
	Vm = -80; 			// mV
	Rm_blue = 150; 		// MOhm
	Rm_red = 50;		// MOhm
	// upstroke related parameters
	slope_thresh = 5.0; // mV
	V_cutoff = -40;		// mV
	V_light_on = -60; // mV
	pulse_strength = 3; //V
	// file reading related parameters
	filename = "No file loaded.";
	gain = 1;
	offset = 0;
	loop = 0;
	nloops = 100;
	length = 0;
	iAP=-80;
	// correction parameters
	act = 0;
	corr_start = 0;
	PID_tresh = 0.1;
	min_PID = 0.2;
	blue_Vrev = -20;	// mV

	PID = 0;
	P = 0;
	I = 0;
	D = 0;
	K_p = 1;
	K_i = 0.1;
	K_d = 0.1;
	Int = 0;
	dlength = 10;
	num = 0;
	denom = 1;
	slope = 0;
	PID_diff = 0;

	// standard loop parameters
	idx = 0;
	idx2 = 0;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
	VLED = 0;
	output(0) = 0;
	output(1) = 0;
}

/*
loadFile
--------
Function that sets up the loadFile slot in the GUI. It links all the file's properties
to its associated values.

IN:
	*) None
OUT:
	*) None
*/
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

/*
loadFile
--------
Function that reads in an ASCII file and gives a value to all file-related parameters.

IN:
	*) filename
OUT:
	*) None
*/
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

/*
previewFile
-----------
Function that opens a new dialog window to show the file that has been read in.
This allows to quickly double check that you will be imprinting the correct AP(s).

IN:
	*) None
OUT:
	*) None
*/
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
