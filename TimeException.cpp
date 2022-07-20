// TimeException.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <numeric>

#include <Windows.h>
#include <Rpc.h>

#include "trimmean.h"

using namespace std;

#define MIN_SAMPLE_SIZE 500

#define DIR_MODE 0
#define EXT_MODE 1
#define PROC_MODE 2

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

map<string, string> argvToMap(int argc, char* argv[])
{
	map<string, string> args;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			const string key = argv[i];
			string value = "";
			if (i + 1 < argc && argv[i + 1][0] != '-') {
				value = string(argv[i + 1]);
				i++;
			}

			args[key] = value;
		}
	}

	return args;
}

void printUsage(char* programName) {
	std::cout << "Usage:" << endl;
	std::cout << programName << " root_folder [sample_size] [depth] [sensitivity]" << endl;
}

int main(int argc, char** argv)
{
	auto args = argvToMap(argc, argv);
	if (args.count("-h") || args.count("--help")) {
		printUsage(argv[0]);
	}

	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;
	ofstream myfile;
	vector<vector<char>> sampleSet = {};
	vector<long long int> localResultSet = {};
	vector<double> globalResultSet = {};
	vector<string> paths = {};
	//std::filesystem::path basepath = argv[1];
	size_t sample_size = MIN_SAMPLE_SIZE;
	size_t depth = 0;
	double sensitivity = 0.25;
	int opMode = DIR_MODE;
	ifstream targetsFile;

	if (args.count("--targets")) {
		targetsFile.open(args["--targets"], ios_base::in);
		if (!targetsFile.is_open())
		{
			cerr << "[-] Failed to open targets file for reading" << endl;
			return 1;
		}
	}
	else {
		printUsage(argv[0]);
		return 1;
	}

	if (args.count("--mode")) {
		switch (stoi(args["--mode"])) {
		case DIR_MODE:
			opMode = DIR_MODE;
			break;
		case EXT_MODE:
			opMode = EXT_MODE;
			break;
		case PROC_MODE:
			opMode = PROC_MODE;
			break;
		default:
			printUsage(argv[0]);
			return 1;
		}
	}

	if (args.count("--sample-size")) {
		try {
			sample_size = stoi(args["--sample-size"]);
		}
		catch (...) {
			cerr << "[-] Failed to parse sample size" << endl;
			return 1;
		}
	}

	if (args.count("--depth")) {
		try {
			depth = stoi(args["--depth"]);
		}
		catch (...) {
			cerr << "[-] Failed to parse depth" << endl;
			return 1;
		}
	}

	if (args.count("--sensitivity")) {
		try {
			sensitivity = stof(args["--sensitivity"]);
		}
		catch (...) {
			cerr << "[-] Failed to parse sensitivity" << endl;
			return 1;
		}
	}

	QueryPerformanceFrequency(&Frequency);
	//auto dit = std::filesystem::recursive_directory_iterator(basepath);

	//std::cout << "\"Path\",\"ElapsedMicroseconds\"" << endl;
	std::cout << "[+] sample_size:" << sample_size << endl;
	if (sample_size < MIN_SAMPLE_SIZE)
	{
		std::cout << "[-] sample_size is too short and might results in a large number of false positives" << endl;
	}
	std::cout << "[+] depth:" << depth << endl;
	std::cout << "[+] sensitivity:" << sensitivity << endl;

	string line;
	while (std::getline(targetsFile, line))
	{
		//	std::cout << "line:" << line << std::endl;
		//	// TODO: assign item_name based on line (or if the entire line is 
		//	// the item name, replace line with item_name in the code above)
		//}

		//for (const auto& path : dit) {
		try {
			//if (dit.depth() > depth)
			//{
			//	continue;
			//}
			std::filesystem::path path(line);
			if (std::filesystem::is_directory(path)) {

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
					std::filesystem::path filePath = path / cstrUuid;

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
				paths.push_back(path.string());
				std::cout << "[.] " << path << "," << tm << endl;
				sampleSet.clear();
				localResultSet.clear();
			}
		}
		catch (...) {
			std::cerr << "[-] Failed to process " << line << endl;
		}
	}
	for (size_t i = 0; i < paths.size(); i++)
	{
		double tm = globalResultSet[i];
		double gMean = getAverage(globalResultSet);
		double gMeanDistance = (1 - (tm / gMean));
		if (gMeanDistance >= sensitivity)
		{
			std::cout << "[+] Writing to " << paths[i] << " is " << gMeanDistance * 100 << "% faster than average!" << endl;
			std::cout << "[.] Checking for false positive ..." << endl;
			std::filesystem::path candidatePath(paths[i]);
			for (size_t i = 0; i < 5000; i++)
			{
				sampleSet.push_back(getRandomBytes(512));
			}

			for (size_t i = 0; i < 5000; i++)
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
				std::filesystem::path filePath = candidatePath / cstrUuid;
				myfile.open(candidatePath / cstrUuid, ios::out | ios::binary);
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
			gMeanDistance = (1 - (tm / gMean));
			if (gMeanDistance >= sensitivity)
			{
				std::cout << "[!] " << paths[i] << " seems to be excluded from real-time scanning!" << endl;
			}
			else {
				std::cout << "[-] " << paths[i] << " seems like a false positive" << endl;
			}
			sampleSet.clear();
			localResultSet.clear();
		}
	}
	return 0;
}
