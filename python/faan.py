import platform
from ctypes import *
if platform.system() == "Windows":
    lib = WinDLL("../x64/Release/FanCalculator.dll")
else:
    lib = CDLL("../lib/fan_calculator.so")


from core import *


SUIT = lambda t: (t-1) // 9
NUMBER = lambda t: (t-1) % 9 + 1

pack_t = c_uint16
intptr_t = c_int64
tile_t = c_uint8
win_flag_t = c_uint8
wind_t = c_uint

PACK_TYPE_NONE = 0
PACK_TYPE_CHOW = 1
PACK_TYPE_PUNG = 2
PACK_TYPE_KONG = 3
PACK_TYPE_PAIR = 4

WIN_FLAG_SELF_DRAWN = 1
WIN_FLAG_4TH_TILE = 2
WIN_FLAG_ABOUT_KONG = 4
WIN_FLAG_WALL_LAST = 8

_TILE2TILE_T = [
	0x0,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
]

TILE_T = lambda t: _TILE2TILE_T[t]

def make_pack(offer, type, tile):
    return (offer << 12) | (type << 8) | tile

class hand_tiles_t(Structure):
    _fields_ = [
        ("fixed_packs", pack_t * 5),
        ("pack_count", intptr_t),
        ("standing_tiles", tile_t * 13),
        ("tile_count", intptr_t)
    ]

class calculate_param_t(Structure):
    _fields_ = [
        ("hand_tiles", hand_tiles_t),
        ("win_tile", tile_t),
        ("flower_count", c_uint8),
        ("win_flag", win_flag_t),
        ("prevalent_wind", wind_t),
        ("seat_wind", wind_t)
    ]


def GetScore(game, need):
    hand_count = game.hand_count[game.me]
    param = calculate_param_t()

    param.flower_count = game.flower_count[game.me]
    param.prevalent_wind = game.dealer
    param.seat_wind = game.me

    param.hand_tiles.pack_count = game.steal_count[game.me]
    for i in range(game.steal_count[game.me]):
        # TODO: the offering player of a steal
        steal = game.steal[game.me][i]
        if steal[0] == steal[1]: # Peng
            param.hand_tiles.fixed_packs[i] = make_pack(1, PACK_TYPE_PUNG, TILE_T(steal[0]))
        elif steal[0] + 1 == steal[1]: # Chi
            param.hand_tiles.fixed_packs[i] = make_pack(1, PACK_TYPE_CHOW, TILE_T(steal[0]))
        elif steal[1] == 0: # Gang, assume Ming Gang
            param.hand_tiles.fixed_packs[i] = make_pack(1, PACK_TYPE_KONG, TILE_T(steal[0]))

    param.hand_tiles.tile_count = hand_count
    param.win_flag = 0
    # only consider the flag of 4th tile
    if game.remain[need] == 1:
        param.win_flag |= WIN_FLAG_4TH_TILE
    for i, t in enumerate(game.hand[:hand_count]):
        param.hand_tiles.standing_tiles[i] = TILE_T(t)
    param.win_tile = TILE_T(need)

    score = lib.calculate_fan(byref(param), None)
    assert score == -3 or score > 0
    return score


def SearchOneNeed(game):
    assert game.hand_count[game.me] % 3 == 1
    needs = []
    for t in range(1, NUM_SYMBOL):
        if game.remain[t] > 0:
            if GetScore(game, t) >= 8:
                needs.append(t)
    return needs