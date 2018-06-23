from core import *
from ctypes import *
from utils import *

NUM_TURN = NUM_TILE * 2

class Simulator(Structure):

    _fields_ = [
        ("game", Game * NUM_PLAYER),
        ("history", Message * NUM_PLAYER * NUM_TURN),
        ("response", Message * NUM_PLAYER * NUM_TURN),
        ("turn", c_int)
    ]

    def seek(self, turn):
        assert turn <= self.turn
        for i, game in enumerate(self.game):
            game.reset()
            for j, history in enumerate(self.history[:turn]):
                game.play(history[i])
                if j > 0:
                    # print("player = %d, turn = %d" % (i, j))
                    game.check()


    def load(self, fin):
        assert fin.mode == "rb"
        self.turn = fread(c_int, fin)
        for i in range(self.turn):
            for j in range(NUM_PLAYER):
                self.history[i][j] = fread(Message, fin)
        for i in range(NUM_PLAYER):
            assert self.history[0][i].action == ACT_QUAN
            assert self.history[1][i].action == ACT_RECEIVE
        for i in range(self.turn):
            for j in range(NUM_PLAYER):
                self.response[i][j] = fread(Message, fin)
        for i in range(NUM_PLAYER):
            assert self.response[0][i].action == ACT_PASS
            assert self.response[1][i].action == ACT_PASS
