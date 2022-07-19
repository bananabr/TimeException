// TimeException.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <filesystem>
#include <numeric>

#include <Windows.h>
#include <Rpc.h>

#include "trimmean.h"

using namespace std;

template<typename T>
double getAverage(std::vector<T> const& v) {
	if (v.empty()) {
		return 0;
	}
	return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

vector<char> getRandomBytes(size_t size) {
	random_device rd;
	uniform_int_distribution<int> dist(0, 255);
	vector<char> data(size);
	for (char& d : data)
	{
		d = static_cast<char>(dist(rd) & 0xFF);
	}
	return data;
}

void usage(char* programName) {
	std::cout << "Usage:" << endl;
	std::cout << programName << " root_folder [sample_size] [depth] [sensitivity]" << endl;
}

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 5)
	{
		usage(argv[0]);
		return 1;
	}

	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;
	ofstream myfile;
	vector<vector<char>> sampleSet = {};
	vector<long long int> localResultSet = {};
	vector<double> globalResultSet = {};
	vector<string> paths = {};
	std::filesystem::path basepath = argv[1];
	size_t sample_size = 5000;
	size_t depth = 0;
	double sensitivity = 0.25;

	if (argc > 2)
	{
		sample_size = atoi(argv[2]);
	}

	if (argc > 3)
	{
		depth = atoi(argv[3]);
	}

	if (argc > 4)
	{
		sensitivity = atof(argv[4]);
	}

	QueryPerformanceFrequency(&Frequency);
	auto dit = std::filesystem::recursive_directory_iterator(basepath);

	//std::cout << "\"Path\",\"ElapsedMicroseconds\"" << endl;
	std::cout << "[+] sample_size:" << sample_size << endl;
	std::cout << "[+] depth:" << depth << endl;
	std::cout << "[+] sensitivity:" << sensitivity << endl;
	for (const auto& path : dit) {
		try {
			if (dit.depth() > depth)
			{
				continue;
			}
			if (path.is_directory()) {

				for (size_t i = 0; i < sample_size; i++)
				{
					sampleSet.push_back(getRandomBytes(512));
				}

				for (size_t i = 0; i < sample_size; i++)
				{
					UUID uuid;
					if (UuidCreate(&uuid) != RPC_S_OK) {
						cerr << "Failed to generate UUID" << endl;
						return 1;
					}
					RPC_CSTR szUuid = NULL;
					string cstrUuid;
					if (UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
					{
						cstrUuid = (char*)szUuid;
						RpcStringFreeA(&szUuid);
					}
					else {
						cerr << "Failed to generate file name" << endl;
						return 1;
					}
					cstrUuid += ".tex";
					std::filesystem::path filePath = path.path() / cstrUuid;

					myfile.open(filePath, ios::out | ios::binary);

					QueryPerformanceCounter(&StartingTime);
					myfile << sampleSet[i].data();
					QueryPerformanceCounter(&EndingTime);

					myfile.close();

					filesystem::remove(filePath);

					ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
					ElapsedMicroseconds.QuadPart *= 1000000;
					ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
					localResultSet.push_back(ElapsedMicroseconds.QuadPart);
				}
				double tm = TRIMMEAN(localResultSet.data(), localResultSet.size(), 0.2);
				globalResultSet.push_back(tm);
				paths.push_back(path.path().string());
				std::cout << "[.] " << path << "," << dit.depth() << "," << tm << endl;
				sampleSet.clear();
				localResultSet.clear();
			}
		}
		catch (...) {
			std::cerr << "[-] Failed to process " << path.path() << endl;
		}
	}
	for (size_t i = 0; i < paths.size(); i++)
	{
		double tm = globalResultSet[i];
		double gMean = getAverage(globalResultSet);
		double gMeanDistance = (1 - (tm / gMean));
		if (gMeanDistance >= sensitivity)
		{
			std::cout << "[!] Writing to " << paths[i] << " is " << gMeanDistance * 100 << "% faster than average!" << endl;
		}
	}
	return 0;
}
