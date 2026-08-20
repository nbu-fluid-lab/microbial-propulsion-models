#define _CRT_SECURE_NO_WARNINGS

#include "cylinder.h"

using namespace std;

int main2() {
    PlotUtils::testAngleOfTangent();
    return 0;
}


/**
 * 生成 [minValue, maxValue] 范围内的均匀随机数。
 * 随机数引擎只初始化一次，避免原 rand_fun() 每次调用都重新播种。
 */
double uniformRandom(double minValue, double maxValue) {
    static std::mt19937 generator(
            static_cast<unsigned int>(
                    std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<double> distribution(minValue, maxValue);
    return distribution(generator);
}

/**
 * 判断主动颗粒初始质心位置是否安全：
 * 1. 不与中心障碍物重叠；
 * 2. 不与任一外圈障碍物重叠；
 * 3. 不靠近计算域边界。
 *
 * 这里用主动颗粒长半轴 init_a 作为保守包围半径，因此即使椭圆朝向改变，
 * 也不会在初始化时与障碍物发生几何重叠。
 */
bool isValidActiveInitialPosition(const CVector &position) {
    const double activeBoundingRadius = init_a;
    const double minimumCenterDistance =
            activeBoundingRadius + OBSTACLE_RADIUS + INITIAL_CLEARANCE;

    // 与计算域边界保持安全距离
    if (position.x <= activeBoundingRadius + INITIAL_CLEARANCE ||
        position.x >= MM - activeBoundingRadius - INITIAL_CLEARANCE ||
        position.y <= activeBoundingRadius + INITIAL_CLEARANCE ||
        position.y >= NN - activeBoundingRadius - INITIAL_CLEARANCE) {
        return false;
    }

    // 中心障碍物（编号 1）
    const CVector centerObstacle(FX, FY);
    if (Modul(position - centerObstacle) <= minimumCenterDistance) {
        return false;
    }

    // 外圈障碍物（编号 2 ~ FNum-1）
    if (FNum > 2) {
        const int outerObstacleNumber = FNum - 2;
        for (int m = 0; m < outerObstacleNumber; m++) {
            const double obstacleAngle = 2.0 * Pi * m / outerObstacleNumber;
            const CVector obstacleCenter(
                    FX + RR * cos(obstacleAngle),
                    FY + RR * sin(obstacleAngle));

            if (Modul(position - obstacleCenter) <= minimumCenterDistance) {
                return false;
            }
        }
    }

    return true;
}

/**
 * 随机生成主动颗粒初始位置。
 * 主动颗粒位于半径 ACTIVE_INIT_RADIUS 的圆周上，并通过重叠检测后才接受。
 */
CVector generateActiveInitialPosition() {
    const int maxAttempts = 10000;

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        const double positionAngle = uniformRandom(0.0, 2.0 * Pi);
        const CVector candidate(
                FX + ACTIVE_INIT_RADIUS * cos(positionAngle),
                FY + ACTIVE_INIT_RADIUS * sin(positionAngle));

        if (isValidActiveInitialPosition(candidate)) {
            return candidate;
        }
    }

    throw std::runtime_error(
            "Failed to generate a non-overlapping initial position for the active particle.");
}

/**
 * 为主动颗粒生成朝向障碍物场内部的初始方向。
 * 不是固定瞄准几何中心，而是在障碍物场内部随机选取一个目标点，
 * 因而既保留随机性，又保证初始运动方向朝向障碍物场，而不是朝外直接离开。
 */
double generateInwardInitialTheta(const CVector &startPosition) {
    const double targetAngle = uniformRandom(0.0, 2.0 * Pi);
    const double targetRadius =
            ACTIVE_TARGET_RADIUS * sqrt(uniformRandom(0.0, 1.0));

    const CVector targetPosition(
            FX + targetRadius * cos(targetAngle),
            FY + targetRadius * sin(targetAngle));

    double theta = atan2(targetPosition.y - startPosition.y,
                         targetPosition.x - startPosition.x);
    if (theta < 0.0) {
        theta += 2.0 * Pi;
    }
    return theta;
}

int main() {
    testMatrix();
    Initialize();

    int step = 0, N = 0;

    char name[50];
    FILE *fp = fopen("contact.dat", "w");
    fclose(fp);

    for (int n = 0; n < FNum; n++) {
        sprintf(name, "orientation%d.dat", n);
        fp = fopen(name, "w");
        fpclose(fp);

        PlotUtils::plot_ellipse(step, fibers[n].num_l, N, n);
        PlotUtils::plot_ellipse_F(step, fibers[n].num_l, N, n);
    }

    N = 2;
    d_min_l = 2.0, d_min_r = 2.0, d_min_u = 2.0, d_min_d = 2.0;

    while (step <= 50000000000000000) {
        //cout
        if (step % 10 == 0) {
            cout << "\tstep=" << step
                 << "\tRe=" << Re
                 << "\tbeta=" << belta
                 << "\tAR1=" << fibers[0].b / fibers[0].a
                 << "\tx1=" << fibers[0].x
                 << "\tx2=" << fibers[1].x
                 << "\tnum_l=" << fibers[0].num_l
                 <<"\t ang=" <<ang
                 <<"\t bng="<<bng
                 << endl;
        }

        if (step % 100 == 0) {
            for (int i = 0; i < FNum; i++) {
                cout << " step=" << step
                     << " u=" << fibers[i].u
                     << " w=" << fibers[i].w * 2400.0
                     << " F=" << fibers[i].F
                     << " Ftotal=" << fiber_total[1].F
                     << " T=" << fibers[i].T
                     << " P=" << fibers[i].P.x
                     << " theta=" << fibers[i].theta
                     << " X=" << fibers[i].x
                     << " X0=" << fibersPre[i].x
                     << " FF=" << fibers[i].FF
                     << " Utotal=" << fiber_total[1].u
                     << endl;
            }
        }

        //plot
        if (step % 500 == 0) {
            for (int i = 0; i < FNum; i++) {
                sprintf(name, "orientation%d.dat", i);
                fp = fopen(name, "a+");

                fprintf(fp, "%lf  %lf  %lf   %lf  %lf  %lf  %lf  %lf  %lf   %lf  %lf %lf %lf %lf %lf %lf %lf %lf\n",
                        fibers[i].x.x + cxx, fibers[i].x.y, fibers[i].P.x, fibers[i].P.y,
                        fibers[i].theta / Pi,
                        fibers[i].F.x, fibers[i].F.y, fibers[i].u.x, fibers[i].u.y, fibers[i].T,
                        fibers[i].w * 2400.0, step * dt, d_min_l, d_min_r, d_min_u, d_min_d, ang, bng);
                fpclose(fp);
            }
        }

        if (step % 100 == 0) {
            PlotUtils::write_track(step);
            PlotUtils::trajectory_track(step);

            bool plot = false;
            if (step <= 3e4) {
                if (step % 1000 == 0) {
                    plot = true;
                }
            } else if (step > 3e4 && step <= 1e5) {
                if (step % 10000 == 0) {
                    plot = true;
                }
            } else {
                if (step % 100000 == 0) {
                    plot = true;
                }
            }

            if (plot) {
                for (int n = 0; n < FNum; n++) {
                    int num_l = fibers[n].num_l;
                    PlotUtils::plot_ellipse(step, num_l, N, n);
                    PlotUtils::plot_ellipse_F(step, num_l, N, n);
                    //PlotUtils::Hydrodynamic_efficiency(step);
                    //PlotUtils::plot_coefficient(step);
                    //PlotUtils::Save_flowfield(step);
                    PlotUtils::SaveVel(step);
                    N += 2;
                }
            }
        }

        if (step % 4000 == 0) {
            //plotUtils.Save2(step);
        }

        Compute(step);

        step++;
    }
    return 0;
}

void Initialize() {
    ParamInit::applyMemory();
    ParamInit::initConstParam();



/**********************************************************************
* 流场参数
* *********************************************************************/
// Poiseuille流
    {
        for (int i = 0; i < MM1; i++) {
            for (int j = 0; j < NN1; j++) {
                ni[i][j][0] = i + 1, ni[i][j][1] = i - 1;
                nj[i][j][0] = j + 1, nj[i][j][1] = j - 1;

                if (i == 0) {
                    ni[i][j][1] = 1;
                }
                if (i == MM) {
                    ni[i][j][0] = MM;
                }
                if (j == 0) {
                    nj[i][j][1] = 1;
                }
                if (j == NN) {
                    nj[i][j][0] = NN;
                }
            }
        }

        for (int i = 0; i < MM1; i++) {
            for (int j = 0; j < NN1; j++) {
                for (int k = 0; k < D; k++) {
                    nb_i[i][j][k] = i;
                    if (k == 1 || k == 5 || k == 8) {
                        nb_i[i][j][k] = ni[i][j][0];
                    }
                    if (k == 3 || k == 6 || k == 7) {
                        nb_i[i][j][k] = ni[i][j][1];
                    }

                    nb_j[i][j][k] = j;
                    if (k == 2 || k == 5 || k == 6) {
                        nb_j[i][j][k] = nj[i][j][0];
                    }
                    if (k == 4 || k == 7 || k == 8) {
                        nb_j[i][j][k] = nj[i][j][1];
                    }
                }
            }
        }

    }

/**********************************************************************
* 颗粒参数
* *********************************************************************/
    {
        FX = (double) MM * dl / 2.0;
        FY = (double) NN * dl / 2.0;

        fiber_total[1].x = CVector(FX, FY);
        
        for (int n = 0; n < FNum; n++) {
            if (n < 1) {
                // 主动颗粒：随机生成不重叠的初始位置，并使初始方向指向障碍物场内部
                fibersPre[n].x = fibers[n].x = generateActiveInitialPosition();
                const double initialTheta = generateInwardInitialTheta(fibers[n].x);
                fibers[n].initTheta(n, initialTheta);

                fibers[n].h = L / 2.0;
                fibers[n].PRou = 1.0;
                fibers[n].initSize(belta * (1.0 + alpha), belta * (1.0 - alpha));

                int num_l = nearMultiple(fibers[n].per, 20);

                fibersPre[n].num_l = num_l;
                fibers[n].initLarg(num_l);

            } 
            else if(n<2){
                fibersPre[n].x = fibers[n].x = CVector(FX, FY);
                fibers[n].initTheta(n, 0.*Pi);

                fibers[n].h = L / 2.0;
                fibers[n].PRou = 1.0;
                fibers[n].initSize(OBSTACLE_RADIUS, OBSTACLE_RADIUS);

                int num_l = nearMultiple(fibers[n].per, 20);

                fibersPre[n].num_l = num_l;
                fibers[n].initLarg(num_l);
            }
            else{
                fibersPre[n].x = fibers[n].x = CVector(FX+RR*cos(2*Pi/(FNum-2)*(n-2)), FY+RR*sin(2*Pi/(FNum-2)*(n-2)));
                fibers[n].initTheta(n, 0.*Pi);

                fibers[n].h = L / 2.0;
                fibers[n].PRou = 1.0;
                fibers[n].initSize(OBSTACLE_RADIUS, OBSTACLE_RADIUS);

                int num_l = nearMultiple(fibers[n].per, 20);

                fibersPre[n].num_l = num_l;
                fibers[n].initLarg(num_l);
            }
            //cout << "num_l(" << n << ")=" << num_l << endl;
            //num_l = 120;

            fibers[n].initLargTheta();
            fibers[n].updateLargePosition();
            fibers[n].initLargPhi();
        }
        cout << "gl=" << gl << endl;
    }

/**********************************************************************
* 其他参数
* *********************************************************************/
    FILE *fp;
    fp = fopen("temp.dat", "w");
    fclose(fp);

    ang=0;
    bng=2*Pi;

    Update_Boundary_Node();
    matrix_calculation();

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            rou[i * NN1 + j] = Rou;
        }
    }


    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            for (int k = 0; k < D; k++) {
                double t1 = e.e[k] * u[i * NN1 + j];
                double t2 = u[i * NN1 + j] * u[i * NN1 + j];
                double feqt;
                if (k == 0) {
                    feqt = rou[i * NN1 + j] * (A0 + D0 * t2);
                } else if (k < 5) {
                    feqt = rou[i * NN1 + j] * (A1 + B1 * t1 + C1 * t1 * t1 + D1 * t2);
                } else {
                    feqt = rou[i * NN1 + j] * (A2 + B2 * t1 + C2 * t1 * t1 + D2 * t2);
                }
                f[i * NN1 + j].f[k] = feqt;
            }
        }
    }
    fp = fopen("contact.dat", "a+");
    fprintf(fp, "MM%d,NN=%d\n", MM, NN);
    fprintf(fp, "r=%lf\n", fibers[0].h * 0.5);

    for (int n = 0; n < FNum; n++) {
        fprintf(fp, "fibers%2d.P=(%lf,%lf)\n", n, fibers[n].P.x, fibers[n].P.y);
        fprintf(fp, "fibers%2d.x=(%lf,%lf)\n", n, fibers[n].x.x, fibers[n].x.y);
    }
    fpclose(fp);
}

void SetF_O_ChazhiNode(FIBER &fiber, int step, int n) {
     int N=9999;
    for (int l = 0; l < fiber.num_l; l++) {
        double rouL = 0.0;
        CVector uL = CVector(0.0, 0.0);

        double tempSum = 0.;
        for (int i = xl[l]; i < xr[l]; i++) {
            for (int j = yb[l]; j < yt[l]; j++) {
                double temp = deltf(CVector(i, j) - fiber.largs[l].x);
                uL += u[i * NN1 + j] * temp; // U*
                rouL += rou[i * NN1 + j] * temp;
                tempSum += temp;
            }
        }
        if (abs(uL.x) > 1 || abs(uL.y) > 1 || tempSum > 1.001) {
            int error = 1;
        }

        double us = (BBB1 * sin(fiber.largs[l].phi) + 2.0 * BBB2 * sin(fiber.largs[l].phi) * cos(fiber.largs[l].phi));
        CVector u_theta;
        
        if (n<1) {
        
            u_theta.x = -(1 + alpha) * us * sin(fiber.largs[l].phi) * cos(fiber.theta) -
                        (1 - alpha) * us * cos(fiber.largs[l].phi) * sin(fiber.theta);
            u_theta.y = -(1 + alpha) * us * sin(fiber.largs[l].phi) * sin(fiber.theta) +
                        (1 - alpha) * us * cos(fiber.largs[l].phi) * cos(fiber.theta);

        } else {

            u_theta = CVector(0.0, 0.0);

        }

        //cout<<"ang="<<ang<<"/t"<<"bang="<<bng<<"/t"<<endl;

        //cout<<"u_theta.x="<<u_theta.x<<"\t"<<"u_theta.y="<<u_theta.y<<endl;
        //cout<<"ang="<<ang;

        fiber.largs[l].u = fiber.u + fiber.w * CVector(-(fiber.largs[l].x.y - fiber.x.y), (fiber.largs[l].x.x - fiber.x.x)) + u_theta; // Ud
        //fiber.largs[l].u = fiber.u - fiber.w * (fiber.largs[l].x - fiber.x) + u_theta; // Ud
        fiber.largs[l].F = rouL * (fiber.largs[l].u - uL) / dt;
        fiber.largs[l].T = vectorCrossMulti(fiber.largs[l].x - fiber.x, fiber.largs[l].F);

        if (abs(fiber.largs[l].T) > 1) {
            int error = 1;
        }
    }
}

/*double compensation_ubl(int i0, FIBER localFiber[]) {

CVector bl=CVector(0.0,0.0);

double **arrayA = matInit(localFiber.num_l);

double arrB[localFiber.num_l][2];
double arrU[localFiber.num_l][2];

double arrE[MM1][NN1][localFiber.num_l];
double arrF[MM1][NN1][localFiber.num_l];

set_transdomain(i0, 5);

for (int l = 0; l < localFiber.num_l; l++) {   // 第l个颗粒
for (int i = xl[l]; i < xr[l]; i++) {   // 第l个颗粒周围的欧拉点
for (int j = yb[l]; j < yt[l]; j++) { // 第l个颗粒周围的欧拉点
double f_ijl = deltf(CVector(i, j) - localFiber.largs[l].x) * dx * dy; // U*
double e_ijl = deltf(CVector(i, j) - localFiber.largs[l].x) * localFiber.largs[l].ds;

arrE[i][j][localFiber.num_l]=e_ijl;
arrF[i][j][localFiber.num_l]=f_ijl;

bl-=u[i*NN1+j]*f_ijl;

arrB[localFiber.num_l][2]=localFiber.largs[l].u +bl;
}
}
}

for (int i = 0; i < localFiber.num_l; i++) {
for (int j = 0; j < localFiber.num_l; j++) {
for (int ik = 0; ik < MM1; ik++) {
for (int jk = 0; jk < NN1; jk++) {
arrayA[i][j] += arrE[ik][jk][j] * arrF[ik][jk][i];
}
}
}
}
double **arrayAInvert = matInvertByLU(arrayA, localFiber.num_l);

for(int l=0; l<localFiber.num_l; l++){
for(int n=0; n<2; n++){
arrU[localFiber.num_l][n]=arrayAInvert[localFiber.num_l][n]*arrB[localFiber.num_l][n];
}
}
return arrU;
}*/

/*void restoreVelocity(int i0, FIBER localFiber[]) {

double **delta_ubl=compensation_ubl(int i0, localFiber[i0]);

set_transdomain(i0, 5);

for(int i0=0; i0<FNum; i0++){
for(int l=0; l<localFiber.num_l; l++){
for (int i = xl[l]; i < xr[l]; i++) {   // 第l个颗粒周围的欧拉点
for (int j = yb[l]; j < yt[l]; j++) { // 第l个颗粒周围的欧拉点
u[i*NN1+j]=u[i*NN1+j]+delta_ubl[l][i0]*deltf(CVector(i,j)-localFiber.largs[l].x)*localFiber.largs[l].ds;
}
}
localFiber.largs[l].F=2*localFiber.largs[l].rousL*delta_ubl[l][i0]/dt;
localFiber.largs[l].T=(localFiber.largs[l].x.x -localFiber.x.x)*localFiber.largs[l].F.y
- localFiber.largs[l].F.x*(localFiber.largs[l].x.y-localFiber.x.y);
}
}
}*/

/**
* 计算两个颗粒之间的距离
*/
double *getDistanceBetweenEllipse(const FIBER &fiber1, const FIBER &fiber2, int step) {
    double *result = arrayInit(3);
    double theta0 = 0., theta1 = 0.; //最开始C、H的初始参数值

    if (Modul(fiber1.x - fiber2.x) > (fiber1.a + fiber2.a + 2 * dx)) {
        result[2] = 2;
        return result;
    }

    double distance = 100;
    double inner = 1;
    for (int i = 0; i < 100; i++) {
        //计算C、H点在椭圆上的坐标
        CVector vectorC = fiber1.convertCoordinate(theta0);
        CVector vectorH = fiber2.convertCoordinate(theta1);

        //C、H点在内切圆上的角度
        double betaK1 = fiber1.culAngleOfTangent(theta0);
        double betaK2 = fiber2.culAngleOfTangent(theta1);

        //计算C、H点的切线
        CVector vectorCO1 = fiber1.b * CVector(cos(betaK1 + Pi / 2), sin(betaK1 + Pi / 2));
        CVector vectorHO2 = fiber2.b * CVector(cos(betaK2 + Pi / 2), sin(betaK2 + Pi / 2));

        //计算内切圆圆心坐标

        CVector o1 = vectorC + vectorCO1;
        CVector o2 = vectorH + vectorHO2;

        //将o1和o2映射到第一个椭圆的标准椭圆上
        CVector o11 = fiber1.convertCoordinateInverse(o1);
        CVector o12 = fiber1.convertCoordinateInverse(o2);

        double theta11 = 0, theta12 = 0, theta21 = 0, theta22 = 0;
        if (o11.x != o12.x) {
            double k = (o11.y - o12.y) / (o11.x - o12.x);
            double b = -(o11.y - o12.y) / (o11.x - o12.x) * o11.x + o11.y;

            double fa = fiber1.b;
            double fb = -fiber1.a * k;
            double fc = b;

            theta11 = angleReSection(asin(fc / sqrt(fa * fa + fb * fb)) - atan(fb / fa));
            theta12 = angleReSection((Pi - asin(fc / sqrt(fa * fa + fb * fb))) - atan(fb / fa));
        } else {
            theta11 = angleReSection(acos(o11.x / fiber1.a));
            theta12 = angleReSection(-acos(o11.x / fiber1.a));
        }

        //将o1和o2映射到第二个椭圆的标准椭圆上
        CVector o21 = fiber2.convertCoordinateInverse(o1);
        CVector o22 = fiber2.convertCoordinateInverse(o2);

        if (o21.x != o22.x) {
            double k = (o21.y - o22.y) / (o21.x - o22.x);
            double b = -(o21.y - o22.y) / (o21.x - o22.x) * o21.x + o21.y;

            double fa = fiber2.b;
            double fb = -fiber2.a * k;
            double fc = b;

            theta21 = angleReSection(asin(fc / sqrt(fa * fa + fb * fb)) - atan(fb / fa));
            theta22 = angleReSection((Pi - asin(fc / sqrt(fa * fa + fb * fb))) - atan(fb / fa));

        } else {
            theta21 = angleReSection(acos(o22.x / fiber2.a));
            theta22 = angleReSection(-acos(o22.x / fiber2.a));
        }

        CVector vec11 = fiber1.convertCoordinate(theta11);
        CVector vec12 = fiber1.convertCoordinate(theta12);
        if (Modul(vec11 - o2) > Modul(vec12 - o2)) {
            theta11 = theta12;
        }

        CVector vec21 = fiber2.convertCoordinate(theta21);
        CVector vec22 = fiber2.convertCoordinate(theta22);
        if (Modul(vec21 - o1) > Modul(vec22 - o1)) {
            theta21 = theta22;
        }

        if (abs(theta0 - theta11) + abs(theta1 - theta21) < 10e-4) {
            break;
        }
        theta0 = theta11;
        theta1 = theta21;

        double distance2 = Modul((fiber1.convertCoordinate(theta0) - fiber2.convertCoordinate(theta1)));
        if (distance2 < distance) {
            distance = distance2;
        }
        if (distance > distance2 || Modul(o1 - o2) < fiber1.b + fiber2.b) {
            inner = -1;
        }

        if (step >= 100000) {
            cout << i
                 << "\t" << fiber1.x.x << "\t" << fiber1.x.y
                 << "\t" << fiber1.a << "\t" << fiber1.b
                 << "\t" << fiber1.theta << "\t" << theta0
                 << "\t" << fiber2.x.x << "\t" << fiber2.x.y
                 << "\t" << fiber2.a << "\t" << fiber2.b
                 << "\t" << fiber2.theta << "\t" << theta1
                 << "\t" << Modul(fiber1.convertCoordinate(theta0) - fiber2.convertCoordinate(theta1))
                 << "\t" << Modul(fiber1.x - fiber2.x) - 20
                 << "\t" << o1.x << "\t" << o1.y
                 << "\t" << o2.x << "\t" << o2.y
                 << endl;
        }
    }


    cout << 1 << "\t" << fiber1.x.x << "\t" << fiber1.x.y
         << "\t" << fiber1.a << "\t" << fiber1.b << "\t" << fiber1.theta << "\t" << theta0
         << "\t" << fiber2.x.x << "\t" << fiber2.x.y
         << "\t" << fiber2.a << "\t" << fiber2.b << "\t" << fiber2.theta << "\t" << theta1
         << "\t" << inner * distance << "\t" << Modul(fiber1.x - fiber2.x) - 20 << "\t" << inner
         << endl;
    result[0] = theta0;
    result[1] = theta1;
    result[2] = inner * distance;

    return result;
}

/*--------ellipse_to_ellipse----------*/
void solid_to_solid_force(FIBER &fiber, int step) {
    double Ew1 = 0.0001;
    double Ew = 0.0001;
    //double Cm = Pi * fibers[0].a * fibers[0].b * gl * (fiber.PRou - FRou);
    double Cm=fibers[0].FMass*((1+alpha)*BBB1/2)*((1+alpha)*BBB1/2)/fibers[0].a;
    double ep;

    // init
    fiber.FF = CVector();
    fiber.TT = 0;
    ep=1.0*dx;

    /*//矩形边界
    if (fiber.x.y > NN - ep - fiber.a) {
        // top
        double upPhi = fiber.culAngleOfTangentInverse(Pi);
        CVector r_up = fiber.convertCoordinate(upPhi);
        double d_up = NN - r_up.y;

        if (d_up <= ep) {
            CVector Fu = 1/ep* (Cm / Ew1) * (d_up - ep) * (d_up - ep) * CVector(0.0, -1.0);
            fiber.TT += vectorCrossMulti(r_up - fiber.x, Fu);// Fu.y * (r_up.x - fiber.x.x) - Fu.x * (r_up.y - fiber.x.y);
            fiber.FF += Fu;
        }
    } else if (fiber.x.y < ep + fiber.a) {
        //bottom
        double bottomPhi = fiber.culAngleOfTangentInverse(0);
        CVector r_bottom = fiber.convertCoordinate(bottomPhi);
        double d_down = r_bottom.y;

        if (d_down <= ep) {
            CVector Fd = 1/ep* (Cm / Ew1) * (d_down - ep) * (d_down - ep) * CVector(0.0, 1.0);
            fiber.TT += vectorCrossMulti(r_bottom - fiber.x, Fd);//Fd.y * (r_bottom.x - fiber.x.x) - Fd.x * (r_bottom.y - fiber.x.y);
            fiber.FF += Fd;
        }
    }

    if (fiber.x.x < ep + fiber.a) {
        // left
        double leftPhi = fiber.culAngleOfTangentInverse(Pi * 3 / 2);
        CVector r_left = fiber.convertCoordinate(leftPhi);
        double d_left = r_left.x;

        if (d_left <= ep) {
            CVector Fl = 1/ep* (Cm / Ew1) * (d_left - ep) * (d_left - ep) * CVector(1.0, 0.0);
            fiber.TT += vectorCrossMulti(r_left - fiber.x, Fl);//Tl = Fl.y * (r_left.x - fiber.x.x) - Fl.x * (r_left.y - fiber.x.y);
            fiber.FF += Fl;
        }
    } else if (fiber.x.x > MM - ep - fiber.a) {
        // right
        double rightPhi = fiber.culAngleOfTangentInverse(Pi / 2);
        CVector r_right = fiber.convertCoordinate(rightPhi);
        double d_right = MM - r_right.x;

        if (d_right <= ep) {
            CVector Fr = 1/ep* (Cm / Ew1) * (d_right - ep) * (d_right - ep) * CVector(-1.0, 0.0);
            fiber.TT += vectorCrossMulti(r_right - fiber.x, Fr);//Fr.y * (r_right.x - fiber.x.x) - Fr.x * (r_right.y - fiber.x.y);
            fiber.FF += Fr;
        }
    }*/

    /*//圆形边界
    double dxxc=sqrt((x_center-fiber.x.x)*(x_center-fiber.x.x)+(y_center-fiber.x.y)*(y_center-fiber.x.y));
    double drrc=r_center-dxxc-fiber.a;

    if(drrc<=ep){
        CVector Fr = 1/(ep*ep)* (Cm / Ew1) * (drrc - ep) * (drrc - ep) * CVector((x_center-fiber.x.x)/dxxc, (y_center-fiber.x.y)/dxxc);
        fiber.FF += Fr;
    }*/

 //椭圆-椭圆碰撞
    for (int i = 0; i < FNum; i++) {
        if (fiber.index == i) {
            continue;
        }
        double *distance = getDistanceBetweenEllipse(fiber, fibers[i], step);
        CVector xn = fiber.convertCoordinate(distance[0]);
        CVector xi = fibers[i].convertCoordinate(distance[1]);
        double dij = distance[2];

        CVector Fpi = CVector();
        double Tpi = 0;
        if (dij <= dx) {
            Fpi = 1/ep* (Cm / Ew) * (dij - ep) * (dij - ep) * (xn - xi) / dij;
            Tpi = vectorCrossMulti(xn - fiber.x, Fpi);

            fiber.FF += Fpi;
            fiber.TT += Tpi;
        }


        //FILE *fp;
        //char name[50];
        //sprintf(name, "orientation_double.dat");
        //fp = fopen(name, "a+");
        //fprintf(fp, "%lf %d %d %lf %lf %lf %lf %lf  %lf   %lf  %lf  %lf %lf %lf %lf\n",
                //step * dt, fiber.index, i, fiber.x.x, fiber.x.y, // 质心1
                //fibers[i].x.x, fibers[i].x.y, // 质心2
                //xn.x, xn.y, xi.x, xi.y, //交点2
                //dij, //距离
                //Fpi.x, Fpi.y, Tpi);
        //fpclose(fp);
    }
}

void Equ() {
    double feqt[D];

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            tau[i][j] = 3 * nju + 0.5;//tau[i][j] = 0.6364;
//            double t2 = u[i * NN1 + j] * u[i * NN1 + j];
//
//            for (int k = 0; k < D; k++) {
//                double t1 = e.e[k] * u[i * NN1 + j];
//                double feqt2 = 0.;
//                if (k == 0) {
//                    feqt2 = rou[i * NN1 + j] * (A0 + D0 * t2);
//                } else if (k < 5) {
//                    feqt2 = rou[i * NN1 + j] * (A1 + B1 * t1 + C1 * t1 * t1 + D1 * t2);
//                } else {
//                    feqt2 = rou[i * NN1 + j] * (A2 + B2 * t1 + C2 * t1 * t1 + D2 * t2);
//                }
//                f[i * NN1 + j].f[k] -= (f[i * NN1 + j].f[k] - feqt2) / tau[i][j];
//
//                if (abs(f[i * NN1 + j].f[k]) > 1) {
//                    int ccc = 1;
//                }
//            }
//            FOL[i * NN1 + j] = CVector(0.0, 0.0);



            feqt[0] = rou[i * NN1 + j] * (A0 + D0 * u[i * NN1 + j] * u[i * NN1 + j]);

            feqt[1] = rou[i * NN1 + j] * (A1 + B1 * e.e[1] * u[i * NN1 + j] + C1 * e.e[1] * u[i * NN1 + j] * e.e[1] * u[i * NN1 + j] + D1 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[2] = rou[i * NN1 + j] * (A1 + B1 * e.e[2] * u[i * NN1 + j] + C1 * e.e[2] * u[i * NN1 + j] * e.e[2] * u[i * NN1 + j] + D1 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[3] = rou[i * NN1 + j] * (A1 + B1 * e.e[3] * u[i * NN1 + j] + C1 * e.e[3] * u[i * NN1 + j] * e.e[3] * u[i * NN1 + j] + D1 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[4] = rou[i * NN1 + j] * (A1 + B1 * e.e[4] * u[i * NN1 + j] + C1 * e.e[4] * u[i * NN1 + j] * e.e[4] * u[i * NN1 + j] + D1 * u[i * NN1 + j] * u[i * NN1 + j]);

            feqt[5] = rou[i * NN1 + j] * (A2 + B2 * e.e[5] * u[i * NN1 + j] + C2 * e.e[5] * u[i * NN1 + j] * e.e[5] * u[i * NN1 + j] + D2 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[6] = rou[i * NN1 + j] * (A2 + B2 * e.e[6] * u[i * NN1 + j] + C2 * e.e[6] * u[i * NN1 + j] * e.e[6] * u[i * NN1 + j] + D2 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[7] = rou[i * NN1 + j] * (A2 + B2 * e.e[7] * u[i * NN1 + j] + C2 * e.e[7] * u[i * NN1 + j] * e.e[7] * u[i * NN1 + j] + D2 * u[i * NN1 + j] * u[i * NN1 + j]);
            feqt[8] = rou[i * NN1 + j] * (A2 + B2 * e.e[8] * u[i * NN1 + j] + C2 * e.e[8] * u[i * NN1 + j] * e.e[8] * u[i * NN1 + j] + D2 * u[i * NN1 + j] * u[i * NN1 + j]);

            for (int k = 0; k < D; k++) {
                f[i * NN1 + j].f[k] -= (f[i * NN1 + j].f[k] - feqt[k]) / tau[i][j];
            }
            FOL[i * NN1 + j] = CVector(0.0, 0.0);
        }
    }
}

void Equ2() {
    double ww[D] = {4.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0};

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            for (int k = 0; k < D; k++) {
                f[i * NN1 + j].f[k] += (1.0 - 0.50 / tau[i][j]) * ww[k] * (3.0 * (e.e[k] - u[i * NN1 + j]) + 9.0 * e.e[k] * u[i * NN1 + j] * e.e[k]) * FOL[i * NN1 + j];
            }
        }
    }
}

void propagate() {
    for (int i = 1; i < MM; i++) {
        int is = MM - i;
        for (int j = 1; j < NN; j++) {
            int js = NN - j;
            {
                if (f[i * NN1 + j].flag != WALL) {
                    f[i * NN1 + j].f[3] = f[(i + 1) * NN1 + j].f[3];
                    f[i * NN1 + j].f[4] = f[i * NN1 + j + 1].f[4];
                    f[i * NN1 + j].f[7] = f[(i + 1) * NN1 + j + 1].f[7];
                }

                if (f[is * NN1 + j].flag != WALL) {
                    f[is * NN1 + j].f[1] = f[(is - 1) * NN1 + j].f[1];
                    f[is * NN1 + j].f[8] = f[(is - 1) * NN1 + j + 1].f[8];
                }

                if (f[i * NN1 + js].flag != WALL) {
                    f[i * NN1 + js].f[2] = f[i * NN1 + js - 1].f[2];
                    f[i * NN1 + js].f[6] = f[(i + 1) * NN1 + js - 1].f[6];
                }

                if (f[is * NN1 + js].flag != WALL) {
                    f[is * NN1 + js].f[5] = f[(is - 1) * NN1 + js - 1].f[5];
                }
            }
        }
    }
}

void Get_Density_Velocity() {

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            rou[i * NN1 + j] = 0.0;

            for (int k = 0; k < D; k++) {
                rou[i * NN1 + j] += f[i * NN1 + j].f[k];
            }

            int fl = f[i * NN1 + j].flag;

            if ((fl >= 0) || (fl < 0)) {
                u[i * NN1 + j] = CVector(0.0, 0.0);
                for (int k = 0; k < D; k++) {
                    u[i * NN1 + j] += f[i * NN1 + j].f[k] * e.e[k];
                }
                u[i * NN1 + j] = (u[i * NN1 + j] / rou[i * NN1 + j]) + 0.5 * FOL[i * NN1 + j];
            }
            if(f[i * NN1 + j].flag==WALL||f[i * NN1 + j].flag==BLANK)
            {
                u[i * NN1 + j]=CVector(0.0, 0.0);
            }
        }
    }

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            if ((i >= 2) && (i <= (MM - 2)) && (j >= 2) && (j <= (NN - 2))) {
                dudx[i * NN1 + j] = (2.0 / (3.0 * dx)) * (u[(i + 1) * NN1 + j] - u[(i - 1) * NN1 + j]) + (1.0 / (12.0 * dx)) * (u[(i + 2) * NN1 + j] - u[(i - 2) * NN1 + j]);
                dudy[i * NN1 + j] = (2.0 / (3.0 * dx)) * (u[i * NN1 + j + 1] - u[i * NN1 + j - 1]) + (1.0 / (12.0 * dx)) * (u[i * NN1 + j + 2] - u[i * NN1 + j - 2]);

            } else if (((i == 0) || (i == 1)) && (j >= 2) && (j <= (NN - 2))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i + 1) * NN1 + j] - u[(i + 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (1.0 / (2.0 * dx)) * (u[i * NN1 + j + 1] - u[i * NN1 + j - 1]);

            } else if (((i == (MM - 1)) || (i == MM)) && (j >= 2) && (j <= (NN - 2))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i - 1) * NN1 + j] - u[(i - 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (1.0 / (2.0 * dx)) * (u[i * NN1 + j + 1] - u[i * NN1 + j - 1]);

            } else if (((j == 0) || (j == 1)) && (i >= 2) && (i <= (MM - 2))) {
                dudx[i * NN1 + j] = (1.0 / (2.0 * dx)) * (u[(i + 1) * NN1 + j] - u[(i - 1) * NN1 + j]);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j + 1] - u[i * NN1 + j + 2]) / (2.0 * dx);
            } else if (((j == (NN - 1)) || (j == NN)) && (i >= 2) && (i <= (MM - 2))) {
                dudx[i * NN1 + j] = (1.0 / (2.0 * dx)) * (u[(i + 1) * NN1 + j] - u[(i - 1) * NN1 + j]);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j - 1] - u[i * NN1 + j - 2]) / (2.0 * dx);

            } else if (((i == 0) || (i == 1)) && ((j == 0) || (j == 1))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i + 1) * NN1 + j] - u[(i + 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j + 1] - u[i * NN1 + j + 2]) / (2.0 * dx);
            } else if (((i == 0) || (i == 1)) && ((j == (NN - 1)) || (j == NN))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i + 1) * NN1 + j] - u[(i + 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j - 1] - u[i * NN1 + j - 2]) / (2.0 * dx);
            } else if (((i == (MM - 1)) || (i == MM)) && ((j == 0) || (j == 1))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i - 1) * NN1 + j] - u[(i - 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j + 1] - u[i * NN1 + j + 2]) / (2.0 * dx);
            } else if (((i == (MM - 1)) || (i == MM)) && ((j == (NN - 1)) || (j == NN))) {
                dudx[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[(i - 1) * NN1 + j] - u[(i - 2) * NN1 + j]) / (2.0 * dx);
                dudy[i * NN1 + j] = (-3.0 * u[i * NN1 + j] + 4.0 * u[i * NN1 + j - 1] - u[i * NN1 + j - 2]) / (2.0 * dx);
            }


            gama[i][j] = sqrt(
                    2.0 * dudx[i * NN1 + j].x * dudx[i * NN1 + j].x + 2.0 * dudy[i * NN1 + j].y * dudy[i * NN1 + j].y +
                    (dudx[i * NN1 + j].y + dudy[i * NN1 + j].x) * (dudx[i * NN1 + j].y + dudy[i * NN1 + j].x));

        }
    }
}

void Update_Boundary_Node() {

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            double dr=(i-x_center)*(i-x_center)+(j-y_center)*(j-y_center)-r_center*r_center;
            if (dr<0){
                f[i * NN1 + j].flag = FLUID;}//-1
            else if (dr==0){
                f[i * NN1 + j].flag = FLUID;
            }
            else {
                f[i * NN1 + j].flag = FLUID;
            }
        }
    }
}

void Boundary_fout() {
    int i, j, i1, j1, fft, fft1;
    double BB;

//非平衡外推
    for (i = 0; i < MM1; i++) {
        f[i * NN1 + NN].f[4] = rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[4] * CVector(UU1, 0.0)) + C1 * (e.e[4] * CVector(UU1, 0.0)) * (e.e[4] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[4] - rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[4] * u[i * NN1 + NN - 1]) + C1 * (e.e[4] * u[i * NN1 + NN - 1]) * (e.e[4] * u[i * NN1 + NN - 1]) + D1 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[6] = rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[6] * CVector(UU1, 0.0)) + C2 * (e.e[6] * CVector(UU1, 0.0)) * (e.e[6] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[6] - rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[6] * u[i * NN1 + NN - 1]) + C2 * (e.e[6] * u[i * NN1 + NN - 1]) * (e.e[6] * u[i * NN1 + NN - 1]) + D2 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[8] = rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[8] * CVector(UU1, 0.0)) + C2 * (e.e[8] * CVector(UU1, 0.0)) * (e.e[8] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[8] - rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[8] * u[i * NN1 + NN - 1]) + C2 * (e.e[8] * u[i * NN1 + NN - 1]) * (e.e[8] * u[i * NN1 + NN - 1]) + D2 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));

        f[i * NN1 + NN].f[1] = rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[1] * CVector(UU1, 0.0)) + C1 * (e.e[1] * CVector(UU1, 0.0)) * (e.e[1] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[1] - rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[1] * u[i * NN1 + NN - 1]) + C1 * (e.e[1] * u[i * NN1 + NN - 1]) * (e.e[1] * u[i * NN1 + NN - 1]) + D1 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[2] = rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[2] * CVector(UU1, 0.0)) + C1 * (e.e[2] * CVector(UU1, 0.0)) * (e.e[2] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[2] - rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[2] * u[i * NN1 + NN - 1]) + C1 * (e.e[2] * u[i * NN1 + NN - 1]) * (e.e[2] * u[i * NN1 + NN - 1]) + D1 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[3] = rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[3] * CVector(UU1, 0.0)) + C1 * (e.e[3] * CVector(UU1, 0.0)) * (e.e[3] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[3] - rou[i * NN1 + NN - 1] * (A1 + B1 * (e.e[3] * u[i * NN1 + NN - 1]) + C1 * (e.e[3] * u[i * NN1 + NN - 1]) * (e.e[3] * u[i * NN1 + NN - 1]) + D1 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[7] = rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[7] * CVector(UU1, 0.0)) + C2 * (e.e[7] * CVector(UU1, 0.0)) * (e.e[7] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[7] - rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[7] * u[i * NN1 + NN - 1]) + C2 * (e.e[7] * u[i * NN1 + NN - 1]) * (e.e[7] * u[i * NN1 + NN - 1]) + D2 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));
        f[i * NN1 + NN].f[5] = rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[5] * CVector(UU1, 0.0)) + C2 * (e.e[5] * CVector(UU1, 0.0)) * (e.e[5] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + NN - 1].f[5] - rou[i * NN1 + NN - 1] * (A2 + B2 * (e.e[5] * u[i * NN1 + NN - 1]) + C2 * (e.e[5] * u[i * NN1 + NN - 1]) * (e.e[5] * u[i * NN1 + NN - 1]) + D2 * (u[i * NN1 + NN - 1] * u[i * NN1 + NN - 1])));

        f[i * NN1].f[3] = rou[i * NN1 + 1] * (A1 + B1 * (e.e[3] * CVector(UU1, 0.0)) + C1 * (e.e[3] * CVector(UU1, 0.0)) * (e.e[3] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[3] - rou[i * NN1 + 1] * (A1 + B1 * (e.e[3] * u[i * NN1 + 1]) + C1 * (e.e[3] * u[i * NN1 + 1]) * (e.e[3] * u[i * NN1 + 1]) + D1 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[5] = rou[i * NN1 + 1] * (A2 + B2 * (e.e[5] * CVector(UU1, 0.0)) + C2 * (e.e[5] * CVector(UU1, 0.0)) * (e.e[5] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[5] - rou[i * NN1 + 1] * (A2 + B2 * (e.e[5] * u[i * NN1 + 1]) + C2 * (e.e[5] * u[i * NN1 + 1]) * (e.e[5] * u[i * NN1 + 1]) + D2 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[7] = rou[i * NN1 + 1] * (A2 + B2 * (e.e[7] * CVector(UU1, 0.0)) + C2 * (e.e[7] * CVector(UU1, 0.0)) * (e.e[7] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[7] - rou[i * NN1 + 1] * (A2 + B2 * (e.e[7] * u[i * NN1 + 1]) + C2 * (e.e[7] * u[i * NN1 + 1]) * (e.e[7] * u[i * NN1 + 1]) + D2 * (u[i * NN1 + 1] * u[i * NN1 + 1])));

        f[i * NN1].f[4] = rou[i * NN1 + 1] * (A1 + B1 * (e.e[4] * CVector(UU1, 0.0)) + C1 * (e.e[4] * CVector(UU1, 0.0)) * (e.e[4] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[4] - rou[i * NN1 + 1] * (A1 + B1 * (e.e[4] * u[i * NN1 + 1]) + C1 * (e.e[4] * u[i * NN1 + 1]) * (e.e[4] * u[i * NN1 + 1]) + D1 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[6] = rou[i * NN1 + 1] * (A2 + B2 * (e.e[6] * CVector(UU1, 0.0)) + C2 * (e.e[6] * CVector(UU1, 0.0)) * (e.e[6] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[6] - rou[i * NN1 + 1] * (A2 + B2 * (e.e[6] * u[i * NN1 + 1]) + C2 * (e.e[6] * u[i * NN1 + 1]) * (e.e[6] * u[i * NN1 + 1]) + D2 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[8] = rou[i * NN1 + 1] * (A2 + B2 * (e.e[8] * CVector(UU1, 0.0)) + C2 * (e.e[8] * CVector(UU1, 0.0)) * (e.e[8] * CVector(UU1, 0.0)) + D2 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[8] - rou[i * NN1 + 1] * (A2 + B2 * (e.e[8] * u[i * NN1 + 1]) + C2 * (e.e[8] * u[i * NN1 + 1]) * (e.e[8] * u[i * NN1 + 1]) + D2 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[1] = rou[i * NN1 + 1] * (A1 + B1 * (e.e[1] * CVector(UU1, 0.0)) + C1 * (e.e[1] * CVector(UU1, 0.0)) * (e.e[1] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[1] - rou[i * NN1 + 1] * (A1 + B1 * (e.e[1] * u[i * NN1 + 1]) + C1 * (e.e[1] * u[i * NN1 + 1]) * (e.e[1] * u[i * NN1 + 1]) + D1 * (u[i * NN1 + 1] * u[i * NN1 + 1])));
        f[i * NN1].f[2] = rou[i * NN1 + 1] * (A1 + B1 * (e.e[2] * CVector(UU1, 0.0)) + C1 * (e.e[2] * CVector(UU1, 0.0)) * (e.e[2] * CVector(UU1, 0.0)) + D1 * (CVector(UU1, 0.0) * CVector(UU1, 0.0))) + (f[i * NN1 + 1].f[2] - rou[i * NN1 + 1] * (A1 + B1 * (e.e[2] * u[i * NN1 + 1]) + C1 * (e.e[2] * u[i * NN1 + 1]) * (e.e[2] * u[i * NN1 + 1]) + D1 * (u[i * NN1 + 1] * u[i * NN1 + 1])));

    }
}

void Boundary_period() {

    for (int j = 1; j < NN; j++) {
        // INLET

        f[j].f[1] = f[MM * NN1 + j].f[1];
        f[j].f[5] = f[MM * NN1 + j - 1].f[5];
        f[j].f[8] = f[MM * NN1 + j + 1].f[8];

        // OUTLET
        f[MM * NN1 + j].f[2] = f[j].f[2];
        f[MM * NN1 + j].f[6] = f[j + 1].f[6];
        f[MM * NN1 + j].f[7] = f[j - 1].f[7];
    }
}

void f_stream() {
    int i1, j1;

    // stream
    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            for (int k = 1; k < D; k++) {
                Rf[i * NN1 + j].f[k] = f[i * NN1 + j].f[k];
            }
        }
    }

    for (int i = 0; i < MM1; i++) {
        for (int j = 0; j < NN1; j++) {
            for (int k = 1; k < D; k++) {
                if (k == 1 || k == 2 || k == 5 || k == 6) {
                    i1 = nb_i[i][j][k + 2];
                    j1 = nb_j[i][j][k + 2];
                } else {
                    i1 = nb_i[i][j][k - 2];
                    j1 = nb_j[i][j][k - 2];
                }
                f[i * NN1 + j].f[k] = Rf[i1 * NN1 + j1].f[k];
            }
        }
    }

    /*----------------boundary condition---------------------*/


    //periodic top&bottom
    for (int i = 0; i < MM1; i++) {
        //bottom
        f[i * NN1].f[2] = f[i * NN1 + NN].f[2];
        f[i * NN1].f[5] = f[i * NN1 + NN].f[5];
        f[i * NN1].f[6] = f[i * NN1 + NN].f[6];

        //top
        f[i * NN1 + NN].f[4] = f[i * NN1].f[4];
        f[i * NN1 + NN].f[7] = f[i * NN1].f[7];
        f[i * NN1 + NN].f[8] = f[i * NN1].f[8];
    }
    
	//periodic left&right
    for (int j = 1; j < NN; j++) {
        f[j].f[1] = f[MM * NN1 + j].f[1];
        f[j].f[5] = f[MM * NN1 + j].f[5];
        f[j].f[8] = f[MM * NN1 + j].f[8];

        f[MM * NN1 + j].f[3] = f[j].f[3];
        f[MM * NN1 + j].f[7] = f[j].f[7];
        f[MM * NN1 + j].f[6] = f[j].f[6];
    }

/*    for (int j = 1; j < NN; j++) {
        // f[j].f[1] = f[NN1+j].f[1];
        // f[j].f[5] = f[NN1+j].f[5];
        // f[j].f[8] = f[NN1+j].f[8];

        f[j].f[1] = f[j].f[3];
        f[j].f[5] = f[j].f[7] + 6 * rou[j] * w[5] * e.e[5].x * (UU1);
        f[j].f[8] = f[j].f[6] + 6 * rou[j] * w[8] * e.e[8].x * (UU1);

        f[MM * NN1 + j].f[3] = f[MM * NN1 + j].f[1];
        f[MM * NN1 + j].f[7] = f[MM * NN1 + j].f[5] + 6 * rou[MM * NN1 + j] * w[7] * e.e[7].x * (UU1);
        f[MM * NN1 + j].f[6] = f[MM * NN1 + j].f[8] + 6 * rou[MM * NN1 + j] * w[6] * e.e[6].x * (UU1);
    }*/

  /*  // wall-boundary 壁面边界条件 top&bottom
    for (int i = 0; i < MM1; i++) {
        // bottom
        f[i * NN1].f[2] = f[i * NN1].f[4];
        f[i * NN1].f[5] = f[i * NN1].f[7] + 6 * rou[i * NN1] * w[5] * e.e[5].x * (UU1);
        f[i * NN1].f[6] = f[i * NN1].f[8] + 6 * rou[i * NN1] * w[6] * e.e[6].x * (UU1);

        // top
        f[i * NN1 + NN].f[4] = f[i * NN1 + NN].f[2];
        f[i * NN1 + NN].f[7] = f[i * NN1 + NN].f[5] + 6 * rou[i * NN1] * w[7] * e.e[7].x * (UU1);
        f[i * NN1 + NN].f[8] = f[i * NN1 + NN].f[6] + 6 * rou[i * NN1] * w[8] * e.e[8].x * (UU1);
    }
    // wall-boundary 壁面边界条件 left&right
    for (int j = 1; j < NN; j++) {
        f[j].f[1] = f[j].f[3];
        f[j].f[5] = f[j].f[7] + 6 * rou[j] * w[5] * e.e[5].x * (UU1);
        f[j].f[8] = f[j].f[6] + 6 * rou[j] * w[8] * e.e[8].x * (UU1);

        f[MM * NN1 + j].f[3] = f[MM * NN1 + j].f[1];
        f[MM * NN1 + j].f[7] = f[MM * NN1 + j].f[5] + 6 * rou[MM * NN1 + j] * w[7] * e.e[7].x * (UU1);
        f[MM * NN1 + j].f[6] = f[MM * NN1 + j].f[8] + 6 * rou[MM * NN1 + j] * w[6] * e.e[6].x * (UU1);
    }*/


    //update_boundary_link();

}

void set_transdomain(const FIBER &fiber, int nnd) {
    for (int k = 0; k < fiber.num_l; k++) {
        xl[k] = (int) ((fiber.largs[k].x.x) / dx) - nnd;
        xr[k] = (int) ((fiber.largs[k].x.x) / dx) + nnd;
        yb[k] = (int) ((fiber.largs[k].x.y) / dx) - nnd;
        yt[k] = (int) ((fiber.largs[k].x.y) / dx) + nnd;

        if (xl[k] < 0) {
            xl[k] = 0;
        }
        if (xr[k] > MM) {
            xr[k] = MM + 1;
        }
        if (yb[k] < 0) {
            yb[k] = 0;
        }
        if (yt[k] > NN) {
            yt[k] = NN + 1;
        }
    }
}


void matrix_calculation() {
// double D1[D][D]={0},F[D][D]={0};//;
// double Dm[3][3]={{1,1,1},{1,1,1},{1,1,1}};
    double A[D][D] = {{1,  1,  1,  1,  1,  1, 1,  1,  1},
                      {-4, -1, -1, -1, -1, 2, 2,  2,  2},
                      {4,  -2, -2, -2, -2, 1, 1,  1,  1},
                      {0,  1,  0,  -1, 0,  1, -1, -1, 1},
                      {0,  -2, 0,  2,  0,  1, -1, -1, 1},
                      {0,  0,  1,  0,  -1, 1, 1,  -1, -1},
                      {0,  0,  -2, 0,  2,  1, 1,  -1, -1},
                      {0,  1,  -1, 1,  -1, 0, 0,  0,  0},
                      {0,  0,  0,  0,  0,  1, -1, 1,  -1}};

    double E[D][D] = {{1.0, 0,   0,   0,   0,   0,   0,   0,   0},
                      {0,   1.4, 0,   0,   0,   0,   0,   0,   0},
                      {0,   0,   1.4, 0,   0,   0,   0,   0,   0},
                      {0,   0,   0,   1.0, 0,   0,   0,   0,   0},
                      {0,   0,   0,   0,   1.2, 0,   0,   0,   0},
                      {0,   0,   0,   0,   0,   1.0, 0,   0,   0},
                      {0,   0,   0,   0,   0,   0,   1.2, 0,   0},
                      {0,   0,   0,   0,   0,   0,   0,   S89, 0},
                      {0,   0,   0,   0,   0,   0,   0,   0,   S89}};

/*double E[D][D]={{0.344827586,0,0,0,0,0,0,0,0},
{0,0.344827586,0,0,0,0,0,0,0},
{0,0,0.344827586,0,0,0,0,0,0},
{0,0,0,0.344827586,0,0,0,0,0},
{0,0,0,0,0.344827586,0,0,0,0},
{0,0,0,0,0,0.344827586,0,0,0},
{0,0,0,0,0,0,0.344827586,0,0},
{0,0,0,0,0,0,0,0.344827586,0},
{0,0,0,0,0,0,0,0,0.344827586}};*/

    double I[D][D] = {{1, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 1, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 1, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 1, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 1, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 1, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 1, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 1, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 1}};

    double B[D][D] = {{0.1111111111, -0.1111111111, 0.1111111111,  0,             0,             0,             0,             0,     0},
                      {0.1111111111, -0.0277777778, -0.0555555556, 0.1666666667,  -0.1666666667, 0,             0,             0.25,  0},
                      {0.1111111111, -0.0277777778, -0.0555555556, 0,             0,             0.1666666667,  -0.1666666667, -0.25, 0},
                      {0.1111111111, -0.0277777778, -0.0555555556, -0.1666666667, 0.1666666667,  0,             0,             0.25,  0},
                      {0.1111111111, -0.0277777778, -0.0555555556, 0,             0,             -0.1666666667, 0.1666666667,  -0.25, 0},
                      {0.1111111111, 0.0555555556,  0.0277777778,  0.1666666667,  0.0833333333,  0.1666666667,  0.0833333333,  0,     0.25},
                      {0.1111111111, 0.0555555556,  0.0277777778,  -0.1666666667, -0.0833333333, 0.1666666667,  0.0833333333,  0,     -0.25},
                      {0.1111111111, 0.0555555556,  0.0277777778,  -0.1666666667, -0.0833333333, -0.1666666667, -0.0833333333, 0,     0.25},
                      {0.1111111111, 0.0555555556,  0.0277777778,  0.1666666667,  0.0833333333,  -0.1666666667, -0.0833333333, 0,     -0.25}};

    for (int i = 0; i < D; i++) {
        for (int j = 0; j < D; j++) {
            Dm[i][j] = 0.0;
            for (int k = 0; k < D; k++) {
                Dm[i][j] = Dm[i][j] + B[i][k] * E[k][j];
            }
        }
    }

    for (int i = 0; i < D; i++) {
        for (int j = 0; j < D; j++) {
            M[i][j] = 0.0;
            for (int k = 0; k < D; k++) {
                M[i][j] = M[i][j] + Dm[i][k] * A[k][j];
            }
        }
    }

    for (int i = 0; i < D; i++) {
        for (int j = 0; j < D; j++) {
            for (int k = 0; k < D; k++) {
                Fm[i][j] = Fm[i][j] + B[i][k] * (I[k][j] - 0.5 * E[k][j]);
            }
        }
    }

    for (int i = 0; i < D; i++) {
        for (int j = 0; j < D; j++) {
            for (int k = 0; k < D; k++) {
                M1[i][j] = M1[i][j] + Fm[i][k] * A[k][j];
            }
        }
    }
}


void Compute(int step) {

    Equ();
    // Boundary_period();
    // Boundary_fout();
    // propagate();
    f_stream();

    Get_Density_Velocity();

    for (int n = 0; n < FNum; n++) {
        set_transdomain(fibers[n], 5);
        SetF_O_ChazhiNode(fibers[n], step, n);
    }

    for (int n = 0; n < FNum; n++) {
        solid_to_solid_force(fibers[n], step);
    }

    for (int n = 0; n < FNum; n++) {
        fibers[n].F = CVector(0.0, 0.0);
        fibers[n].T = 0.0;

        for (int l = 0; l < fibers[n].num_l; l++) {
            fibers[n].F -= fibers[n].largs[l].F * fibers[n].largs[l].ds;
            fibers[n].T -= fibers[n].largs[l].T * fibers[n].largs[l].ds;
        }

        if(n<1) {
            fibers[n].F += fibers[n].FF + fibers[n].Fg;
            fibers[n].T += fibers[n].TT;
            fibers[n].u += fibers[n].F / fibers[n].FMass;
            fibers[n].x += fibers[n].u * dt;
            fibersPre[n].x += fibers[n].u * dt;

            fibers[n].w += fibers[n].T * dt / fibers[n].I;
            fibers[n].theta += fibers[n].w * dt;
            fibers[n].P = CVector(cos(fibers[n].theta), sin(fibers[n].theta));
            fibers[n].checkStatus();
        }
        else
        {
            fibers[n].F += 0.0;
            fibers[n].T += 0.0;
            fibers[n].u += 0.0;
            fibers[n].x += 0.0;
            fibersPre[n].x += 0.0;

            fibers[n].w += 0.0;
            fibers[n].theta += 0.0;
            fibers[n].P = 0.0;
            fibers[n].checkStatus();
        }
    }

    for (int n = 0; n < FNum; n++) {
        set_transdomain(fibers[n], 5);
         for (int l = 0; l < fibers[n].num_l; l++) {
                for (int i = xl[l]; i < xr[l]; i++) {
                    for (int j = yb[l]; j < yt[l]; j++) {
                        FOL[i * NN1 + j] += fibers[n].largs[l].F * deltf(CVector(i, j) - fibers[n].largs[l].x) *
                                            fibers[n].largs[l].ds;
                    }
                }
            }
    }
    Update_Boundary_Node();
    Equ2();
    // Boundary_period();
    // Boundary_fout();
    Get_Density_Velocity();
    //mesh_moving();
    for (int n = 0; n < FNum; n++) {
        fibers[n].updateLargePosition();
    }
}

void mesh_moving() {
    double temp_cx = 0.0;
    for (int n = 0; n < FNum; n++) {
        temp_cx += fibers[n].x.x;
    }
    temp_cx = temp_cx / FNum;

    int orient = 0;

    if (temp_cx > (dx + MM / 2.0)) {
        orient = 1;
    } else if (temp_cx < (MM / 2.0 - dx)) {
        orient = -1;
    } else {
        return;
    }

    cxx += orient * dx;

    for (int i0 = 0; i0 < FNum; i0++) {
        fibers[i0].x.x -= orient * dx;
    }
    if (orient == 1) {
        for (int i = 0; i < MM; i++) {
            for (int j = 0; j <= NN; j++) {
                for (int k = 0; k < 9; k++) {
                    f[i * NN1 + j].f[k] = f[(i + orient) * NN1 + j].f[k];
                }
                u[i * NN1 + j] = u[(i + orient) * NN1 + j];
                rou[i * NN1 + j] = rou[(i + orient) * NN1 + j];
                gama[i][j] = gama[i + orient][j];
            }
        }
    } else if (orient == -1) {
        for (int i = MM; i > 0; i--) {
            for (int j = NN; j >= 0; j--) {
                for (int k = 0; k < 9; k++) {
                    f[i * NN1 + j].f[k] = f[(i + orient) * NN1 + j].f[k];
                }
                u[i * NN1 + j] = u[(i + orient) * NN1 + j];
                rou[i * NN1 + j] = rou[(i + orient) * NN1 + j];
                gama[i][j] = gama[i + orient][j];
            }
        }
    }
}

void update_boundary_link()
{
    double q;
    int rvs_k;

    for(int i=0; i<MM1; i++){
        for(int j=0; j<NN1; j++){

            if(f[i * NN1 + j].flag == -1){  //流体点
                int xa=i, ya=j;
                for(int k=1; k<D; k++)
                {
                    int xb=xa+e.e[k].x;
                    int yb=ya+e.e[k].y;

                    if(xb<0){xb=0;}
                    if(xb>MM){xb=MM1;}
                    if(yb<0){yb=0;}
                    if(yb>NN){yb=NN1;}

                    double dxyb=(xb-x_center)*(xb-x_center)+(yb-y_center)*(yb-y_center)-r_center*r_center;

                    if(dxyb>=0){        //固体点

                        int xc=xa-e.e[k].x;
                        int yc=ya-e.e[k].y;

                        int xd=xa-e.e[k].x-e.e[k].x;
                        int yd=ya-e.e[k].y-e.e[k].y;

                        if(xa==xb){

                            double xe=xa;
                            double ye1=+sqrt(r_center*r_center-(xe-x_center)*(xe-x_center))+y_center;
                            double ye2=-sqrt(r_center*r_center-(xe-x_center)*(xe-x_center))+y_center;

                            q=sqrt((xe-xa)*(xe-xa)+(ye1-ya)*(ye1-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));

                            if(q>1.0+1.0e-8){

                                q=sqrt((xe-xa)*(xe-xa)+(ye2-ya)*(ye2-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));
                            }
                        }
                        else if(ya==yb){

                            double ye=ya;
                            double xe1=+sqrt(r_center*r_center-(ye-y_center)*(ye-y_center))+x_center;
                            double xe2=-sqrt(r_center*r_center-(ye-y_center)*(ye-y_center))+x_center;

                            q=sqrt((xe1-xa)*(xe1-xa)+(ye-ya)*(ye-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));

                            if(q>1.0+1.0e-8){

                                q=sqrt((xe2-xa)*(xe2-xa)+(ye-ya)*(ye-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));
                            }
                        }
                        else{

                            double k1=1.0*(yb-ya)/(yb-ya);

                            double xe1=-2*(x_center+k1*y_center)+sqrt(2*(x_center+k1*y_center)*2*(x_center+k1*y_center)-4*(1+k1*k1)*(x_center*x_center+y_center*y_center-r_center*r_center))/2/(1+k1*k1);
                            double xe2=-2*(x_center+k1*y_center)-sqrt(2*(x_center+k1*y_center)*2*(x_center+k1*y_center)-4*(1+k1*k1)*(x_center*x_center+y_center*y_center-r_center*r_center))/2/(1+k1*k1);

                            double ye1=ya+k1*(xe1-xa);
                            double ye2=ya+k1*(xe2-xa);

                            double q=sqrt((xe1-xa)*(xe1-xa)+(ye1-ya)*(ye1-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));

                            if(q>1.0+1.0e-8){

                                q=sqrt((xe2-xa)*(xe2-xa)+(ye2-ya)*(ye2-ya))/sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb));
                            }
                        }

                        if(k==0){rvs_k=0;}
                        else if (k == 1 || k == 2 || k == 5 || k == 6) {
                            rvs_k = k + 2;
                        } else {
                            rvs_k = k - 2;
                        }

                        if(q<0.5)
                        {
                            f[xa*MM1+ya].f[rvs_k]=q*(1.+2.*q)*f[xb*MM1+yb].f[k]+(1.-4.*q*q)*f[xa*MM1+ya].f[k]-q*(1.-2.*q)*f[xc*MM1+yc].f[k];
                        }
                        else{
                            f[xa*MM1+ya].f[rvs_k]=1./(q*(2.*q+1.))*f[xb*MM1+yb].f[k]+(2.*q-1.)/q*f[xc*MM1+yc].f[rvs_k]-(2.*q-1.)/(2.*q+1)*f[xd*MM1+yd].f[rvs_k];
                        }
                    }
                }
            }
        }
    }
}
