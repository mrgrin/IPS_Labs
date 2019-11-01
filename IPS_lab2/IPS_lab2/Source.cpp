#include <stdio.h>
#include <ctime>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_opmul.h>
#include <cilk/reducer_opadd.h>
#include <chrono>

using namespace std::chrono;

const int MATRIX_SIZE = 1500; // количество строк в исходной квадратной матрице
const int PARALLEL_ACTIVE = 1; // флаг параллельных расчетов


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
	// прямой ход метода Гаусса
	for (int k = 0; k < rows; ++k)
		for (int i = k + 1; i < rows; ++i)
		{
			double koef = -matrix[i][k] / matrix[k][k];
			for (int j = k; j <= rows; ++j)
				matrix[i][j] += koef * matrix[k][j];
		}

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for (int k = rows - 2; k >= 0; --k)
	{
		result[k] = matrix[k][rows];
		for (int j = k + 1; j < rows; ++j)
			result[k] -= matrix[k][j] * result[j];
		result[k] /= matrix[k][k];
	}
}

void ParallelGaussMethod(double **matrix, const int rows, double* result)
{
	// прямой ход метода Гаусса
	for (int k = 0; k < rows; ++k)
		cilk_for(int i = k + 1; i < rows; ++i)
		{
			double koef = -matrix[i][k] / matrix[k][k];
			for(int j = k; j <= rows; ++j) {
				matrix[i][j] += koef * matrix[k][j];
			}
		}

	// обратный ход метода Гаусса
	result[rows - 1] = matrix[rows - 1][rows] / matrix[rows - 1][rows - 1];

	for (int k = rows - 2; k >= 0; --k)
	{
		cilk::reducer < cilk::op_add<double> > par_result(matrix[k][rows]);
		for(int j = k + 1; j < rows; ++j)
			*par_result += -matrix[k][j] * result[j];
		result[k] = par_result.get_value() / matrix[k][k];
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
	//cilk::reducer<cilk::op_vector<int>>red_vec;
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

	// PrintResults(result, test_matrix_lines);

	bool isCorrect = true;
	for(int i = 0; i < test_matrix_lines; i++)
		if (result[i] != true_result[i]) {
			printf("Wrong answer in line %d.\n Method returns %f but must return %f\n",
				i + 1, result[i], true_result[i]);
			isCorrect = false;
			break;
		}

	DeleteMatrix(test_matrix, test_matrix_lines);
	delete[]result;

	return isCorrect;
}


int main()
{
	if (PARALLEL_ACTIVE) {
		__cilkrts_set_param("nworkers", "8");
	}
	else


	if (!TestMethod(PARALLEL_ACTIVE)) {
		getchar();
		return 0;
	}

	srand((unsigned)time(0));

	double **slau_matrix = new double*[MATRIX_SIZE];
	double **slau_matrix2 = new double*[MATRIX_SIZE];
	double *result = new double[MATRIX_SIZE];
	double *result2 = new double[MATRIX_SIZE];

	high_resolution_clock::time_point t_begin, t_end;

	InitMatrix(slau_matrix);
	InitMatrix(slau_matrix2);
	for (int i = 0; i < MATRIX_SIZE; i++)
		for (int j = 0; j < MATRIX_SIZE; j++)
			slau_matrix2[i][j] = slau_matrix[i][j];
	t_begin = high_resolution_clock::now();
	ParallelGaussMethod(slau_matrix, MATRIX_SIZE, result);
	t_end = high_resolution_clock::now();

	duration<double> duration_parallel = (t_end - t_begin);
	printf("Time passed Parallel: %f\n", duration_parallel.count());

	t_begin = high_resolution_clock::now();
	SerialGaussMethod(slau_matrix2, MATRIX_SIZE, result2);
	t_end = high_resolution_clock::now();

	duration<double> duration_serial = (t_end - t_begin);
	printf("Time passed Serial7: %f\n", duration_serial.count());
	printf("Time comparison: %lf\n", duration_serial.count() / duration_parallel.count());

	//printf("Solution:\n");
	//PrintResults(result, MATRIX_SIZE);

	DeleteMatrix(slau_matrix, MATRIX_SIZE);
	DeleteMatrix(slau_matrix2, MATRIX_SIZE);
	delete[] result;
	delete[] result2;

	getchar();
	return 0;
}