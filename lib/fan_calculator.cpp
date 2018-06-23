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

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>
#include <iterator>

#ifndef _BOTZONE_ONLINE
#include "fan_calculator.h"
#include "standard_tiles.h"
#include "shanten.h"
#endif

/**
 * �㷬���̸�����
 * 1. �ж��������
 *   (1) ����״̬�����߶ԡ�ʮ���ۡ��������ȫ����
 *   (2) 1��¶״̬���������
 * 2. ���������ͻ��֣������߶ԣ�
 * 3. �Ե�2���еĻ��ֽ�������㷬��ȡ���ֵ

 * �Ի��ֺ�Ľ���㷬���̣�
 * 1. ����˳�Ӹ����Ϳ��Ӹ������Կ��������㷬���漰���۾ſ̡����̣�
 * 2. ������������㷬
 *   (1) 4˳�������ж���ɫ/һɫ˫���ᣬû���ټ���4˳�ķ�
 *   (2) 3˳1�̡�������3˳�ķ�
 *   (3) 2˳2�̡�������2˳�ķ�������2�̵ķ�
 *   (4) 1˳3�̡�������3�̵ķ�
 * 3. ���������ж�
 * 4. ���ݺ��Ʒ�ʽ���������漰�������ˡ�ȫ����
 * 5. ����ȸͷ���������漰��ƽ�͡�С��Ԫ��С��ϲ
 * 6. ���������������������漰��ȫ���ۡ�ȫ���塢ȫ˫��
 * 7. ���ݻ�ɫ���������漰�����֡�ȱһ�š���һɫ����һɫ��������
 * 8. ���������Ե��������漰�����ۡ��Ʋ�������һɫ����һɫ�����۾š����۾�
 * 9. �������Ƶķ�Χ���������漰�������塢С���塢ȫ��ȫ�С�ȫС
 * 10. �����Ĺ�һ
 * 11. �������Ʒ�ʽ���������漰�����š�Ƕ�š�������
 * 12. ���ݷ���������漰��Ȧ��̡��ŷ��
 * 13. ���ͳһ���������й涨���Ƶģ��õ��㷬��������Ϊ0���������Ϊ�޷���
 */

#define MAX_DIVISION_CNT 20  // һ�������Ҳû��20�ֻ��ְɣ�������

#if 0
#define LOG(fmt_, ...) printf(fmt_, ##__VA_ARGS__)
#else
#define LOG(...) do { } while (0)
#endif

//#define STRICT_98_RULE

namespace mahjong {

#if 0  // Debug
extern intptr_t packs_to_string(const pack_t *packs, intptr_t pack_cnt, char *str, intptr_t max_size);
#endif

//-------------------------------- ���� --------------------------------

namespace {

    // ����
    struct division_t {
        pack_t packs[5];  // ���顣4����1ȸͷ����5��
    };

    // ���ֽ��
    struct division_result_t {
        division_t divisions[MAX_DIVISION_CNT];  // ÿһ�ֻ���
        intptr_t count;  // ���ַ�ʽ����
    };
}

// �ݹ黮���㷨�����һ������ӻ���
static void divide_tail_add_division(intptr_t fixed_cnt, const division_t *work_division, division_result_t *result) {
    // ����һ�ݵ�ǰ�Ļ��ֳ��������ӣ��������ֵ�����
    // ���ﲻ��ֱ����work_division->packs�����򣬷�����ƻ��ݹ���������
    division_t temp;
    memcpy(&temp, work_division, sizeof(temp));
    std::sort(temp.packs + fixed_cnt, temp.packs + 4);

    // ������ֻ����Ƿ��Ѿ�������
    if (std::none_of(&result->divisions[0], &result->divisions[result->count],
        [&temp, fixed_cnt](const division_t &od) {
        return std::equal(&od.packs[fixed_cnt], &od.packs[4], &temp.packs[fixed_cnt]);
    })) {
        // д�뻮�ֽ����
        memcpy(&result->divisions[result->count], &temp, sizeof(temp));
        ++result->count;
    }
    else {
        LOG("same case");
    }
}

// �ݹ黮�ֵ����һ��
static bool divide_tail(tile_table_t &cnt_table, intptr_t fixed_cnt, division_t *work_division, division_result_t *result) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 2) {
            continue;
        }

        cnt_table[t] -= 2;  // ����
        // ������ȫ��ʹ�����
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] += 2;  // ��ԭ

            // ��2����Ϊȸͷ
            work_division->packs[4] = make_pack(0, PACK_TYPE_PAIR, t);
            divide_tail_add_division(fixed_cnt, work_division, result);  // ��¼
            return true;
        }
        cnt_table[t] += 2;  // ��ԭ
    }

    return false;
}

// �ж�һ�����ַ�֧�Ƿ�����
static bool is_division_branch_exist(intptr_t fixed_cnt, intptr_t step, const division_t *work_division, const division_result_t *result) {
    // û�л���ʱ���Լ����ֲ���С��3ʱ������⣬��Ϊ����Ҫ��3���ݹ�Ż������ͬ����
    if (result->count <= 0 || step < 3) {
        return false;
    }

    // std::includesҪ������
    // ���ﲻ��ֱ����work_division->packs�����򣬷�����ƻ��ݹ���������
    division_t temp;
    memcpy(&temp.packs[fixed_cnt], &work_division->packs[fixed_cnt], step * sizeof(pack_t));
    std::sort(&temp.packs[fixed_cnt], &temp.packs[fixed_cnt + step]);

    // ֻ��Ҫ�Ƚ������Ƿ��ظ���֧��ȸͷ������Ƚϣ������±���4
    return std::any_of(&result->divisions[0], &result->divisions[result->count],
        [&temp, fixed_cnt, step](const division_t &od) {
        return std::includes(&od.packs[fixed_cnt], &od.packs[4], &temp.packs[fixed_cnt], &temp.packs[fixed_cnt + step]);
    });
}

// �ݹ黮��
static bool divide_recursively(tile_table_t &cnt_table, intptr_t fixed_cnt, intptr_t step, division_t *work_division, division_result_t *result) {
    const intptr_t idx = step + fixed_cnt;
    if (idx == 4) {  // 4�����Ӷ�����
        return divide_tail(cnt_table, fixed_cnt, work_division, result);
    }

    bool ret = false;

    // ���Ʊ��ű�����
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // ����
        if (cnt_table[t] > 2) {
            work_division->packs[idx] = make_pack(0, PACK_TYPE_PUNG, t);  // ��¼����
            if (!is_division_branch_exist(fixed_cnt, step + 1, work_division, result)) {
                // ����������ӣ��ݹ�
                cnt_table[t] -= 3;
                if (divide_recursively(cnt_table, fixed_cnt, step + 1, work_division, result)) {
                    ret = true;
                }
                // ��ԭ
                cnt_table[t] += 3;
            }
        }

        // ˳�ӣ�ֻ�������ƣ�
        if (is_numbered_suit(t)) {
            if (tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                work_division->packs[idx] = make_pack(0, PACK_TYPE_CHOW, static_cast<tile_t>(t + 1));  // ��¼˳��
                if (!is_division_branch_exist(fixed_cnt, step + 1, work_division, result)) {
                    // ��������˳�ӣ��ݹ�
                    --cnt_table[t];
                    --cnt_table[t + 1];
                    --cnt_table[t + 2];
                    if (divide_recursively(cnt_table, fixed_cnt, step + 1, work_division, result)) {
                        ret = true;
                    }
                    // ��ԭ
                    ++cnt_table[t];
                    ++cnt_table[t + 1];
                    ++cnt_table[t + 2];
                }
            }
        }
    }

    return ret;
}

// ����һ����
static bool divide_win_hand(const tile_t *standing_tiles, const pack_t *fixed_packs, intptr_t fixed_cnt, division_result_t *result) {
    intptr_t standing_cnt = 14 - fixed_cnt * 3;

    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    result->count = 0;

    // ���Ƹ�¶������
    division_t work_division;
    memcpy(work_division.packs, fixed_packs, fixed_cnt * sizeof(pack_t));
    return divide_recursively(cnt_table, fixed_cnt, 0, &work_division, result);
}

//-------------------------------- �㷬 --------------------------------

// 4�����1
static forceinline bool is_four_shifted_1(rank_t r0, rank_t r1, rank_t r2, rank_t r3) {
    return (r0 + 1 == r1 && r1 + 1 == r2 && r2 + 1 == r3);
}

// 4�����2
static forceinline bool is_four_shifted_2(rank_t r0, rank_t r1, rank_t r2, rank_t r3) {
    return (r0 + 2 == r1 && r1 + 2 == r2 && r2 + 2 == r3);
}

// 3�����1
static forceinline bool is_shifted_1(rank_t r0, rank_t r1, rank_t r2) {
    return (r0 + 1 == r1 && r1 + 1 == r2);
}

// 3�����2
static forceinline bool is_shifted_2(rank_t r0, rank_t r1, rank_t r2) {
    return (r0 + 2 == r1 && r1 + 2 == r2);
}

// ��ɫ
static forceinline bool is_mixed(suit_t s0, suit_t s1, suit_t s2) {
    return (s0 != s1 && s0 != s2 && s1 != s2);
}

// 3�����1����
static forceinline bool is_shifted_1_unordered(rank_t r0, rank_t r1, rank_t r2) {
    return is_shifted_1(r1, r0, r2) || is_shifted_1(r2, r0, r1) || is_shifted_1(r0, r1, r2)
        || is_shifted_1(r2, r1, r0) || is_shifted_1(r0, r2, r1) || is_shifted_1(r1, r2, r0);
}

// 4��˳�ӵķ�
static fan_t get_4_chows_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // ������Ƶ��˳��

    // һɫ�Ĳ���
    if (is_four_shifted_2(t0, t1, t2, t3) || is_four_shifted_1(t0, t1, t2, t3)) {
        return FOUR_PURE_SHIFTED_CHOWS;
    }
    // һɫ��ͬ˳
    if (t0 == t1 && t0 == t2 && t0 == t3) {
        return QUADRUPLE_CHOW;
    }
    // ���϶�û��
    return FAN_NONE;
}

// 3��˳�ӵķ�
static fan_t get_3_chows_fan(tile_t t0, tile_t t1, tile_t t2) {
    suit_t s0 = tile_get_suit(t0);
    suit_t s1 = tile_get_suit(t1);
    suit_t s2 = tile_get_suit(t2);

    rank_t r0 = tile_get_rank(t0);
    rank_t r1 = tile_get_rank(t1);
    rank_t r2 = tile_get_rank(t2);

    // ������Ƶ��˳��

    if (is_mixed(s0, s1, s2)) {  // ��ɫ
        // ��ɫ������
        if (is_shifted_1_unordered(r1, r0, r2)) {
            return MIXED_SHIFTED_CHOWS;
        }
        // ��ɫ��ͬ˳
        if (r0 == r1 && r1 == r2) {
            return MIXED_TRIPLE_CHOW;
        }
        // ����
        if ((r0 == 2 && r1 == 5 && r2 == 8) || (r0 == 2 && r1 == 8 && r2 == 5)
            || (r0 == 5 && r1 == 2 && r2 == 8) || (r0 == 5 && r1 == 8 && r2 == 2)
            || (r0 == 8 && r1 == 2 && r2 == 5) || (r0 == 8 && r1 == 5 && r2 == 2)) {
            return MIXED_STRAIGHT;
        }
    }
    else {  // һɫ
        // ����
        if (t0 + 3 == t1 && t1 + 3 == t2) {
            return PURE_STRAIGHT;
        }
        // һɫ������
        if (is_shifted_2(t0, t1, t2) || is_shifted_1(t0, t1, t2)) {
            return PURE_SHIFTED_CHOWS;
        }
        // һɫ��ͬ˳
        if (t0 == t1 && t0 == t2) {
            return PURE_TRIPLE_CHOW;
        }
    }
    // ���϶�û��
    return FAN_NONE;
}

// 2��˳�ӵķ�
static fan_t get_2_chows_fan_unordered(tile_t t0, tile_t t1) {
    // ������Ƶ��˳��

    if (!is_suit_equal_quick(t0, t1)) {  // ��ɫ
        // ϲ���
        if (is_rank_equal_quick(t0, t1)) {
            return MIXED_DOUBLE_CHOW;
        }
    }
    else {  // һɫ
        // ����
        if (t0 + 3 == t1 || t1 + 3 == t0) {
            return SHORT_STRAIGHT;
        }
        // ���ٸ�
        rank_t r0 = tile_get_rank(t0);
        rank_t r1 = tile_get_rank(t1);
        if ((r0 == 2 && r1 == 8) || (r0 == 8 && r1 == 2)) {
            return TWO_TERMINAL_CHOWS;
        }
        // һ���
        if (t0 == t1) {
            return PURE_DOUBLE_CHOW;
        }
    }
    // ���϶�û��
    return FAN_NONE;
}

// 4����ӵķ�
static fan_t get_4_pungs_fan(tile_t t0, tile_t t1, tile_t t2, tile_t t3) {
    // һɫ�Ľڸ�
    if (is_numbered_suit_quick(t0) && t0 + 1 == t1 && t1 + 1 == t2 && t2 + 1 == t3) {
        return FOUR_PURE_SHIFTED_PUNGS;
    }
    // ����ϲ
    if (t0 == TILE_E && t1 == TILE_S && t2 == TILE_W && t3 == TILE_N) {
        return BIG_FOUR_WINDS;
    }
    // ���϶�û��
    return FAN_NONE;
}

// 3����ӵķ�
static fan_t get_3_pungs_fan(tile_t t0, tile_t t1, tile_t t2) {
    // ������Ƶ��˳��

    if (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1) && is_numbered_suit_quick(t2)) {  // ����
        suit_t s0 = tile_get_suit(t0);
        suit_t s1 = tile_get_suit(t1);
        suit_t s2 = tile_get_suit(t2);

        rank_t r0 = tile_get_rank(t0);
        rank_t r1 = tile_get_rank(t1);
        rank_t r2 = tile_get_rank(t2);

        if (is_mixed(s0, s1, s2)) {  // ��ɫ
            // ��ɫ���ڸ�
            if (is_shifted_1_unordered(r1, r0, r2)) {
                return MIXED_SHIFTED_PUNGS;
            }
            // ��ͬ��
            if (r0 == r1 && r1 == r2) {
                return TRIPLE_PUNG;
            }
        }
        else {
            // һɫ���ڸ�
            if (t0 + 1 == t1 && t1 + 1 == t2) {
                return PURE_SHIFTED_PUNGS;
            }
        }
    }
    else {
        // �����
        if ((t0 == TILE_E && t1 == TILE_S && t2 == TILE_W)
            || (t0 == TILE_E && t1 == TILE_S && t2 == TILE_N)
            || (t0 == TILE_E && t1 == TILE_W && t2 == TILE_N)
            || (t0 == TILE_S && t1 == TILE_W && t2 == TILE_N)) {
            return BIG_THREE_WINDS;
        }
        // ����Ԫ
        if (t0 == TILE_C && t1 == TILE_F && t2 == TILE_P) {
            return BIG_THREE_DRAGONS;
        }
    }

    // ���϶�û��
    return FAN_NONE;
}

// 2����ӵķ�
static fan_t get_2_pungs_fan_unordered(tile_t t0, tile_t t1) {
    // ������Ƶ��˳��
    if (is_numbered_suit_quick(t0) && is_numbered_suit_quick(t1)) {  // ����
        // ˫ͬ��
        if (is_rank_equal_quick(t0, t1)) {
            return DOUBLE_PUNG;
        }
    }
    else {
        // ˫����
        if (is_dragons(t0) && is_dragons(t1)) {
            return TWO_DRAGONS_PUNGS;
        }
    }
    // ���϶�û��
    return FAN_NONE;
}

// 1����ӵķ�
static fan_t get_1_pung_fan(tile_t mid_tile) {
    // ����
    if (is_dragons(mid_tile)) {
        return DRAGON_PUNG;
    }
    // �۾ſ�
    if (is_terminal(mid_tile) || is_winds(mid_tile)) {
        return PUNG_OF_TERMINALS_OR_HONORS;
    }
    // ���϶�û��
    return FAN_NONE;
}

// ����3��˳�ӵķ���ʱ�����µĵ�4��˳�������1��
static fan_t get_1_chow_extra_fan(tile_t tile0, tile_t tile1, tile_t tile2, tile_t tile_extra) {
    fan_t fan0 = get_2_chows_fan_unordered(tile0, tile_extra);
    fan_t fan1 = get_2_chows_fan_unordered(tile1, tile_extra);
    fan_t fan2 = get_2_chows_fan_unordered(tile2, tile_extra);

    // ������˳�򷵻�
    // һ���
    if (fan0 == PURE_DOUBLE_CHOW || fan1 == PURE_DOUBLE_CHOW || fan2 == PURE_DOUBLE_CHOW) {
        return PURE_DOUBLE_CHOW;
    }
    // ϲ���
    if (fan0 == MIXED_DOUBLE_CHOW || fan1 == MIXED_DOUBLE_CHOW || fan2 == MIXED_DOUBLE_CHOW) {
        return MIXED_DOUBLE_CHOW;
    }
    // ����
    if (fan0 == SHORT_STRAIGHT || fan1 == SHORT_STRAIGHT || fan2 == SHORT_STRAIGHT) {
        return SHORT_STRAIGHT;
    }
    // ���ٸ�
    if (fan0 == TWO_TERMINAL_CHOWS || fan1 == TWO_TERMINAL_CHOWS || fan2 == TWO_TERMINAL_CHOWS) {
        return TWO_TERMINAL_CHOWS;
    }

    return FAN_NONE;
}

// ����һ��ԭ��
// �������δ��Ϲ���һ���ƣ�ֻ��ͬ����Ϲ�����Ӧ��һ��������һ��
//
// ������ͬԭ��
// ���Ѿ��Ϲ�ĳһ���ֵ��ƣ�������ͬ����һ���������ͬ�ķ��ּƷ�
//
// ��������һ��ԭ��234567s234567p��ֻ�ܼ�Ϊ��ϲ���*2 ����*1�����ߡ�ϲ���*1 ����*2���������ǡ�ϲ���*2 ����*2��
// �����������㣬234s223344567p��ֻ�ܼ�Ϊ����һ��ߡ�ϲ��ꡢ�������������ǡ�ϲ���*2��������
//
// ֱ�Ӱ�������д�������ͼ���㷨��̫������
// �����㴦����ͳ���ж��ٷ���������ʱ�����������Ŀʱ�����ظ��Ŀ�ʼ����
static void exclusionary_rule(const fan_t *all_fans, long fan_cnt, long max_cnt, fan_table_t &fan_table) {
    // ͳ���ж��ٷ�
    uint16_t table[4] = { 0 };
    long cnt = 0;
    for (long i = 0; i < fan_cnt; ++i) {
        if (all_fans[i] != FAN_NONE) {
            ++cnt;
            ++table[all_fans[i] - PURE_DOUBLE_CHOW];
        }
    }

    // ������ʱ�����ظ��Ŀ�ʼ����
    int limit_cnt = 1;
    // ��һ��������ʣ��1���ڶ�������ʣ��0
    while (cnt > max_cnt && limit_cnt >= 0) {
        int idx = 4;  // �����ٸ���ʼ����
        while (cnt > max_cnt && idx-- > 0) {
            while (static_cast<int>(table[idx]) > limit_cnt && cnt > max_cnt) {
                --table[idx];
                --cnt;
            }
        }
        --limit_cnt;
    }

    fan_table[PURE_DOUBLE_CHOW] = table[0];
    fan_table[MIXED_DOUBLE_CHOW] = table[1];
    fan_table[SHORT_STRAIGHT] = table[2];
    fan_table[TWO_TERMINAL_CHOWS] = table[3];
}

// 4��˳���㷬
static void calculate_4_chows(const tile_t (&mid_tiles)[4], fan_table_t &fan_table) {
    fan_t fan;
    // ����4��˳�ӵķ���ʱ�����ټ����������
    if ((fan = get_4_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 3��˳���ж�
    // 012����3��˳�ӵķ���
    if ((fan = get_3_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        // �������4��˳�ӹ��ɵķ�
        if ((fan = get_1_chow_extra_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 013����3��˳�ӵķ���
    else if ((fan = get_3_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        // �������4��˳�ӹ��ɵķ�
        if ((fan = get_1_chow_extra_fan(mid_tiles[0], mid_tiles[1], mid_tiles[3], mid_tiles[2])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 023����3��˳�ӵķ���
    else if ((fan = get_3_chows_fan(mid_tiles[0], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        // �������4��˳�ӹ��ɵķ�
        if ((fan = get_1_chow_extra_fan(mid_tiles[0], mid_tiles[2], mid_tiles[3], mid_tiles[1])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }
    // 123����3��˳�ӵķ���
    else if ((fan = get_3_chows_fan(mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        // �������4��˳�ӹ��ɵķ�
        if ((fan = get_1_chow_extra_fan(mid_tiles[1], mid_tiles[2], mid_tiles[3], mid_tiles[0])) != FAN_NONE) {
            fan_table[fan] = 1;
        }
        return;
    }

    // ������3��˳�ӵķ���ʱ��4��˳�����3��
    fan_t all_fans[6] = {
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[3]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[3]),
        get_2_chows_fan_unordered(mid_tiles[2], mid_tiles[3])
    };

    int max_cnt = 3;

    // 0������3��˳�����κι�ϵ
    if (all_fans[0] == FAN_NONE && all_fans[1] == FAN_NONE && all_fans[2] == FAN_NONE) {
        --max_cnt;
    }

    // 1������3��˳�����κι�ϵ
    if (all_fans[0] == FAN_NONE && all_fans[3] == FAN_NONE && all_fans[4] == FAN_NONE) {
        --max_cnt;
    }

    // 2������3��˳�����κι�ϵ
    if (all_fans[1] == FAN_NONE && all_fans[3] == FAN_NONE && all_fans[5] == FAN_NONE) {
        --max_cnt;
    }

    // 3������3��˳�����κι�ϵ
    if (all_fans[2] == FAN_NONE && all_fans[4] == FAN_NONE && all_fans[5] == FAN_NONE) {
        --max_cnt;
    }

    if (max_cnt > 0) {
        exclusionary_rule(all_fans, 6, max_cnt, fan_table);
    }
}

// 3��˳���㷬
static void calculate_3_chows(const tile_t (&mid_tiles)[3], fan_table_t &fan_table) {
    fan_t fan;

    // ����3��˳�ӵķ���ʱ�����ټ��������
    if ((fan = get_3_chows_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // ��������������ʱ��3��˳�����2��
    fan_t all_fans[3] = {
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1]),
        get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[2]),
        get_2_chows_fan_unordered(mid_tiles[1], mid_tiles[2])
    };
    exclusionary_rule(all_fans, 3, 2, fan_table);
}

// 2��˳���㷬
static void calculate_2_chows_unordered(const tile_t (&mid_tiles)[2], fan_table_t &fan_table) {
    fan_t fan;
    if ((fan = get_2_chows_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// ���ӣ��ܣ��㷬
static void calculate_kongs(const pack_t *pung_packs, intptr_t pung_cnt, fan_table_t &fan_table) {
    // ͳ������ ���� ���� ����
    int melded_kong_cnt = 0;
    int concealed_kong_cnt = 0;
    int concealed_pung_cnt = 0;
    for (intptr_t i = 0; i < pung_cnt; ++i) {
        if (is_pack_melded(pung_packs[i])) {
            if (pack_get_type(pung_packs[i]) == PACK_TYPE_KONG) {
                ++melded_kong_cnt;
            }
        }
        else {
            if (pack_get_type(pung_packs[i]) == PACK_TYPE_KONG) {
                ++concealed_kong_cnt;
            }
            else {
                ++concealed_pung_cnt;
            }
        }
    }

    // ����
    // ����
    // ���� ���� ���� ���� -> ����+˫����+������
    // ���� ���� ���� ���� -> ����+˫����+������
    // ���� ���� ���� ���� -> ����+������+������
    // ���� ���� ���� ���� -> ����+������+������
    // ���� ���� ���� ���� -> ����+�İ���
    //
    // �ĸ�
    // ���� ���� ���� ���� -> �ĸ�
    // ���� ���� ���� ���� -> �ĸ�+˫����
    // ���� ���� ���� ���� -> �ĸ�+������
    // ���� ���� ���� ���� -> �ĸ�+�İ���
    //

    int kong_cnt = melded_kong_cnt + concealed_kong_cnt;
    switch (kong_cnt) {
    case 0:  // 0����
        switch (concealed_pung_cnt) {  // ���̵ĸ���
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: break;
        }
        break;
    case 1:  // 1����
        if (melded_kong_cnt == 1) {  // ����
            fan_table[MELDED_KONG] = 1;
            switch (concealed_pung_cnt) {  // ���̵ĸ���
            case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
        }
        else {  // ����
            fan_table[CONCEALED_KONG] = 1;
            switch (concealed_pung_cnt) {  // ���̵ĸ���
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 3: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
        }
        break;
    case 2:  // 2����
        switch (concealed_kong_cnt) {
        case 0:  // ˫����
            fan_table[TWO_MELDED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // ���̵ĸ���
            case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
            break;
        case 1:  // ������
#if SUPPORT_CONCEALED_KONG_AND_MELDED_KONG
            fan_table[CONCEALED_KONG_AND_MELDED_KONG] = 1;
#else
            fan_table[MELDED_KONG] = 1;
            fan_table[CONCEALED_KONG] = 1;
#endif
            switch (concealed_pung_cnt) {  // ���̵ĸ���
            case 1: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
            break;
        case 2:  // ˫����
            fan_table[TWO_CONCEALED_KONGS] = 1;
            switch (concealed_pung_cnt) {  // ���̵ĸ���
            case 1: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
            case 2: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
            default: break;
            }
            break;
        default:
            break;
        }
        break;
    case 3:  // 3����
        fan_table[THREE_KONGS] = 1;
        switch (concealed_kong_cnt) {  // ���̵ĸ���
        case 0:  // 3����
            break;
        case 1:  // 1����2����
            if (concealed_pung_cnt > 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            break;
        case 2:  // 2����1����
            if (concealed_pung_cnt == 0) {
                fan_table[TWO_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            break;
        case 3:  // 3����
            if (concealed_pung_cnt == 0) {
                fan_table[THREE_CONCEALED_PUNGS] = 1;
            }
            else {
                fan_table[FOUR_CONCEALED_PUNGS] = 1;
            }
            break;
        default:
            break;
        }
        break;
    case 4:  // 4����
        fan_table[FOUR_KONGS] = 1;
        switch (concealed_kong_cnt) {
        case 2: fan_table[TWO_CONCEALED_PUNGS] = 1; break;
        case 3: fan_table[THREE_CONCEALED_PUNGS] = 1; break;
        case 4: fan_table[FOUR_CONCEALED_PUNGS] = 1; break;
        default: break;
        }
        break;
    default:
        break;
    }

    // �ĸܺ��İ��̲��������ͣ������ȼ��������͵ķ�
    if (pung_cnt == 4) {
        if (fan_table[FOUR_KONGS] == 0 && fan_table[FOUR_CONCEALED_PUNGS] == 0) {
            fan_table[ALL_PUNGS] = 1;
        }
    }

    // ������ӵķ������̡��۾ſ̣�
    for (intptr_t i = 0; i < pung_cnt; ++i) {
        fan_t fan = get_1_pung_fan(pack_get_tile(pung_packs[i]));
        if (fan != FAN_NONE) {
            ++fan_table[fan];
        }
    }
}

// 4������㷬
static void calculate_4_pungs(const tile_t (&mid_tiles)[4], fan_table_t &fan_table) {
    fan_t fan;
    // ����4����ӵķ���ʱ�����ټ����������
    if ((fan = get_4_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // 3������ж�
    bool _3_pungs_has_fan = false;
    int free_pack_idx = -1;  // δʹ�õ�1�����
    // 012����3����ӵķ���
    if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 3;
        _3_pungs_has_fan = true;
    }
    // 013����3����ӵķ���
    else if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 2;
        _3_pungs_has_fan = true;
    }
    // 023����3����ӵķ���
    else if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 1;
        _3_pungs_has_fan = true;
    }
    // 123����3����ӵķ���
    else if ((fan = get_3_pungs_fan(mid_tiles[1], mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        fan_table[fan] = 1;
        free_pack_idx = 0;
        _3_pungs_has_fan = true;
    }

    // ����3����ӵķ���ʱ�����µĵ�4�����ֻ�����һ��
    if (_3_pungs_has_fan) {
        for (int i = 0; i < 4; ++i) {
            if (i == free_pack_idx) {
                continue;
            }
            // ������δʹ�õ�������Ӳ��Է���
            if ((fan = get_2_pungs_fan_unordered(mid_tiles[i], mid_tiles[free_pack_idx])) != FAN_NONE) {
                ++fan_table[fan];
                break;
            }
        }
        return;
    }

    // ������3����ӵķ���ʱ���������㷬��
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[2], mid_tiles[3])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 3������㷬
static void calculate_3_pungs(const tile_t (&mid_tiles)[3], fan_table_t &fan_table) {
    fan_t fan;

    // ����3����ӵķ��֣����ڸ� ��ͬ�� ����� ����Ԫ��ʱ�����ټ��������
    if ((fan = get_3_pungs_fan(mid_tiles[0], mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        fan_table[fan] = 1;
        return;
    }

    // ������3����ӵķ���ʱ���������㷬��
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
    if ((fan = get_2_pungs_fan_unordered(mid_tiles[1], mid_tiles[2])) != FAN_NONE) {
        ++fan_table[fan];
    }
}

// 2������㷬
static void calculate_2_pungs_unordered(const tile_t (&mid_tiles)[2], fan_table_t &fan_table) {
    fan_t fan = get_2_pungs_fan_unordered(mid_tiles[0], mid_tiles[1]);
    if (fan != FAN_NONE) {
        ++fan_table[fan];
    }
}

// ��������
static bool is_nine_gates(const tile_t *tiles) {
    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(tiles, 13, &cnt_table);

    // 1��9����ö��2~8��һö
    return (cnt_table[0x11] == 3 && cnt_table[0x19] == 3 && std::all_of(cnt_table + 0x12, cnt_table + 0x19, [](int n) { return n == 1; }))
        || (cnt_table[0x21] == 3 && cnt_table[0x29] == 3 && std::all_of(cnt_table + 0x22, cnt_table + 0x29, [](int n) { return n == 1; }))
        || (cnt_table[0x31] == 3 && cnt_table[0x39] == 3 && std::all_of(cnt_table + 0x32, cnt_table + 0x39, [](int n) { return n == 1; }));
}

// һɫ˫����
static bool is_pure_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_get_rank(pack_get_tile(pair_pack)) != 5) {  // 5��ȸͷ
        return false;
    }

    int _123_cnt = 0, _789_cnt = 0;
    suit_t pair_suit = tile_get_suit(pack_get_tile(pair_pack));
    for (int i = 0; i < 4; ++i) {
        suit_t suit = tile_get_suit(pack_get_tile(chow_packs[i]));
        if (suit != pair_suit) {  // ��ɫ��ȸͷ��ͬ
            return false;
        }
        rank_t rank = tile_get_rank(pack_get_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++_123_cnt; break;
        case 8: ++_789_cnt; break;
        default: return false;
        }
    }
    return (_123_cnt == 2 && _789_cnt == 2);  // 123��789��2��
}

// ��ɫ˫����
static bool is_three_suited_terminal_chows(const pack_t (&chow_packs)[4], pack_t pair_pack) {
    if (tile_get_rank(pack_get_tile(pair_pack)) != 5) {  // 5��ȸͷ
        return false;
    }

    int _123_suit_table[4] = { 0 };
    int _789_suit_table[4] = { 0 };
    suit_t pair_suit = tile_get_suit(pack_get_tile(pair_pack));
    for (int i = 0; i < 4; ++i) {
        suit_t suit = tile_get_suit(pack_get_tile(chow_packs[i]));
        if (suit == pair_suit) {  // ��ɫ��ȸͷ����ͬ
            return false;
        }
        rank_t rank = tile_get_rank(pack_get_tile(chow_packs[i]));
        switch (rank) {
        case 2: ++_123_suit_table[suit]; break;
        case 8: ++_789_suit_table[suit]; break;
        default: return false;
        }
    }

    // ��ȸͷ��ɫ��ͬ����ɫ123��789��һ��
    switch (pair_suit) {
    case 1: return (_123_suit_table[2] && _123_suit_table[3] && _789_suit_table[2] && _789_suit_table[3]);
    case 2: return (_123_suit_table[1] && _123_suit_table[3] && _789_suit_table[1] && _789_suit_table[3]);
    case 3: return (_123_suit_table[1] && _123_suit_table[2] && _789_suit_table[1] && _789_suit_table[2]);
    default: return false;
    }
}

// ���ݺ��Ʒ�ʽ���������漰���֣������ˡ�ȫ����
static void adjust_by_self_drawn(const pack_t (&packs)[5], intptr_t fixed_cnt, bool self_drawn, fan_table_t &fan_table) {
    ptrdiff_t melded_cnt = std::count_if(&packs[0], &packs[fixed_cnt], &is_pack_melded);  // ����¶������

    switch (melded_cnt) {
    case 0:  // 0�����ģ�����Ϊ�����ˣ����Ϊ��ǰ��
        fan_table[self_drawn ? FULLY_CONCEALED_HAND : CONCEALED_HAND] = 1;
        break;
    case 4:
        // 4�����ģ������������������Ϊȫ����
        fan_table[self_drawn ? SELF_DRAWN : MELDED_HAND] = 1;
        break;
    default:
        if (self_drawn) {
            fan_table[SELF_DRAWN] = 1;
        }
        break;
    }
}

// ����ȸͷ���������漰���֣�ƽ�͡�С��Ԫ��С��ϲ
static void adjust_by_pair_tile(tile_t pair_tile, intptr_t chow_cnt, fan_table_t &fan_table) {
    if (chow_cnt == 4) {  // 4�鶼��˳��
        if (is_numbered_suit_quick(pair_tile)) {  // ����ȸͷ
            fan_table[ALL_CHOWS] = 1;  // ƽ��
        }
        return;
    }

    // ��˫���̵Ļ����ϣ����ȸͷ�Ǽ��ƣ�������ΪС��Ԫ
    if (fan_table[TWO_DRAGONS_PUNGS]) {
        if (is_dragons(pair_tile)) {
            fan_table[LITTLE_THREE_DRAGONS] = 1;
            fan_table[TWO_DRAGONS_PUNGS] = 0;
        }
        return;
    }
    // ������̵Ļ����ϣ����ȸͷ�Ƿ��ƣ�������ΪС��ϲ
    if (fan_table[BIG_THREE_WINDS]) {
        if (is_winds(pair_tile)) {
            fan_table[LITTLE_FOUR_WINDS] = 1;
            fan_table[BIG_THREE_WINDS] = 0;
        }
        return;
    }
}

// ���ݻ�ɫ���������漰���֣����֡�ȱһ�š���һɫ����һɫ��������
static void adjust_by_suits(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
    // ���������Щ��ɫ����bit����
    uint8_t suit_flag = 0;
    for (intptr_t i = 0; i < tile_cnt; ++i) {
        suit_flag |= (1 << tile_get_suit(tiles[i]));
    }

    // 1111 0001
    if (!(suit_flag & 0xF1U)) {
        fan_table[NO_HONORS] = 1;  // ����
    }

    // 1110 0011
    if (!(suit_flag & 0xE3U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // ȱһ�ţ���
    }
    // 1110 0101
    if (!(suit_flag & 0xE5U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // ȱһ�ţ�����
    }
    // 1110 1001
    if (!(suit_flag & 0xE9U)) {
        ++fan_table[ONE_VOIDED_SUIT];  // ȱһ�ţ�����
    }

    // ��ȱ2��ʱ���������ֺ����֣�����Ϊ��һɫ����һɫ
    if (fan_table[ONE_VOIDED_SUIT] == 2) {
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[suit_flag & 0xF1U ? HALF_FLUSH : FULL_FLUSH] = 1;
    }

    // 0001 1110
    if (suit_flag == 0x1EU) {  // �������ƺ����ƶ���
        if (std::any_of(tiles, tiles + tile_cnt, &is_winds)
            && std::any_of(tiles, tiles + tile_cnt, &is_dragons)) {
            fan_table[ALL_TYPES] = 1;  // ������
        }
    }
}

// �������Ƶķ�Χ���������漰���֣������塢С���塢ȫ��ȫ�С�ȫС
static void adjust_by_rank_range(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // �ϸ�98������߶Բ�֧�ֵ�����Щ
    }
#endif

    // ���������Щ��
    uint16_t rank_flag = 0;
    for (intptr_t i = 0; i < tile_cnt; ++i) {
        if (!is_numbered_suit_quick(tiles[i])) {
            return;
        }
        rank_flag |= (1 << tile_get_rank(tiles[i]));
    }

    // 1111 1111 1110 0001
    // ����Ƿ�ֻ����1234
    if (!(rank_flag & 0xFFE1)) {
        // ����4ΪС���壬����ΪȫС
        fan_table[rank_flag & 0x0010 ? LOWER_FOUR : LOWER_TILES] = 1;
        return;
    }
    // 1111 1100 0011 1111
    // ����Ƿ�ֻ����6789
    if (!(rank_flag & 0xFC3F)) {
        // ����6Ϊ�����壬����Ϊȫ��
        fan_table[rank_flag & 0x0040 ? UPPER_FOUR : UPPER_TILES] = 1;
        return;
    }
    // 1111 1111 1000 1111
    // ����Ƿ�ֻ����456
    if (!(rank_flag & 0xFF8F)) {
        // ȫ��
        fan_table[MIDDLE_TILES] = 1;
    }
}

// ���������������������漰���֣�ȫ���ۡ�ȫ���塢ȫ˫��
static void adjust_by_packs_traits(const pack_t (&packs)[5], fan_table_t &fan_table) {
    // ͳ�ư�������19�����ơ�5��˫���Ƶ�����
    int terminal_pack = 0;
    int honor_pack = 0;
    int _5_pack = 0;
    int even_pack = 0;
    for (int i = 0; i < 5; ++i) {
        tile_t tile = pack_get_tile(packs[i]);
        if (is_numbered_suit_quick(tile)) {  // ����
            rank_t rank = tile_get_rank(tile);
            if (pack_get_type(packs[i]) == PACK_TYPE_CHOW) {  // ˳��
                switch (rank) {
                case 2: case 8: ++terminal_pack; break;  // ����19
                case 4: case 5: case 6: ++_5_pack; break;  // ����
                default: break;
                }
            }
            else {  // ���ӻ�ȸͷ
                switch (rank) {
                case 1: case 9: ++terminal_pack; break;  // ����19
                case 5: ++_5_pack; break;  // ����
                case 2: case 4: case 6: case 8: ++even_pack; break;  // ˫��
                default: break;
                }
            }
        }
        else {
            ++honor_pack;  // ����
        }
    }

    // 5���ƶ���������19�����ƣ�����ʱ��Ϊȫ����
    if (terminal_pack + honor_pack == 5) {
        fan_table[OUTSIDE_HAND] = 1;
        return;
    }
    // ȫ����
    if (_5_pack == 5) {
        fan_table[ALL_FIVE] = 1;
        return;
    }
    // ȫ˫��
    if (even_pack == 5) {
        fan_table[ALL_EVEN_PUNGS] = 1;
    }
}

// ���������Ե��������漰���֣����ۡ��Ʋ�������һɫ����һɫ�����۾š����۾�
static void adjust_by_tiles_traits(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
    // ����
    if (std::none_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        fan_table[ALL_SIMPLES] = 1;
    }

    // �Ʋ���
    if (std::all_of(tiles, tiles + tile_cnt, &is_reversible)) {
        fan_table[REVERSIBLE_TILES] = 1;
    }

#ifdef STRICT_98_RULE
    if (fan_table[SEVEN_PAIRS]) {
        return;  // �ϸ�98������߶Բ�֧����һɫ����һɫ�����۾š����۾�
    }
#endif

    // ��һɫ
    if (std::all_of(tiles, tiles + tile_cnt, &is_green)) {
        fan_table[ALL_GREEN] = 1;
    }

    // ��������˾�û��Ҫ�����һɫ�����۾š����۾���
    if (fan_table[ALL_SIMPLES] != 0) {
        return;
    }

    // ��һɫ
    if (std::all_of(tiles, tiles + tile_cnt, &is_honor)) {
        fan_table[ALL_HONORS] = 1;
        return;
    }
    // ���۾�
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal)) {
        fan_table[ALL_TERMINALS] = 1;
        return;
    }
    // ���۾�
    if (std::all_of(tiles, tiles + tile_cnt, &is_terminal_or_honor)) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 1;
    }
}

// �Ĺ�һ����
static void adjust_by_tiles_hog(const tile_t *tiles, intptr_t tile_cnt, fan_table_t &fan_table) {
    intptr_t kong_cnt = tile_cnt - 14;  // ��׼����14�ţ�������ž�˵���м�����
    tile_table_t cnt_table;
    map_tiles(tiles, tile_cnt, &cnt_table);
    // �ж������Ѿ���ȥ4�ŵ��Ƽ�ȥ�ܵ���������Ϊ�Ĺ�һ������
    intptr_t _4_cnt = std::count(std::begin(cnt_table), std::end(cnt_table), 4);
    fan_table[TILE_HOG] = static_cast<uint8_t>(_4_cnt - kong_cnt);
}

// �������Ʒ�ʽ���������漰���֣����š�Ƕ�š�������
static void adjust_by_waiting_form(const pack_t *concealed_packs, intptr_t pack_cnt, const tile_t *standing_tiles, intptr_t standing_cnt,
    tile_t win_tile, fan_table_t &fan_table) {
    // ȫ���˺��ĸܲ��Ƶ�������Ҳ�������б��š�Ƕ��
    if (fan_table[MELDED_HAND] || fan_table[FOUR_KONGS]) {
        return;
    }

    useful_table_t waiting_table;  // ���Ʊ�Ǳ�
    if (!is_basic_form_wait(standing_tiles, standing_cnt, &waiting_table)) {
        return;
    }

    if (pack_cnt == 5) {  // ����״̬
        // �ж��Ƿ�Ϊ�߶�����
        useful_table_t temp_table;
        if (is_seven_pairs_wait(standing_tiles, standing_cnt, &temp_table)) {
            // �ϲ����Ʊ�Ǳ�
            std::transform(std::begin(temp_table), std::end(temp_table), std::begin(waiting_table),
                std::begin(waiting_table), [](bool w, bool t) { return w || t; });
        }
    }

    // ͳ����������������������1�ţ����Ʊ��š�Ƕ�š�������
    if (1 != std::count(std::begin(waiting_table), std::end(waiting_table), true)) {
        return;
    }

    // ��1�ŵ�������������Ŵ���ʲôλ��
    // ����0x01 Ƕ��0x02 ������0x04
    uint8_t pos_flag = 0;

    for (intptr_t i = 0; i < pack_cnt; ++i) {
        switch (pack_get_type(concealed_packs[i])) {
        case PACK_TYPE_CHOW: {
            tile_t mid_tile = pack_get_tile(concealed_packs[i]);
            if (mid_tile == win_tile) {
                pos_flag |= 0x02U;  // Ƕ��
            }
            else if (mid_tile + 1 == win_tile || mid_tile - 1 == win_tile) {
                pos_flag |= 0x01U;  // ����
            }
            break;
        }
        case PACK_TYPE_PAIR: {
            tile_t mid_tile = pack_get_tile(concealed_packs[i]);
            if (mid_tile == win_tile) {
                pos_flag |= 0x04U;  // ������
            }
            break;
        }
        default:
            break;
        }
    }

    // �����ֿ��ܴ���ʱ��ֻ�ܼ�����һ��
    if (pos_flag & 0x01U) {
        fan_table[EDGE_WAIT] = 1;
    }
    else if (pos_flag & 0x02U) {
        fan_table[CLOSED_WAIT] = 1;
    }
    else if (pos_flag & 0x04U) {
        fan_table[SINGLE_WAIT] = 1;
    }
}

// ���ݷ���������漰���֣�Ȧ��̡��ŷ��
static void adjust_by_winds(tile_t tile, wind_t prevalent_wind, wind_t seat_wind, fan_table_t &fan_table) {
    rank_t delta = tile - TILE_E;
    if (delta == static_cast<int>(prevalent_wind) - static_cast<int>(wind_t::EAST)) {
        fan_table[PREVALENT_WIND] = 1;
    }
    if (delta == static_cast<int>(seat_wind) - static_cast<int>(wind_t::EAST)) {
        fan_table[SEAT_WIND] = 1;
    }
}

// ͳһ����һЩ���Ƶ�
static void adjust_fan_table(fan_table_t &fan_table, bool prevalent_eq_seat) {
    // ����ϲ��������̡������͡�Ȧ��̡��ŷ�̡��۾ſ�
    if (fan_table[BIG_FOUR_WINDS]) {
        fan_table[BIG_THREE_WINDS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[PREVALENT_WIND] = 0;
        fan_table[SEAT_WIND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }
    // ����Ԫ����˫���̡����̣��ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[BIG_THREE_DRAGONS]) {
        fan_table[TWO_DRAGONS_PUNGS] = 0;
        fan_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // ��һɫ���ƻ�һɫ��ȱһ��
    if (fan_table[ALL_GREEN]) {
        fan_table[HALF_FLUSH] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // �������Ʋ�����һɫ����ǰ�塢ȱһ�š����֣�����1���۾ſ̣��Ѳ���������Ϊ����
    if (fan_table[NINE_GATES]) {
        fan_table[FULL_FLUSH] = 0;
        fan_table[CONCEALED_HAND] = 0;
        --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
        if (fan_table[FULLY_CONCEALED_HAND]) {
            fan_table[FULLY_CONCEALED_HAND] = 0;
            fan_table[SELF_DRAWN] = 1;
        }
    }
    // �ĸܲ��Ƶ�����
    if (fan_table[FOUR_KONGS]) {
        fan_table[SINGLE_WAIT] = 0;
    }
    // ���߶Բ����߶ԡ���һɫ����ǰ�塢ȱһ�š�����
    if (fan_table[SEVEN_SHIFTED_PAIRS]) {
        fan_table[SEVEN_PAIRS] = 0;
        fan_table[FULL_FLUSH] = 0;
        fan_table[CONCEALED_HAND] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // ʮ���۲��������롢��ǰ�塢������
    if (fan_table[THIRTEEN_ORPHANS]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
        fan_table[SINGLE_WAIT] = 0;
    }

    // ���۾Ų��ƻ��۾š���������ȫ���ۡ��۾ſ̡����֣��ϸ�98���򲻼�˫ͬ�̡�������ͬ�̣�
    if (fan_table[ALL_TERMINALS]) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        fan_table[NO_HONORS] = 0;
        fan_table[DOUBLE_PUNG] = 0;  // ͨ�мƷ�����˫ͬ��
#ifdef STRICT_98_RULE
        fan_table[TRIPLE_PUNG] = 0;
        fan_table[DOUBLE_PUNG] = 0;
#endif
    }

    // С��ϲ���������
    if (fan_table[LITTLE_FOUR_WINDS]) {
        fan_table[BIG_THREE_WINDS] = 0;
        // С��ϲ�ĵ������������19�Ŀ��ӣ����ǻ��۾ţ�����Ǽ���������һɫ�������ֶ��ǲ����۾ſ̵�
        // �����˳�ӻ���2-8�Ŀ��ӣ��򲻴��ڶ�����۾ſ�
        // �������ｫ�۾ſ���Ϊ0
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // С��Ԫ����˫���̡����̣��ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[LITTLE_THREE_DRAGONS]) {
        fan_table[TWO_DRAGONS_PUNGS] = 0;
        fan_table[DRAGON_PUNG] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // ��һɫ���ƻ��۾š���������ȫ���ۡ��۾ſ̡�ȱһ��
    if (fan_table[ALL_HONORS]) {
        fan_table[ALL_TERMINALS_AND_HONORS] = 0;
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // �İ��̲��������͡���ǰ�壬�Ѳ���������Ϊ����
    if (fan_table[FOUR_CONCEALED_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[CONCEALED_HAND] = 0;
        if (fan_table[FULLY_CONCEALED_HAND]) {
            fan_table[FULLY_CONCEALED_HAND] = 0;
            fan_table[SELF_DRAWN] = 1;
        }
    }
    // һɫ˫���᲻���߶ԡ���һɫ��ƽ�͡�һ��ߡ����ٸ���ȱһ�š�����
    if (fan_table[PURE_TERMINAL_CHOWS]) {
        fan_table[SEVEN_PAIRS] = 0;
        fan_table[FULL_FLUSH] = 0;
        fan_table[ALL_CHOWS] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // һɫ��ͬ˳����һɫ��ͬ˳��һ��ߡ��Ĺ�һ���ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[QUADRUPLE_CHOW]) {
        fan_table[PURE_SHIFTED_PUNGS] = 0;
        fan_table[TILE_HOG] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }
    // һɫ�Ľڸ߲���һɫ���ڸߡ������ͣ��ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[FOUR_PURE_SHIFTED_PUNGS]) {
        fan_table[PURE_TRIPLE_CHOW] = 0;
        fan_table[ALL_PUNGS] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // һɫ�Ĳ��߲���һɫ�����ߡ����ٸ����������ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[FOUR_PURE_SHIFTED_CHOWS]) {
        fan_table[PURE_SHIFTED_CHOWS] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
        fan_table[SHORT_STRAIGHT] = 0;
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // ���۾Ų��������͡�ȫ���ۡ��۾ſ�
    if (fan_table[ALL_TERMINALS_AND_HONORS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[OUTSIDE_HAND] = 0;
        fan_table[PUNG_OF_TERMINALS_OR_HONORS] = 0;
    }

    // �߶Բ�����ǰ�塢������
    if (fan_table[SEVEN_PAIRS]) {
        fan_table[CONCEALED_HAND] = 0;
        fan_table[SINGLE_WAIT] = 0;
    }
    // ���ǲ������������롢��ǰ��
    if (fan_table[GREATER_HONORS_AND_KNITTED_TILES]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
    }
    // ȫ˫�̲��������������ۡ�����
    if (fan_table[ALL_EVEN_PUNGS]) {
        fan_table[ALL_PUNGS] = 0;
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // ��һɫ����ȱһ�š�����
    if (fan_table[FULL_FLUSH]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // һɫ��ͬ˳����һɫ���ڸߡ�һ���
    if (fan_table[PURE_TRIPLE_CHOW]) {
        fan_table[PURE_SHIFTED_PUNGS] = 0;
        fan_table[PURE_DOUBLE_CHOW] = 0;
    }
    // һɫ���ڸ߲���һɫ��ͬ˳
    if (fan_table[PURE_SHIFTED_PUNGS]) {
        fan_table[PURE_TRIPLE_CHOW] = 0;
    }
    // ȫ�󲻼ƴ����塢����
    if (fan_table[UPPER_TILES]) {
        fan_table[UPPER_FOUR] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // ȫ�в��ƶ���
    if (fan_table[MIDDLE_TILES]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }
    // ȫС����С���塢����
    if (fan_table[LOWER_TILES]) {
        fan_table[LOWER_FOUR] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // ��ɫ˫���᲻��ƽ�͡����֡�ϲ��ꡢ���ٸ�
    if (fan_table[THREE_SUITED_TERMINAL_CHOWS]) {
        fan_table[ALL_CHOWS] = 0;
        fan_table[NO_HONORS] = 0;
        fan_table[MIXED_DOUBLE_CHOW] = 0;
        fan_table[TWO_TERMINAL_CHOWS] = 0;
    }
    // ȫ���岻�ƶ��ۡ�����
    if (fan_table[ALL_FIVE]) {
        fan_table[ALL_SIMPLES] = 0;
        fan_table[NO_HONORS] = 0;
    }

    // ���ǲ������������롢��ǰ��
    if (fan_table[LESSER_HONORS_AND_KNITTED_TILES]) {
        fan_table[ALL_TYPES] = 0;
        fan_table[CONCEALED_HAND] = 0;
    }
    // �����岻������
    if (fan_table[UPPER_FOUR]) {
        fan_table[NO_HONORS] = 0;
    }
    // С���岻������
    if (fan_table[LOWER_FOUR]) {
        fan_table[NO_HONORS] = 0;
    }
    // ������ڲ����ټ��۾ſ̣��ϸ�98���򲻼�ȱһ�ţ�
    if (fan_table[BIG_THREE_WINDS]) {
        // ���������һɫ����۾ţ���Ҫ��ȥ3���۾ſ�
        if (!fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] >= 3);
            fan_table[PUNG_OF_TERMINALS_OR_HONORS] -= 3;
        }
#ifdef STRICT_98_RULE
        fan_table[ONE_VOIDED_SUIT] = 0;
#endif
    }

    // �Ʋ�������ȱһ��
    if (fan_table[REVERSIBLE_TILES]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // ���ֻش���������
    if (fan_table[LAST_TILE_DRAW]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // ���Ͽ�����������
    if (fan_table[OUT_WITH_REPLACEMENT_TILE]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // ���ܺͲ��ƺ;���
    if (fan_table[ROBBING_THE_KONG]) {
        fan_table[LAST_TILE] = 0;
    }
    // ˫���ܲ��ư���
    if (fan_table[TWO_CONCEALED_KONGS]) {
        fan_table[CONCEALED_KONG] = 0;
    }

    // ��һɫ����ȱһ��
    if (fan_table[HALF_FLUSH]) {
        fan_table[ONE_VOIDED_SUIT] = 0;
    }
    // ȫ���˲��Ƶ�����
    if (fan_table[MELDED_HAND]) {
        fan_table[SINGLE_WAIT] = 0;
    }
    // ˫���̲��Ƽ���
    if (fan_table[TWO_DRAGONS_PUNGS]) {
        fan_table[DRAGON_PUNG] = 0;
    }

    // �����˲�������
    if (fan_table[FULLY_CONCEALED_HAND]) {
        fan_table[SELF_DRAWN] = 0;
    }
    // ˫���ܲ�������
    if (fan_table[TWO_MELDED_KONGS]) {
        fan_table[MELDED_KONG] = 0;
    }

    // Ȧ����Լ������۾ſ�
    if (fan_table[PREVALENT_WIND]) {
        // �����������̡�С��ϲ����һɫ�����۾ţ���Ҫ��ȥ1���۾ſ�
        if (!fan_table[BIG_THREE_WINDS] && !fan_table[LITTLE_FOUR_WINDS]
            && !fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // �ŷ���Լ������۾ſ�
    if (fan_table[SEAT_WIND]) {
        // ���Ȧ�����ŷ粻��ͬ�����Ҳ�������̡�С��ϲ����һɫ�����۾ţ���Ҫ��ȥ1���۾ſ�
        if (!prevalent_eq_seat && !fan_table[BIG_THREE_WINDS] && !fan_table[LITTLE_FOUR_WINDS]
            && !fan_table[ALL_HONORS] && !fan_table[ALL_TERMINALS_AND_HONORS]) {
            assert(fan_table[PUNG_OF_TERMINALS_OR_HONORS] > 0);
            --fan_table[PUNG_OF_TERMINALS_OR_HONORS];
        }
    }
    // ƽ�Ͳ�������
    if (fan_table[ALL_CHOWS]) {
        fan_table[NO_HONORS] = 0;
    }
    // ���۲�������
    if (fan_table[ALL_SIMPLES]) {
        fan_table[NO_HONORS] = 0;
    }
}

// ���ݺ��Ʊ�ǵ��������漰���֣��;��š����ֻش����������¡�����
static void adjust_by_win_flag(win_flag_t win_flag, fan_table_t &fan_table) {
    if (win_flag & WIN_FLAG_4TH_TILE) {
        fan_table[LAST_TILE] = 1;
    }
    if (win_flag & WIN_FLAG_WALL_LAST) {
        fan_table[win_flag & WIN_FLAG_SELF_DRAWN ? LAST_TILE_DRAW : LAST_TILE_CLAIM] = 1;
    }
    if (win_flag & WIN_FLAG_ABOUT_KONG) {
        fan_table[win_flag & WIN_FLAG_SELF_DRAWN ? OUT_WITH_REPLACEMENT_TILE : ROBBING_THE_KONG] = 1;
    }
    if (win_flag & WIN_FLAG_SELF_DRAWN) {
        fan_table[SELF_DRAWN] = 1;
    }
}

// ���������㷬
static void calculate_basic_form_fan(const pack_t (&packs)[5], const calculate_param_t *calculate_param, fan_table_t &fan_table) {
    pack_t pair_pack = 0;
    pack_t chow_packs[4];
    pack_t pung_packs[4];
    intptr_t chow_cnt = 0;
    intptr_t pung_cnt = 0;
    for (int i = 0; i < 5; ++i) {
        switch (pack_get_type(packs[i])) {
        case PACK_TYPE_CHOW: chow_packs[chow_cnt++] = packs[i]; break;
        case PACK_TYPE_PUNG:
        case PACK_TYPE_KONG: pung_packs[pung_cnt++] = packs[i]; break;
        case PACK_TYPE_PAIR: pair_pack = packs[i]; break;
        default: assert(0); return;
        }
    }

    if (pair_pack == 0 || chow_cnt + pung_cnt != 4) {
        return;
    }

    tile_t win_tile = calculate_param->win_tile;
    win_flag_t win_flag = calculate_param->win_flag;

    // ���ݺ��Ʊ�ǵ��������漰���֣��;��š����ֻش����������¡�����
    adjust_by_win_flag(win_flag, fan_table);

    // ��͵����ţ�������ܽ���Ϊ˳���е�һ�ţ���ô�������Ϊ���ӣ�������������Ϊ����
    if ((win_flag & WIN_FLAG_SELF_DRAWN) == 0) {
        // ���Ʋ��ܽ���Ϊ˳���е�һ��
        if (std::none_of(chow_packs, chow_packs + chow_cnt, [win_tile](pack_t chow_pack) {
            tile_t tile = pack_get_tile(chow_pack);
            return !is_pack_melded(chow_pack)
                && (tile - 1 == win_tile || tile == win_tile || tile + 1 == win_tile);
        })) {
            for (intptr_t i = 0; i < pung_cnt; ++i) {
                if (pack_get_tile(pung_packs[i]) == win_tile && !is_pack_melded(pung_packs[i])) {
                    pung_packs[i] |= (1 << 12);  // ���Ϊ����¶
                }
            }
        }
    }

    if (pung_cnt > 0) { // �п���
        calculate_kongs(pung_packs, pung_cnt, fan_table);
    }

    switch (chow_cnt) {
    case 4: {  // 4��˳��
        // �����ɫ/һɫ˫����
        if (is_three_suited_terminal_chows(chow_packs, pair_pack)) {
            fan_table[THREE_SUITED_TERMINAL_CHOWS] = 1;
            break;
        }
        if (is_pure_terminal_chows(chow_packs, pair_pack)) {
            fan_table[PURE_TERMINAL_CHOWS] = 1;
            break;
        }

        tile_t mid_tiles[4];
        mid_tiles[0] = pack_get_tile(chow_packs[0]);
        mid_tiles[1] = pack_get_tile(chow_packs[1]);
        mid_tiles[2] = pack_get_tile(chow_packs[2]);
        mid_tiles[3] = pack_get_tile(chow_packs[3]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_4_chows(mid_tiles, fan_table);
        break;
    }
    case 3: {  // 3��˳��+1�����
        tile_t mid_tiles[3];
        mid_tiles[0] = pack_get_tile(chow_packs[0]);
        mid_tiles[1] = pack_get_tile(chow_packs[1]);
        mid_tiles[2] = pack_get_tile(chow_packs[2]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_3_chows(mid_tiles, fan_table);
        break;
    }
    case 2: {  // 2��˳��+2�����
        tile_t mid_tiles_chow[2];
        mid_tiles_chow[0] = pack_get_tile(chow_packs[0]);
        mid_tiles_chow[1] = pack_get_tile(chow_packs[1]);

        tile_t mid_tiles_pung[2];
        mid_tiles_pung[0] = pack_get_tile(pung_packs[0]);
        mid_tiles_pung[1] = pack_get_tile(pung_packs[1]);

        calculate_2_chows_unordered(mid_tiles_chow, fan_table);
        calculate_2_pungs_unordered(mid_tiles_pung, fan_table);
        break;
    }
    case 1: {  // 1��˳��+3�����
        tile_t mid_tiles[3];
        mid_tiles[0] = pack_get_tile(pung_packs[0]);
        mid_tiles[1] = pack_get_tile(pung_packs[1]);
        mid_tiles[2] = pack_get_tile(pung_packs[2]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_3_pungs(mid_tiles, fan_table);
        break;
    }
    case 0: {  // 4�����
        tile_t mid_tiles[4];
        mid_tiles[0] = pack_get_tile(pung_packs[0]);
        mid_tiles[1] = pack_get_tile(pung_packs[1]);
        mid_tiles[2] = pack_get_tile(pung_packs[2]);
        mid_tiles[3] = pack_get_tile(pung_packs[3]);
        std::sort(std::begin(mid_tiles), std::end(mid_tiles));

        calculate_4_pungs(mid_tiles, fan_table);
        break;
    }
    default:
        break;
    }

    intptr_t fixed_cnt = calculate_param->hand_tiles.pack_count;
    const tile_t *standing_tiles = calculate_param->hand_tiles.standing_tiles;
    intptr_t standing_cnt = calculate_param->hand_tiles.tile_count;
    wind_t prevalent_wind = calculate_param->prevalent_wind;
    wind_t seat_wind = calculate_param->seat_wind;

    // ��������
    if (standing_cnt == 13) {
        if (is_nine_gates(standing_tiles)) {
            fan_table[NINE_GATES] = 1;
        }
    }

    // ���ݺ��Ʒ�ʽ���������漰���֣������ˡ�ȫ����
    adjust_by_self_drawn(packs, fixed_cnt, (win_flag & WIN_FLAG_SELF_DRAWN) != 0, fan_table);
    // ����ȸͷ���������漰���֣�ƽ�͡�С��Ԫ��С��ϲ
    adjust_by_pair_tile(pack_get_tile(pair_pack), chow_cnt, fan_table);
    // ���������������������漰���֣�ȫ���ۡ�ȫ���塢ȫ˫��
    adjust_by_packs_traits(packs, fan_table);

    tile_t tiles[18];
    memcpy(tiles, standing_tiles, standing_cnt * sizeof(tile_t));
    intptr_t tile_cnt = packs_to_tiles(packs, fixed_cnt, &tiles[standing_cnt], 18 - standing_cnt);
    tile_cnt += standing_cnt;
    tiles[tile_cnt++] = win_tile;

    // ���ݻ�ɫ���������漰���֣����֡�ȱһ�š���һɫ����һɫ��������
    adjust_by_suits(tiles, tile_cnt, fan_table);
    // ���������Ե��������漰���֣����ۡ��Ʋ�������һɫ����һɫ�����۾š����۾�
    adjust_by_tiles_traits(tiles, tile_cnt, fan_table);
    // �������Ƶķ�Χ���������漰���֣������塢С���塢ȫ��ȫ�С�ȫС
    adjust_by_rank_range(tiles, tile_cnt, fan_table);
    // �Ĺ�һ����
    adjust_by_tiles_hog(tiles, tile_cnt, fan_table);
    // �������Ʒ�ʽ���������漰���֣����š�Ƕ�š�������
    adjust_by_waiting_form(packs + fixed_cnt, 5 - fixed_cnt, standing_tiles, standing_cnt, win_tile, fan_table);

    // ���ݷ���������漰���֣�Ȧ��̡��ŷ��
    for (intptr_t i = 0; i < pung_cnt; ++i) {
        adjust_by_winds(pack_get_tile(pung_packs[i]), prevalent_wind, seat_wind, fan_table);
    }

    // ͳһ����һЩ���Ƶ�
    adjust_fan_table(fan_table, prevalent_wind == seat_wind);

    // ���ʲô����û�У����Ϊ�޷���
    if (std::all_of(std::begin(fan_table), std::end(fan_table), [](uint8_t p) { return p == 0; })) {
        fan_table[CHICKEN_HAND] = 1;
    }
}

// �������+����+ȸͷ�������㷬
static bool calculate_knitted_straight_fan(const calculate_param_t *calculate_param, fan_table_t &fan_table) {
    const hand_tiles_t *hand_tiles = &calculate_param->hand_tiles;
    tile_t win_tile = calculate_param->win_tile;
    win_flag_t win_flag = calculate_param->win_flag;
    wind_t prevalent_wind = calculate_param->prevalent_wind;
    wind_t seat_wind = calculate_param->seat_wind;

    intptr_t fixed_cnt = hand_tiles->pack_count;
    if (fixed_cnt > 1) {
        return false;
    }

    const pack_t *fixed_packs = hand_tiles->fixed_packs;
    intptr_t standing_cnt = hand_tiles->tile_count;

    // �����ƺͺ��Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(hand_tiles->standing_tiles, standing_cnt, &cnt_table);
    ++cnt_table[win_tile];

    // ƥ�������
    const tile_t (*matched_seq)[9] = std::find_if(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&cnt_table](const tile_t (&seq)[9]) {
        return std::all_of(std::begin(seq), std::end(seq), [&cnt_table](tile_t t) { return cnt_table[t] > 0; });
    });

    if (matched_seq == &standard_knitted_straight[6]) {
        return false;
    }

    // �޳������
    std::for_each(std::begin(*matched_seq), std::end(*matched_seq), [&cnt_table](tile_t t) { --cnt_table[t]; });

    // ���������ͻ���
    division_result_t result;
    result.count = 0;
    division_t work_division;
    memset(&work_division, 0, sizeof(work_division));

    // �˴��߼�Ϊ���������9���Ƶ������Ѿ���ɵ�3�����ӣ��ճ�0 1 2�±괦��3��
    // �����4���Ǹ�¶�ģ���������±�3��
    // Ȼ�󰴻������ʹӴ�fixed_cnt + 3��ʼ�ݹ�
    // ���ֺ�Ľ�����±�3��Ϊ���������ӣ��±�4��Ϊȸͷ
    if (fixed_cnt == 1) {
        work_division.packs[3] = fixed_packs[0];
    }
    divide_recursively(cnt_table, fixed_cnt + 3, 0, &work_division, &result);
    if (result.count != 1) {
        return false;
    }

    const pack_t *packs = result.divisions[0].packs;  // packs��0 1 2�±궼��û�õ�

    // ��Ƿ�
    fan_table[KNITTED_STRAIGHT] = 1;  // �����
    if (pack_get_type(packs[3]) == PACK_TYPE_CHOW) {  // ��4����˳��
        if (is_numbered_suit_quick(pack_get_tile(packs[4]))) {
            fan_table[ALL_CHOWS] = 1;  // ȸͷ������ʱ��Ϊƽ��
        }
    }
    else {
        calculate_kongs(&packs[3], 1, fan_table);

        // ���ݷ���������漰���֣�Ȧ��̡��ŷ��
        adjust_by_winds(pack_get_tile(packs[3]), prevalent_wind, seat_wind, fan_table);
    }

    adjust_by_win_flag(win_flag, fan_table);
    // ��ǰ�壨���ܲ�Ӱ�죩
    if (fixed_cnt == 0 || (pack_get_type(packs[3]) == PACK_TYPE_KONG && !is_pack_melded(packs[3]))) {
        if (win_flag & WIN_FLAG_SELF_DRAWN) {
            fan_table[FULLY_CONCEALED_HAND] = 1;
        }
        else {
            fan_table[CONCEALED_HAND] = 1;
        }
    }

    // ��ԭ��
    tile_t tiles[15];  // ���������Ϊ�ܣ��������Ϊ15��
    memcpy(tiles, matched_seq, sizeof(*matched_seq));  // ������Ĳ���
    intptr_t tile_cnt = packs_to_tiles(&packs[3], 2, tiles + 9, 6);  // һ������+һ��ȸͷ ���6����
    tile_cnt += 9;

    // ���ݻ�ɫ���������漰���֣����֡�ȱһ�š���һɫ����һɫ��������
    adjust_by_suits(tiles, tile_cnt, fan_table);
    // �����Լ��������Ͳ���Ҫ�����ˣ���������Ĵ��ھ��Բ����ܴ���ȫ���ۡ�ȫ���塢ȫ˫�̣����ۡ��Ʋ�������һɫ����һɫ�����۾š����۾�
    // �Ĺ�һ����
    adjust_by_tiles_hog(tiles, tile_cnt, fan_table);

    // ���������������Χ���ƣ����Ʊ��š�Ƕ�š�������
    if (std::none_of(std::begin(*matched_seq), std::end(*matched_seq), [win_tile](tile_t t) { return t == win_tile; })) {
        if (fixed_cnt == 0) {  // ��������п��ܴ��ڱ��š�Ƕ�š�������
            // ����ȥ������Ĳ��ָֻ�����
            --cnt_table[win_tile];
            tile_t temp[4];
            intptr_t cnt = table_to_tiles(cnt_table, temp, 4);

            // �������Ʒ�ʽ���������漰���֣����š�Ƕ�š�������
            adjust_by_waiting_form(packs + 3, 2, temp, cnt, win_tile, fan_table);
        }
        else {
            // ������״̬������Ʋ����������Χ�ڣ���Ȼ�ǵ�����
            fan_table[SINGLE_WAIT] = 1;
        }
    }

    // ͳһ����һЩ���Ƶ�
    adjust_fan_table(fan_table, prevalent_wind == seat_wind);
    return true;
}

// ʮ����
static forceinline bool is_thirteen_orphans(const tile_t (&tiles)[14]) {
    return std::all_of(std::begin(tiles), std::end(tiles), &is_terminal_or_honor)
        && std::includes(std::begin(tiles), std::end(tiles),
        std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans));
}

// ȫ����/���ǲ����㷬
static bool calculate_honors_and_knitted_tiles(const tile_t (&standing_tiles)[14], fan_table_t &fan_table) {
    const tile_t *honor_begin = std::find_if(std::begin(standing_tiles), std::end(standing_tiles), &is_honor);
    int numbered_cnt = static_cast<int>(honor_begin - standing_tiles);
    // ������������9����С��7��Ȼ��������ȫ����
    if (numbered_cnt > 9 || numbered_cnt < 7) {
        return false;
    }

    // ƥ�������
    if (std::none_of(&standard_knitted_straight[0], &standard_knitted_straight[6],
        [&standing_tiles, honor_begin](const tile_t (&seq)[9]) {
        return std::includes(std::begin(seq), std::end(seq), std::begin(standing_tiles), honor_begin);
    })) {
        return false;
    }

    if (numbered_cnt == 7 && std::equal(std::begin(standard_thirteen_orphans) + 6, std::end(standard_thirteen_orphans), standing_tiles + 7)) {
        // ���������룬Ϊ���ǲ���
        fan_table[GREATER_HONORS_AND_KNITTED_TILES] = 1;
        return true;
    }
    else if (std::includes(std::begin(standard_thirteen_orphans) + 6, std::end(standard_thirteen_orphans), honor_begin, std::end(standing_tiles))) {
        // ȫ����
        fan_table[LESSER_HONORS_AND_KNITTED_TILES] = 1;
        if (numbered_cnt == 9) {  // ��9�����ƣ�Ϊ���������ȫ����
            fan_table[KNITTED_STRAIGHT] = 1;
        }
        return true;
    }

    return false;
}

// ��������㷬
static bool calculate_special_form_fan(const tile_t (&standing_tiles)[14], win_flag_t win_flag, fan_table_t &fan_table) {
    // �߶�
    if (standing_tiles[0] == standing_tiles[1]
        && standing_tiles[2] == standing_tiles[3]
        && standing_tiles[4] == standing_tiles[5]
        && standing_tiles[6] == standing_tiles[7]
        && standing_tiles[8] == standing_tiles[9]
        && standing_tiles[10] == standing_tiles[11]
        && standing_tiles[12] == standing_tiles[13]) {

        if (is_numbered_suit_quick(standing_tiles[0])
            && standing_tiles[0] + 1 == standing_tiles[2]
            && standing_tiles[2] + 1 == standing_tiles[4]
            && standing_tiles[4] + 1 == standing_tiles[6]
            && standing_tiles[6] + 1 == standing_tiles[8]
            && standing_tiles[8] + 1 == standing_tiles[10]
            && standing_tiles[10] + 1 == standing_tiles[12]) {
            // ���߶�
            fan_table[SEVEN_SHIFTED_PAIRS] = 1;
            adjust_by_tiles_traits(standing_tiles, 14, fan_table);
        }
        else {
            // ��ͨ�߶�
            fan_table[SEVEN_PAIRS] = 1;

            // ���ݻ�ɫ���������漰���֣����֡�ȱһ�š���һɫ����һɫ��������
            adjust_by_suits(standing_tiles, 14, fan_table);
            // ���������Ե��������漰���֣����ۡ��Ʋ�������һɫ����һɫ�����۾š����۾�
            adjust_by_tiles_traits(standing_tiles, 14, fan_table);
            // �������Ƶķ�Χ���������漰���֣������塢С���塢ȫ��ȫ�С�ȫС
            adjust_by_rank_range(standing_tiles, 14, fan_table);
            // �Ĺ�һ����
            adjust_by_tiles_hog(standing_tiles, 14, fan_table);
        }
    }
    // ʮ����
    else if (is_thirteen_orphans(standing_tiles)) {
        fan_table[THIRTEEN_ORPHANS] = 1;
    }
    // ȫ����/���ǲ���
    else if (calculate_honors_and_knitted_tiles(standing_tiles, fan_table)) {
    }
    else {
        return false;
    }

    adjust_by_win_flag(win_flag, fan_table);
    // ���ݷ������û��Ҫ�ˣ���Щ������Ͷ�û�����ӣ�������Ȧ��̡��ŷ��
    // ͳһ����һЩ���Ƶ�
    adjust_fan_table(fan_table, false);
    return true;
}

// �ӷ�����㷬��
static int get_fan_by_table(const fan_table_t &fan_table) {
    int fan = 0;
    for (int i = 1; i < FAN_TABLE_SIZE; ++i) {
        if (fan_table[i] == 0) {
            continue;
        }
        fan += fan_value_table[i] * fan_table[i];
#if 0  // Debug
        if (fan_table[i] == 1) {
            LOG("%s %hu\n", fan_name[i], fan_value_table[i]);
        }
        else {
            LOG("%s %hu*%hu\n", fan_name[i], fan_value_table[i], fan_table[i]);
        }
#endif
    }
    return fan;
}

// �ж������Ƿ��������
bool is_standing_tiles_contains_win_tile(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t win_tile) {
    return std::any_of(standing_tiles, standing_tiles + standing_cnt,
        [win_tile](tile_t tile) { return tile == win_tile; });
}

// ͳ�ƺ����ڸ�¶�����г��ֵ�����
size_t count_win_tile_in_fixed_packs(const pack_t *fixed_packs, intptr_t fixed_cnt, tile_t win_tile) {
    tile_table_t cnt_table = { 0 };
    for (intptr_t i = 0; i < fixed_cnt; ++i) {
        pack_t pack = fixed_packs[i];
        tile_t tile = pack_get_tile(pack);
        switch (pack_get_type(pack)) {
        case PACK_TYPE_CHOW: ++cnt_table[tile - 1]; ++cnt_table[tile]; ++cnt_table[tile + 1]; break;
        case PACK_TYPE_PUNG: cnt_table[tile] += 3; break;
        case PACK_TYPE_KONG: cnt_table[tile] += 4; break;
        default: break;
        }
    }
    return cnt_table[win_tile];
}

// �жϸ�¶�����Ƿ������
bool is_fixed_packs_contains_kong(const pack_t *fixed_packs, intptr_t fixed_cnt) {
    return std::any_of(fixed_packs, fixed_packs + fixed_cnt,
        [](pack_t pack) { return pack_get_type(pack) == PACK_TYPE_KONG; });
}

// ����㷬�������Ƿ�Ϸ�
int check_calculator_input(const hand_tiles_t *hand_tiles, tile_t win_tile) {
    // ���
    tile_table_t cnt_table;
    if (!map_hand_tiles(hand_tiles, &cnt_table)) {
        return ERROR_WRONG_TILES_COUNT;
    }
    if (win_tile != 0) {
        ++cnt_table[win_tile];
    }

    // ���ĳ���Ƴ���4
    if (std::any_of(std::begin(cnt_table), std::end(cnt_table), [](int cnt) { return cnt > 4; })) {
        return ERROR_TILE_COUNT_GREATER_THAN_4;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// �㷬
//
MODIFIER int calculate_fan(calculate_param_t *calculate_param, fan_table_t *fan_table) {
    const hand_tiles_t *hand_tiles = &calculate_param->hand_tiles;
    tile_t win_tile = calculate_param->win_tile;
    win_flag_t win_flag = calculate_param->win_flag;

    if (int ret = check_calculator_input(hand_tiles, win_tile)) {
        return ret;
    }

    intptr_t fixed_cnt = hand_tiles->pack_count;
    intptr_t standing_cnt = hand_tiles->tile_count;

    // У�����Ʊ��
    // ������ư������ƣ����Ȼ���Ǻ;���
    const bool standing_tiles_contains_win_tile = is_standing_tiles_contains_win_tile(hand_tiles->standing_tiles, standing_cnt, win_tile);
    if (standing_tiles_contains_win_tile) {
        win_flag &= ~WIN_FLAG_4TH_TILE;
    }

    // ��������ڸ�¶�г���3�ţ����ȻΪ�;���
    const size_t win_tile_in_fixed_packs = count_win_tile_in_fixed_packs(hand_tiles->fixed_packs, fixed_cnt, win_tile);
    if (3 == win_tile_in_fixed_packs) {
        win_flag |= WIN_FLAG_4TH_TILE;
    }

    // ���Ӹܱ��
    if (win_flag & WIN_FLAG_ABOUT_KONG) {
        if (win_flag & WIN_FLAG_SELF_DRAWN) {  // ����
            // �������û�иܣ����Ȼ���Ǹ��Ͽ���
            if (!is_fixed_packs_contains_kong(hand_tiles->fixed_packs, fixed_cnt)) {
                win_flag &= ~WIN_FLAG_ABOUT_KONG;
            }
        }
        else {  // ���
            // ������������Ʒ�Χ�ڳ��ֹ������Ȼ�������ܺ�
            if (win_tile_in_fixed_packs > 0 || standing_tiles_contains_win_tile) {
                win_flag &= ~WIN_FLAG_ABOUT_KONG;
            }
        }
    }
	calculate_param->win_flag = win_flag;

    // �ϲ���������ƣ����������Ϊ14��
    tile_t standing_tiles[14];
    memcpy(standing_tiles, hand_tiles->standing_tiles, standing_cnt * sizeof(tile_t));
    standing_tiles[standing_cnt] = win_tile;
    std::sort(standing_tiles, standing_tiles + standing_cnt + 1);

    // ��󷬱��
    int max_fan = 0;
    const fan_table_t *selected_fan_table = nullptr;

    // ������͵ķ�
    fan_table_t special_fan_table = { 0 };

    // ���жϸ����������
    if (fixed_cnt == 0) {  // ����״̬���п����ǻ������������
        if (calculate_knitted_straight_fan(calculate_param, special_fan_table)) {
            max_fan = get_fan_by_table(special_fan_table);
            selected_fan_table = &special_fan_table;
            LOG("fan = %d\n\n", max_fan);
        }
        else if (calculate_special_form_fan(standing_tiles, win_flag, special_fan_table)) {
            max_fan = get_fan_by_table(special_fan_table);
            selected_fan_table = &special_fan_table;
            LOG("fan = %d\n\n", max_fan);
        }
    }
    else if (fixed_cnt == 1) {  // 1��¶״̬���п����ǻ������������
        if (calculate_knitted_straight_fan(calculate_param, special_fan_table)) {
            max_fan = get_fan_by_table(special_fan_table);
            selected_fan_table = &special_fan_table;
            LOG("fan = %d\n\n", max_fan);
        }
    }

    // �޷�����������ͻ���Ϊ�߶�
    // �߶�ҲҪ���������ͻ��֣���Ϊ��������£��������͵ķ��ᳬ���߶Եķ�
    if (selected_fan_table == nullptr || special_fan_table[SEVEN_PAIRS] == 1) {
        // ����
        division_result_t result;
        if (divide_win_hand(standing_tiles, hand_tiles->fixed_packs, fixed_cnt, &result)) {
            fan_table_t fan_tables[MAX_DIVISION_CNT] = { { 0 } };

            // �������ֻ��ַ�ʽ���ֱ��㷬���ҳ����ķ��Ļ��ַ�ʽ
            for (intptr_t i = 0; i < result.count; ++i) {
#if 0  // Debug
                char str[64];
                packs_to_string(result.divisions[i].packs, 5, str, sizeof(str));
                puts(str);
#endif
                calculate_basic_form_fan(result.divisions[i].packs, calculate_param, fan_tables[i]);
                int current_fan = get_fan_by_table(fan_tables[i]);
                if (current_fan > max_fan) {
                    max_fan = current_fan;
                    selected_fan_table = &fan_tables[i];
                }
                LOG("fan = %d\n\n", current_fan);
            }
        }
    }

    if (selected_fan_table == nullptr) {
        return ERROR_NOT_WIN;
    }

    // �ӻ���
    max_fan += calculate_param->flower_count;

    if (fan_table != nullptr) {
        memcpy(*fan_table, *selected_fan_table, sizeof(*fan_table));
        (*fan_table)[FLOWER_TILES] = calculate_param->flower_count;
    }

    return max_fan;
}

}
