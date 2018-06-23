# restrict GPU memory consumption
import types
from functools import partial
import tensorflow as tf
from keras.backend.tensorflow_backend import set_session

config = tf.ConfigProto()
config.gpu_options.allow_growth = True
set_session(tf.Session(config=config))

from keras import backend as K
from keras.layers import Input, Reshape, Permute, Concatenate, \
    Conv2D, Flatten, Dense, Add, \
    BatchNormalization, Activation
from keras.metrics import top_k_categorical_accuracy
from keras.regularizers import l2
from keras.optimizers import Adam, SGD
from keras.models import Model

from core import *
from data import history_length

model_funcs = {}

def register(func):
    global model_funcs
    if func.__doc__ in model_funcs:
        raise ValueError("Signature `%s` is registered twice" % func.__do__)
    model_funcs[func.__doc__] = func
    return func


def smoothL1(y_true, y_pred, thresh=0.5):
    l1 = K.abs(y_true - y_pred)
    l2 = K.square(l1)
    return K.mean(K.switch(K.less(l1, thresh), 0.5 * l2, thresh * (l1 - 0.5 * thresh)))


def multilabel_crossentropy(y_true, y_pred, positive_weight=(NUM_SYMBOL - 3) / 2):
    # one tile: positive_weight = NUM_SYMBOL - 2
    # two tiles: positive_weight = (NUM_SYMBOL - 3) / 2
    # three tiles: positive_weight = (NUM_SYMBOL - 4) / 3
    # assume average case is two tile
    loss = K.binary_crossentropy(y_true, y_pred)
    weight = K.switch(K.equal(y_true, 1), positive_weight * K.ones_like(loss), K.ones_like(loss))
    # currently we use sum weight as norm, so that the scale of loss is stable
    # more reasonable solution?
    return K.sum(weight * loss, axis=-1) / K.sum(weight, axis=-1)


def multilabel_accuracy(y_true, y_pred, positive_weight=(NUM_SYMBOL - 3) / 2):
    accuracy = K.cast(K.equal(y_true, K.round(y_pred)), K.floatx())
    weight = K.switch(K.equal(y_true, 1), positive_weight * K.ones_like(accuracy), K.ones_like(accuracy))
    return K.sum(weight * accuracy, axis=-1) / K.sum(weight, axis=-1)


def multilabel_top_k_accuracy(y_true, y_pred, k=20):
    return top_k_categorical_accuracy(y_true, y_pred, k)


def BuildModel(x_names, y_names):
    doc = "%s -> %s" % (", ".join(x_names), ", ".join(y_names))
    return model_funcs[doc]()


@register
def Model0():
    """my_remain -> other_hand"""
    input = x = Input(shape=(4, 9))
    x = Reshape((4, 9, -1))(x)
    x = Conv2D(32, (1, 3), padding="same", activation="relu")(x)
    x = Conv2D(32, (1, 3), padding="same", dilation_rate=(1, 3), activation="relu")(x)
    x = Conv2D(1, (1, 1), activation=None)(x)
    x = Reshape((4, 9))(x)

    model = Model([input], [x])
    model.compile(optimizer=Adam(1e-4), loss=smoothL1)
    return model



def Model1():
    """my_remain -> other_need"""
    input = x = Input(shape=(4, 9))
    x = Reshape((4, 9, -1))(x)
    x = Conv2D(32, (1, 3), padding="same", activation="relu")(x)
    x = Conv2D(32, (1, 3), padding="same", activation="relu")(x)
    x = Conv2D(16, (1, 3), padding="same", dilation_rate=(1, 3), activation="relu")(x)
    x = Flatten()(x)
    x = Dense(128, activation="relu")(x)
    x = Dense(4 * 9, activation="sigmoid")(x)

    model = Model([input], [x])
    model.compile(optimizer=Adam(1e-4), loss=multilabel_crossentropy, metrics=["accuracy", multilabel_accuracy])
    return model


@register
def Model2():
    """my_remain -> other_is_one_need"""
    input = x = Input(shape=(4, 9))
    x = Reshape((4, 9, -1))(x)
    x = Conv2D(32, (1, 3), padding="same", activation="relu")(x)
    x = Conv2D(32, (1, 3), padding="same", activation="relu")(x)
    x = Conv2D(16, (1, 3), padding="same", dilation_rate=(1, 3), activation="relu")(x)
    x = Flatten()(x)
    x = Dense(1, activation="sigmoid")(x)

    model = Model([input], [x])
    model.compile(optimizer=Adam(1e-4), loss="binary_crossentropy", metrics=["accuracy"])
    return model


@register
def Model3():
    """my_remain -> other_need"""
    kwargs = {
        #"kernel_initializer": "he_uniform",
        "kernel_regularizer": l2(0.01)
    }

    remain = Input(shape=(4, 9))
    x0 = Reshape((4, 9, -1))(remain)
    x = x0
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    residual = x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (3, 1), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Add()([x, residual])
    residual = x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (3, 1), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Add()([x, residual])
    residual = x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (3, 1), padding="same", **kwargs)(x)
    x = BatchNormalization()(x)
    x = Activation("relu")(x)
    x = Conv2D(64, (1, 3), padding="same", dilation_rate=(1, 3), **kwargs)(x)
    x = BatchNormalization()(x)
    x = Add()([x, residual])
    x = Activation("relu")(x)
    x = Flatten()(x)
    x = Dense(256, activation="relu", **kwargs)(x)
    x = Dense(4 * 9, activation="sigmoid", **kwargs)(x)

    model = Model([remain], [x])
    model.compile(optimizer=SGD(5e-3, momentum=0.9), loss=multilabel_crossentropy,
                  metrics=["accuracy", multilabel_accuracy, multilabel_top_k_accuracy])
    return model