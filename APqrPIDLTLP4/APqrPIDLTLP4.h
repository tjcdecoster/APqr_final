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
* Reads values from ACSII file and adjusts a light-based signal in real-time
*/

#include <default_gui_model.h>
#include <plotdialog.h>
#include <basicplot.h>

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
    // inputs, states

    double dt;
    QString filename;
    size_t idx;
    size_t idx2;
    double act_copy;
    double PID_copy;
    double idx_copy;
    double idx2_copy;
    double Vm_V;
    double iAP_V;
    size_t sync_AP;
    size_t loop;
    size_t nloops;
    std::vector<double> wave;
    double length;
    double gain;
    double offset;
    double RMP;
    double slope_thresh;
    double pulse_strength;
    double Vm;
    double Rm_blue;
    double Rm_red;
    double VLED;
    double blue;
    double red;
    double systime;
    double act;
    double Vm_log[10000] = {0};
    double Vm_diff_log[10000] = {0};
    double iAP;
    double modulo;
    double V_light_on;
    double V_cutoff;
    double corr_start;
    double blue_Vrev;
    double PID;
    double dlength;
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
    
    // WaveMaker functions
    void initParameters();
    void cleanup();
    double sumy(double arr[], int n, double length, double modulo);
    double sumxy(double arr[], int n, double length, double period, double modulo);
    double sumx(double period, double length);
    double sumx2(double period, double length);

private slots:
    // all custom slots
    void loadFile();
    void loadFile(QString);
    void previewFile();
};
