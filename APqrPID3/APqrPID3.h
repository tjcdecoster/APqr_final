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

class gAPqrPID3 : public DefaultGUIModel
{

	public:
		gAPqrPID3(void);
		virtual ~gAPqrPID3(void);

		virtual void execute(void);
		
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
		
	private:
		void cleanup();
		void initParameters();
		double sumy(double arr[], int n, double length, double modulo);
		double sumxy(double arr[], int n, double length, double period, double modulo);
		double sumx(double period, double length);
		double sumx2(double period, double length);
		double Vm;
		double period;
		double Rm_blue;
		double Rm_red;
		double slope_thresh;
		double VLED;
		double systime;
		double count_r;
		double count2_r;
		long long count;
		double Vm_log[10000] = {0};		
		double ideal_AP[10000] = {0};
		long long count2;
		double enter;
		double BCL;
		double BCL_cutoff;
		double noise_tresh;
		double V_cutoff;
		double log_ideal_on;
		double APs;
		double act;
		long long i;
		double Vm_diff_log[10000] = {0};
		double iAP;
		double lognum;
		double modulo;
		double corr_start;
		double blue_Vrev;
		double PID;
		double length;
		double Int;
		double num;
		double denom;
		double slope;
		double P;
		double I;
		double D;
		double K_p;
		double K_i;
		double K_d;
		double PID_diff;
		double PID_tresh;
		double min_PID;
		double reset_I_on;
		double idx_diff;
		double prev_idx;
		double reset_I_counter;
};
