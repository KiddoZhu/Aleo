import os
import pickle
import numpy as np
from easydict import EasyDict
from progressbar import ProgressBar, Bar, Timer, AdaptiveETA

from core import *
from faan import *
from simulator import Simulator

history_length = 25
widgets = [Bar(), " ", Timer(), " ", AdaptiveETA()]


def Steal2Tile(steal):
    if steal[0] == steal[1]: # Peng
        return [steal[0]] * 3
    elif steal[0] + 1 == steal[1]: # Chi
        return [steal[0] + i for i in range(-1, 2)]
    elif steal[0] > 0: # Ming Gang
        return [steal[0]] * 4
    else: # An Gang
        return []


def TileCount(tiles):
    count = np.zeros((4, 9), dtype=np.float32)
    for t in tiles:
        count[SUIT(t), NUMBER(t) - 1] += 1
    return count


def DataFromTurn(simulator):
    datas = []
    for turn in range(2, simulator.turn):
        if simulator.history[turn][0].action not in [ACT_PLAY, ACT_PENG, ACT_CHI]:
            continue
        # only consider turns when someone is going to play a card
        simulator.seek(turn)
        my = simulator.game[simulator.history[turn][0].player]
        for other in simulator.game:
            if other.me == my.me:
                continue
            data = EasyDict()
            data.my_remain = np.zeros(4 * 9, dtype=np.float32)
            data.my_remain[:NUM_SYMBOL - 1] = my.remain[1:]
            data.my_remain = data.my_remain.reshape((4, 9))
            data.history = np.zeros((NUM_PLAYER, history_length, 4, 9), dtype=np.float32)
            for i in range(NUM_PLAYER):
                player = (other.me + i) % NUM_PLAYER
                # right aligned in temporal axis
                for j, t in enumerate(my.player_history[player][:my.player_discard_count[player]]):
                    data.history[i, -my.player_discard_count[player] + j, :] = TileCount([t])
            my_hand = my.hand[:my.hand_count[my.me]]
            data.my_hand = TileCount(my_hand)
            # steals excluding An Gang
            data.my_other_steal = np.zeros((NUM_STEAL, 4, 9), dtype=np.float32)
            for i in range(my.steal_count[other.me]):
                tiles = Steal2Tile(my.steal[other.me][i])
                data.my_other_steal[i, :] = TileCount(tiles)

            other_hand = other.hand[:other.hand_count[other.me]]
            data.other_hand = TileCount(other_hand)
            need_indexes = SearchOneNeed(other)
            if len(need_indexes) > 0:
                data.other_is_one_need = np.ones(1, dtype=np.float32)
            else:
                data.other_is_one_need = np.zeros(1, dtype=np.float32)
            need_indexes = [index - 1 for index in need_indexes]
            data.other_need = np.zeros(4 * 9, dtype=np.float32)
            data.other_need[need_indexes] = 1
            data.other_need = data.other_need.reshape((4, 9))
            datas.append(data)
    return datas


def LoadDataset(file_name):
    print("loading %s" % file_name)
    dump_name = "%s.pickle" % os.path.splitext(file_name)[0]
    if os.path.exists(dump_name):
        with open(dump_name, "rb") as fin:
            dataset = pickle.load(fin)
        return dataset

    dataset = []
    size = os.stat(file_name).st_size
    bar = ProgressBar(widgets=widgets, max_value=size)
    with open(file_name, "rb") as fin:
        while len(fin.read(1)) > 0:
            fin.seek(-1, 1)
            simulator = Simulator()
            simulator.load(fin)
            try:
                dataset.extend(DataFromTurn(simulator))
            except AssertionError:
                print("Ignore one match")
            bar.update(fin.tell())

    with open(dump_name, "wb") as fout:
        pickle.dump(dataset, fout)
    return dataset


class DataAugmentator(object):
    def __init__(self, suit_shuffle=True, wind_shuffle=True, dragon_shuffle=True, number_flip=True):
        self.suit_shuffle = suit_shuffle
        self.wind_shuffle = wind_shuffle
        self.dragon_shuffle = dragon_shuffle
        self.number_flip = number_flip

    def transform(self, X, Y):
        mapping = np.arange(4 * 9).reshape((4, 9))
        if self.suit_shuffle:
            np.random.shuffle(mapping[:3])
        if self.wind_shuffle:
            np.random.shuffle(mapping[3, :4])
        if self.dragon_shuffle:
            np.random.shuffle(mapping[3, 4:7])
        if self.number_flip and np.random.randint(2):
            mapping[:3, :] = mapping[:3, ::-1]

        X_new = []
        Y_new = []
        for x in X:
            if len(x.shape) >= 2 and x.shape[-2:] == (4, 9):
                x_old = x.reshape(list(x.shape[:-2]) + [-1])
                x_new = np.empty_like(x_old)
                for old, new in enumerate(mapping.flat):
                    x_new[..., new] = x_old[..., old]
                X_new.append(x_new.reshape(x.shape))
            else:
                X_new.append(x)
        for y in Y:
            if len(y.shape) >= 2 and y.shape[-2:] == (4, 9):
                y_old = y.reshape(list(y.shape[:-2]) + [-1])
                y_new = np.empty_like(y_old)
                for old, new in enumerate(mapping.flat):
                    y_new[..., new] = y_old[..., old]
                Y_new.append(y_new.reshape(y.shape))
            else:
                Y_new.append(y)
        return X_new, Y_new


class DataIterator(object):
    def __init__(self, dataset, x_names, y_names, batch_size=32, augmentator=None):
        self.dataset = dataset
        self.x_names = x_names
        self.y_names = y_names
        self.batch_size = batch_size
        self.steps_per_epoch = len(dataset) // batch_size
        if len(dataset) % batch_size != 0:
            self.steps_per_epoch += 1
        self.augmentator = augmentator
        self.index = 0

    def __iter__(self):
        return self

    def __next__(self):
        Xs = []
        Ys = []
        for data in self.dataset[self.index * self.batch_size: (self.index + 1) * self.batch_size]:
            X = [getattr(data, name) for name in self.x_names]
            Y = [getattr(data, name) for name in self.y_names]
            if self.augmentator is not None:
                X, Y = self.augmentator.transform(X, Y)
            Xs.append(X)
            Ys.append([y.flatten() for y in Y])
        Xs = [np.asarray(X) for X in zip(*Xs)]
        Ys = [np.asarray(Y) for Y in zip(*Ys)]
        self.index = (self.index + 1) % self.steps_per_epoch
        return (Xs, Ys)
