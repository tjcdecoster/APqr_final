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
	Note that the description was wrong in APqr10 and APqr11 (the description of the functionality of the two output channels were mixed up).
	
	In version 13 compared to version 12: 
	*) Output on the 'blue' channel is produced only if Vm is more negative than the Blue_Vrev parameter. This allows to use blue channelrhodopsins for depolarizing effect.
	*) Blue_Vrev is set to 60 mV by default.
	*) Noise threshold is set to 0.5 mV by default.
	*) corr_start is set to 0 by default.
 */

#include <APqr13.h>
#include <math.h>
#include <vector>

extern "C" Plugin::Object *createRTXIPlugin(void)
{
	return new gAPqr13();
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
	{ "noise_tresh (mV)", "The noise level that is allowed before correcting", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, }, 
	{ "Rm_blue (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_red (MOhm)", "MOhm", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "lognum", "Number of APs that need to be logged as a reference", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_corr_up", "To increase Rm when necessary", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, },
	{ "Rm_corr_down", "To decrease Rm when necessary", DefaultGUIModel::PARAMETER
	| DefaultGUIModel::DOUBLE, }, 
	{ "Correction start", "iAP count (index+1) when correction starts",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Blue_Vrev", "Apparent reversal potential of the 'blue' ChR current",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "VLED_max", "Maximum VLED output value (V).",
	DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },	
	{ "Vm2 (mV)", "Membrane potential (mV)", DefaultGUIModel::STATE, },
	{ "Period (ms)", "Period (ms)", DefaultGUIModel::STATE, }, 
	{ "Time (ms)", "Time (ms)", DefaultGUIModel::STATE, },
	{ "APs2", "APs", DefaultGUIModel::STATE, },
	{ "log_ideal_on2", "log_ideal_on", DefaultGUIModel::STATE, },
	{ "BCL2", "BCL", DefaultGUIModel::STATE, },
	{ "enter2", "enter", DefaultGUIModel::STATE, },
	{ "Rm_blue2 (MOhm)", "MOhm", DefaultGUIModel::STATE, },
	{ "Rm_red2 (MOhm)", "MOhm", DefaultGUIModel::STATE, },
	{ "act2", "0 or 1", DefaultGUIModel::STATE, },
	{ "count", "number", DefaultGUIModel::STATE, },
	{ "count2", "number", DefaultGUIModel::STATE, },
	{ "modulo_state", "number", DefaultGUIModel::STATE, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

gAPqr13::gAPqr13(void) : DefaultGUIModel("APqr13", ::vars, ::num_vars)
{
	setWhatsThis(
		"<p><b>APqr:</b><br>APqr13 </p>");
	DefaultGUIModel::createGUI(vars, num_vars);
	initParameters();
	update(INIT);
	refresh();
	resizeMe();
}

gAPqr13::~gAPqr13(void)
{
}

void gAPqr13::cleanup()
{
	for(i=0;i<10000;i++){
		Vm_log[i]=0;
		Vm_diff_log[i]=0;
		ideal_AP[i]=0;
		
	}
}

void gAPqr13::execute(void)
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
		iAP = ideal_AP[count];
		Vm_diff_log[count] = Vm - ideal_AP[count];		

		if(count >= corr_start-1 && abs(Vm_diff_log[count])>noise_tresh && blue == 1)
		{
			if((Vm_diff_log[count-1] / Vm_diff_log[count]) < 0) //they have the opposite sign, so an overshoot occured
			{
				Rm_blue = Rm_blue * Rm_corr_up;
			}
			if(abs(Vm_diff_log[count-1]) < abs(Vm_diff_log[count]) && (Vm_diff_log[count-1] / Vm_diff_log[count]) > 0 && Rm_blue >= 0.01*Rm_corr_down) //the difference is getting larger, so increase current
			{
				Rm_blue = Rm_blue / Rm_corr_down;
			}
		}

		if(count >= corr_start-1 && abs(Vm_diff_log[count])>noise_tresh && red == 1)
		{
			if((Vm_diff_log[count-1] / Vm_diff_log[count]) < 0) //they have the opposite sign, so an overshoot occured
			{
				Rm_red = Rm_red * Rm_corr_up;
			}
			if(abs(Vm_diff_log[count-1]) < abs(Vm_diff_log[count]) && (Vm_diff_log[count-1] / Vm_diff_log[count]) > 0 && Rm_red >= 0.01*Rm_corr_down) //the difference is getting larger, so increase current
			{
				Rm_red = Rm_red / Rm_corr_down;
			}
		}		

		if (count >= corr_start-1){
			if (Vm_diff_log[count] < 0 && Vm < blue_Vrev)
			{		
				VLED1 = -Vm_diff_log[count] * (1/Rm_blue);			
				if (VLED1 > VLED_max){VLED1 = VLED_max;}
				blue = 1;
				red = 0;			
				
				
				output(0) = VLED1;
				output(1) = 0;
			}
			else if (Vm_diff_log[count] > 0)
			{
				VLED2 = Vm_diff_log[count] * (1/Rm_red);
				if (VLED2 > VLED_max){VLED2 = VLED_max;}
				blue = 0;
				red = 1;
				output(1) = VLED2;
				output(0) = 0;		
			}
			else
			{
				output(0) = 0;
				output(1) = 0;
				blue = 0;
				red = 0;
			}		
		
		}
		else
		{
			output(0) = 0;
			output(1) = 0;
			blue = 0;
			red = 0;
		}
	}
	else
	{
		output(0) = 0;
		output(1) = 0;
		blue = 0;
		red = 0;
	}
	if (count > BCL_cutoff*BCL){
		act = 0;
		output(0) = 0;
	}

	count++;

	count_r = (double)count/1000.0;
	count2_r = (double)count2/1000.0;

}

void gAPqr13::update(DefaultGUIModel::update_flags_t flag)
{
	switch (flag)
	{
	case INIT:
		setParameter("V_cutoff (mV)", V_cutoff);
		setParameter("Rm_blue (MOhm)", Rm_blue);
		setParameter("Rm_red (MOhm)", Rm_red);
		setParameter("Rm_corr_up", Rm_corr_up);
		setParameter("Rm_corr_down", Rm_corr_down);
		setParameter("noise_tresh (mV)", noise_tresh);
		setParameter("lognum", lognum);
		setParameter("BCL_cutoff (pct)", BCL_cutoff);
		setParameter("Slope_thresh (mV/ms)", slope_thresh);
		setParameter("Correction start", corr_start);
		setParameter("Blue_Vrev", blue_Vrev);
		setParameter("VLED_max", VLED_max);
		setState("Vm2 (mV)", Vm);
		setState("Time (ms)", systime);
		setState("Period (ms)", period);
		setState("APs2", APs);
		setState("log_ideal_on2", log_ideal_on);
		setState("BCL2", BCL);
		setState("enter2", enter);
		setState("Rm_blue2 (MOhm)", Rm_blue);
		setState("Rm_red2 (MOhm)", Rm_red);
		setState("act2", act);
		setState("count", count_r);
		setState("count2", count2_r);
		setState("modulo_state", modulo);
		setState("iAP", iAP);
		break;
	case MODIFY:
		lognum = getParameter("lognum").toDouble();
		BCL_cutoff = getParameter("BCL_cutoff (pct)").toDouble();
		noise_tresh = getParameter("noise_tresh (mV)").toDouble();
		Rm_blue = getParameter("Rm_blue (MOhm)").toDouble();
		Rm_red = getParameter("Rm_red (MOhm)").toDouble();
		Rm_corr_up = getParameter("Rm_corr_up").toDouble();
		Rm_corr_down = getParameter("Rm_corr_down").toDouble();
		slope_thresh = getParameter("Slope_thresh (mV/ms)").toDouble();
		V_cutoff = getParameter("V_cutoff (mV)").toDouble();
		corr_start = getParameter("Correction start").toDouble();
		blue_Vrev = getParameter("Blue_Vrev").toDouble();
		VLED_max = getParameter("VLED_max").toDouble();
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
		output(1) = 0.0;
		act = 0;
		systime = 0;
		VLED_max = getParameter("VLED_max").toDouble();
		break;
	case UNPAUSE:
		break;
	default:
		break;
	}
}

void gAPqr13::initParameters()
{
	Vm = -80; // mV
	Rm_blue = 150; // MOhm
	Rm_red = 150; // MOhm
	slope_thresh = 5.0; // mV
	corr_start = 0;
	blue_Vrev = -20;
	VLED_max = 5;
	VLED1 = 0;
	VLED2 = 0;
	output(0) = 0;
	output(1) = 0;
	period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
	systime = 0;
	count = 0;
	blue = 0;
	red = 0;
	act = 0;
	iAP=0;
	Rm_corr_up=8;
	Rm_corr_down=2;
	noise_tresh = 0.5; // mV
	BCL = 0;
	count2 = 0;
	APs = -1;
	V_cutoff = -40;
	BCL_cutoff = 0.98;
	enter = 0;
	log_ideal_on = 0;
	lognum = 3;
	count_r = 0;
	count2_r = 0;
	modulo = (1.0/(RT::System::getInstance()->getPeriod() * 1e-6)) * 1000.0;
}
