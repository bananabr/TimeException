# Disclaimer

This tool is provided for educational purposes only.

# What is this?
A proof-of-concept tool built to identify folders that are exempted from AV real-time scanning in Windows systems.

# How does it work?
The tool receives a folder path as an argument and iterates through its child directories which are Y deep.  Y is a maximum depth argument provided by the user.  The tool creates N 512 bytes long files with random content in each directory.  N is the sample size provided by the user.  A larger N offers more precision and reduces the incidence of false positives.  The larger the N, the larger the number of files created.  Therefore, the longer the tool will take to run.  Five thousand seemed to provide a good time x accuracy relationship during my tests.  The tool will then record the average amount of time it took to create files in each of the directories.  At last, the tool checks to see if any of the directories has a mean time-to-write time that is at least X% faster than the global average.  X is called the sensitivity parameter.  Based on my tests, 0.25 (25%) seems to work for most cases.

# Why does it work?
AV solutions will check most of the content written to disk and hook the calls related to file writing.  This extra step can produce a measurable time difference when writing to a folder that is excluded from real-time scanning.
The following graph depicts the average number of microseconds taken to write a file in several folders during the tool's initial tests.  The two folders excluded from real-time scanning are highlighted in the picture.

![path x elapsed time graph](https://github.com/bananabr/TimeException/blob/main/graph.png)

# How to use it?

## Compilation
* Install and configure https://vcpkg.io/en/index.html
* Install the [boost-process](https://www.boost.org/doc/libs/1_64_0/doc/html/process.html) dependency using vcpkg
* Compile the project using Visual Studio 2019+.

## Execution modes
### Folder execution mode
Execution mode 0 (--mode 0) will try to map folders in Defender's exclusion list. A user must provide a list of folders to be tested through the **--targets** argument. The **--sample-size** argument can be used to adjust the number of files created in each directory.

Example:
```
TimeException.exe --sample-size 1000 ---mode 0 --targets dirs.txt
```

### Exntension execution mode
Execution mode 1 (--mode 1) will try to map extensions in Defender's exclusion list. A user must provide a list of extensions to be tested through the **--targets** argument. The extensions in the targets file must have the format **.ext** (e.g. .png, .txt, .foobar, etc).  The **--sample-size** argument can be used to adjust the number of files created for each extension.

Example:
```
TimeException.exe --sample-size 1000 ---mode 1 --targets exts.txt
```

### Process execution mode
Execution mode 2 (--mode 2) will try to map processes in Defender's exclusion list. A user must provide a list of process names to be tested through the **--targets** argument. The extensions in the targets file must have the format **program.exe** (e.g. calc.exe, mspaint.exe, etc).  Process execution mode requires a benchmarker to be provided.  A benchmarker is a console application who accepts --sample-size as a valid argument and outputs a double floating point number.  TimeException itself is a benchmarker. The **--sample-size** argument can be used to adjust the number of operations performed by the benchmarker to calculate its result.

Example:
```
TimeException.exe --sample-size 1500 --ref 3.7225 --mode 2 --benchmarker .\TimeException.exe --targets procs.txt
```

### File execution mode
Execution mode 3 (--mode 3) is not implemented yet.

### Benchmark execution mode
Execution mode 4 (--mode 4) will measure the average time it takes to write a 512 bytes file to the current directory.  The **--sample-size** argument can be used to adjust the number of files used to calculate the mean time to write.

The benchmark mode is TimeException's default mode of operation.

Example:
```
TimeException.exe --mode 4 --sample-size 5000
```

## General usage instructions

```
Usage:
TimeException.exe [options]

Options:
Usage:
-h|--help     Print usage instructions
--mode        Exceptions to look folder. 0=Folder, 1=Extensions, 2=Process, 3=file, 4=benchmark (Default=4)
--ref         Reference time value. Default=global average
--sample-size Sample size to use. Default=500
--sensitivity Sensitivity value. Default=0.25
--benchmarker Fully qualified path to a benchmarker executable
--verbose     Increase verbosity
--targets     File containing folder paths, extensions, or process names depending on the mode selected
```
# Notes

* I used Microsoft Defender for most of my testing.  Results may vary with different AV/EDR engines.
* In a real Red Team engagement I would run the tool multiple times in different hours of the day before actually trying to write any payloads to the folders detected by the tool.  Disk load varies and false positives are to be expected.
