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

#include <boost/process/pipe.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>

#include "trimmean.h"

using namespace std;

#define MIN_SAMPLE_SIZE 500
#define DEFAULT_SENSITIVITY 0.25

#define DIR_MODE 0
#define EXT_MODE 1
#define PROC_MODE 2
#define FILE_MODE 3
#define BENCH_MODE 4

std::string getBenchmarkResult(const std::string& cmd)
{
	boost::process::ipstream is; //reading pipe-stream
	boost::process::child c(cmd, boost::process::std_out > is);

	std::vector<std::string> data;
	std::string line;

	while (is && std::getline(is, line) && !line.empty())
		data.push_back(line);

	c.wait();

	if (data.size() == 0)
	{
		return string("10000000.00");
	}
	return data[0];
}

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

string getUuid() {
	UUID uuid;
	if (UuidCreate(&uuid) != RPC_S_OK) {
		throw std::runtime_error("Failed to generate UUID");
	}
	RPC_CSTR szUuid = NULL;
	string cstrUuid;
	if (UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
	{
		cstrUuid = (char*)szUuid;
		RpcStringFreeA(&szUuid);
	}
	else {
		throw std::runtime_error("Failed to generate file name");
	}
	return cstrUuid;
}

void printUsage() {
	std::cout << "Usage:" << endl;
	std::cout << "-h|--help     Print usage instructions" << endl;
	std::cout << "--mode        Exceptions to look folder. 0=Folder, 1=Extensions, 2=Process, 3=file, 4=benchmark (Default=4)" << endl;
	std::cout << "--ref         Reference time value. Default=global average" << endl;
	std::cout << "--sample-size Sample size to use. Default=500" << endl;
	std::cout << "--sensitivity Sensitivity value. Default=0.25" << endl;
	std::cout << "--benchmarker Fully qualified path to a benchmarker executable" << endl;
	std::cout << "--verbose     Increase verbosity" << endl;
	std::cout << "--targets     File containing folder paths, extensions, or process names depending on the mode selected" << endl;
}

long long int timeFileWrite(const std::filesystem::path& filePath, vector<char>& content) {
	ofstream _file;
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);

	_file.open(filePath, ios::out | ios::binary);

	QueryPerformanceCounter(&StartingTime);
	_file << content.data();
	QueryPerformanceCounter(&EndingTime);

	_file.close();

	filesystem::remove(filePath);

	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	return ElapsedMicroseconds.QuadPart;
}

int main(int argc, char** argv)
{
	auto args = argvToMap(argc, argv);
	if (args.count("-h") || args.count("--help")) {
		printUsage();
		return 0;
	}

	vector<vector<char>> sampleSet = {};
	vector<long long int> localResultSet = {};
	vector<double> globalResultSet = {};
	vector<string> paths = {};
	//std::filesystem::path basepath = argv[1];
	size_t sample_size = MIN_SAMPLE_SIZE;
	double sensitivity = DEFAULT_SENSITIVITY;
	int opMode = BENCH_MODE;
	ifstream targetsFile;
	double ref = 0;
	bool verbose = false;
	string benchmarker = argv[0];

	if (args.count("--verbose")) {
		verbose = true;
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
		case FILE_MODE:
			cerr << "[-] File mode is not implemented yet!" << endl;
			return 1;
			break;
		case BENCH_MODE:
			opMode = BENCH_MODE;
			break;
		default:
			printUsage();
			return 1;
		}
	}

	if (args.count("--ref")) {
		ref = stod(args["--ref"]);
	}

	if (opMode != BENCH_MODE) {
		if (args.count("--targets")) {
			targetsFile.open(args["--targets"], ios_base::in);
			if (!targetsFile.is_open())
			{
				cerr << "[-] Failed to open targets file for reading" << endl;
				return 1;
			}
		}
		else {
			cerr << "[-] The targets file is required" << endl;
			return 1;
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
	}

	if (opMode == PROC_MODE)
	{
		if (!args.count("--benchmarker")) {
			cerr << "[-] A benchmarker is required in process mode" << endl;
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

	if (verbose)
	{
		std::cout << "[+] sample_size:" << sample_size << endl;
		std::cout << "[+] sensitivity:" << sensitivity << endl;
	}

	if (sample_size < MIN_SAMPLE_SIZE)
	{
		std::cerr << "[-] sample_size is too short and might results in a large number of false positives" << endl;
	}

	if (opMode == BENCH_MODE)
	{
		try {
			for (size_t i = 0; i < sample_size; i++)
			{
				sampleSet.push_back(getRandomBytes(512));
			}

			for (size_t i = 0; i < sample_size; i++)
			{
				string uuid = getUuid();
				std::filesystem::path filePath = uuid + ".tex";
				long long int sample = timeFileWrite(filePath, sampleSet[i]);
				localResultSet.push_back(sample);
			}
			double tm = TRIMMEAN(localResultSet.data(), localResultSet.size(), 0.2);
			std::cout << tm << endl;
			return 0;
		}
		catch (...) {
			return 1;
		}
	}

	std::cout << "[.] Processing targets ..." << endl;
	string line;
	while (std::getline(targetsFile, line))
	{
		switch (opMode) {
		case DIR_MODE:
			try {
				std::filesystem::path folderPath(line);
				if (std::filesystem::is_directory(folderPath)) {

					for (size_t i = 0; i < sample_size; i++)
					{
						sampleSet.push_back(getRandomBytes(512));
					}

					for (size_t i = 0; i < sample_size; i++)
					{
						string uuid = getUuid();
						std::filesystem::path filePath = folderPath / (uuid + ".tex");
						long long int sample = timeFileWrite(filePath, sampleSet[i]);
						localResultSet.push_back(sample);
					}
					double tm = TRIMMEAN(localResultSet.data(), localResultSet.size(), 0.2);
					globalResultSet.push_back(tm);
					paths.push_back(folderPath.string());
					if (verbose)
					{
						std::cout << "[.] " << folderPath << "," << tm << endl;
					}
					sampleSet.clear();
					localResultSet.clear();
				}
			}
			catch (...) {
				std::cerr << "[-] Failed to process " << line << endl;
			}
			break;
		case EXT_MODE:
			try {
				for (size_t i = 0; i < sample_size; i++)
				{
					sampleSet.push_back(getRandomBytes(512));
				}

				for (size_t i = 0; i < sample_size; i++)
				{
					string uuid = getUuid();
					std::filesystem::path filePath = uuid + line;
					long long int sample = timeFileWrite(filePath, sampleSet[i]);
					localResultSet.push_back(sample);
				}
				double tm = TRIMMEAN(localResultSet.data(), localResultSet.size(), 0.2);
				globalResultSet.push_back(tm);
				paths.push_back(line);
				if (verbose)
				{
					std::cout << "[.] " << line << "," << tm << endl;
				}
				sampleSet.clear();
				localResultSet.clear();
			}
			catch (...) {
				std::cerr << "[-] Failed to process " << line << endl;
			}
			break;
		case PROC_MODE:
			try {
				std::filesystem::path procPath = std::filesystem::temp_directory_path() / line;
				filesystem::copy(benchmarker, procPath);
				double tm = stod(getBenchmarkResult(procPath.string() + string(" --sample-size ") + to_string(sample_size)));
				filesystem::remove(procPath);
				globalResultSet.push_back(tm);
				paths.push_back(line);
				if (verbose)
				{
					std::cout << "[.] " << line << "," << tm << endl;
				}
				sampleSet.clear();
				localResultSet.clear();
			}
			catch (...) {
				std::cerr << "[-] Failed to process " << line << endl;
			}
			break;
		}
	}

	// Check results
	std::cout << "[.] Processing results ..." << endl;
	for (size_t i = 0; i < paths.size(); i++)
	{
		double tm = globalResultSet[i];
		double gMean;
		if (ref > 0)
		{
			gMean = ref;

		}
		else {
			gMean = getAverage(globalResultSet);
		}
		double gMeanDistance = (1 - (tm / gMean));
		if (gMeanDistance >= sensitivity)
		{
			if (opMode == PROC_MODE)
			{
				std::cout << "[+] Writing as " << paths[i] << " is " << gMeanDistance * 100 << "% faster than average!" << endl;
			}
			else {
				std::cout << "[+] Writing to " << paths[i] << " is " << gMeanDistance * 100 << "% faster than average!" << endl;
			}
			std::cout << "[.] Checking for false positive ..." << endl;
			std::filesystem::path candidatePath(paths[i]);
			for (size_t i = 0; i < 5000; i++)
			{
				sampleSet.push_back(getRandomBytes(512));
			}
			double tm;
			if (opMode == PROC_MODE) {
				std::filesystem::path procPath = std::filesystem::temp_directory_path() / paths[i];
				filesystem::copy(benchmarker, procPath);
				tm = stod(getBenchmarkResult(procPath.string() + string(" --sample-size 5000")));
				filesystem::remove(procPath);
			}
			else {
				for (size_t i = 0; i < 5000; i++)
				{
					string uuid = getUuid();
					std::filesystem::path filePath = candidatePath / (uuid + ".tex");
					long long int sample = timeFileWrite(filePath, sampleSet[i]);
					localResultSet.push_back(sample);
				}
				tm = TRIMMEAN(localResultSet.data(), localResultSet.size(), 0.2);
			}
			gMeanDistance = (1 - (tm / gMean));
			if (gMeanDistance >= sensitivity)
			{
				std::cout << "[!] " << paths[i] << " seems to be excluded from real-time scanning!" << endl;
			}
			else {
				std::cerr << "[-] " << paths[i] << " seems like a false positive" << endl;
			}
			sampleSet.clear();
			localResultSet.clear();
		}
	}
	return 0;
}
