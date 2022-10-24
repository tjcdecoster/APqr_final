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
	In this version, output(0) is used to control a 470 nm LED to have depolarizing effect (e.g. by activating 
	Cheriff) and output(1) is used to control a 617 nm LED to have depolarizing effect (e.g. Jaws).
	This version makes use of a new concept, namely that of the PID-controller, where output is regulated based on
	present information (P), past infromation (I) and expected information (D)

	
 */

#include <APqrPID3.h>
#include <math.h>
#include <vector>

extern "C" Plugin::Object *createRTXIPlugin(void)
{
	return new gAPqrPID3();
}

static DefaultGUIModel::variable_t vars[] = {
	{ "Vm (mV)", "Membrane potential (mV)", DefaultGUIModel::INPUT, },
	{ "VLED1", "Output for LED driver", DefaultGUIModel::OUTPUT, },
	{ "VLED2", "Output for LED driver", DefaultGUIModel::OUTPUT, },	
	{ "iAP", "ideal AP", DefaultGUIModel::STATE, },
	{ "V_cutoff (mV)", "Threshold potential for the detection of the beginning of an AP, together with Slope_thresh", 
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Slope_thresh (mV/ms)", "SLope threshold that defines the beginning of the AP (mV/ms)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "BCL_cutoff (pct)", "Threshold value for the end of an AP, given as a percentage of the total APD",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Rm_blue (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_red (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "lognum", "Number of APs that need to be logged as a reference", DefaultGUIModel::PARAMETER
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
	{ "length", "Amount of points that need to be taken into account to find the derivative (slope of the linear trend line of these points)",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "PID_tresh", "treshold value under which the same output as before gets repeated",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "min_PID", "value under which the lights get switched off",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "reset_I_on", "value that indicates whetehr or ont to reset I at RMP",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Vm2 (mV)", "Membrane potential (mV)", DefaultGUIModel::STATE, },
	{ "P", "P term", DefaultGUIModel::STATE, },
	{ "I", "I term", DefaultGUIModel::STATE, },
	{ "D", "D term", DefaultGUIModel::STATE, },
	{ "PID", "PID term", DefaultGUIModel::STATE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, 
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, },
	{ "APs2", "APs", DefaultGUIModel::STATE, },
	{ "log_ideal_on2", "log_ideal_on", DefaultGUIModel::STATE, },
	{ "BCL2", "BCL", DefaultGUIModel::STATE, },
	{ "enter2", "enter", DefaultGUIModel::STATE, },
	{ "act2", "0 or 1", DefaultGUIModel::STATE, },
	{ "count", "number", DefaultGUIModel::STATE, },
	{ "count2", "number", DefaultGUIModel::STATE, },
	{ "modulo_state", "number", DefaultGUIModel::STATE, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

gAPqrPID3::gAPqrPID3(void) : DefaultGUIModel("APqrPID3", ::vars, ::num_vars)
{
	setWhatsThis(
		"<p><b>APqr:</b><br>APqrPID3 </p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	update(INIT);
	refresh();
	resizeMe();
}

gAPqrPID3::~gAPqrPID3(void)
{
}

void gAPqrPID3::cleanup()
{
	for(i=0;i<10000;i++){
		Vm_log[i]=0;
		Vm_diff_log[i]=0;
		ideal_AP[i]=0;
		
	}
}

double gAPqrPID3::sumy(double arr[], int n, double length, double modulo)
{
    double sumy = 0; // initialize sum
 
    // Iterate through all elements
    // and add them to sum
    for (int i = n-length+1 ; i < n+1; i++)
    sumy += arr[i % (int)modulo];
 
    return sumy;
}

double gAPqrPID3::sumxy(double arr[], int n, double length, double period, double modulo)
{
    double sumxy = 0; // initialize sum
	int j = 0;

    // Iterate through all elements
    // and add them to sum
    for (int i = n-length+1 ; i < n+1; i++)
    {
		sumxy += arr[i % (int)modulo] * (j*period);
		j++;
	}
 
    return sumxy;
}

double gAPqrPID3::sumx(double period, double length)
{
	double sumx = 0;

	for (int i = 0; i<length; i++)
	sumx += i*period;

	return sumx;
}

double gAPqrPID3::sumx2(double period, double length)
{
	double sumx2 = 0;

	for (int i = 0; i<length; i++)
	sumx2 += i*period*i*period;

	return sumx2;
}

void gAPqrPID3::execute(void)
{
	systime = count * period; // time in milli-seconds
	Vm = input(0) * 1e2; // convert 10V to mV. Divided by 10 because the amplifier produces 10-fold amplified voltages. Multiplied by 1000 to convert V to mV.

	Vm_log[count % (int)modulo] = Vm;

	// Part of the code to create the ideal AP from the first three beats.
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
	
	// Part of the code that resets the counter once a new AP is detected	
	if (act == 0 && (Vm - Vm_log[(count-(int)(1/period)) % (int)modulo]) >= slope_thresh && APs >= lognum && Vm > V_cutoff)
	{
		count = 0;		
		act = 1;
	}

	// Part of the code that implements the PID
	if (act == 1)
	{
		iAP = ideal_AP[count];
		Vm_diff_log[count] = Vm - ideal_AP[count];
		if (VLED < 5 && (Vm < blue_Vrev || Vm_diff_log[count] > 0)) //this line was modified so that Int is only calculated when the output has not reached its maximum (5V) and either hyperpolarization is needed (red ch) or depolarization is needed (blue ch) and Vm is more negative than blue_Vrev.
			{Int = Int + Vm_diff_log[count];}
		num = length *(sumxy(Vm_diff_log, count, length, period, modulo)) - sumx(period, length)*sumy(Vm_diff_log, count, length, modulo);
		denom = length*sumx2(period, length) - sumx(period, length)*sumx(period, length);
		if (abs(denom) < 0.001)
			{slope = 10000;}
		else
			{slope = num/denom;} // Slope is measured in mV/ms

		P = K_p * Vm_diff_log[count]; // Term that is proportional to the instantaneous difference in voltage.
		I = K_i * Int; // Term that speeds up or slows down the rate of change based on the history of voltage differences.
		D = K_d * slope; // Term that predicts the behaviour that is about to happen and helps in stabilizing.

		PID_diff = PID;
		PID = P + I + D;
		PID_diff = PID_diff - PID;

		if (count >= corr_start-1 && abs(PID_diff) > PID_tresh){
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
	else
	{
		output(0) = 0;
		output(1) = 0;
	}

	// This part of the code resets the I and D terms when the measured AP is very close to resting membrane potential (This
	// is taken as the value close to the end of the ideal AP)
	if (reset_I_on && abs(Vm - ideal_AP[int(BCL_cutoff*BCL)]) < 0.005){
		idx_diff = count - prev_idx;
		prev_idx = count;
		if (idx_diff == 1){
			reset_I_counter +=1;
		}
		else{
			reset_I_counter = 0;
		}
		if (reset_I_counter == length){
			Int = 0;
			reset_I_counter = 0;
		}
	}

	// This part of the code makes sure no output is produced in the last part of the action potential to let the cell come to rest.
	if (count > BCL_cutoff*BCL){
		act = 0;
		output(0) = 0;
		output(1) = 0;
	}

	count++;

	count_r = (double)count/1000.0; // For display purposes
	count2_r = (double)count2/1000.0; // For display purposes

}

void gAPqrPID3::update(DefaultGUIModel::update_flags_t flag)
{
	switch (flag)
	{
	case INIT:
		setParameter("V_cutoff (mV)", V_cutoff);
		setParameter("Rm_blue (MOhm)", Rm_blue);
		setParameter("Rm_red (MOhm)", Rm_red);
		setParameter("lognum", lognum);
		setParameter("BCL_cutoff (pct)", BCL_cutoff);
		setParameter("Slope_thresh (mV/ms)", slope_thresh);
		setParameter("Correction start", corr_start);
		setParameter("Blue_Vrev", blue_Vrev);
		setParameter("K_p", K_p);
		setParameter("K_i", K_i);
		setParameter("K_d", K_d);
		setParameter("length", length);
		setParameter("PID_tresh", PID_tresh);
		setParameter("min_PID", min_PID);
		setParameter("reset_I_on", reset_I_on);
		setState("Vm2 (mV)", Vm);
		setState("Time (ms)", systime);
		setState("Period (ms)", period);
		setState("APs2", APs);
		setState("log_ideal_on2", log_ideal_on);
		setState("BCL2", BCL);
		setState("enter2", enter);
		setState("act2", act);
		setState("count", count_r);
		setState("count2", count2_r);
		setState("modulo_state", modulo);
		setState("iAP", iAP);
		setState("P", P);
		setState("I", I);
		setState("D", D);
		setState("PID", PID);
		break;
	case MODIFY:
		lognum = getParameter("lognum").toDouble();
		BCL_cutoff = getParameter("BCL_cutoff (pct)").toDouble();
		Rm_blue = getParameter("Rm_blue (MOhm)").toDouble();
		Rm_red = getParameter("Rm_red (MOhm)").toDouble();
		slope_thresh = getParameter("Slope_thresh (mV/ms)").toDouble();
		V_cutoff = getParameter("V_cutoff (mV)").toDouble();
		corr_start = getParameter("Correction start").toDouble();
		blue_Vrev = getParameter("Blue_Vrev").toDouble();
		K_p = getParameter("K_p").toDouble();
		K_i = getParameter("K_i").toDouble();
		K_d = getParameter("K_d").toDouble();
		length = getParameter("length").toDouble();
		PID_tresh = getParameter("PID_tresh").toDouble();
		min_PID = getParameter("min_PID").toDouble();
		reset_I_on = getParameter("reset_I_on").toDouble();
		systime = 0;
		count = 0;
		APs = -1;
		BCL = 0;
		log_ideal_on = 0;
		enter = 0;
		count2 = 0;
		PID = 0;
		PID_diff = 0;
		Int = 0;
		cleanup();
		break;
	case PERIOD:
		period = RT::System::getInstance()->getPeriod() * 1e-6; // time in milli-seconds
		modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
		break;
	case PAUSE:
		output(0) = 0.0;
		output(1) = 0.0;
		act = 0;
		systime = 0;
		break;
	case UNPAUSE:
		break;
	default:
		break;
	}
}

void gAPqrPID3::initParameters()
{
	Vm = -80; // mV
	Rm_blue = 150; // MOhm
	Rm_red = 50; // MOhm
	slope_thresh = 5.0; // mV
	corr_start = 0;
	blue_Vrev = -20;
	VLED = 0;
	output(0) = 0;
	output(1) = 0;
	period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	systime = 0;
	count = 0;
	idx_diff = 0;
	prev_idx = 0;
	reset_I_counter = 0;
	PID = 0;
	length = 10;
	act = 0;
	iAP=0;
	BCL = 0;
	count2 = 0;
	APs = -1;
	V_cutoff = -40;
	BCL_cutoff = 0.8;
	enter = 0;
	log_ideal_on = 0;
	lognum = 3;
	count_r = 0;
	count2_r = 0;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
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
	reset_I_on = 0;
}
