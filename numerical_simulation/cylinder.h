#ifndef cylinder
#define cylinder

#include <stdexcept>
#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>



// 计算域尺寸：600 × 600
const int MM = 600;
const int NN = 600;

// 外圈障碍物圆心所在圆的半径。
// RR = 125 时，最左与最右外圈障碍物圆心间距为 250；
// 障碍物直径为 15，因此整个障碍物场外缘直径为 250 + 15 = 265。
const int RR = 125;

// 几何参数：将主动颗粒初始位置与障碍物布置半径彻底分离。
const double OBSTACLE_RADIUS = 7.5;
const double ACTIVE_INIT_RADIUS = 170.0;
const double ACTIVE_TARGET_RADIUS = 0.5 * RR;
const double INITIAL_CLEARANCE = 2.0;

const double dl = 1.0;
const double dx = 1.0;
const double dy = 1.0;
const double dt = 1.0;


double Re = 0.01;
double Bata = -1;
const int FNum = 4;
const int D = 9;


#define Rou 1.0
#define FRou 1.000
#define dddt 5e-6
#define dddx 1e-5

double UU1 = 0.0;

int FLUID = -1;
int WALL = -2;
int INLET = -3;
int OUTLET = -4;
int BLANK = -5;

double n1 = 1.0;

const double AR = 0.72;
const double init_a = 5.8;
const double alpha = (1 - AR) / (1 + AR);
const double belta = init_a * (1 + AR) / 2;
const double g = 9.8;
const double gl = g * dddt * dddt / dddx;
const double L = 10 * 4;
const double nju=0.05;
const int MM1 = MM + 1, NN1 = NN + 1;

int x_center=MM/2, y_center=NN/2, r_center=MM/2;

int ni[MM1][NN1][2], nj[MM1][NN1][2];
int nb_i[MM1][NN1][D], nb_j[MM1][NN1][D];


double cxx;
double d_min_l, d_min_r, d_min_u, d_min_d;
double BBB1 = Re * nju/(belta*(1+alpha)*(1+alpha));
double BBB2 = Bata * BBB1;
double S89 = 1.0 / (0.750 * BBB1 * L / Re + 0.50);
double ang,bng;

double w[9];
double M[D][D];
double M1[D][D];
double Dm[D][D], Fm[D][D];
double A0, D0, A1, B1, C1, D1, A2, B2, C2, D2;
double FX, FY;
int storage;
int xl[2000];
int xr[2000];
int yb[2000];
int yt[2000];


double *rou;
double **gama;
double **tau;
CVector *u;
CVector *dudx;
CVector *dudy;
CVector *FOL;


double deltf(CVector r) {
    double drx = r.x / dx;
    double dry = r.y / dy;

    double adrx = fabs(drx);
    double adry = fabs(dry);

    double df, dfx, dfy;
    if (adrx <= 0.5) {
        dfx = 3. / 8. + Pi / 32. - pow(drx, 2) / 4.;
    } else if (adrx <= 1.5) {
        dfx = 1. / 4. + (1 - adrx) / 8. * sqrt(-2 + 8 * adrx - 4 * pow(drx, 2)) - 1. / 8. * asin(sqrt(2) * (adrx - 1));
    } else if (adrx <= 2.5) {
        dfx = 17. / 16. - Pi / 64 + (adrx - 2) / 16. * sqrt(-14 + 16 * adrx - 4 * pow(drx, 2)) + 1. / 16. * asin(sqrt(2) * (adrx - 2)) + pow(drx, 2) / 8. - 3 * adrx / 4.;
    } else {
        dfx = 0;
    }

    if (adry <= 0.5) {
        dfy = 3. / 8. + Pi / 32. - pow(dry, 2) / 4.;
    } else if (adry <= 1.5) {
        dfy = 1. / 4. + (1 - adry) / 8. * sqrt(-2 + 8 * adry - 4 * pow(dry, 2)) - 1. / 8. * asin(sqrt(2) * (adry - 1));
    } else if (adry <= 2.5) {
        dfy = 17. / 16. - Pi / 64 + (adry - 2) / 16. * sqrt(-14 + 16 * adry - 4 * pow(dry, 2)) + 1. / 16. * asin(sqrt(2) * (adry - 2)) + pow(dry, 2) / 8. - 3 * adry / 4.;
    } else {
        dfy = 0;
    }
    df = 1 / dx * dfx * 1 / dx * dfy;
    return df;
}

class SPEED {
public:
    CVector e[D];
};

class DISTRIB {
public:
    double f[D];
    int flag;
};

/**
 * 二维数组初始化
 * @param size 方阵大小
 * @return 初始化后方阵
 */
DISTRIB **distribMatInit(int m, int n) {
    DISTRIB **res = new DISTRIB *[m];

    for (int i = 0; i < m; i++) {
        res[i] = new DISTRIB[n];
    }
    return res;
}

class Larg {
public:
    double theta = 0;
    double phi = 0;

    double ds = 0;
    double T = 0;
    CVector x = CVector();
    CVector u = CVector();
    CVector F = CVector();

    bool checkStatus() const {

        return true;
    }
};

class FIBER {
public:
    int index = 0;
    int num_l = 0;

    double a = 0;
    double b = 0;
    /**
    * 周长
    */
    double per = 0.;

    double theta = 0.;   //椭圆的旋转角度

    double h = 0.;
    double w = 0.;
    double T = 0.;
    double TT = 0.;

    double PRou = 1.0;
    double FMass = 0.;
    double I = 0.;

    CVector P0 = CVector();
    CVector P = CVector();
    CVector x = CVector();
    CVector u = CVector();

    CVector F = CVector();
    CVector FF = CVector();
    CVector Fg = CVector();

    Larg *largs{};


    void initTheta(int _index, double _theta) {
        index = _index;
        theta = _theta;
        P = CVector(cos(theta), sin(theta));
    }

    friend ostream &operator<<(ostream &os, const FIBER &fiber) {
        os << "x=" << fiber.x << "\ta=" << fiber.a << "\tb=" << fiber.b << "\ttheta=" << fiber.theta;
        return os;
    }

    bool checkStatus() const {
        if (x.x < 0 || x.y < 0) {
            cout << "超出边界" << endl;
        }
        for (int l = 0; l < num_l; l++) {
            largs[l].checkStatus();
        }
        return true;
    }

    /**
  * 初始化椭圆大小，并计算质量，惯性矩
  * @param a 长轴
  * @param b 短轴
  */
    void initSize(double _a, double _b) {
        a = _a;
        b = _b;
        if (a >= b) {
            per = 2 * Pi * b + 4 * (a - b);
        } else {
            per = 2 * Pi * a + 4 * (b - a);
        }
        //	fibers[i].P0=fibers[i].x-fibers[i].P*fibers[i].h/2.0;
        //	FMass=FRou*Pi*fibersPre[0].h*fibersPre[0].r*fibersPre[0].r;
        //	I=FMass*(fibersPre[0].h*fibersPre[0].h+3.0*fibersPre[0].r*fibersPre[0].r)/12.0;

        // FMass=FRou*fibers[0].h*fibers[0].h*Pi*0.25;
        // I=FMass*fibers[0].h*fibers[0].h*0.5*0.25;

        // FMass = FRou * fibers[0].h * fibers[0].h * Pi * 0.25 * FNum;

        // I = FMass * fibers[0].h * fibers[0].h;
        FMass = PRou * Pi * a * b;
        I = 0.25 * FMass * (pow(a, 2) + pow(b, 2));
        Fg = CVector(0., -(1 - FRou / PRou) * FMass * gl);
    }

    void initLarg(int _num_l) {
        num_l = _num_l;
        largs = new Larg[num_l];
    }

    void initLargPhi() const {
        for (int l = 0; l < num_l; l++) {
            double x0 = a * cos(largs[l].theta);
            double y0 = b * sin(largs[l].theta);
            double tPhi; // =tan(phi)
            if (x0 == 0) {
                tPhi = 0;
            } else {
                tPhi = a * y0 / (b * x0);
            }

            if (tPhi == 0) {
                if (x0 == 0) {
                    if (y0 > 0) {
                        largs[l].phi = Pi / 2;
                    } else {
                        largs[l].phi = 3 * Pi / 2;
                    }
                }
            } else {
                double beta = atan(tPhi);
                if (x0 > 0 && y0 > 0) {//第一象限
                    largs[l].phi = beta;
                } else if (x0 < 0 && y0 >= 0) {//第二象限
                    largs[l].phi = beta + Pi;
                } else if (x0 < 0 && y0 <= 0) {//第三象限
                    largs[l].phi = beta + Pi;
                } else {//第四象限
                    largs[l].phi = beta + 2 * Pi;
                }
            }
        }
    }

    void initLargTheta() const {
        int P_dis_n = 50000;
        double P_dis[50000];
        double PL_ds_temp[num_l + 4], PL_theta_temp[num_l + 4];
        splitAngleIntoN(a, b, P_dis_n, P_dis);
        double dL = 4. * P_dis[P_dis_n - 1];
        double ds = 4. * P_dis[P_dis_n - 1] / num_l;
        double dtheta = Pi / 2 / P_dis_n;
        double pre = 0.;
        int pre_index = 0;
        for (int l = 0; l < num_l + 4; l++) {
            if (l < (num_l + 4) / 4) {
                for (int ii = pre_index; ii < P_dis_n; ii++) {
                    int i = ii;
                    double t_start = i * dtheta;
                    if ((P_dis[ii] >= (l + 1.0 / 2) * ds) || ii == P_dis_n - 1) {
                        PL_ds_temp[l] = P_dis[ii] - pre;
                        PL_theta_temp[l] = t_start;
                        pre = P_dis[ii];
                        pre_index = ii + 1;
                        break;
                    }
                }
            } else if (l < (num_l + 4) / 2 - 1) {
                int i = (num_l + 4) / 2 - l - 2;
                PL_ds_temp[l] = PL_ds_temp[i + 1];
                PL_theta_temp[l] = Pi - PL_theta_temp[i];
            } else if (l == (num_l + 4) / 2 - 1) {
                PL_ds_temp[l] = PL_ds_temp[0];
                PL_theta_temp[l] = Pi;
            } else if (l < (num_l + 4) / 4 * 3) {
                int i = l - (num_l + 4) / 2;
                PL_ds_temp[l] = PL_ds_temp[i];
                PL_theta_temp[l] = Pi + PL_theta_temp[i];
            } else if (l < (num_l + 4) - 1) {
                int i = (num_l + 4) - l - 2;
                PL_ds_temp[l] = PL_ds_temp[i + 1];
                PL_theta_temp[l] = 2 * Pi - PL_theta_temp[i];
            } else if (l == (num_l + 4) - 1) {
                PL_ds_temp[l] = PL_ds_temp[0];
                PL_theta_temp[l] = 2 * Pi;
            }
        }
        if (a < b) {
            for (int l = 0; l < (num_l + 4); l++) {
                PL_theta_temp[l] = PL_theta_temp[l] + Pi / 2;
            }
        }
        for (int l = 0; l < num_l; l++) {
            largs[l].ds = ds;
            // if (l == 0) {
            // largs[l].theta = PL_theta_temp[l];
            // } else
            if (l < num_l / 4) {
                largs[l].theta = PL_theta_temp[l];
                // } else if (l == num_l / 4) {
                // largs[l].theta = PL_theta_temp[l + 1];
            } else if (l < num_l / 2) {
                largs[l].theta = PL_theta_temp[l + 1];
                // } else if (l == num_l / 2) {
                // largs[l].theta = PL_theta_temp[l + 2];
            } else if (l < num_l / 4 * 3) {
                largs[l].theta = PL_theta_temp[l + 2];
                // } else if (l == num_l / 4 * 3) {
                // largs[l].theta = PL_theta_temp[l + 3];
            } else if (l <= num_l - 1) {
                largs[l].theta = PL_theta_temp[l + 3];
            }
        }

    }


    CVector convertCoordinate(double theta1) const {
        return CVector(cos(theta) * a * cos(theta1) - sin(theta) * b * sin(theta1) + x.x,
                       sin(theta) * a * cos(theta1) + cos(theta) * b * sin(theta1) + x.y);
    }

    CVector convertCoordinate(CVector point) const {
        return CVector(cos(theta) * point.x - sin(theta) * point.y + x.x,
                       sin(theta) * point.x + cos(theta) * point.y + x.y);
    }

    CVector convertCoordinateInverse(CVector point) const {
        return CVector(
                cos(theta) * (point.x - x.x) + sin(theta) * (point.y - x.y),
                -sin(theta) * (point.x - x.x) + cos(theta) * (point.y - x.y)
        );
    }

    void updateLargePosition() const {
        for (int l = 0; l < num_l; l++) {
            this->largs[l].x = this->convertCoordinate(this->largs[l].theta);
            //this->largs[l].x.x = a * cos(largs[l].theta) * cos(theta) - b * sin(largs[l].theta) * sin(theta) + x.x;
            //this->largs[l].x.y = a * cos(largs[l].theta) * sin(theta) + b * sin(largs[l].theta) * cos(theta) + x.y;
        }
    }

    /**
    * 计算参数\theta 对应的点的切线角度 (0-2π)(指向逆时针方向)
    * @return
    */
    double culAngleOfTangent(double _theta) const {
        _theta = angleReSection(_theta);
        if (approximateEqual(_theta, 0)) { //x轴+
            return Pi / 2 + theta;
        } else if (approximateEqual(_theta, Pi / 2)) { //y轴+
            return Pi + theta;
        } else if (approximateEqual(_theta, Pi)) { //x轴-
            return Pi * 3 / 2 + theta;
        } else if (approximateEqual(_theta, Pi * 3 / 2)) { //y轴-
            return 0 + theta;
        }
        double beta = -(atan(b / a / tan(_theta))) + theta;

        if (_theta < Pi / 2) {
            // 第一象限
            return beta + Pi;
        } else if (_theta < Pi) {
            // 第二象限
            return beta + Pi;
        } else if (_theta < Pi * 3 / 2) {
            // 第三象限
            return beta + 2 * Pi;
        } else if (_theta < Pi * 2) {
            // 第四象限
            return beta;
        }
        cout << "the theta is error:" << _theta << endl;
        return 0.;
    }

    /**
   * 根据切线角度，反过来计算参数\theta (0-2π)(指向逆时针方向)
   * @return
   */
    double culAngleOfTangentInverse(double _theta) const {
        _theta = angleReSection(_theta - theta);
        if (approximateEqual(_theta, 0)) { //x轴+
            return Pi * 3 / 2;
        } else if (approximateEqual(_theta, Pi / 2)) { //y轴+
            return 0;
        } else if (approximateEqual(_theta, Pi)) { //x轴-
            return Pi / 2;
        } else if (approximateEqual(_theta, Pi * 3 / 2)) { //y轴-
            return Pi;
        }
        double beta = -atan(b / a / tan(_theta));

        if (_theta < Pi / 2) {
            // 第四象限
            return angleReSection(beta);
        } else if (_theta < Pi) {
            // 第二象限
            return beta;
        } else if (_theta < Pi * 3 / 2) {
            // 第三象限
            return beta + Pi;
        } else if (_theta < Pi * 2) {
            // 第四象限
            return beta + Pi;
        }

        throw std::runtime_error("the theta is error: ");
        //return 0.;
    }
};

FIBER fibersPre[FNum];
FIBER fibers[FNum];
FIBER fiber_total[2];

SPEED e;
DISTRIB *f;
DISTRIB **flows;
DISTRIB *Rf;


class ParamInit {
public:

    static void applyMemory() {
        storage = int(MM1 * NN1);
        rou = new double[storage];
        gama = matInit(MM1, NN1);
        tau = matInit(MM1, NN1);
        u = new CVector[storage];
        dudx = new CVector[storage];
        dudy = new CVector[storage];
        FOL = new CVector[storage];
        Rf = new DISTRIB[storage];
        f = new DISTRIB[storage];
        //flows = distribMatInit(MM1, NN1);
    }

/**
* 常量，直接赋值，无需计算，不跟随实例变化
*/
    static void initConstParam() {
        e.e[0] = CVector(0.0, 0.0);
        e.e[1] = CVector(1.0, 0.0);
        e.e[3] = CVector(-1.0, 0.0);
        e.e[2] = CVector(0.0, 1.0);
        e.e[4] = CVector(0.0, -1.0);

        e.e[5] = CVector(1.0, 1.0);
        e.e[7] = CVector(-1.0, -1.0);
        e.e[6] = CVector(-1.0, 1.0);
        e.e[8] = CVector(1.0, -1.0);

        A0 = 4.0 / 9.0;
        D0 = -2.0 / 3.0;
        A1 = 1.0 / 9.0;
        B1 = 1.0 / 3.0;
        C1 = 1.0 / 2.0;
        D1 = -1.0 / 6.0;
        A2 = 1.0 / 36.0;
        B2 = 1.0 / 12.0;
        C2 = 1.0 / 8.0;
        D2 = -1.0 / 24.0;

        for (int k = 0; k < D; k++) {
            if (k == 0) {
                w[k] = 4.0 / 9.0;
            } else if (k <= 4) {
                w[k] = 1.0 / 9.0;
            } else {
                w[k] = 1.0 / 36.0;
            }
        }
    }
};

class PlotUtils {
public:
    static void plot_speed(int step) {
        FILE *fp;
        char name[50];
        sprintf(name, "speed_simu%d.dat", step);
        fp = fopen(name, "w");

        int i = MM;
        for (int j = 0; j < NN1; j++) {
            fprintf(fp, "%d %lf\n", j, u[i * NN1 + j].x);
        }
        fclose(fp);
    };

    static void plot_coefficient(int step) {
        // Cdp&Cdf
        FILE *fp;

        char name[50];
        sprintf(name, "Coefficient.dat"); // print formatted data to string
        fp = fopen(name, "w");
        fprintf(fp, "VARIABLES = step,theta,cd,cl\n");


        for (int n = 0; n < FNum; n++) {
            double P_u;
            double PL_rho[fibers[n].num_l];
            double PL_cd[fibers[n].num_l], PL_cl[fibers[n].num_l];


            for (int l = 0; l < fibers[n].num_l; l++) {
                PL_rho[l] = 0.0;
            }

            double UU = (1 + alpha) * BBB1 / 2.;
            P_u = fibers[n].x.x * fibers[n].x.x + fibers[n].x.y * fibers[n].x.y;
            // cout << P_u[n] << endl;
            for (int l = 0; l < fibers[n].num_l; l++) {
                if (fibers[n].a <= fibers[n].b) {
                    PL_cd[l] = 2.0 * (-fibers[n].largs[l].F.x) / (Rou * UU * UU * 2 * fibers[n].a);
                    PL_cl[l] = 2.0 * (-fibers[n].largs[l].F.y) / (Rou * UU * UU * 2 * fibers[n].a);
                } else {
                    PL_cd[l] = 2.0 * (-fibers[n].largs[l].F.x) / (Rou * UU * UU * 2 * fibers[n].b);
                    PL_cl[l] = 2.0 * (-fibers[n].largs[l].F.y) / (Rou * UU * UU * 2 * fibers[n].b);
                }
            }

            for (int l = 0; l < fibers[n].num_l; l++) {
                fprintf(fp, "%d %lf %lf %lf\n", step, fibers[n].largs[l].theta * 180 / Pi, PL_cd[l], PL_cl[l]);
            }
        }
        fclose(fp);
    };

    static void plot_ellipse_F(int step, int num_l, int N, int n) {
        mkdir("03_particle_force");
        FILE *fp;
        int l, a1, b1, c1;
        a1 = b1 = 1;
        c1 = 2;

        char name[50];
        sprintf(name, "03_particle_force/ParticleF_%d_%d.dat", n, step); // print formatted data to string
        fp = fopen(name, "w");
        fprintf(fp, "VARIABLES = X,Y,t,ux,uy,FX,FY,T,theta\n");
        fprintf(fp, "ZONE F=FEPOINT, N=%d, E=%d,ET=TRIANGLE\n", num_l + 1, num_l);

        fprintf(fp, "%lf %lf %d %lf %lf %lf %lf %lf %lf\n", fibers[n].x.x, fibers[n].x.y, 1, 0.0, 0.0, 0.0, 0.0, 0.0,
                0.0);

        for (l = 0; l < num_l; l++) {
            fprintf(fp, "%lf %lf %d %lf %lf %lf %lf %lf %lf\n", fibers[n].largs[l].x.x, fibers[n].largs[l].x.y, 1, fibers[n].largs[l].u.x,
                    fibers[n].largs[l].u.y, fibers[n].largs[l].F.x, fibers[n].largs[l].F.y, fibers[n].largs[l].T,
                    fibers[n].largs[l].theta - fibers[n].theta);
        }

        for (l = 0; l < num_l; l++) {
            b1 = b1 + 1;
            c1 = c1 + 1;
            if (l == num_l - 1) {
                c1 = 2;
            }
            fprintf(fp, "%d %d %d\n", a1, b1, c1);
        }
        for (n = 0; n < FNum; n++) {
            char color[10];
            sprintf(color, "WHITE");
            PlotUtils::plot_linewitharr(fp, n, color, N);
        }
        fclose(fp);
    };

    static void plot_ellipse(int step, int num_l, int N, int n) {
        mkdir("04_particle_profile");

        FILE *fp;
        int a1 = 1, b1 = 1, c1 = 2;

        char name[50];
        sprintf(name, "04_particle_profile/Particle_%d_%d.dat", n, step); // print formatted data to string
        fp = fopen(name, "w");
        fprintf(fp, "VARIABLES = X,Y,T\n");
        fprintf(fp, "ZONE F=FEPOINT, N=%d, E=%d,ET=TRIANGLE\n", num_l + 1, num_l);

        for (int l = 0; l < num_l; l++) {
            if (l == 0) {
                fprintf(fp, "%lf %lf %d\n", fibers[n].x.x, fibers[n].x.y, 1);
            }
            fprintf(fp, "%lf %lf %d\n", fibers[n].largs[l].x.x, fibers[n].largs[l].x.y, 1);
        }
        for (int l = 0; l < num_l; l++) {
            b1 = b1 + 1;
            c1 = c1 + 1;
            if (l == num_l - 1) {
                c1 = 2;
            }
            fprintf(fp, "%d %d %d\n", a1, b1, c1);
        }

        char color[10];
        sprintf(color, "WHITE");
        PlotUtils::plot_linewitharr(fp, n, color, N);

        fpclose(fp);
    };

    static void write_track(int step) {
        FILE *fp;
        double ddt;
        double U_final[FNum], P_u[FNum];

        for (int n = 0; n < FNum; n++) {
            U_final[n] = (1 + alpha) * BBB1 / 2.;
            if (fibers[n].a <= fibers[n].b) {
                ddt = U_final[n] * step / (2 * fibers[n].a);
            } else {
                ddt = U_final[n] * step / (2 * fibers[n].b);
            }

            P_u[n] = sqrt(fibers[n].u.x * fibers[n].u.x + fibers[n].u.y * fibers[n].u.y);
        }
        if (step == 0) {

            fp = fopen("particel_track.dat", "w");
            fprintf(fp, "%d\t%f\t%f\t%f\n", step, ddt, P_u[0], P_u[1]);

            fclose(fp);
        } else {
            fp = fopen("particel_track.dat", "a");
            fprintf(fp, "%d\t%f\t%f\t%f\n", step, ddt, P_u[0], P_u[1]);

            fclose(fp);
        }
    };

    static void trajectory_track(int step) {
        mkdir("02_particle_trajectory");
        FILE *fp;
        char name1[50], name2[50];
        // sprintf(name,"particle_trajectory/%d.dat", n);
        if (step == 0) {
            fp = fopen("02_particle_trajectory/0.dat", "w");
            fprintf(fp, "%f\t%f\t%f\t%f\n", (fibers[0].x.x + cxx - MM / 2.) / NN, (fibers[0].x.y - NN / 2.) / NN,
                    fibers[0].theta, fibers[0].w);

            fclose(fp);
        } else {
            fp = fopen("02_particle_trajectory/0.dat", "a");
            fprintf(fp, "%f\t%f\t%f\t%f\n", (fibers[0].x.x + cxx - MM / 2.) / NN, (fibers[0].x.y - NN / 2.) / NN,
                    fibers[0].theta, fibers[0].w);

            fclose(fp);
        }

        if (step == 0) {
            fp = fopen("02_particle_trajectory/1.dat", "w");
            fprintf(fp, "%f\t%f\t%f\t%f\n", (fibers[1].x.x + cxx - MM / 2.) / NN, (fibers[1].x.y - NN / 2.) / NN,
                    fibers[1].theta, fibers[1].w);
            fclose(fp);
        } else {
            fp = fopen("02_particle_trajectory/1.dat", "a");
            fprintf(fp, "%f\t%f\t%f\t%f\n", (fibers[1].x.x + cxx - MM / 2.) / NN, (fibers[1].x.y - NN / 2.) / NN,
                    fibers[1].theta, fibers[1].w);

            fclose(fp);
        }
    };

    // 画带箭头线段
    static void plot_linewitharr(FILE *fp, int n, char *color, int N) {
        fprintf(fp, "GEOMETRY\nF=POINT\nCS=GRID\n");
        fprintf(fp, "X=%lf,Y=%lf,Z=0\n", fibers[n].x.x, fibers[n].x.y);
        fprintf(fp, "C=%s\nS=LOCAL\nL=SOLID\nPL=2\nLT=0.4\nCLIPPING=CLIPTOVIEWPORT\nDRAWORDER=AFTERDATA\n", color);
// fprintf(fp, "ZN=%d\n", N);
        fprintf(fp, "MFC=\"\"\nAST=PLAIN\nAAT=END\nASZ=2\nAAN=12\nT=LINE\nDT=SINGLE\n");
        fprintf(fp, "MFC=\"\"\nAST=PLAIN\nT=LINE\nDT=SINGLE\n");
        fprintf(fp, "1\n2\n0 0\n");
        fprintf(fp, "%lf %lf\n\n", (fibers[n].largs[0].x.x + fibers[n].largs[fibers[n].num_l - 1].x.x) / 2 - fibers[n].x.x,
                (fibers[n].largs[0].x.y + fibers[n].largs[fibers[n].num_l - 1].x.y) / 2 - fibers[n].x.y);
    };

    static void plot_circular(FILE *fp, double x0, double y0, double r, char *clr, int step) {
        fprintf(fp, "GEOMETRY\nF=POINT\nCS=GRID\n");
        fprintf(fp, "X=%lf, Y=%lf, Z=0\n", x0, y0);
        fprintf(fp, "C=CUST41\nS=LOCAL\nL=SOLID\nPL=2\nLT=0.1\nFC=CUST41 CLIPPING=CLIPTOVIEWPORT\n", clr);
//    fprintf(fp,"C=CUST41\nS=LOCAL\nL=SOLID\nPL=2\nLT=0.1\nFC=CUST41 CLIPPING=CLIPTOVIEWPORT\n");
        fprintf(fp, "DRAWORDER=AFTERDATA\nMFC=\"\"\nEP=72\n");
        fprintf(fp, "ZN=%d\n", 1 + step / 100000);
        fprintf(fp, "T=CIRCLE  %lf\n", r);
    };

    static void plot_linewitharr(FILE *fp, double x, double y, double s, double l, char *color, int step) {
        fprintf(fp, "GEOMETRY\nF=POINT\nCS=GRID\n");
        fprintf(fp, "X=%lf,Y=%lf,Z=0\n", x, y);
        fprintf(fp, "C=%s\nS=LOCAL\nL=SOLID\nPL=2\nLT=0.2\nCLIPPING=CLIPTOVIEWPORT\nDRAWORDER=AFTERDATA\n", color);
        fprintf(fp, "ZN=%d\n", 1 + step / 100000);
        fprintf(fp, "MFC=\"\"\nAST=PLAIN\nAAT=END\nASZ=2\nAAN=12\nT=LINE\nDT=SINGLE\n");
        fprintf(fp, "MFC=\"\"\nAST=PLAIN\nT=LINE\nDT=SINGLE\n");
        fprintf(fp, "1\n2\n0 0\n");
        fprintf(fp, "%lf %lf\n\n", l * cos(s), l * sin(s));
    };

    static void Hydrodynamic_efficiency(int step) {
        double P_P[FNum] = {0.};
        for (int n = 0; n < FNum; n++) {
            for (int l = 0; l < fibers[n].num_l; l++) {
                P_P[n] += -((fibers[n].u.x + fibers[n].largs[l].u.x) * (-fibers[n].largs[l].F.x) + (fibers[n].u.y + fibers[n].largs[l].u.y) * (-fibers[n].largs[l].F.y)) * fibers[n].largs[l].ds;
            }
        }
        FILE *fp;

        char name[50];
        sprintf(name, "Hydrodynamic_efficiency.dat"); // print formatted data to string
        if (step == 0) {
            fp = fopen(name, "w");
            fprintf(fp, "%d %lf\n", step, P_P[0]);
            fclose(fp);
        } else {
            fp = fopen(name, "a");
            fprintf(fp, "%d %lf\n", step, P_P[0]);
            fclose(fp);
        }
    }

    static void Save_flowfield(int step) {
        int i, j;
        char name[50];

        FILE *fp0 = NULL;
        sprintf(name, "V_new%d.dat", step);
        fp0 = fopen(name, "w");

        fprintf(fp0, "variables =\"x\",\"y\",\"U\",\"V\"\n");
        fprintf(fp0, "zone i=%d, j=%d ,f=point \n", MM - 1, NN - 1);

        for (j = 1; j < NN; j++)

            for (i = 1; i < MM; i++) {
                fprintf(fp0, "%f %f %.9lf %.9lf\n", dx * i, dy * j, u[i * NN1 + j].x, u[i * NN1 + j].y);
            }

        fclose(fp0);
    }

    static void SaveVel(int step) {
        mkdir("01_flowfield");
        FILE *fp;
        char name[50];
        double p;

        sprintf(name, "01_flowfield/V%d.dat", step); // print formatted data to string
        fp = fopen(name, "w");

        fprintf(fp, "variables=\"x\",\"y\",\"rho\",\"ux\",\"uy\",\"w\",\"p\",\n");
        fprintf(fp, "zone I=%d,J=%d,F=POINT\n", MM - 1, NN - 1);

        for (int j = 1; j < NN; j++) {
            for (int i = 1; i < MM; i++) {
                double w = 0.0;
                if (f[i * NN1 + j].flag < 0) {
                    w = (u[(i + 1) * NN1 + j].y - u[(i - 1) * NN1 + j].y) / 2.0 / dx - (u[i * NN1 + j + 1].x - u[i * NN1 + j - 1].x) / 2.0 / dy;
                }

                p = 0.0;
                for (int i1 = 0; i1 < D; i1++) {
                    p += f[i * NN1 + j].f[i1];
                }
                p = p / 3.0;

                fprintf(fp, "%lf\t%lf\t%.9lf\t%.9lf\t%.9lf\t%.9lf\t%.9lf\n", dx * i, dy * j, rou[i * NN1 + j], u[i * NN1 + j].x, u[i * NN1 + j].y, w, p);
            }
        }

        for (int j = 0; j < NN1; j++) {
            fprintf(fp, " %.9lf  %.9lf \n", u[MM * NN1 + j].x, u[MM * NN1 + j].y);
        }

        fpclose(fp);
    };

    static void Save2(int step) {
        FILE *fp;
        double P11, P10;
        char name[50];

/*sprintf(name,"dis%d.log",step);
fp=fopen(name,"w");
for(i=0;i<MM1;i++)
for(j=0;j<NN1;j++)
{
for(s=0;s<D;s++)
fprintf(fp,"%f\n",f[i*NN1+j].f[s]);
fprintf(fp,"%d\n",f[i*NN1+j].flag);


fprintf(fp,"%e\n",rou[i*NN1+j]);
fprintf(fp,"%e %e\n",u[i*NN1+j].x,u[i*NN1+j].y);
}
fclose(fp);*/

        sprintf(name, "force(x)%d.log", step); // fx
        fp = fopen(name, "w");
        fprintf(fp, "step=%d\n", step);
        for (int i = 0; i < FNum; i++) {
            fprintf(fp, "n=%d\n", i);
            fprintf(fp, "Fx=%e\n\n", fibers[i].F.x);

            for (int k0 = 0; k0 < fibers[i].num_l; k0++) {
                fprintf(fp, "%e\n", fibers[i].largs[k0].F.x * fibers[i].largs[k0].ds);
            }
        }
        fpclose(fp);

        sprintf(name, "force(y)%d.log", step); // fy
        fp = fopen(name, "w");
        fprintf(fp, "%d\n", step);
        for (int i = 0; i < FNum; i++) {
            fprintf(fp, "n=%d\n", i);
            fprintf(fp, "Fy=%e\n\n", fibers[i].F.y);

            for (int k0 = 0; k0 < fibers[i].num_l; k0++) {
                fprintf(fp, "%e\n", fibers[i].largs[k0].F.y * fibers[i].largs[k0].ds);
            }
        }
        fpclose(fp);

        sprintf(name, "P%d.log", step);
        fp = fopen(name, "w");
        fprintf(fp, "step=%d\n", step);
//	fprintf(fp,"%e\n\n",fibers[0].F.y);
        P11 = 0;
        for (int i = 0; i < FNum; i++) {
            for (int k0 = 0; k0 < fibers[i].num_l; k0++) {
                P10 = fibers[i].largs[k0].F.x * fibers[i].largs[k0].ds * fibers[i].largs[k0].u.x + fibers[i].largs[k0].F.y * fibers[i].largs[k0].ds * fibers[i].largs[k0].u.y;
                P10 = P10 / (BBB1 * BBB1 * (1 + alpha) * (1 + alpha) * 0.250 * 2 * fibers[i].b * (pow(BBB1 / 2 * (1 + alpha), (2 - n1)) * pow(2 * fibers[i].b, n1) / Re));
                P11 = P11 + P10;
            }

            fprintf(fp, "n=%d\n", i);
            fprintf(fp, "P=%e\n", P11);
        }

        fpclose(fp);

        sprintf(name, "fibre%d.log", step);
        fp = fopen(name, "w");
        fprintf(fp, "step=%d\n", step);


        fprintf(fp, "mass=%e\nI=%e\n", fibers[0].FMass, fibers[0].I);

        for (int i = 0; i < FNum; i++) {
            fprintf(fp, "n=%d\n\n", i);
            fprintf(fp, "%e\n%e\n", fibers[i].P0.x, fibers[i].P0.y);
            fprintf(fp, "Px=%e\nPy=%e\n", fibers[i].P.x, fibers[i].P.y);
            fprintf(fp, "x=%e\ny=%e\n", fibers[i].x.x, fibers[i].x.y);
            fprintf(fp, "%lf\n%lf\n", fibersPre[i].x.x, fibersPre[i].x.y);
            fprintf(fp, "ux0=%e\nuy0=%e\nu0=%e\nRet=%e\n", fibers[i].u.x, fibers[i].u.y, sqrt(pow(fibers[i].u.x, 2) + pow(fibers[i].u.y, 2)), sqrt(pow(fibers[i].u.x, 2) + pow(fibers[i].u.y, 2))*2*belta*(1+alpha)/nju);
            fprintf(fp, "ux=%e\nuy=%e\nu=%e\n", fibers[i].u.x / (0.50 * (1 + alpha) * BBB1), fibers[i].u.y / (0.50 * (1 + alpha) * BBB1), sqrt(pow(fibers[i].u.x, 2) + pow(fibers[i].u.y, 2)) / (0.50 * (1 + alpha) * BBB1));
            fprintf(fp, "w=%e\n", fibers[i].w);
            fprintf(fp, "theta=%e\n", fibers[i].theta);
            fprintf(fp, "Fx=%e\nFy=%e\n", fibers[i].F.x, fibers[i].F.y);
            fprintf(fp, "T=%e\n", fibers[i].T);
            fprintf(fp, "FFx=%lf\nFFy=%lf\n", fibers[i].FF.x, fibers[i].FF.y);
        }
        fprintf(fp, "Fx=%lf\nFy=%lf\n", FX, FY);
        fprintf(fp, "total_Fx=%lf\ntotal_Fy=%lf\n", fiber_total[1].F.x, fiber_total[1].F.y);
        fpclose(fp);
    };

    static void testAngleOfTangent() {
        FIBER fiber = FIBER();
        fiber.initSize(10, 10);
        //fiber.initTheta(0, 45. / 180 * Pi);
        for (int i = 0; i < 205; i++) {
            double theta1 = angleReSection(i / 101. * Pi);
            double theta2 = fiber.culAngleOfTangent(theta1);
            double theta3 = angleReSection(fiber.culAngleOfTangentInverse(theta2));

            cout << theta1 * 180 / Pi
                 << "\t" << theta2 * 180 / Pi
                 << "\t" << theta3 * 180 / Pi;
            if (approximateEqual(theta1, theta3)) {
                cout << endl;
            } else {
                cout << "++++++++++++++++" << endl;
            }
        }

    }

};


ParamInit paramInit = ParamInit();
PlotUtils plotUtils = PlotUtils();

void Initialize();

void Equ();

void Equ2();

void propagate();

void Get_Density_Velocity();

void Compute(int step);

void Update_Boundary_Node();

void SetF_O_ChazhiNode(FIBER &fiber, int step, int n);

//double Compensation_ubl(FIBER localFiber[]);

//void Compute_Delta_u();

void solid_to_solid_force(FIBER &fiber, int step);

void set_transdomain(const FIBER &fiber, int nnd);

void f_stream();

void Boundary_fout();

void Boundary_period();

void matrix_calculation();

void mesh_moving();

void update_boundary_link();

double rand_fun();
#endif
