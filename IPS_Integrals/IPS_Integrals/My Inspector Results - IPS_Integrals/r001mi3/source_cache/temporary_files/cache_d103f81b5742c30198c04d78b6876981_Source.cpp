#include <iostream>
#include <chrono>
#include <thread>
#include <locale.h>
#include <cilk/cilk.h>
#include <cilk/reducer_opadd.h>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;

double MyInt(double x) {
	return 6 / sqrt(x * (2 - x));
}

//Serial integral computation
double SerialInt(double (*f)(double res), double a, double b, int n) {
	double step = (b - a) / n;
	double area = 0.0;  // signed area
	for (int i = 0; i < n; i++) {
		area += f(a + (i + 0.5) * step) * step; // sum up each small rectangle
	}
	return area;
}

//Serial integral computation
double ParallelInt(double(*f)(double res), double a, double b, int n) {
	double step = (b - a) / n;
	
	cilk::reducer<cilk::op_add<double>>area(0.0);  // signed area
	cilk_for (int i = 0; i < n; i++) {
		*area += f(a + (i + 0.5) * step) * step; // sum up each small rectangle
	}
	return area.get_value();
}

void CalcInt(double a, double b, int n, bool IsParallel) {
	double res;
	//cilk::reducer<cilk::op_min_index<long, int>> minimum;
	auto start = std::chrono::system_clock::now();
	if (IsParallel == true) {
		cout << "PARALLEL COMPUTATION:";
		cout << endl;
		res = ParallelInt(MyInt, a, b, n);
	}
	else {
		cout << "SERIAL COMPUTATION:" << endl;
		res = SerialInt(MyInt, a, b, n);
	}
	auto end = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	cout.precision(10);

	cout << "Time Elapsed: " << duration.count() / 1000.0 << "ms"<< endl;
	cout << "Result: " << res << endl;

	cout << "=============================" << endl;
}

int main()
{
	double a = 0.5;
	double b = 1;
	int steps_num = 10000000000;

	CalcInt(a, b, steps_num, false);
	CalcInt(a, b, steps_num, true);
	getchar();
}