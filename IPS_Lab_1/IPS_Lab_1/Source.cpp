/*
This progtam is using Intel Parallel Studio v2019 Update 4
*/
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/reducer_max.h>
#include <cilk/reducer_min.h>
#include <cilk/reducer_vector.h>
#include <chrono>
#include <vector>
#include <iostream>

using namespace std::chrono;

/// ‘ункци€ ReducerMaxTest() определ€ет максимальный элемент массива,
/// переданного ей в качестве аргумента, и его позицию
/// mass_pointer - указатель исходный массив целых чисел
/// size - количество элементов в массиве
void ReducerMaxTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_max_index<long, int>> maximum;
	cilk_for(long i = 0; i < size; ++i)
	{
		maximum->calc_max(i, mass_pointer[i]);
	}
	printf("Maximal element = %d has index = %d\n",
		maximum->get_reference(), maximum->get_index_reference());
}

/// ‘ункци€ ReducerMinTest() определ€ет минимальный элемент массива,
/// переданного ей в качестве аргумента, и его позицию
/// mass_pointer - указатель исходный массив целых чисел
/// size - количество элементов в массиве
void ReducerMinTest(int *mass_pointer, const long size)
{
	cilk::reducer<cilk::op_min_index<long, int>> minimum;
	cilk_for(long i = 0; i < size; ++i)
	{
		minimum->calc_min(i, mass_pointer[i]);
	}
	printf("Minimum element = %d has index = %d\n",
		minimum->get_reference(), minimum->get_index_reference());
}

/// ‘ункци€ ParallelSort() сортирует массив в пор€дке возрастани€
/// begin - указатель на первый элемент исходного массива
/// end - указатель на последний элемент исходного массива
void ParallelSort(int *begin, int *end)
{
	if (begin != end)
	{
		--end;
		int *middle = std::partition(begin, end, std::bind2nd(std::less<int>(), *end));
		std::swap(*end, *middle);
		cilk_spawn
			ParallelSort(begin, middle);
		ParallelSort(++middle, ++end);
		cilk_sync;
	}
}

void Pad() {
	std::cout << "-------------------------------------" << std::endl;
}

int main()
{
	srand((unsigned)time(0));

	// устанавливаем количество работающих потоков
	__cilkrts_set_param("nworkers", "8");

	const long mass_size = 1000000;
	int *mass_begin, *mass_end;
	int *mass = new int[mass_size];

	duration<double> duration;
	high_resolution_clock::time_point t_begin, t_end;

	std::cout << "Creating array with " << mass_size << " elements." << std::endl;
	Pad();

	for (long i = 0; i < mass_size; ++i)
	{
		mass[i] = (rand() % 25000) + 1;
	}

	mass_begin = mass;
	mass_end = mass_begin + mass_size;

	std::cout << "Unsorted array results: " << std::endl;
	ReducerMaxTest(mass, mass_size);
	ReducerMinTest(mass, mass_size);

	Pad();
	t_begin = high_resolution_clock::now();
	ParallelSort(mass_begin, mass_end);
	t_end = high_resolution_clock::now();
	duration = (t_end - t_begin);
	std::cout << "Duration is: " << duration.count() << " seconds" << std::endl;

	Pad();
	std::cout << "Sorted array results: " << std::endl;
	ReducerMaxTest(mass, mass_size);
	ReducerMinTest(mass, mass_size);

	delete[]mass;

	std::cout << std::endl << "Computing vectors from task 4" << std::endl;
	Pad();

	int data_size[8] = { 1000000, 100000, 10000, 1000, 500, 100, 50, 10 };
	for (int n = 0; n < 8; n++) {
		std::vector<int> data;
		cilk::reducer<cilk::op_vector<int>>red_vec;

		//Casual vector creation
		std::cout << "Num of elements: " << data_size[n] << std::endl;
		t_begin = high_resolution_clock::now();
		for (int i = 0; i < data_size[n]; i++) {
			data.push_back(rand() % 20000 + 1);
		}
		t_end = high_resolution_clock::now();
		duration = (t_end - t_begin);
		std::cout << "Vector was created in: " << duration.count() << " seconds" << std::endl;

		//Reducer vector creation
		t_begin = high_resolution_clock::now();
		cilk_for(int i = 0; i < data_size[n]; i++) {
			red_vec->push_back(rand() % 20000 + 1);
		}
		t_end = high_resolution_clock::now();
		duration = (t_end - t_begin);
		std::cout << "Reducer was created in: " << duration.count() << " seconds" << std::endl << std::endl;
		Pad();
	}

	system("PAUSE");
	return 0;
}