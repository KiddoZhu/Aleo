from ctypes import *

NUM_PLAYER = 4
NUM_HAND = 14
NUM_STEAL = 4
NUM_FLOWER = 8
NUM_TILE = 144
# 34 symbols in total excluding flowers, while 0 is invalid
NUM_SYMBOL = 35

Action = c_uint
ACT_QUAN = 0
ACT_RECEIVE = 1
ACT_DRAW = 2
ACT_BU_HUA = 3
ACT_OTHER_DRAW = 4
ACT_PLAY = 5
ACT_PENG = 6
ACT_CHI = 7
ACT_GANG = 8
ACT_BU_GANG = 9
ACT_PASS = 10
ACT_HU = 11
_ACTION2NAME = ["Quan", "Receive", "Draw", "Bu Hua", "Other Draw",
                "Play", "Peng", "Chi", "Gang", "Bu Gang", "Pass", "Hu"]
_TILE2NAME = [
	"Invalid",
	"W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", "W9",
	"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9",
	"T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8", "T9",
	"F1", "F2", "F3", "F4", "J1", "J2", "J3",
	"H1", "H2", "H3", "H4", "H5", "H6", "H7", "H8"
]
ACTION_REPR = lambda name: _ACTION2NAME[name]
TILE_REPR = lambda name: _TILE2NAME[name]

tile = c_ubyte

class Message(Structure):
    _fields_ = [
        ("action", Action),
        ("player", c_int),
        ("dealer", c_int),
        ("flower_count", c_int * NUM_PLAYER),
        ("card", tile * NUM_HAND)
    ]

    def print(self, level = 0):
        indent = " " * level * 4
        print("%saction = %s" % (indent, ACTION_REPR(self.action)))
        print("%splayer = %s" % (indent, self.player))
        if (self.action == ACT_RECEIVE):
            print("%sflower_count = " % indent, end="")
            print(", ".join([str(count) for count in self.flower_count]))
            print("%scard = " % indent, end="")
            print(", ".join([TILE_REPR(card) for card in self.card[:13]]))
        elif (self.action in [ACT_DRAW, ACT_PLAY, ACT_GANG, ACT_BU_GANG]):
            print("%scard = %s" % (indent, TILE_REPR(self.card[0])))
        elif (self.action in [ACT_PENG, ACT_CHI]):
            print("%scard = %s, %s" % (indent, TILE_REPR(self.card[0]), TILE_REPR(self.card[1])))


class Game(Structure):
    _fields_ = [
        ("hand", tile * NUM_HAND),
        ("steal", tile * 2 * NUM_STEAL * NUM_PLAYER),
        ("me", c_int),
        ("dealer", c_int),
        ("hand_count", c_int * NUM_PLAYER),
        ("steal_count", c_int * NUM_PLAYER),
        ("flower_count", c_int * NUM_PLAYER),
        ("wall_count", c_int),
        ("discard_count", c_int),
        ("history", tile * NUM_TILE),
        ("remain", c_int * NUM_SYMBOL),
        ("last_turn", Message),
        ("last_card", tile),
        # extra fields
        ("player_history", tile * (NUM_TILE // NUM_PLAYER) * NUM_PLAYER),
        ("player_discard_count", c_int * NUM_PLAYER)
    ]

    def __init__(self):
        self.reset()

    def reset(self):
        self.wall_count = NUM_TILE
        self.discard_count = 0
        memset(self.hand_count, 0, sizeof(self.hand_count))
        memset(self.steal_count, 0, sizeof(self.steal_count))
        memset(self.flower_count, 0, sizeof(self.flower_count))
        for t in range(1, NUM_SYMBOL):
            self.remain[t] = 4
        memset(self.player_discard_count, 0, sizeof(self.player_discard_count))

    def play(self, msg):
        if msg.action == ACT_QUAN:
            self.me = msg.player
            self.dealer = msg.dealer
        elif msg.action == ACT_RECEIVE:
            self.flower_count = msg.flower_count
            self.hand = msg.card
            self.hand[:13] = sorted(self.hand[:13])
            for card in self.hand[:13]:
                self.remain[card] -= 1
            for j in range(NUM_PLAYER):
                self.wall_count -= self.flower_count[j]
                self.hand_count[j] = 13
            self.wall_count -= 13 * NUM_PLAYER
        elif msg.action == ACT_DRAW:
            self.add_to_hand(msg.card[0])
            self.remain[msg.card[0]] -= 1
            self.wall_count -= 1
        elif msg.action == ACT_BU_HUA:
            self.flower_count[msg.player] += 1
            self.wall_count -= 1
        elif msg.action == ACT_OTHER_DRAW:
            self.hand_count[msg.player] += 1
            self.wall_count -= 1
        elif msg.action == ACT_PLAY:
            if msg.player == self.me:
                self.remove_from_hand(msg.card[0])
            else:
                self.hand_count[msg.player] -= 1
                self.remain[msg.card[0]] -= 1
            self.history[self.discard_count] = msg.card[0]
            self.discard_count += 1
            self.player_history[msg.player][self.player_discard_count[msg.player]] = msg.card[0]
            self.player_discard_count[msg.player] += 1
        elif msg.action == ACT_PENG:
            if msg.player == self.me:
                self.remove_from_hand(msg.card[0])
                self.remove_from_hand(msg.card[0])
                self.remove_from_hand(msg.card[1])
            else:
                self.hand_count[msg.player] -= 3
                self.remain[msg.card[0]] -= 2
                self.remain[msg.card[1]] -= 1
            self.steal[msg.player][self.steal_count[msg.player]][0] = msg.card[0]
            self.steal[msg.player][self.steal_count[msg.player]][1] = msg.card[0]
            self.steal_count[msg.player] += 1
            self.history[self.discard_count - 1] = msg.card[1]
            self.player_history[msg.player][self.player_discard_count[msg.player]] = msg.card[1]
            self.player_discard_count[msg.player] += 1
        elif msg.action == ACT_CHI:
            target = self.history[self.discard_count - 1]
            if msg.player == self.me:
                for j in range(-1, 2):
                    if msg.card[0] + j != target:
                        self.remove_from_hand(msg.card[0] + j)
                self.remove_from_hand(msg.card[1])
            else:
                self.hand_count[msg.player] -= 3
                for j in range(-1, 2):
                    if msg.card[0] + j != target:
                        self.remain[msg.card[0] + j] -= 1
                self.remain[msg.card[1]] -= 1
            self.steal[msg.player][self.steal_count[msg.player]][0] = msg.card[0]
            self.steal[msg.player][self.steal_count[msg.player]][1] = msg.card[0] + 1
            self.steal_count[msg.player] += 1
            self.history[self.discard_count - 1] = msg.card[1]
            self.player_history[msg.player][self.player_discard_count[msg.player]] = msg.card[1]
            self.player_discard_count[msg.player] += 1
        elif msg.action == ACT_GANG:
            if msg.player == self.me: # my
                self.remove_from_hand(msg.card[0])
                self.remove_from_hand(msg.card[0])
                self.remove_from_hand(msg.card[0])
                if self.hand_count[self.me] % 3 == 2: # An Gang
                    self.remove_from_hand(msg.card[0])
                else: # Ming Gang
                    self.discard_count -= 1
            else: # other's
                self.hand_count[msg.player] -= 3
                if msg.card[0] == 0: # An Gang
                    self.hand_count[msg.player] -= 1
                else: # Ming Gang
                    self.remain[msg.card[0]] -= 3
                    self.discard_count -= 1
            self.steal[msg.player][self.steal_count[msg.player]][0] = msg.card[0]
            self.steal[msg.player][self.steal_count[msg.player]][1] = 0
            self.steal_count[msg.player] += 1
        elif msg.action == ACT_BU_GANG:
            if msg.player == self.me:
                self.remove_from_hand(msg.card[0])
            else:
                self.hand_count[msg.player] -= 1
                self.remain[msg.card[0]] -= 1
            for steal in self.steal[msg.player]:
                if steal[0] == msg.card[0]:
                    steal[1] = 0
                    break

        self.last_turn = msg
        if msg.action == ACT_PENG or msg.action == ACT_CHI:
            self.last_card = msg.card[1]
        else:
            self.last_card = msg.card[0]

    def check(self):
        num_in_hand = 0
        num_visible = self.hand_count[self.me]
        num_invisible = 0
        for i in range(self.hand_count[self.me] - 1):
            assert self.hand[i] <= self.hand[i + 1]
        for i in range(NUM_PLAYER):
            assert self.hand_count[i] % 3 != 0
            assert self.hand_count[i] // 3 + self.steal_count[i] == 4
            num_in_hand += self.hand_count[i]
            for j in range(self.steal_count[i]):
                if self.steal[i][j][1] != 0: # Peng, Chi
                    num_in_hand += 3
                    num_visible += 3
                else: # Gang
                    num_in_hand += 4;
                    if self.steal[i][j][0] != 0: # Ming Gang
                        num_visible += 4;
            num_in_hand += self.flower_count[i];
        for i in range(1, NUM_SYMBOL):
            assert 0 <= self.remain[i] <= 4;
            num_invisible += self.remain[i]
        assert num_in_hand + self.discard_count + self.wall_count == NUM_TILE
        assert num_visible + num_invisible + self.discard_count == NUM_TILE - NUM_FLOWER

    def print_hand(self, level = 0):
        indent = " " * level * 4
        hand_count = self.hand_count[self.me]
        print("%shand = " % indent, end = "")
        print(", ".join([TILE_REPR(t) for t in self.hand[:hand_count]]))

    def add_to_hand(self, t):
        hand_count = self.hand_count[self.me]
        for i in range(hand_count + 1):
            if self.hand[i] > t or i == hand_count:
                for j in range(hand_count, i, -1):
                    self.hand[j] = self.hand[j - 1]
                self.hand[i] = t
                break
        self.hand_count[self.me] += 1

    def remove_from_hand(self, t):
        hand_count = self.hand_count[self.me]
        for i in range(hand_count):
            if self.hand[i] == t:
                for j in range(i, hand_count - 1):
                    self.hand[j] = self.hand[j + 1]
                self.hand[hand_count - 1] = 0
                break
        self.hand_count[self.me] -= 1