# What is this?
A proof-of-concept tool built to identify folders that are exempted from AV real-time scanning in Windows systems.

# How does it work?
The tool receives a folder path as an argument and iterates through its child directories which are Y deep.  Y is a maximum depth argument provided by the user.  The tool creates N 512 bytes long files with random content in each directory.  N is the sample size provided by the user.  A larger N offers more precision and reduces the incidence of false positives.  The larger the N, the larger the number of files created.  Therefore, the longer the tool will take to run.  Five thousand seemed to provide a good time x accuracy relationship during my tests.  The tool will then record the average amount of time it took to create files in each of the directories.  At last, the tool checks to see if any of the directories has a mean time-to-write time that is at least X% faster than the global average.  X is called the sensitivity parameter.  Based on my tests, 0.25 (25%) seems to work for most cases.

# Why does it work?
AV solutions will check most of the content written to disk and hook the calls related to file writing.  This extra step will produce a measurable time difference when writing to a folder that is excluded from real-time scanning.
The following graph depicts the average number of microseconds taken to write a file in several folders.  The two folders excluded from real-time scanning are highlighted in the picture.

![path x elapsed time graph](https://github.com/bananabr/TimeException/blob/main/graph.png)

# How to use it?
Compile the project using Visual Studio 2019+.  Select a target folder and run the tool, passing its path as the first argument.  Optionally, you can also provide a custom sample_size (default: 500), depth (default: 0), and sensitivity (default: 0.25).

```
Usage:
TimeException.exe [options]

Options:
-h|--help     Print usage instructions
--mode        Exceptions to look folder. 0=Folder, 1=Extensions, 2=Process
--sample-size Sample size to use. Default=500
--sensitivity Sensitivity value. Default=0.25
--verbose     Increase verbosity
--targets     File containing folder paths, extensions, or process names depending on the mode selected
```
# Notes

I used Microsoft Defender for most of my testing.  Results may vary with different AV/EDR engines.
