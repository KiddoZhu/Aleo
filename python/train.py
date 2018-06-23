import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

import numpy as np
import random
from easydict import EasyDict

from data import LoadDataset, DataIterator, DataAugmentator
from model import BuildModel

cfg = EasyDict()
cfg.train_filename = "../data/MaxProbabilityBot[38]_train_1000.match"
cfg.test_filename = "../data/MaxProbabilityBot[38]_val_100.match"
cfg.x_names = ("my_remain",)
cfg.y_names = ("other_need",)

if __name__ == "__main__":
    train_dataset = LoadDataset(cfg.train_filename)
    test_dataset = LoadDataset(cfg.test_filename)

    positive = [data for data in train_dataset if np.max(data.other_need) > 0]
    negative = [data for data in train_dataset if np.max(data.other_need) == 0]
    print("[player Ting Pai] positive : negative = %d : %d" % (len(positive), len(negative)))
    # negative = random.sample(negative, len(positive))
    # train_dataset = positive + negative
    train_dataset = positive
    random.shuffle(train_dataset)

    positive = [data for data in test_dataset if np.max(data.other_need) > 0]
    negative = [data for data in test_dataset if np.max(data.other_need) == 0]
    print("[player Ting Pai] positive : negative = %d : %d" % (len(positive), len(negative)))
    # negative = random.sample(negative, len(positive))
    # test_dataset = positive + negative
    test_dataset = positive
    random.shuffle(test_dataset)

    augmentator = DataAugmentator()
    train_iter = DataIterator(train_dataset, x_names=cfg.x_names, y_names=cfg.y_names,
                              batch_size=128, augmentator=augmentator)
    test_iter = DataIterator(test_dataset, x_names=cfg.x_names, y_names=cfg.y_names,
                             batch_size=128)
    model = BuildModel(cfg.x_names, cfg.y_names)
    model.summary()
    model.fit_generator(train_iter, steps_per_epoch=train_iter.steps_per_epoch, epochs=200,
                        validation_data=test_iter, validation_steps=test_iter.steps_per_epoch)