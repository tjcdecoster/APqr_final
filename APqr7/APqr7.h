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

#include <default_gui_model.h>
#include <math.h>
#include <string>
#include <vector>

// All parameters and functions related to the gAPqr7 class.
class gAPqr7 : public DefaultGUIModel
{

	public:
		gAPqr7(void);
		virtual ~gAPqr7(void);

		virtual void execute(void);
		
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
		
	private:
		// functions
		void cleanup();
		int i;
		void initParameters();
		// system related parameters
		double systime;
		double period;
		// arrays
		double Vm_log[10000] = {0};		
		double ideal_AP[10000] = {0};
		double Vm_diff_log[10000] = {0};
		// cell related parameters
		double Vm;
		double Cm;
		double Rm;
		// Upstroke related parameters
		double slope_thresh;
		double V_cutoff;
		// logging parameters
		double log_ideal_on;
		double lognum;
		double APs;
		long long count2;
		double iAP;
		// correction parameters
		double act;
		int corr;
		double noise_tresh;
		double Rm_corr_up;
		double Rm_corr_down;

		// standard loop parameters
		long long count;
		double enter;
		double BCL;
		double BCL_cutoff;
		double modulo;
		double Iout;
};
