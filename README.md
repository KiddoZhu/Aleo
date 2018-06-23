# Aleo: a toolkit for Mahjong AI

Aleo is a flexible Mahjong AI development toolkit written in C++. It was developed to facilate the design of high-level Mahjong algorithms. With Aleo, you can easily implement AI for Mahjong under GuoBiao(aka Chinese Official) rules, and deploy it to [Botzone](https://www.botzone.org.cn/).

## Features
- Easy composition and decomposition of functions
- **Extremely fast** search of winning hands
- Simulator for self play
- Clear Python interface for machine learning

## Benchmark

Aleo won the first place in [the competition of 2018 Game AI course](https://eduv4.botzone.org.cn/group/5a5dc7d5fe46681ed454794e#5b224a440edf94798cdbf04e), with a remarkable margin over the second competitor. It has an average winning rate of 46.8%, as well as excellent winning scores.

Aleo can search over all possible combinations of 5 waiting tiles in 0.5s on a single i7-4720HQ CPU.

## Usage

### Windows
For Windows, open `Aleo.sln` in Visual Studio. There are three main projects in Aleo:
- Bot 
- Data generator
- Faan calculator

It is recommended to compile each project in **Release** mode to achieve full speed.

### Linux
Aleo has only tested with data generator and faan calculator on Linux.

For data generator, run `make generator`. For faan calculator, run `make fan_calculator`.

Note that `make clean` is required between two different `make` commands.

## Requirements
- Visual Studio 2013 or later
- G++ 4.8.4 or later
- Python 2/3 with Tensorflow and Keras

## Authors
Aleo is authored by [Zhaocheng Zhu](https://https://github.com/KiddoZhu) and [Fangyin Wei](https://github.com/weify627).

## License
Licensed under an [Apache-2.0](https://github.com/KiddoZhu/Aleo/blob/master/LICENSE) license.