#include <stdio.h>
#include <ctime>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_opmul.h>
#include <chrono>

using namespace std::chrono;

// количество строк в исходной квадратной матрице
const int MATRIX_SIZE = 10;
const int PARALLEL_ACTIVE = 1;


/// Функция InitMatrix() заполняет переданную в качестве 
/// параметра квадратную матрицу случайными значениями
/// matrix - исходная матрица СЛАУ
void InitMatrix(double** matrix)
{
	for (int i = 0; i < MATRIX_SIZE; ++i)
		matrix[i] = new double[MATRIX_SIZE + 1];

	for (int i = 0; i < MATRIX_SIZE; ++i)
		for (int j = 0; j <= MATRIX_SIZE; ++j)
			matrix[i][j] = rand() % 2500 + 1;
}


/// Функция SerialGaussMethod() решает СЛАУ методом Гаусса 
/// matrix - исходная матрица коэффиициентов уравнений, входящих в СЛАУ,
/// последний столбей матрицы - значения правых частей уравнений
/// rows - количество строк в исходной матрице
/// result - массив ответов СЛАУ
void SerialGaussMethod(double **matrix, const int rows, double* result)
{
	int k;
	double koef;

	// прямой ход метода Гаусса
	for (k = 0; k < rows; ++k)
		for (int i = k + 1; i < rows; ++i)
		{
			koef = -matrix[i][k] / matrix[k][k];
			for (int j = k; j <= rows; ++j)
				matrix[i][j] += koef * matrix[k][j];
		}

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for (k = rows - 2; k >= 0; --k)
	{
		result[k] = matrix[k][rows];
		for (int j = k + 1; j < rows; ++j)
			result[k] -= matrix[k][j] * result[j];
		result[k] /= matrix[k][k];
	}
}

void ParallelGaussMethod(double **matrix, const int rows, double* result)
{
	int k;
	double koef;

	// прямой ход метода Гаусса
	for (k = 0; k < rows; ++k)
		for (int i = k + 1; i < rows; ++i)
		{
			koef = -matrix[i][k] / matrix[k][k];
			//cilk::reducer <cilk::op_mul<double>> res;
			cilk_for(int j = k; j <= rows; ++j) {
				matrix[i][j] += koef * matrix[k][j];
			}
		}

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for (k = rows - 2; k >= 0; --k)
	{
		result[k] = matrix[k][rows];
		cilk_for(int j = k + 1; j < rows; ++j)
			result[k] -= matrix[k][j] * result[j];
		result[k] /= matrix[k][k];
	}
}


void PrintResults(double *result, int result_size) {
	for (int i = 0; i < result_size; ++i)
		printf("x(%d) = %lf\n", i, result[i]);
}


void DeleteMatrix(double **matrix, int matrix_size) {
	for (int i = 0; i < matrix_size; ++i)
		delete[]matrix[i];
}


bool TestMethod(bool isParallel) {

	const int test_matrix_lines = 4;
	const double true_result[4] = { 1, 2, 2, -0 };

	double **test_matrix = new double*[test_matrix_lines];
	double *result = new double[test_matrix_lines];

	// цикл по строкам
	for (int i = 0; i < test_matrix_lines; ++i)
	{
		// (test_matrix_lines + 1)- количество столбцов в тестовой матрице,
		// последний столбец матрицы отведен под правые части уравнений, входящих в СЛАУ
		test_matrix[i] = new double[test_matrix_lines + 1];
	}

	// инициализация тестовой матрицы
	test_matrix[0][0] = 2; test_matrix[0][1] = 5;  test_matrix[0][2] = 4;  test_matrix[0][3] = 1;  test_matrix[0][4] = 20;
	test_matrix[1][0] = 1; test_matrix[1][1] = 3;  test_matrix[1][2] = 2;  test_matrix[1][3] = 1;  test_matrix[1][4] = 11;
	test_matrix[2][0] = 2; test_matrix[2][1] = 10; test_matrix[2][2] = 9;  test_matrix[2][3] = 7;  test_matrix[2][4] = 40;
	test_matrix[3][0] = 3; test_matrix[3][1] = 8;  test_matrix[3][2] = 9;  test_matrix[3][3] = 2;  test_matrix[3][4] = 37;

	if (isParallel)
		ParallelGaussMethod(test_matrix, test_matrix_lines, result);
	else
		SerialGaussMethod(test_matrix, test_matrix_lines, result);

	PrintResults(result, test_matrix_lines);

	bool isCorrect = true;
	for(int i = 0; i < test_matrix_lines; i++)
		if (result[i] != true_result[i]) {
			printf("Wrong answer in line %d.\n Method returns %f but must return %f\n",
				i + 1, result[i], true_result[i]);
			isCorrect = false;
		}

	DeleteMatrix(test_matrix, test_matrix_lines);
	delete[]result;

	return isCorrect;
}


int main()
{
	if(PARALLEL_ACTIVE) 
		__cilkrts_set_param("nworkers", "4");

	if (!TestMethod(PARALLEL_ACTIVE)) {
		getchar();
		return 0;
	}

	srand((unsigned)time(0));

	double **slau_matrix = new double*[MATRIX_SIZE];
	double *result = new double[MATRIX_SIZE];
	high_resolution_clock::time_point t_begin, t_end;

	t_begin = high_resolution_clock::now();
	InitMatrix(slau_matrix);

	if(PARALLEL_ACTIVE)
		ParallelGaussMethod(slau_matrix, MATRIX_SIZE, result);
	else
		SerialGaussMethod(slau_matrix, MATRIX_SIZE, result);
	t_end = high_resolution_clock::now();

	printf("Solution:\n");
	PrintResults(result, MATRIX_SIZE);

	duration<double> duration = (t_end - t_begin);
	printf("Time passed: %f\n", duration.count());

	DeleteMatrix(slau_matrix, MATRIX_SIZE);
	delete[] result;

	getchar();
	return 0;
}