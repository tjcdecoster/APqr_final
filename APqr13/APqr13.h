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

class gAPqr13 : public DefaultGUIModel
{

	public:
		gAPqr13(void);
		virtual ~gAPqr13(void);

		virtual void execute(void);
		
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
		
	private:
		void cleanup();
		void initParameters();
		double Vm;
		double period;
		double Rm_blue;
		double Rm_red;
		double slope_thresh;
		double VLED1;
		double VLED2;
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
		double blue;
		double red;
		double act;
		double Vm_diff_log[10000] = {0};
		double iAP;
		double Rm_corr_up;
		double Rm_corr_down;
		double lognum;
		double modulo;
		int i;
		double corr_start;
		double blue_Vrev;
		double VLED_max;
};
