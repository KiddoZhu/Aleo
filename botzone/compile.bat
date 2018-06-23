@echo off
rem change CXX & PATH according your path of MinGW64 or other compilers
set CXX=D:\Dev-C++\MinGW64\bin\g++.exe
set PATH=%PATH%;D:\Dev-C++\MinGW64\bin
set TARGET_FILE=botzone.cpp

%CXX% %TARGET_FILE% -o botzone -O2 -DNDEBUG=1 -D_BOTZONE_ONLINE -std=c++11