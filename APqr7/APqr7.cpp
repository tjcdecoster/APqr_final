/*
 Copyright (C) 2022 Leiden University Medical Center

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

#include <APqr7.h>
#include <math.h>
#include <vector>

/*
 *********
 * APqr7 *
 *********

This software provides an Action Potential Cure (APqr) to correct divergent
membrane potentials in excitable biological systems. The first X action
potentials are logged when the software starts, after which AP correction
starts from the (X+1)-st AP onwards. This correction occurs with the use of
dynamic pacth-clamp currrent injection.

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
	*) Vout 			voltage that is used to inject the calculated amount
						of current into the excitable system
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
	return new gAPqr7();
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
	{ "iAP", "ideal AP", DefaultGUIModel::OUTPUT, },
	{ "Cm (pF)", "pF", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "V_cutoff (mV)", "Threshold potential for the detection of the beginning of an AP, together with Slope_thresh",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Slope_thresh (mV/ms)", "Slope threshold that defines the beginning of the AP (mV/ms)",
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
	{ "Vm2 (mV)", "Membrane potential (mV)", DefaultGUIModel::STATE, },
	{ "Iout2 (pA)", "Output Current (pA)", DefaultGUIModel::STATE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, 
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, },
	{ "APs2", "APs", DefaultGUIModel::STATE, },
	{ "log_ideal_on2", "log_ideal_on", DefaultGUIModel::STATE, },
	{ "BCL2", "BCL", DefaultGUIModel::STATE, },
	{ "enter2", "enter", DefaultGUIModel::STATE, },
	{ "Rm2 (MOhm)", "MOhm", DefaultGUIModel::STATE, },
	{ "act2", "0 or 1", DefaultGUIModel::STATE, },
	{ "count", "number", DefaultGUIModel::STATE, },
	{ "count2", "number", DefaultGUIModel::STATE, },
	{ "modulo_state", "number", DefaultGUIModel::STATE, },
};

/*
num_vars
--------

*/
static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

/*
gAPqr7
------

IN:
	*)
OUT:
	*)
*/
gAPqr7::gAPqr7(void) : DefaultGUIModel("APqr7", ::vars, ::num_vars)
{
	setWhatsThis(
		"<p><b>APqr:</b><br>APqr7 </p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	update(INIT);
	refresh();
	resizeMe();
}

gAPqr7::~gAPqr7(void) {}

/*
cleanup
-------

IN:
	*) None
OUT:
	*) None
*/
void gAPqr7::cleanup()
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

IN:
	*) None
OUT:
	*) Vout				voltage that is used to inject the calculated amount
						of current into the excitable system
*/
void gAPqr7::execute(void)
{
	systime = count * period; // time in milli-seconds
	Vm = input(0) * 1e2; // convert 10V to mV. Divided by 10 because the amplifier produces 10-fold amplified voltages. Multiplied by 1000 to convert V to mV.

	Vm_log[count % (int)modulo] = Vm;

	if(count>(int)(1/period)-1 && (Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) >= slope_thresh && APs<lognum && enter == 0 && Vm > V_cutoff)
	{
		BCL = (APs==-1? 0: (BCL*APs + count2)/(APs+1));
		log_ideal_on = 1;
		count2 = 0;
		enter = 1;
		APs++;
	}

	if((Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) < 0 && enter == 1)
	{
		enter = 0;
	}

	if(APs<lognum && log_ideal_on == 1)
	{
		ideal_AP[count2] = (ideal_AP[count2]*APs + Vm)/(APs+1);
		count2++;
	}

	if (act == 0 && (Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) >= slope_thresh && APs >= lognum && Vm > V_cutoff)
	{
		count = 0;		
		act = 1;
	}

	
	if (act == 1)
	{
		Iout = Cm * (1/Rm) * (Vm - ideal_AP[count]);
		output(0) = -Iout * 2.5e-3; // This is equal to Vout
		Vm_diff_log[count] = Vm - ideal_AP[count];

		iAP = ideal_AP[count];
		output(1) = iAP;
	}

	if(corr == 1 && act == 1 && count > 1 && abs(Vm_diff_log[count])>noise_tresh)
	{
		if((Vm_diff_log[count-1] / Vm_diff_log[count]) < 0) //they have the opposite sign, so an overshoot occured
		{
			Rm = Rm * Rm_corr_up;
		}
		if(abs(Vm_diff_log[count-1]) < abs(Vm_diff_log[count]) && (Vm_diff_log[count-1] / Vm_diff_log[count]) > 0) //the difference is getting larger, so increase current
		{
			Rm = Rm / Rm_corr_down;
		}
	}


	if (count > BCL_cutoff*BCL)
	{
		act = 0;
		output(0) = 0;
	}

	count++;

	count_r = (double)count/1000.0;
	count2_r = (double)count2/1000.0;

}

/*
Update
------
ABC

IN:
	*) flag				Indicating the state of the update:
						INIT, MODIFY, PERIOD, PAUSE, UNPAUSE
OUT:
	*) None
*/
void gAPqr7::update(DefaultGUIModel::update_flags_t flag)
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
		setState("Vm2 (mV)", Vm);
		setState("Iout2 (pA)", Iout);
		setState("Time (ms)", systime);
		setState("Period (ms)", period);
		setState("APs2", APs);
		setState("log_ideal_on2", log_ideal_on);
		setState("BCL2", BCL);
		setState("enter2", enter);
		setState("Rm2 (MOhm)", Rm);
		setState("act2", act);
		setState("count", count_r);
		setState("count2", count2_r);
		setState("modulo_state", modulo);
		break;
	case MODIFY:
		Cm = getParameter("Cm (pF)").toDouble();
		Rm = getParameter("Rm (MOhm)").toDouble();
		lognum = getParameter("lognum").toDouble();
		V_cutoff = getParameter("V_cutoff (mV)").toDouble();
		BCL_cutoff = getParameter("BCL_cutoff (pct)").toDouble();
		noise_tresh = getParameter("noise_tresh (mV)").toDouble();
		Rm_corr_up = getParameter("Rm_corr_up").toDouble();
		Rm_corr_down = getParameter("Rm_corr_down").toDouble();
		slope_thresh = getParameter("Slope_thresh (mV/ms)").toDouble();
		corr = getParameter("Correction (0 or 1)").toDouble();		
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
void gAPqr7::initParameters()
{
	Vm = -80; 			// mV
	Cm = 150; 			// pF
	Rm = 150; 			// MOhm
	slope_thresh = 5.0; // mV
	corr = 1;
	Iout = 0;			// pA
	output(0) = -Iout * 0.5e-3;
	period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	systime = 0;
	count = 0;
	act = 0;
	iAP=0;
	Rm_corr_up=8;
	Rm_corr_down=2;
	noise_tresh = 0.5; 	// mV
	BCL = 0;			// ms
	count2 = 0;
	APs = -1;
	V_cutoff = -40;		// mV
	BCL_cutoff = 0.98;
	enter = 0;
	log_ideal_on = 0;
	lognum = 3;
	count_r = 0;
	count2_r = 0;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
}
