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

#include <default_gui_model.h>
#include <math.h>
#include <string>
#include <vector>

// All parameters and functions related to the gAPqrPID3 class.
class gAPqrPID3 : public DefaultGUIModel
{

	public:
		gAPqrPID3(void);
		virtual ~gAPqrPID3(void);

		virtual void execute(void);
		
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
		
	private:
		// functions
		void cleanup();
		long long i;
		void initParameters();
		double sumy(double arr[], int n, double length, double modulo);
		double sumxy(double arr[], int n, double length, double period, double modulo);
		double sumx(double period, double length);
		double sumx2(double period, double length);
		// system related parameters
		double systime;
		double period;
		// arrays
		double Vm_log[10000] = {0};		
		double ideal_AP[10000] = {0};
		double Vm_diff_log[10000] = {0};
		// cell related parameters
		double Vm;
		double Rm_blue;
		double Rm_red;
		// Upstroke related parameters
		double slope_thresh;
		double V_cutoff;
		// logging parameters
		double log_ideal_on;
		double lognum;
		double APs;
		long long count2;
		// correction parameters
		double act;
		double corr_start;
		double PID_tresh;
		double min_PID;
		double blue_Vrev;

		double PID;
		double P;
		double I;
		double D;
		double K_p;
		double K_i;
		double K_d;
		double Int;
		double length;
		double num;
		double denom;
		double slope;
		double PID_diff;

		double reset_I_on;
		double idx_diff;
		double prev_idx;
		double reset_I_counter;

		// standard loop parameters
		long long count;
		double enter;
		double BCL;
		double BCL_cutoff;
		double modulo;
		double VLED;
};
