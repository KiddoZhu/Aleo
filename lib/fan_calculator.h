/****************************************************************************
 Copyright (c) 2016-2018 Jeff Wang <summer_insects@163.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
****************************************************************************/

#ifndef __MAHJONG_ALGORITHM__FAN_CALCULATOR_H__
#define __MAHJONG_ALGORITHM__FAN_CALCULATOR_H__

#ifndef _BOTZONE_ONLINE
#include "tile.h"
#endif

#define SUPPORT_CONCEALED_KONG_AND_MELDED_KONG 1  // ֧��������

namespace mahjong {

/**
 * @addtogroup calculator
 * @{
 */

/**
 * @brief ����
 */
enum fan_t {
    FAN_NONE = 0,                       ///< ��Ч
    BIG_FOUR_WINDS,                     ///< ����ϲ
    BIG_THREE_DRAGONS,                  ///< ����Ԫ
    ALL_GREEN,                          ///< ��һɫ
    NINE_GATES,                         ///< ��������
    FOUR_KONGS,                         ///< �ĸ�
    SEVEN_SHIFTED_PAIRS,                ///< ���߶�
    THIRTEEN_ORPHANS,                   ///< ʮ����

    ALL_TERMINALS,                      ///< ���۾�
    LITTLE_FOUR_WINDS,                  ///< С��ϲ
    LITTLE_THREE_DRAGONS,               ///< С��Ԫ
    ALL_HONORS,                         ///< ��һɫ
    FOUR_CONCEALED_PUNGS,               ///< �İ���
    PURE_TERMINAL_CHOWS,                ///< һɫ˫����

    QUADRUPLE_CHOW,                     ///< һɫ��ͬ˳
    FOUR_PURE_SHIFTED_PUNGS,            ///< һɫ�Ľڸ�

    FOUR_PURE_SHIFTED_CHOWS,            ///< һɫ�Ĳ���
    THREE_KONGS,                        ///< ����
    ALL_TERMINALS_AND_HONORS,           ///< ���۾�

    SEVEN_PAIRS,                        ///< �߶�
    GREATER_HONORS_AND_KNITTED_TILES,   ///< ���ǲ���
    ALL_EVEN_PUNGS,                     ///< ȫ˫��
    FULL_FLUSH,                         ///< ��һɫ
    PURE_TRIPLE_CHOW,                   ///< һɫ��ͬ˳
    PURE_SHIFTED_PUNGS,                 ///< һɫ���ڸ�
    UPPER_TILES,                        ///< ȫ��
    MIDDLE_TILES,                       ///< ȫ��
    LOWER_TILES,                        ///< ȫС

    PURE_STRAIGHT,                      ///< ����
    THREE_SUITED_TERMINAL_CHOWS,        ///< ��ɫ˫����
    PURE_SHIFTED_CHOWS,                 ///< һɫ������
    ALL_FIVE,                           ///< ȫ����
    TRIPLE_PUNG,                        ///< ��ͬ��
    THREE_CONCEALED_PUNGS,              ///< ������

    LESSER_HONORS_AND_KNITTED_TILES,    ///< ȫ����
    KNITTED_STRAIGHT,                   ///< �����
    UPPER_FOUR,                         ///< ������
    LOWER_FOUR,                         ///< С����
    BIG_THREE_WINDS,                    ///< �����

    MIXED_STRAIGHT,                     ///< ����
    REVERSIBLE_TILES,                   ///< �Ʋ���
    MIXED_TRIPLE_CHOW,                  ///< ��ɫ��ͬ˳
    MIXED_SHIFTED_PUNGS,                ///< ��ɫ���ڸ�
    CHICKEN_HAND,                       ///< �޷���
    LAST_TILE_DRAW,                     ///< ���ֻش�
    LAST_TILE_CLAIM,                    ///< ��������
    OUT_WITH_REPLACEMENT_TILE,          ///< ���Ͽ���
    ROBBING_THE_KONG,                   ///< ���ܺ�

    ALL_PUNGS,                          ///< ������
    HALF_FLUSH,                         ///< ��һɫ
    MIXED_SHIFTED_CHOWS,                ///< ��ɫ������
    ALL_TYPES,                          ///< ������
    MELDED_HAND,                        ///< ȫ����
    TWO_CONCEALED_KONGS,                ///< ˫����
    TWO_DRAGONS_PUNGS,                  ///< ˫����

    OUTSIDE_HAND,                       ///< ȫ����
    FULLY_CONCEALED_HAND,               ///< ������
    TWO_MELDED_KONGS,                   ///< ˫����
    LAST_TILE,                          ///< ������

    DRAGON_PUNG,                        ///< ����
    PREVALENT_WIND,                     ///< Ȧ���
    SEAT_WIND,                          ///< �ŷ��
    CONCEALED_HAND,                     ///< ��ǰ��
    ALL_CHOWS,                          ///< ƽ��
    TILE_HOG,                           ///< �Ĺ�һ
    DOUBLE_PUNG,                        ///< ˫ͬ��
    TWO_CONCEALED_PUNGS,                ///< ˫����
    CONCEALED_KONG,                     ///< ����
    ALL_SIMPLES,                        ///< ����

    PURE_DOUBLE_CHOW,                   ///< һ���
    MIXED_DOUBLE_CHOW,                  ///< ϲ���
    SHORT_STRAIGHT,                     ///< ����
    TWO_TERMINAL_CHOWS,                 ///< ���ٸ�
    PUNG_OF_TERMINALS_OR_HONORS,        ///< �۾ſ�
    MELDED_KONG,                        ///< ����
    ONE_VOIDED_SUIT,                    ///< ȱһ��
    NO_HONORS,                          ///< ����
    EDGE_WAIT,                          ///< ����
    CLOSED_WAIT,                        ///< Ƕ��
    SINGLE_WAIT,                        ///< ������
    SELF_DRAWN,                         ///< ����

    FLOWER_TILES,                       ///< ����

#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    CONCEALED_KONG_AND_MELDED_KONG,     ///< ������
#endif

    FAN_TABLE_SIZE
};

/**
 * @brief �磨������ʾȦ���ŷ磩
 */
enum class wind_t {
    EAST, SOUTH, WEST, NORTH
};

/**
 * @brief ���Ʊ��
 */
typedef uint8_t win_flag_t;

/**
 * @name win flag
 * @{
 */
#define WIN_FLAG_DISCARD    0   ///< ���
#define WIN_FLAG_SELF_DRAWN 1   ///< ����
#define WIN_FLAG_4TH_TILE   2   ///< ����
#define WIN_FLAG_ABOUT_KONG 4   ///< ���ڸܣ����ϵ��ʱΪǹ�ܺͣ�����������Ϊ���Ͽ���
#define WIN_FLAG_WALL_LAST  8   ///< ��ǽ���һ�ţ����ϵ��ʱΪ�������£�����������Ϊ���ֻش�
/**
 * @}
 */

/**
 * @name error codes
 * @{
 */
#define ERROR_WRONG_TILES_COUNT -1              ///< ���������
#define ERROR_TILE_COUNT_GREATER_THAN_4 -2      ///< ĳ���Ƴ��ֳ���4ö
#define ERROR_NOT_WIN -3                        ///< û����
/**
 * @}
 */

/**
 * @brief ����㷬�������Ƿ�Ϸ�
 *
 * @param [in] hand_tiles ����
 * @param [in] win_tile ������
 * @retval 0 �ɹ�
 * @retval ERROR_WRONG_TILES_COUNT ���������
 * @retval ERROR_TILE_COUNT_GREATER_THAN_4 ĳ���Ƴ��ֳ���4ö
 */
int check_calculator_input(const hand_tiles_t *hand_tiles, tile_t win_tile);

/**
 * @brief �㷬����
 */
struct calculate_param_t {
    hand_tiles_t hand_tiles;    ///< ����
    tile_t win_tile;            ///< ������
    uint8_t flower_count;       ///< ������
    win_flag_t win_flag;        ///< ���Ʊ��
    wind_t prevalent_wind;      ///< Ȧ��
    wind_t seat_wind;           ///< �ŷ�
};

/**
 * @brief ����
 */
typedef uint16_t fan_table_t[FAN_TABLE_SIZE];

/**
 * @brief �㷬
 *
 * @param [in] calculate_param �㷬����
 * @param [out] fan_table ��������ĳ�ַ�ʱ����Ӧ�Ļ�����Ϊ���ַ����ֵĴ���
 * @retval >0 ����
 * @retval ERROR_WRONG_TILES_COUNT ���������
 * @retval ERROR_TILE_COUNT_GREATER_THAN_4 ĳ���Ƴ��ֳ���4ö
 * @retval ERROR_NOT_WIN û����
 */
#ifdef WIN32
#define MODIFIER extern "C" __declspec(dllexport)
#else
#define MODIFIER extern "C"
#endif

MODIFIER int calculate_fan(calculate_param_t *calculate_param, fan_table_t *fan_table);

#if 0

/**
 * @brief ������Ӣ�ģ�
 */
static const char *fan_name[] = {
    "None",
    "Big Four Winds", "Big Three Dragons", "All Green", "Nine Gates", "Four Kongs", "Seven Shifted Pairs", "Thirteen Orphans",
    "All Terminals", "Little Four Winds", "Little Three Dragons", "All Honors", "Four Concealed Pungs", "Pure Terminal Chows",
    "Quadruple Chow", "Four Pure Shifted Pungs",
    "Four Pure Shifted Chows", "Three Kongs", "All Terminals and Honors",
    "Seven Pairs", "Greater Honors and Knitted Tiles", "All Even Pungs", "Full Flush", "Pure Triple Chow", "Pure Shifted Pungs", "Upper Tiles", "Middle Tiles", "Lower Tiles",
    "Pure Straight", "Three-Suited Terminal Chows", "Pure Shifted Chows", "All Five", "Triple Pung", "Three Concealed Pungs",
    "Lesser Honors and Knitted Tiles", "Knitted Straight", "Upper Four", "Lower Four", "Big Three Winds",
    "Mixed Straight", "Reversible Tiles", "Mixed Triple Chow", "Mixed Shifted Pungs", "Chicken Hand", "Last Tile Draw", "Last Tile Claim", "Out with Replacement Tile", "Robbing The Kong",
    "All Pungs", "Half Flush", "Mixed Shifted Chows", "All Types", "Melded Hand", "Two Concealed Kongs", "Two Dragons Pungs",
    "Outside Hand", "Fully Concealed Hand", "Two Melded Kongs", "Last Tile",
    "Dragon Pung", "Prevalent Wind", "Seat Wind", "Concealed Hand", "All Chows", "Tile Hog", "Double Pung",
    "Two Concealed Pungs", "Concealed Kong", "All Simples",
    "Pure Double Chow", "Mixed Double Chow", "Short Straight", "Two Terminal Chows", "Pung of Terminals or Honors", "Melded Kong", "One Voided Suit", "No Honors", "Edge Wait", "Closed Wait", "Single Wait", "Self-Drawn",
    "Flower Tiles"
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    , "Concealed Kong and Melded Kong"
#endif
};

#else
/*
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#ifndef __UTF8_TEXT
// VS2015 GCC4.7 Clang5.0
#if (defined(_MSC_VER) && (_MSC_VER >= 1900)) || (defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x407)) || (defined(__clang__) && (__clang_major__ >= 5))
#define __UTF8_TEXT(quote) u8 ## quote
#else
#define __UTF8_TEXT(quote) quote
#endif
#endif
    
#ifndef __UTF8
#define __UTF8(quote) (quote) //__UTF8_TEXT(quote)
#endif
*/
/**
 * @brief �������������ģ�
 */
#define __UTF8(quote) (quote)
static const char *fan_name[] = {
    __UTF8("��"),
    __UTF8("����ϲ"), __UTF8("����Ԫ"), __UTF8("��һɫ"), __UTF8("��������"), __UTF8("�ĸ�"), __UTF8("���߶�"), __UTF8("ʮ����"),
    __UTF8("���۾�"), __UTF8("С��ϲ"), __UTF8("С��Ԫ"), __UTF8("��һɫ"), __UTF8("�İ���"), __UTF8("һɫ˫����"),
    __UTF8("һɫ��ͬ˳"), __UTF8("һɫ�Ľڸ�"),
    __UTF8("һɫ�Ĳ���"), __UTF8("����"), __UTF8("���۾�"),
    __UTF8("�߶�"), __UTF8("���ǲ���"), __UTF8("ȫ˫��"), __UTF8("��һɫ"), __UTF8("һɫ��ͬ˳"), __UTF8("һɫ���ڸ�"), __UTF8("ȫ��"), __UTF8("ȫ��"), __UTF8("ȫС"),
    __UTF8("����"), __UTF8("��ɫ˫����"), __UTF8("һɫ������"), __UTF8("ȫ����"), __UTF8("��ͬ��"), __UTF8("������"),
    __UTF8("ȫ����"), __UTF8("�����"), __UTF8("������"), __UTF8("С����"), __UTF8("�����"),
    __UTF8("����"), __UTF8("�Ʋ���"), __UTF8("��ɫ��ͬ˳"), __UTF8("��ɫ���ڸ�"), __UTF8("�޷���"), __UTF8("���ֻش�"), __UTF8("��������"), __UTF8("���Ͽ���"), __UTF8("���ܺ�"),
    __UTF8("������"), __UTF8("��һɫ"), __UTF8("��ɫ������"), __UTF8("������"), __UTF8("ȫ����"), __UTF8("˫����"), __UTF8("˫����"),
    __UTF8("ȫ����"), __UTF8("������"), __UTF8("˫����"), __UTF8("�;���"),
    __UTF8("����"), __UTF8("Ȧ���"), __UTF8("�ŷ��"), __UTF8("��ǰ��"), __UTF8("ƽ��"), __UTF8("�Ĺ�һ"), __UTF8("˫ͬ��"), __UTF8("˫����"), __UTF8("����"), __UTF8("����"),
    __UTF8("һ���"), __UTF8("ϲ���"), __UTF8("����"), __UTF8("���ٸ�"), __UTF8("�۾ſ�"), __UTF8("����"), __UTF8("ȱһ��"), __UTF8("����"), __UTF8("����"), __UTF8("Ƕ��"), __UTF8("������"), __UTF8("����"),
    __UTF8("����")
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    , __UTF8("������")
#endif
};

#endif

/**
 * @brief ��ֵ
 */
static const uint16_t fan_value_table[FAN_TABLE_SIZE] = {
    0,
    88, 88, 88, 88, 88, 88, 88,
    64, 64, 64, 64, 64, 64,
    48, 48,
    32, 32, 32,
    24, 24, 24, 24, 24, 24, 24, 24, 24,
    16, 16, 16, 16, 16, 16,
    12, 12, 12, 12, 12,
    8, 8, 8, 8, 8, 8, 8, 8, 8,
    6, 6, 6, 6, 6, 6, 6,
    4, 4, 4, 4,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
    , 5
#endif
};

/**
 * @brief �ж������Ƿ��������
 * ����ǣ����Ȼ���Ǻ;���
 *
 * @param [in] standing_tiles ����
 * @param [in] standing_cnt ������
 * @param [in] win_tile ������
 * @return bool
 */
bool is_standing_tiles_contains_win_tile(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t win_tile);

/**
 * @brief ͳ�ƺ����ڸ�¶�����г��ֵ�����
 * �������3�ţ����Ȼ�;���
 *
 * @param [in] fixed_packs ��¶����
 * @param [in] fixed_cnt ��¶������
 * @param [in] win_tile ������
 * @return size_t
 */
size_t count_win_tile_in_fixed_packs(const pack_t *fixed_packs, intptr_t fixed_cnt, tile_t win_tile);

/**
 * @brief �жϸ�¶�����Ƿ������
 *
 * @param [in] fixed_packs ��¶����
 * @param [in] fixed_cnt ��¶������
 * @return bool
 */
bool is_fixed_packs_contains_kong(const pack_t *fixed_packs, intptr_t fixed_cnt);

/**
 * end group
 * @}
 */

}

#endif
