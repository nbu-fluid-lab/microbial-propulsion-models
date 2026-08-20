#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include <cmath>

#include <sstream>
#include <string>
#include <iostream>
#include <iostream>
#include <ctime>
#include<random>
#include<chrono>

using namespace std;

double Pi = 3.141592653589793;

/*******************************************************************************************/
double splitAngleIntoN(double a, double b, const int P_dis_n, double P_dis[]) {
    if (a < b) {
        double a1 = a;
        a = b;
        b = a1;
    }
    double c = sqrt(a * a - b * b);
    double e1 = c / a;
    double th_split = Pi / 2 / P_dis_n;
    for (int i = 0; i < P_dis_n; i++) {
        double temp = a * sqrt(1 - e1 * e1 * cos(i * th_split) * cos(i * th_split)) * th_split;
        if (i == 0) {
            P_dis[i] = temp;
        } else {
            P_dis[i] = P_dis[i - 1] + temp;
        }
    }
    return 1.;
}

int nearMultiple(double data, int num) {
    int res = floor(data);
    for (int i = 0; i < num * 2; i++) {
        if (res % num == 0) {
            break;
        }
        res = res - 1;
    }
    return res;
}

void mkdir(const string &_dirs) {
    string command = "mkdir -p " + _dirs;
    system(command.c_str());
}


void fpclose(FILE *fp) {
    if (fp == NULL) {
    } else {
        fclose(fp);
    }
}

/**
 * 二维坐标/向量
 */
class CVector {
public:
    double x = 0.;
    double y = 0.;

    CVector() {
    };

    CVector(double a) {
        x = y = a;
    };

    CVector(double a, double b) {
        x = a;
        y = b;
    };

    double getX() { return x; }

    double getY() { return y; }

    CVector &operator+=(const CVector &a) {
        x += a.x;
        y += a.y;
        return *this;
    };

    CVector &operator-=(const CVector &a) {
        x -= a.x;
        y -= a.y;
        return *this;
    };

    friend ostream &operator<<(ostream &os, const CVector &vector) {
        os << "(" << vector.x << "," << vector.y << ")";
        return os;
    }

    friend CVector operator+(const CVector &a, const CVector &b) {
        return CVector(a.x + b.x, a.y + b.y);
    };


    friend CVector operator-(const CVector &a, const CVector &b) {
        return CVector(a.x - b.x, a.y - b.y);
    };

    friend double operator*(const CVector &a, const CVector &b) {
        return (a.x * b.x + a.y * b.y);
    };

    friend CVector operator*(const double &a, const CVector &b) {
        return CVector(a * b.x, a * b.y);
    };

    friend CVector operator*(const CVector &a, const double &b) {
        return CVector(a.x * b, a.y * b);
    };

    friend double operator%(const CVector &a, const CVector &b) {
        return (a.x * b.y - a.y * b.x);
    };

    friend CVector operator/(const double &a, const CVector &b) {
        return CVector(a / b.x, a / b.y);
    };

    friend CVector operator/(const CVector &a, const double &b) {
        return CVector(a.x / b, a.y / b);
    };

    friend bool operator==(const CVector &a, const CVector &b) {
        if (a.x == b.x && a.y == b.y) {
            return true;
        } else {
            return false;
        }
    };

    friend double Modul(const CVector &a) {
        return sqrt(a * a);
    };

    /**
     * 叉乘
     * @param a 向量a
     * @param b  向量b
     * @return 叉乘的结果
     */
    friend double vectorCrossMulti(const CVector &a, const CVector &b) {
        return a.x * b.y - a.y * b.x;
    };

    /**
    * 计算向量的倾斜角 (0-2π)
    * @param vec 向量
    * @return
    */
    double culAngleOfInclination() {
        if (x == 0 && y == 0) { //原点
            return 0;
        } else if (x == 0 && y > 0) { //y轴+
            return Pi / 2;
        } else if (x == 0 && y < 0) { //y轴-
            return 3 * Pi / 2;
        } else if (y == 0 && x > 0) { //x轴+
            return 0;
        } else if (y == 0 && x < 0) { //x轴-
            return Pi / 2;
        } else if (x > 0 && y > 0) { //第一象限
            return atan(y / x);
        } else if (x < 0 && y > 0) {//第二象限
            return atan(y / x) + Pi;
        } else if (x < 0 && y < 0) {//第三象限
            return atan(y / x) + Pi;
        } else if (x > 0 && y < 0) {//第四象限
            return atan(y / x) + 2 * Pi;
        }
        return 0;
    }

};


/**
 * 一维数组初始化
 * @param size 数组大小
 * @return 初始化后的一维数组
 */
double *arrayInit(int size) {
    return (double *) malloc(sizeof(double) * size);
}

/**
 * 一维数组初始化并且随机赋值
 * @param size 数组大小
 * @return 赋值后的数组
 */
double *arrayInitWithRandom(int size) {
    double *res = arrayInit(size);

    for (int i = 0; i < size; i++) {
        res[i] = (double) (rand() % 10000) / 10000;

    }
    return res;
}

/**
 * 方阵初始化
 * @param size 方阵大小
 * @return 初始化后方阵
 */
double **matInit(int size) {
    double **res = (double **) malloc(sizeof(double *) * size);

    for (int i = 0; i < size; i++) {
        res[i] = (double *) malloc(sizeof(double) * size);
    }
    return res;
}


/**
 * 二维数组初始化
 * @param size 方阵大小
 * @return 初始化后方阵
 */
double **matInit(int m, int n) {
    double **res = (double **) malloc(sizeof(double *) * m);

    for (int i = 0; i < m; i++) {
        res[i] = (double *) malloc(sizeof(double) * n);
    }
    return res;
}

/**
 * 二维数组初始化
 * @param size 方阵大小
 * @return 初始化后方阵
 */
CVector **vectorMatInit(int m, int n) {
    CVector **res = (CVector **) malloc(sizeof(CVector *) * m);

    for (int i = 0; i < m; i++) {
        res[i] = (CVector *) malloc(sizeof(CVector) * n);
    }
    return res;
}

/**
 * 方阵的释放
 * @param mat 需要释放的方阵
 * @param size 方阵大小
 * @return 是否释放成功
 */
bool matDestroy(double **mat, int size) {
    for (int i = 0; i < size; i++) {
        delete mat[i];
        mat[i] = NULL;
    }
    delete[] mat;
    mat = NULL;
    return true;
}

/**
 * 复制方阵
 * @param mat 需要复制的方阵
 * @param size  方阵大小
 * @return
 */
double **matCopy(double **mat, int size) {
    double **res = matInit(size);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            res[i][j] = mat[i][j];
        }
    }
    return res;
}

/**
 * 方阵初始化并且随机赋值
 * @param size 方阵大小
 * @return 初始化后的方阵
 */
double **matInitWithRandom(int size) {
    double **res = matInit(size);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            res[i][j] = (double) (rand() % 10000) / 10000;
        }
    }
    return res;
}

/**
 * 输出方阵
 * @param mat 需要输出的方阵
 * @param size 方阵大小
 */
void matPrint(double **mat, int size) {
    printf("*******************************************************************************************\n");
    printf("[\n");
    for (int i = 0; i < size; i++) {
        printf("[");
        for (int j = 0; j < size; j++) {
            printf("%9.4lf,", mat[i][j]);
        }
        printf("],\n");
    }
    printf("]\n");
    printf("*******************************************************************************************\n");
}

/**
 * 矩阵乘法
 * @param matA 方阵A
 * @param matB 方阵B
 * @param size 矩阵大小
 * @return 矩阵相乘的结果
 */
double **matMul(double **matA, double **matB, int size) {
    double **C = matInit(size);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < size; k++) {
                C[i][j] += matA[i][k] * matB[k][j];
            }
        }
    }

    //若绝对值小于10^-10,则置为0（这是我自己定的）
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; ++j) {
            if (abs(C[i][j]) < pow(10, -10)) {
                C[i][j] = 0;
            }
        }
    }

    return C;
}


/**
 * 方阵求逆-LUP分解
 * 参考链接 https://blog.csdn.net/qq_54434938/article/details/120896660
 *
 * @param mat 需要求逆的方阵
 * @param size 矩阵大小
 * @return 方阵的逆
 */
double **matInvertByLU(double **mat, int size) {
    double **res = matInit(size);
    double **matC = matCopy(mat, size);
    double tmp;
    for (int i = 0; i < size; i++) {
        res[i][i] = 1;
    }
    //下来进行自上而下的初等行变换，使得矩阵 matC 变成单位上三角矩阵
    for (int i = 0; i < size; i++) {
        //因为要判断最后一行化为上三角矩阵的最后一行最后一列元素是否为 0
        //寻找第 i 列不为零的元素
        int k;
        for (k = i; k < size; k++) {
            if (fabs(matC[k][i]) > 1e-10) {//满足这个条件时，认为这个元素不为0
                break;
            }
            //说明第 i 列有全为0，不可逆
            if (k == size - 1) {
                cout << "不可逆！" << endl;
                return res;
            }
        }

        //说明第 i 行 第 i 列元素为零，需要和其他行交换
        if (k != i) {
            //交换第 i 行和第 k 行所有元素
            //需从第一个元素交换，注意与之前化上三角矩阵不同
            for (int j = 0; j < size; j++) {
                //使用mat[0][j]作为中间变量交换元素,两个矩阵都要交换
                tmp = matC[i][j];
                matC[i][j] = matC[k][j];
                matC[k][j] = tmp;
                tmp = res[i][j];
                res[i][j] = res[k][j];
                res[k][j] = tmp;
            }
        }

        tmp = matC[i][i];//倍数
        //将矩阵 matC 的主对角线元素化为 1
        for (int j = 0; j < size; j++) {
            matC[i][j] /= tmp;
            res[i][j] /= tmp;
        }
        for (int j = i + 1; j < size; j++) {
            //注意本来为 -res[j][i]/res[i][i],因为a.res[i][i]等于 1，则不需要除它
            tmp = -matC[j][i];
            for (k = 0; k < size; k++) {
                matC[j][k] += tmp * matC[i][k];//第 i 行 b 倍加到第 j 行
                res[j][k] += tmp * res[i][k];
            }
        }

    }

    //下面进行自下而上的行变换，将 matC 矩阵化为单位矩阵
    for (int i = size - 1; i > 0; i--) {
        for (int j = i - 1; j >= 0; j--) {
            tmp = -matC[j][i];
            matC[j][i] = 0; //实际上是通过初等行变换将这个元素化为 0,
            for (int k = 0; k < size; k++) {//通过相同的初等行变换来变换右边矩阵
                res[j][k] += tmp * res[i][k];
            }
        }
    }
    matDestroy(matC, size);
    return res;
};


int testMatrix() {
    int size = 10;
    double **matrix1 = matInitWithRandom(size);
    matPrint(matrix1, size);

    double **matrix2 = matInvertByLU(matrix1, size);
    matPrint(matrix2, size);

    double **matrix12 = matMul(matrix1, matrix2, size);
    matPrint(matrix12, size);

    double **matrix3 = matInvertByLU(matrix2, size);
    matPrint(matrix3, size);

    return 0;
}


int sgn(double x) {
    if (x > 0) {
        return 1;
    } else if (x == 0) {
        return 0;
    } else {
        return -1;
    }
}

double angleReSection(double angle, int step) {
    if (step <= 0) {
        return angle;
    }
    if (angle < 0) {
        return angleReSection(angle + 2 * Pi, step - 1);
    } else if (angle >= 2 * Pi) {
        return angleReSection(angle - 2 * Pi, step - 1);
    }
    return angle;
}

double angleReSection(double angle) {
    return angleReSection(angle, 10);
}

/**
 * 判断约等于，去除精度误差
 */
bool approximateEqual(double a, double b) {
    return abs(a - b) < 10e-8;
}

double rand_fun() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine gen(seed);
    std::normal_distribution<double> dis(0, 1);
    double res = dis(gen);

    //    正态分布中“sigma原则”、“2sigma原则”、“3sigma原则”分别是：
    //    sigma原则：数值分布在（μ-σ，μ+σ）中的概率为0.6826；
    //    2sigma原则：数值分布在（μ-2σ，μ+2σ）中的概率为0.9544；
    //    3sigma原则：数值分布在（μ-3σ，μ+3σ）中的概率为0.9974；
    // [-3,3]
    if (res < -3) {
        res = -3;
    } else if (res > 3) {
        res = 3;
    }
    // [-pi,pi]
    return (res / 3 + 1) * 3.1415926;
}
