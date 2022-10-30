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
#include <plotdialog.h>
#include <basicplot.h>

// All parameters and functions related to the gAPqrPIDLTLP4 class.
class APqrPIDLTLP4 : public DefaultGUIModel
{
    Q_OBJECT

public:
    APqrPIDLTLP4(void);
    virtual ~APqrPIDLTLP4(void);

    void execute(void);
    void customizeGUI(void);

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
	double dt;
	// arrays
	double Vm_log[10000] = {0};		
	double Vm_diff_log[10000] = {0};
	// cell related parameters
	double Vm;
	double Rm_blue;
	double Rm_red;
	// Upstroke related parameters
	double slope_thresh;
	double V_cutoff;
    double pulse_strength;
    double V_light_on;
    // file reading related parameters
    QString filename;
    std::vector<double> wave;
    double gain;
    double offset;
    size_t loop;
    size_t nloops;
    double length;
    double iAP;
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
	double dlength;
	double num;
	double denom;
	double slope;
	double PID_diff;

	// standard loop parameters
    size_t idx;
    size_t idx2;
    double act_copy;
    double PID_copy;
    double idx_copy;
    double idx2_copy;
	double modulo;
	double VLED;

private slots:
    // all custom slots
    void loadFile();
    void loadFile(QString);
    void previewFile();
};
