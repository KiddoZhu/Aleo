@echo off
rem pay attention to inclusion order
set SOURCE_FILES=lib\tile.h; lib\standard_tiles.h; lib\shanten.h; lib\fan_calculator.h; lib\shanten.cpp; lib\fan_calculator.cpp
set SOURCE_FILES=%SOURCE_FILES%; core.h; utils.h; faan.h; parser.h; bot.h; core.cpp; faan.cpp; parser.cpp; bot.cpp; main.cpp
set TARGET_FILE=botzone.cpp

del %TARGET_FILE% 2> nul

for %%f in (%SOURCE_FILES%) do (
	type ..\%%f >> %TARGET_FILE%
	echo. >> %TARGET_FILE%
)