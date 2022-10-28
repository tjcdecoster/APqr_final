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

#include <APqr8.h>
#include <math.h>
#include <vector>

/*
 *********
 * APqr8 *
 *********

This software provides an Action Potential Cure (APqr) to correct divergent
membrane potentials in excitable biological systems. The first X action
potentials are logged when the software starts, after which AP correction
starts from the (X+1)-st AP onwards. This correction occurs with the use of
LED-controlled illumination on optogenetically modified cells.

IN:
	*) Cm				Capacitance of the cell
	*) V_cutoff			Threshold potential for the detection of the beginning
						of an AP
	*) Slope_tresh		Slope threshold that defines the beginning of the
						AP (mV/ms)
	*) BCL_cutoff		Threshold value for the end of an AP, given as a
						percentage of the total APD
	*) lognum			Number of APs that need to be logged as a reference
	*) Rm				Initial resistance
	*) Rm_corr_up		Factor to increase Rm with when necessary
	*) Rm_corr_down		Factor to decrease Rm with when necessary
	*) noise_tresh		The noise level that is allowed around the ideal value
						before correcting
OUT:
	*) Vout 			voltage that is used to power the LED driver that
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
extern "C" Plugin::Object *createRTXIPlugin(void)
{
	return new gAPqr8();
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
	{ "Vm (mV)", "Membrane potential (mV)", DefaultGUIModel::INPUT, },
	{ "Iout (pA)", "Output current (pA)", DefaultGUIModel::OUTPUT, },
	{ "Cm (pF)", "pF", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "V_cutoff (mV)", "Threshold potential for the detection of the beginning of an AP, together with Slope_thresh", 		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Slope_thresh (mV/ms)", "SLope threshold that defines the beginning of the AP (mV/ms)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "BCL_cutoff (pct)", "Threshold value for the end of an AP, given as a percentage of the total APD",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "noise_tresh (mV)", "The noise level that is allowed before correcting", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, }, 
	{ "Rm (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "lognum", "Number of APs that need to be logged as a reference", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_corr_up", "To increase Rm when necessary", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_corr_down", "To decrease Rm when necessary", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, }, 
	{ "Correction (0 or 1)", "Switch Rm correction off (0) or on (1)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, // To check that the period taken by the algorithm is the same as the one i nthe control panel module
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, }, // To check that the algorithm is running
	{ "APs2", "APs", DefaultGUIModel::STATE, }, // To check whether APs are being logged and the counter increases
	{ "BCL2", "BCL", DefaultGUIModel::STATE, }, // To check what the eventual BCL of the ideal AP has become. You can see then if the APs were logged correctly
	{ "act2", "0 or 1", DefaultGUIModel::STATE, }, // Switches from 0 to 1 and back continuously as a check to see whether you are computing error values and corrected values
};

/*
num_vars
--------
variable denoting the amount of variables that is displayed in the GUI
*/
static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

/*
gAPqr8
------
This function constructs the actual GUI by basing itself on the Default GUI Model.
It creates a module with a name, initializes the GUI, initializes the parameters,
adds a refresh, and allows you to resize.

IN:
	*) None
OUT:
	*) None
*/
gAPqr8::gAPqr8(void) : DefaultGUIModel("APqr8", ::vars, ::num_vars)
{
	setWhatsThis(
		"<p><b>APqr:</b><br>APqr8 </p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	update(INIT);
	refresh();
	resizeMe();
}

gAPqr8::~gAPqr8(void){}

/*
cleanup
-------
The APqr software makes use of three list structures which need cleaning after
a reset of parameters. The cleanup function takes care of this.

IN:
	*) None
OUT:
	*) None
*/
void gAPqr8::cleanup()
{
	for(i=0;i<10000;i++){
		Vm_log[i]=0;
		Vm_diff_log[i]=0;
		ideal_AP[i]=0;
	}
}

/*
execute
-------
This is the main funtcion of the code that is looped through real-time.
It contains the four main parts of the algorithm:
	1) Recording the ideal AP
	2) Detecting AP upstrokes
	3) Computing AP correction and outputting this
	4) Updating the necessary variables

IN:
	*) None
OUT:
	*) Vout				voltage that is used to inject the calculated amount
						of current into the excitable system
*/
void gAPqr8::execute(void)
{
	systime = count * period;	// time in milli-seconds
	Vm = input(0) * 1e2;		// convert 10V to mV. Divided by 10 because
								// the amplifier produces 10-fold amplified
								// voltages. Multiplied by 1000 to convert
								// V to mV.

	Vm_log[count % (int)modulo] = Vm;	// Logging the measured Vm in a list
										// where the modulo component makes
										// sure you keep cycling when you have
										// reached the maximum number in the list.

	// ****************************
	// ****************************
	// ** Recording the ideal AP **
	// ****************************
	// ****************************
	if(count>(int)(1/period)-1 && (Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) >= slope_thresh && APs<lognum && enter == 0 && Vm > V_cutoff)
	{
		// This statement is entered whenever an upstroke is detected and the amount of
		// recorded APs is smaller than lognum.
		// The if conditions measure the following:
		// 1) Whether you are far enough in the recording such that you don't accidentaly
		//    start in an ongoing AP
		// 2) Whether two consecutive measuring points show a large enough slope that can
		//    be identified with an upstroke
		// 3) Whether less than lognum APs were recorded
		// 4) Whether you are currently not in an action potential
		// 5) Whether the mesured voltage is above a voltage treshold

		BCL = (APs==-1? 0: (BCL*APs + count2)/(APs+1)); // Rolling average of the basic cycle length
		log_ideal_on = 1; // Switches on logging the AP
		count2 = 0; // Resets the logging counter
		enter = 1; // Switches on the indicator that an AP has started
		APs++; // Counts the AP upstrokes that have passed
	}

	if((Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) < 0 && enter == 1)
	{
		// This statement is entered whenever the upstroke phase of an AP is over.
		// The if conditions measure the following:
		// 1) Whether two consecutive measuring points show a negative slope
		// 2) Whether you currently are in an ongoing AP

		enter = 0; // Switches off the indicator that an AP has started
	}

	if(APs<lognum && log_ideal_on == 1)
	{
		// This statement is entered whenever logging of the AP is on
		// The if conditions measure the following:
		// 1) Whether less than lognum APs were recorded
		// 2) Whether the AP should be logged

		ideal_AP[count2] = (ideal_AP[count2]*APs + Vm)/(APs+1); // Rolling average of the AP values
		count2++; // Increasing the logging counter
	}

	// ****************************
	// ****************************
	// ** Detecting AP upstrokes **
	// ****************************
	// ****************************
	if (act == 0 && (Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) >= slope_thresh && APs >= lognum && Vm > V_cutoff)
	{
		// This statement is entered whenever an upstroke is detected after the
		// ideal APs have been recorded.
		// The if conditions measure the following:
		// 1) Whether currently nothing is being done or corrected
		// 2) Whether two consecutive measuring points show a large enough slope that can
		//    be identified with an upstroke
		// 3) Whether lognum APs were already recorded before
		// 4) Whether the mesured voltage is above a voltage treshold

		count = 0; // Reset the correction counter
		act = 1; // Switch the correction on
	}

	// *************************************************
	// *************************************************
	// ** Computing AP correction and outputting this **
	// *************************************************
	// *************************************************	
	if (act == 1)
	{
		// This statement is entered whenever the instruction to correct the AP has
		// been given.
		
		Iout = Cm * (1/Rm) * (Vm - ideal_AP[count]); 	// Calculate the outward going current as
														// a value proportional to capacitance,
														// conductivity (1/resistance), and the error
		if (Iout < 0){Iout = 0;} 	// Set the ouput to 0 whenever you cannot correct in the direction
									// the channelrhodopsin pushes the membrane potential
		if (Iout > 5){Iout = 5;} // The maximal LED driver output is 5V

		output(0) = Iout; // This is equal to Vout and will drive the LED
		Vm_diff_log[count] = Vm - ideal_AP[count]; // Log the errors
	}

	// **************************************
	// **************************************
	// ** Updating the necessary variables **
	// **************************************
	// **************************************
	if(corr == 1 && act == 1 && count > 1 && abs(Vm_diff_log[count])>noise_tresh)
	{
		// This statement is entered whenever an update is needed in the resistance.
		// The if conditions measure the following:
		// 1) Whether correction adaptation is on
		// 2) Whether currently there is correction going on
		// 3) whether we are not in the very first step (gives errors)
		// 4) whether the current error is larger than the noise threshold

		if((Vm_diff_log[count-1] / Vm_diff_log[count]) < 0)
		{
			// This statement is entered whenever two consecutive error values have
			// an opposite sign. This means that an overshoot in correction occurred.
			// Therefore Iout should become less, and hence Rm should be increased. 

			Rm = Rm * Rm_corr_up; // Increase the resistance
		}
		if(abs(Vm_diff_log[count-1]) < abs(Vm_diff_log[count]) && (Vm_diff_log[count-1] / Vm_diff_log[count]) > 0 && Rm >= 0.01*Rm_corr_down)
		{
			// This statement is entered whenever two consecutive error values have
			// the same sign, and when the error values increase in value. This means
			// that the error is increasing. Hence we need to correct stronger and Iout
			// should increase. As a consequence the resistance Rm should be decreased.
			// However, a lower bounds is put on Rm to prevent the updated Rm value from
			// reaching infinity.

			Rm = Rm / Rm_corr_down; // Decrease the resistance
		}
	}


	if (count > BCL_cutoff*BCL)
	{
		// This statement is entered whenever the end of an AP is reached.
		// The if condition measures the following:
		// 1) Whether the current AP is further than a chosen cutoff of the pre-determined basic cycle length

		act = 0; // Stop correcting during the last phase of the AP (is RMP)
		output(0) = 0; // Send a 0 output since the last output is otherwise kept
	}

	count++; // End of the real-time loop, adjust the counter
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
void gAPqr8::update(DefaultGUIModel::update_flags_t flag)
{
	switch (flag)
	{
	case INIT:
		setParameter("Cm (pF)", Cm);
		setParameter("V_cutoff (mV)", V_cutoff);
		setParameter("Rm (MOhm)", Rm);
		setParameter("Rm_corr_up", Rm_corr_up);
		setParameter("Rm_corr_down", Rm_corr_down);
		setParameter("noise_tresh (mV)", noise_tresh);
		setParameter("lognum", lognum);
		setParameter("BCL_cutoff (pct)", BCL_cutoff);
		setParameter("Slope_thresh (mV/ms)", slope_thresh);
		setParameter("Correction (0 or 1)", corr);
		setState("Time (ms)", systime);
		setState("Period (ms)", period);
		setState("APs2", APs);
		setState("BCL2", BCL);
		setState("act2", act);
		break;
	case MODIFY:
		Cm = getParameter("Cm (pF)").toDouble();
		Rm = getParameter("Rm (MOhm)").toDouble();
		lognum = getParameter("lognum").toDouble();
		BCL_cutoff = getParameter("BCL_cutoff (pct)").toDouble();
		noise_tresh = getParameter("noise_tresh (mV)").toDouble();
		Rm_corr_up = getParameter("Rm_corr_up").toDouble();
		Rm_corr_down = getParameter("Rm_corr_down").toDouble();
		slope_thresh = getParameter("Slope_thresh (mV/ms)").toDouble();
		corr = getParameter("Correction (0 or 1)").toDouble();
		V_cutoff = getParameter("V_cutoff (mV)").toDouble();
		systime = 0;
		count = 0;
		APs = -1;
		BCL = 0;
		log_ideal_on = 0;
		enter = 0;
		count2 = 0;
		cleanup();
		break;
	case PERIOD:
		period = RT::System::getInstance()->getPeriod() * 1e-6; // time in milli-seconds
		modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
		break;
	case PAUSE:
		output(0) = 0.0;
		Iout = 0;
		act = 0;
		systime = 0;
		break;
	case UNPAUSE:
		break;
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
void gAPqr8::initParameters()
{
	// system related parameters
	systime = 0;
	period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	// cell related parameters
	Vm = -80; 			// mV
	Cm = 150; 			// pF
	Rm = 150; 			// MOhm
	// upstroke related parameters
	slope_thresh = 5.0; // mV
	V_cutoff = -40;		// mV
	// logging parameters
	log_ideal_on = 0;
	lognum = 3;
	APs = -1;
	count2 = 0;
	// correction parameters
	act = 0;
	corr = 1;
	noise_tresh = 2; 	// mV
	Rm_corr_up=2;
	Rm_corr_down=2;

	// standard loop parameters
	count = 0;
	enter = 0;
	BCL = 0;			// ms
	BCL_cutoff = 0.98;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
	Iout = 0;			// pA
	output(0) = -Iout * 0.5e-3;
}
