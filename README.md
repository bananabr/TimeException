# What this is?
A proof-of-concept tool built to identify folder that are exempted from AV real-time scanning in Windows systems.

# How it works?
The tool receives a folder path as argument and iterates through its child directories which are Y deep. Y is a maximum depth argument provided by the user. In each directory the tool creates N 512 bytes long files with random content. N is the sample size provided by the user. A larger N provides more precision and reduces the incidence of false positives. The larger the N, the larger the number of files created, therefore, the longer the tool will take to run.  During my tests, 5000 seemed to provide a good time x accuracy relationship.  The tool will then record the average ammount of time it took to create files in each of the directories.  At last, the tool checks to see if any of the directories has a mean time-to-write time that is at least X% faster than the global average. We call X the sensitivy parameter. Based on my tests, 0.25 (25%) seems to work for most cases.

# Why it works?
AV solutions will check most of the content written to disk and will hook the calls related to file writing.  This extra step will produce a measurable time diference when writing to a folder that is excluded from real-time scanning.

# How to use it?
Compile the project using Visual Studio 2019+. Select a target folder and simply run the tool passing its path as the first argument. Optionally, you can also provide a custom sample_size (default: 5000), depth (default: 0), and sensitivity (default: 0.25).

```
Usage:
TimeException.exe root_folder [sample_size] [depth] [sensitivity]
```