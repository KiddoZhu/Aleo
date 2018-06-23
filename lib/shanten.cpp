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
#include <string.h>
#include <limits>
#include <algorithm>
#include <iterator>

#ifndef _BOTZONE_ONLINE
#include "shanten.h"
#include "standard_tiles.h"
#endif

namespace mahjong {

// ����ת������
intptr_t packs_to_tiles(const pack_t *packs, intptr_t pack_cnt, tile_t *tiles, intptr_t tile_cnt) {
    if (packs == nullptr || tiles == nullptr) {
        return 0;
    }

    intptr_t cnt = 0;
    for (int i = 0; i < pack_cnt && cnt < tile_cnt; ++i) {
        tile_t tile = pack_get_tile(packs[i]);
        switch (pack_get_type(packs[i])) {
        case PACK_TYPE_CHOW:
            if (cnt < tile_cnt) tiles[cnt++] = static_cast<tile_t>(tile - 1);
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = static_cast<tile_t>(tile + 1);
            break;
        case PACK_TYPE_PUNG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_KONG:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        case PACK_TYPE_PAIR:
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            if (cnt < tile_cnt) tiles[cnt++] = tile;
            break;
        default:
            break;
        }
    }
    return cnt;
}

// ���ƴ��
void map_tiles(const tile_t *tiles, intptr_t cnt, tile_table_t *cnt_table) {
    memset(*cnt_table, 0, sizeof(*cnt_table));
    for (intptr_t i = 0; i < cnt; ++i) {
        ++(*cnt_table)[tiles[i]];
    }
}

// �����ƴ��
bool map_hand_tiles(const hand_tiles_t *hand_tiles, tile_table_t *cnt_table) {
    // ��ÿһ�鸱¶����3�������㣬��ô������=13
    if (hand_tiles->tile_count <= 0 || hand_tiles->pack_count < 0 || hand_tiles->pack_count > 4
        || hand_tiles->pack_count * 3 + hand_tiles->tile_count != 13) {
        return false;
    }

    // ����¶�ָ�����
    tile_t tiles[18];
    intptr_t tile_cnt = 0;
    if (hand_tiles->pack_count == 0) {
        memcpy(tiles, hand_tiles->standing_tiles, 13 * sizeof(tile_t));
        tile_cnt = 13;
    }
    else {
        tile_cnt = packs_to_tiles(hand_tiles->fixed_packs, hand_tiles->pack_count, tiles, 18);
        memcpy(tiles + tile_cnt, hand_tiles->standing_tiles, hand_tiles->tile_count * sizeof(tile_t));
        tile_cnt += hand_tiles->tile_count;
    }

    // ���
    map_tiles(tiles, tile_cnt, cnt_table);
    return true;
}

// ����ת������
intptr_t table_to_tiles(const tile_table_t &cnt_table, tile_t *tiles, intptr_t max_cnt) {
    intptr_t cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        for (int n = 0; n < cnt_table[t]; ++n) {
            *tiles++ = t;
            ++cnt;
            if (cnt == max_cnt) {
                return max_cnt;
            }
        }
    }
    return cnt;
}

// ������Ч��ö��
int count_useful_tile(const tile_table_t &used_table, const useful_table_t &useful_table) {
    int cnt = 0;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (useful_table[t]) {
            cnt += 4 - used_table[t];
        }
    }
    return cnt;
}

namespace {

    // ·����Ԫ����Ԫ�����ӡ�ȸͷ�����ӵ����࣬������ĺ�
    // ��8λ��ʾ���ͣ���8λ��ʾ��
    // ����˳�Ӻ�˳�Ӵ��ӣ���ָ������С��һ���ƣ�
    // ������˳��123���У���Ϊ1�����������45���У���Ϊ4���ȵ�
    typedef uint16_t path_unit_t;

#define UNIT_TYPE_CHOW 1                // ˳��
#define UNIT_TYPE_PUNG 2                // ����
#define UNIT_TYPE_PAIR 4                // ȸͷ
#define UNIT_TYPE_CHOW_OPEN_END 5       // ������߱��Ŵ���
#define UNIT_TYPE_CHOW_CLOSED 6         // Ƕ�Ŵ���
#define UNIT_TYPE_INCOMPLETE_PUNG 7     // ���Ӵ���

#define MAKE_UNIT(type_, tile_) static_cast<path_unit_t>(((type_) << 8) | (tile_))
#define UNIT_TYPE(unit_) (((unit_) >> 8) & 0xFF)
#define UNIT_TILE(unit_) ((unit_) & 0xFF)

#define MAX_STATE 512
#define UNIT_SIZE 7

    // һ��·��
    struct work_path_t {
        path_unit_t units[UNIT_SIZE];  // 14/2=7���7������
        uint16_t depth;  // ��ǰ·�����
    };

    // ��ǰ����״̬
    struct work_state_t {
        work_path_t paths[MAX_STATE];  // ����·��
        intptr_t count;  // ·������
    };
}

// ·���Ƿ�������
static bool is_basic_form_branch_exist(const intptr_t fixed_cnt, const work_path_t *work_path, const work_state_t *work_state) {
    if (work_state->count <= 0 || work_path->depth == 0) {
        return false;
    }

    // depth������Ϣ�����԰�stl����endӦ��Ҫ+1
    const uint16_t depth = static_cast<uint16_t>(work_path->depth + 1);

    // std::includesҪ�����򣬵��ֲ����ƻ���ǰ����
    work_path_t temp;
    std::copy(&work_path->units[fixed_cnt], &work_path->units[depth], &temp.units[fixed_cnt]);
    std::sort(&temp.units[fixed_cnt], &temp.units[depth]);

    return std::any_of(&work_state->paths[0], &work_state->paths[work_state->count],
        [&temp, fixed_cnt, depth](const work_path_t &path) {
        return std::includes(&path.units[fixed_cnt], &path.units[path.depth], &temp.units[fixed_cnt], &temp.units[depth]);
    });
}

// ����·��
static void save_work_path(const intptr_t fixed_cnt, const work_path_t *work_path, work_state_t *work_state) {
    if (work_state->count < MAX_STATE) {
        work_path_t &path = work_state->paths[work_state->count++];
        path.depth = work_path->depth;
        std::copy(&work_path->units[fixed_cnt], &work_path->units[work_path->depth], &path.units[fixed_cnt]);

        // ����Ƿ��ظ�·��ʱ��std::includesҪ�������������ｫ������
        std::sort(&path.units[fixed_cnt], &path.units[path.depth]);
    }
    else {
        assert(0 && "too many state!");
    }
}

// �ݹ�����������������
// ����˵����
//   cnt_table�Ʊ�
//   has_pair�Ƿ���ȸͷ
//   pack_cnt��ɵ�������
//   incomplete_cnt������
// �����������Ϊ�Ż������õģ�
// work_path���浱ǰ���ڼ����·����
// work_state�����������Ѿ��������·����
// ��0��fixed_cnt�������ǲ�ʹ�õģ���Щ�������˸�¶������
static int basic_form_shanten_recursively(tile_table_t &cnt_table, const bool has_pair, const unsigned pack_cnt, const unsigned incomplete_cnt,
    const intptr_t fixed_cnt, work_path_t *work_path, work_state_t *work_state) {
    if (pack_cnt == 4) {  // �Ѿ���4������
        return has_pair ? -1 : 0;  // �����ȸͷ������ˣ������ȸͷ����������
    }

    int max_ret;  // ��ǰ״̬�ܷ��ص����������

    // �㷨˵����
    // ȱ�ٵ�������=4-��ɵ�������
    // ȱ�ٵĴ�����=ȱ�ٵ�������-���еĴ�����
    // ��ʽ�ϲ���ȱ�ٵĴ�����=4-��ɵ�������-���еĴ�����
    int incomplete_need = 4 - pack_cnt - incomplete_cnt;
    if (incomplete_need > 0) {  // ����Ҫ���ӵ����
        // ��ȸͷʱ��������=���еĴ�����+ȱ�ٵĴ�����*2-1
        // ��ȸͷʱ��������=���еĴ�����+ȱ�ٵĴ�����*2
        max_ret = incomplete_cnt + incomplete_need * 2 - (has_pair ? 1 : 0);
    }
    else {  // �������˵����
        // ��ȸͷʱ��������=3-��ɵ�������
        // ��ȸͷʱ��������=4-��ɵ�������
        max_ret = (has_pair ? 3 : 4) - pack_cnt;
    }

    // ��ǰ·�����
    const unsigned depth = pack_cnt + incomplete_cnt + has_pair;
    work_path->depth = static_cast<uint16_t>(depth);

    int result = max_ret;

    if (pack_cnt + incomplete_cnt > 4) {  // ���ӳ���
        save_work_path(fixed_cnt, work_path, work_state);
        return max_ret;
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // ȸͷ
        if (!has_pair && cnt_table[t] > 1) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PAIR, t);  // ��¼ȸͷ
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // ����ȸͷ���ݹ�
                cnt_table[t] -= 2;
                int ret = basic_form_shanten_recursively(cnt_table, true, pack_cnt, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // ��ԭ
                cnt_table[t] += 2;
            }
        }

        // ����
        if (cnt_table[t] > 2) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_PUNG, t);  // ��¼����
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // ����������ӣ��ݹ�
                cnt_table[t] -= 3;
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt + 1, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // ��ԭ
                cnt_table[t] += 3;
            }
        }

        // ˳�ӣ�ֻ�������ƣ�
        bool is_numbered = is_numbered_suit(t);
        // ˳��t t+1 t+2����Ȼt������8�����ϵ�����
        if (is_numbered && tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW, t);  // ��¼˳��
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // ��������˳�ӣ��ݹ�
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt + 1, incomplete_cnt,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // ��ԭ
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
            }
        }

        // ����Ѿ�ͨ������ȸͷ/���ӽ��������������ٰ����Ӽ�����������϶��������
        if (result < max_ret) {
            continue;
        }

        // ���Ӵ���
        if (cnt_table[t] > 1) {
            work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_INCOMPLETE_PUNG, t);  // ��¼���Ӵ���
            if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                // �������Ӵ��ӣ��ݹ�
                cnt_table[t] -= 2;
                int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                    fixed_cnt, work_path, work_state);
                result = std::min(ret, result);
                // ��ԭ
                cnt_table[t] += 2;
            }
        }

        // ˳�Ӵ��ӣ�ֻ�������ƣ�
        if (is_numbered) {
            // ������߱��Ŵ���t t+1����Ȼt������9�����ϵ�����
            if (tile_get_rank(t) < 9 && cnt_table[t + 1]) {  // ������߱���
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW_OPEN_END, t);  // ��¼������߱��Ŵ���
                if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                    // �������ӣ��ݹ�
                    --cnt_table[t];
                    --cnt_table[t + 1];
                    int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                        fixed_cnt, work_path, work_state);
                    result = std::min(ret, result);
                    // ��ԭ
                    ++cnt_table[t];
                    ++cnt_table[t + 1];
                }
            }
            // Ƕ�Ŵ���t t+2����Ȼt������8�����ϵ�����
            if (tile_get_rank(t) < 8 && cnt_table[t + 2]) {  // Ƕ��
                work_path->units[depth] = MAKE_UNIT(UNIT_TYPE_CHOW_CLOSED, t);  // ��¼Ƕ�Ŵ���
                if (!is_basic_form_branch_exist(fixed_cnt, work_path, work_state)) {
                    // �������ӣ��ݹ�
                    --cnt_table[t];
                    --cnt_table[t + 2];
                    int ret = basic_form_shanten_recursively(cnt_table, has_pair, pack_cnt, incomplete_cnt + 1,
                        fixed_cnt, work_path, work_state);
                    result = std::min(ret, result);
                    // ��ԭ
                    ++cnt_table[t];
                    ++cnt_table[t + 2];
                }
            }
        }
    }

    if (result == max_ret) {
        save_work_path(fixed_cnt, work_path, work_state);
    }

    return result;
}

// �����Ƿ��д���
static bool numbered_tile_has_neighbor(const tile_table_t &cnt_table, tile_t t) {
    rank_t r = tile_get_rank(t);
    if (r < 9) { if (cnt_table[t + 1]) return true; }
    if (r < 8) { if (cnt_table[t + 2]) return true; }
    if (r > 1) { if (cnt_table[t - 1]) return true; }
    if (r > 2) { if (cnt_table[t - 2]) return true; }
    return false;
}

// �Ա��Ϊ���������������������
static int basic_form_shanten_from_table(tile_table_t &cnt_table, intptr_t fixed_cnt, useful_table_t *useful_table) {
    // ����������
    work_path_t work_path;
    work_state_t work_state;
    work_state.count = 0;
    int result = basic_form_shanten_recursively(cnt_table, false, static_cast<uint16_t>(fixed_cnt), 0,
        fixed_cnt, &work_path, &work_state);

    if (useful_table == nullptr) {
        return result;
    }

    // ������е��ƣ���ȡ�ܼ�������������
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] == 4 && result > 0) {
            continue;
        }

        if (cnt_table[t] == 0) {
            // �����������ƺͲ����ŵ����ƣ���Щ�ƶ��޷�����������
            if (is_honor(t) || !numbered_tile_has_neighbor(cnt_table, t)) {
                continue;
            }
        }

        ++cnt_table[t];
        work_state.count = 0;
        int temp = basic_form_shanten_recursively(cnt_table, false, static_cast<uint16_t>(fixed_cnt), 0,
            fixed_cnt, &work_path, &work_state);
        if (temp < result) {
            (*useful_table)[t] = true;  // ���Ϊ��Ч��
        }
        --cnt_table[t];
    }

    return result;
}

// ��������������
int basic_form_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13
        && standing_cnt != 10 && standing_cnt != 7 && standing_cnt != 4 && standing_cnt != 1)) {
        return std::numeric_limits<int>::max();
    }

    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));
    }
    return basic_form_shanten_from_table(cnt_table, (13 - standing_cnt) / 3, useful_table);
}

// ���������ж�1���Ƿ�����
static bool is_basic_form_wait_1(tile_table_t &cnt_table, useful_table_t *waiting_table) {
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] != 1) {
            continue;
        }

        // ������
        cnt_table[t] = 0;
        if (std::all_of(std::begin(cnt_table), std::end(cnt_table), [](int n) { return n == 0; })) {
            cnt_table[t] = 1;
            if (waiting_table != nullptr) {  // ����Ҫ��ȡ�����ţ������ֱ�ӷ���
                (*waiting_table)[t] = true;
            }
            return true;
        }
        cnt_table[t] = 1;
    }

    return false;
}

// ���������ж�2���Ƿ�����
static bool is_basic_form_wait_2(const tile_table_t &cnt_table, useful_table_t *waiting_table) {
    bool ret = false;
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }
        if (cnt_table[t] > 1) {
            if (waiting_table != nullptr) {  // ��ȡ������
                (*waiting_table)[t] = true;  // �Ե�
                ret = true;
                continue;
            }
            else {  // ����Ҫ��ȡ�����ţ������ֱ�ӷ���
                return true;
            }
        }
        if (is_numbered_suit_quick(t)) {  // ���ƴ���
            rank_t r = tile_get_rank(t);
            if (r > 1 && cnt_table[t - 1]) {  // ������߱���
                if (waiting_table != nullptr) {  // ��ȡ������
                    if (r < 9) (*waiting_table)[t + 1] = true;
                    if (r > 2) (*waiting_table)[t - 2] = true;
                    ret = true;
                    continue;
                }
                else {  // ����Ҫ��ȡ�����ţ������ֱ�ӷ���
                    return true;
                }
            }
            if (r > 2 && cnt_table[t - 2]) {  // Ƕ��
                if (waiting_table != nullptr) {  // ��ȡ������
                    (*waiting_table)[t - 1] = true;
                    ret = true;
                    continue;
                }
                else {  // ����Ҫ��ȡ�����ţ������ֱ�ӷ���
                    return true;
                }
            }
        }
    }
    return ret;
}

// ���������ж�4���Ƿ�����
static bool is_basic_form_wait_4(tile_table_t &cnt_table, useful_table_t *waiting_table) {
    bool ret = false;
    // ����ȸͷ
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 2) {
            continue;
        }
        // ����ȸͷ���ݹ�
        cnt_table[t] -= 2;
        if (is_basic_form_wait_2(cnt_table, waiting_table)) {
            ret = true;
        }
        // ��ԭ
        cnt_table[t] += 2;
        if (ret && waiting_table == nullptr) {  // ����Ҫ��ȡ�����ţ������ֱ�ӽ����ݹ�
            return true;
        }
    }

    return ret;
}

// �ݹ������������Ƿ�����
static bool is_basic_form_wait_recursively(tile_table_t &cnt_table, intptr_t left_cnt, useful_table_t *waiting_table) {
    if (left_cnt == 1) {
        return is_basic_form_wait_1(cnt_table, waiting_table);
    }

    bool ret = false;
    if (left_cnt == 4) {
        ret = is_basic_form_wait_4(cnt_table, waiting_table);
        if (ret && waiting_table == nullptr) {  // ����Ҫ��ȡ�����ţ������ֱ�ӽ����ݹ�
            return true;
        }
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // ����
        if (cnt_table[t] > 2) {
            // ����������ӣ��ݹ�
            cnt_table[t] -= 3;
            if (is_basic_form_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                ret = true;
            }
            // ��ԭ
            cnt_table[t] += 3;
            if (ret && waiting_table == nullptr) {  // ����Ҫ��ȡ�����ţ������ֱ�ӽ����ݹ�
                return true;
            }
        }

        // ˳�ӣ�ֻ�������ƣ�
        if (is_numbered_suit(t)) {
            // ˳��t t+1 t+2����Ȼt������8�����ϵ�����
            if (tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                // ��������˳�ӣ��ݹ�
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                if (is_basic_form_wait_recursively(cnt_table, left_cnt - 3, waiting_table)) {
                    ret = true;
                }
                // ��ԭ
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
                if (ret && waiting_table == nullptr) {  // ����Ҫ��ȡ�����ţ������ֱ�ӽ����ݹ�
                    return true;
                }
            }
        }
    }

    return ret;
}

// ���������Ƿ�����
// ����֮���Բ���ֱ�ӵ������������㺯�����ж��䷵��ֵΪ0�ķ�ʽ
// ����Ϊǰ�߻��������ӣ���������ں����ж�����û��Ҫ�ģ����Ե���дһ�׸����߼�
bool is_basic_form_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    if (waiting_table != nullptr) {
        memset(*waiting_table, 0, sizeof(*waiting_table));
    }
    return is_basic_form_wait_recursively(cnt_table, standing_cnt, waiting_table);
}

// ��������2���ܷ����
static bool is_basic_form_win_2(const tile_table_t &cnt_table) {
    // �ҵ�δʹ�õ���
    typedef std::remove_all_extents<tile_table_t>::type table_elem_t;
    const table_elem_t *it = std::find_if(std::begin(cnt_table), std::end(cnt_table), [](table_elem_t n) { return n > 0; });
    // ��������������2
    if (it == std::end(cnt_table) || *it != 2) {
        return false;
    }
    // ��������δʹ�õ���
    return std::none_of(it + 1, std::end(cnt_table), [](int n) { return n > 0; });
}

// �ݹ������������Ƿ����
// ����֮���Բ���ֱ�ӵ������������㺯�����ж��䷵��ֵΪ-1�ķ�ʽ��
// ����Ϊǰ�߻��������ӣ���������ں����ж�����û��Ҫ�ģ����Ե���дһ�׸����߼�
static bool is_basic_form_win_recursively(tile_table_t &cnt_table, intptr_t left_cnt) {
    if (left_cnt == 2) {
        return is_basic_form_win_2(cnt_table);
    }

    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] < 1) {
            continue;
        }

        // ����
        if (cnt_table[t] > 2) {
            // ����������ӣ��ݹ�
            cnt_table[t] -= 3;
            bool ret = is_basic_form_win_recursively(cnt_table, left_cnt - 3);
            // ��ԭ
            cnt_table[t] += 3;
            if (ret) {
                return true;
            }
        }

        // ˳�ӣ�ֻ�������ƣ�
        if (is_numbered_suit(t)) {
            // ˳��t t+1 t+2����Ȼt������8�����ϵ�����
            if (tile_get_rank(t) < 8 && cnt_table[t + 1] && cnt_table[t + 2]) {
                // ��������˳�ӣ��ݹ�
                --cnt_table[t];
                --cnt_table[t + 1];
                --cnt_table[t + 2];
                bool ret = is_basic_form_win_recursively(cnt_table, left_cnt - 3);
                // ��ԭ
                ++cnt_table[t];
                ++cnt_table[t + 1];
                ++cnt_table[t + 2];
                if (ret) {
                    return true;
                }
            }
        }
    }

    return false;
}

// ���������Ƿ����
bool is_basic_form_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);
    ++cnt_table[test_tile];  // ��Ӳ��Ե���
    return is_basic_form_win_recursively(cnt_table, standing_cnt + 1);
}

//-------------------------------- �߶� --------------------------------

// �߶�������
int seven_pairs_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // ���Ƶ�������д����ͳ�ƶ�����
    int pair_cnt = 0;
    tile_table_t cnt_table = { 0 };
    for (intptr_t i = 0; i < standing_cnt; ++i) {
        tile_t tile = standing_tiles[i];
        ++cnt_table[tile];
        if (cnt_table[tile] == 2) {
            ++pair_cnt;
            cnt_table[tile] = 0;
        }
    }

    // ��Ч��
    if (useful_table != nullptr) {
        std::transform(std::begin(cnt_table), std::end(cnt_table), std::begin(*useful_table), [](int n) { return n != 0; });
    }
    return 6 - pair_cnt;
}

// �߶��Ƿ�����
bool is_seven_pairs_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // ֱ�Ӽ�������������������Ϊ0��Ϊ����
    if (waiting_table == nullptr) {
        return (0 == seven_pairs_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == seven_pairs_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// �߶��Ƿ����
bool is_seven_pairs_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    return (0 == seven_pairs_shanten(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

//-------------------------------- ʮ���� --------------------------------

// ʮ����������
int thirteen_orphans_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // ���Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    bool has_pair = false;
    int cnt = 0;
    for (int i = 0; i < 13; ++i) {
        int n = cnt_table[standard_thirteen_orphans[i]];
        if (n > 0) {
            ++cnt;  // �۾��Ƶ�����
            if (n > 1) {
                has_pair = true;  // �۾��ƶ���
            }
        }
    }

    // ���ж���ʱ��������Ϊ��12-�۾��Ƶ�����
    // ��û�ж���ʱ��������Ϊ��13-�۾��Ƶ�����
    int ret = has_pair ? 12 - cnt : 13 - cnt;

    if (useful_table != nullptr) {
        // �ȱ�����е��۾���Ϊ��Ч��
        memset(*useful_table, 0, sizeof(*useful_table));
        std::for_each(std::begin(standard_thirteen_orphans), std::end(standard_thirteen_orphans),
            [useful_table](tile_t t) {
            (*useful_table)[t] = true;
        });

        // ���ж���ʱ�����е��۾��ƶ�����Ҫ��
        if (has_pair) {
            for (int i = 0; i < 13; ++i) {
                tile_t t = standard_thirteen_orphans[i];
                int n = cnt_table[t];
                if (n > 0) {
                    (*useful_table)[t] = false;
                }
            }
        }
    }

    return ret;
}

// ʮ�����Ƿ�����
bool is_thirteen_orphans_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // ֱ�Ӽ�������������������Ϊ0��Ϊ����
    if (waiting_table == nullptr) {
        return (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// ʮ�����Ƿ����
bool is_thirteen_orphans_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    return (0 == thirteen_orphans_shanten(standing_tiles, standing_cnt, &useful_table)
        && useful_table[test_tile]);
}

//-------------------------------- �������+����+ȸͷ������ --------------------------------

// �Ա��Ϊ��������������Ƿ�����
static bool is_knitted_straight_wait_from_table(const tile_table_t &cnt_table, intptr_t left_cnt, useful_table_t *waiting_table) {
    // ƥ�������
    const tile_t (*matched_seq)[9] = nullptr;
    tile_t missing_tiles[9];
    int missing_cnt = 0;
    for (int i = 0; i < 6; ++i) {  // ������������
        missing_cnt = 0;
        for (int k = 0; k < 9; ++k) {
            tile_t t = standard_knitted_straight[i][k];
            if (cnt_table[t] == 0) {  // ȱʧ��
                missing_tiles[missing_cnt++] = t;
            }
        }
        if (missing_cnt < 2) {  // ȱ2�Ż����ϵĿ϶�û��
            matched_seq = &standard_knitted_straight[i];
            break;
        }
    }

    if (matched_seq == nullptr || missing_cnt > 2) {
        return false;
    }

    // �޳������
    tile_table_t temp_table;
    memcpy(&temp_table, &cnt_table, sizeof(temp_table));
    for (int i = 0; i < 9; ++i) {
        tile_t t = (*matched_seq)[i];
        if (temp_table[t]) {
            --temp_table[t];
        }
    }

    if (missing_cnt == 1) {  // ���ȱһ�ţ���ô��ȥ�����֮�����Ӧ�������״̬��������
        if (left_cnt == 10) {
            if (is_basic_form_win_recursively(temp_table, 2)) {
                if (waiting_table != nullptr) {  // ��ȡ�����ţ��������ȱ��һ��
                    (*waiting_table)[missing_tiles[0]] = true;
                }
                return true;
            }
        }
        else {
            if (is_basic_form_win_recursively(temp_table, 5)) {
                if (waiting_table != nullptr) {  // ��ȡ�����ţ��������ȱ��һ��
                    (*waiting_table)[missing_tiles[0]] = true;
                }
                return true;
            }
        }
    }
    else if (missing_cnt == 0) {  // �����������ˣ���ô��ȥ�����֮�����Ҫ�����������Ʋ�����
        if (left_cnt == 10) {
            return is_basic_form_wait_1(temp_table, waiting_table);
        }
        else {
            return is_basic_form_wait_recursively(temp_table, 4, waiting_table);
        }
    }

    return false;
}

// �������Ͱ����������������������ڼ��������� ��ͬ˳ �����������ӵķ����������Ƶ�������
static int basic_form_shanten_specified(const tile_table_t &cnt_table, const tile_t *main_tiles, int main_cnt,
    intptr_t fixed_cnt, useful_table_t *useful_table) {

    tile_table_t temp_table;
    memcpy(&temp_table, &cnt_table, sizeof(temp_table));
    int exist_cnt = 0;

    // ͳ����������
    for (int i = 0; i < main_cnt; ++i) {
        tile_t t = main_tiles[i];
        int n = cnt_table[t];
        if (n > 0) {  // �У�����֮
            ++exist_cnt;
            --temp_table[t];
        }
    }

    // ��¼��Ч��
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        // ͳ������ȱʧ����
        for (int i = 0; i < main_cnt; ++i) {
            tile_t t = main_tiles[i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }
    }

    // �����Ƶ�������
    int result = basic_form_shanten_from_table(temp_table, fixed_cnt + main_cnt / 3, useful_table);

    // ������=����ȱ�ٵ�����+�����Ƶ�������
    return (main_cnt - exist_cnt) + result;
}

// �����������
int knitted_straight_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return std::numeric_limits<int>::max();
    }

    // ���
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    int ret = std::numeric_limits<int>::max();

    // ��Ҫ��ȡ��Ч��ʱ��������������ͬʱ�ͻ�ȡ��Ч����
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        useful_table_t temp_table;

        // 6��������ֱ����
        for (int i = 0; i < 6; ++i) {
            int fixed_cnt = (13 - static_cast<int>(standing_cnt)) / 3;
            int st = basic_form_shanten_specified(cnt_table, standard_knitted_straight[i], 9, fixed_cnt, &temp_table);
            if (st < ret) {  // ������С�ģ�ֱ�Ӹ�������
                ret = st;
                memcpy(*useful_table, temp_table, sizeof(*useful_table));  // ֱ�Ӹ���ԭ������Ч������
            }
            else if (st == ret) {  // ���ֲ�ͬ����������������ȵĻ���ֱ�Ӻϲ���Ч��
                std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                    std::begin(*useful_table), [](bool u, bool t) { return u || t; });
            }
        }
    }
    else {
        // 6��������ֱ����
        for (int i = 0; i < 6; ++i) {
            int fixed_cnt = (13 - static_cast<int>(standing_cnt)) / 3;
            int st = basic_form_shanten_specified(cnt_table, standard_knitted_straight[i], 9, fixed_cnt, nullptr);
            if (st < ret) {
                ret = st;
            }
        }
    }

    return ret;
}

// ������Ƿ�����
bool is_knitted_straight_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    if (standing_tiles == nullptr || (standing_cnt != 13 && standing_cnt != 10)) {
        return false;
    }

    // �����Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    return is_knitted_straight_wait_from_table(cnt_table, standing_cnt, waiting_table);
}

// ������Ƿ����
bool is_knitted_straight_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t waiting_table;
    return (is_knitted_straight_wait(standing_tiles, standing_cnt, &waiting_table)
        && waiting_table[test_tile]);
}

//-------------------------------- ȫ����/���ǲ��� --------------------------------

// 1���������ȫ����������
static int honors_and_knitted_tiles_shanten_1(const tile_t *standing_tiles, intptr_t standing_cnt, int which_seq, useful_table_t *useful_table) {
    if (standing_tiles == nullptr || standing_cnt != 13) {
        return std::numeric_limits<int>::max();
    }

    // ���Ƶ�������д��
    tile_table_t cnt_table;
    map_tiles(standing_tiles, standing_cnt, &cnt_table);

    int cnt = 0;

    // ͳ����������ֵ�����
    for (int i = 0; i < 9; ++i) {
        tile_t t = standard_knitted_straight[which_seq][i];
        int n = cnt_table[t];
        if (n > 0) {  // �У����Ӽ���
            ++cnt;
        }
    }

    // ͳ������
    for (int i = 6; i < 13; ++i) {
        tile_t t = standard_thirteen_orphans[i];
        int n = cnt_table[t];
        if (n > 0) {  // �У����Ӽ���
            ++cnt;
        }
    }

    // ��¼��Ч��
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        // ͳ�����������ȱʧ������
        for (int i = 0; i < 9; ++i) {
            tile_t t = standard_knitted_straight[which_seq][i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }

        // ͳ��ȱʧ������
        for (int i = 6; i < 13; ++i) {
            tile_t t = standard_thirteen_orphans[i];
            int n = cnt_table[t];
            if (n <= 0) {
                (*useful_table)[t] = true;
            }
        }
    }

    // ������=13-�������͵ļ���
    return 13 - cnt;
}

// ȫ����������
int honors_and_knitted_tiles_shanten(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *useful_table) {
    int ret = std::numeric_limits<int>::max();

    // ��Ҫ��ȡ��Ч��ʱ��������������ͬʱ�ͻ�ȡ��Ч����
    if (useful_table != nullptr) {
        memset(*useful_table, 0, sizeof(*useful_table));

        useful_table_t temp_table;

        // 6��������ֱ����
        for (int i = 0; i < 6; ++i) {
            int st = honors_and_knitted_tiles_shanten_1(standing_tiles, standing_cnt, i, &temp_table);
            if (st < ret) {  // ������С�ģ�ֱ�Ӹ�������
                ret = st;
                memcpy(*useful_table, temp_table, sizeof(*useful_table));  // ֱ�Ӹ���ԭ������Ч������
            }
            else if (st == ret) {  // ���ֲ�ͬ����������������ȵĻ���ֱ�Ӻϲ���Ч��
                std::transform(std::begin(*useful_table), std::end(*useful_table), std::begin(temp_table),
                    std::begin(*useful_table), [](bool u, bool t) { return u || t; });
            }
        }
    }
    else {
        // 6��������ֱ����
        for (int i = 0; i < 6; ++i) {
            int st = honors_and_knitted_tiles_shanten_1(standing_tiles, standing_cnt, i, nullptr);
            if (st < ret) {
                ret = st;
            }
        }
    }
    return ret;
}

// ȫ�����Ƿ�����
bool is_honors_and_knitted_tiles_wait(const tile_t *standing_tiles, intptr_t standing_cnt, useful_table_t *waiting_table) {
    // ֱ�Ӽ�������������������Ϊ0��Ϊ����
    if (waiting_table == nullptr) {
        return (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, nullptr));
    }

    useful_table_t useful_table;
    if (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, &useful_table)) {
        memcpy(*waiting_table, useful_table, sizeof(*waiting_table));
        return true;
    }
    return false;
}

// ȫ�����Ƿ����
bool is_honors_and_knitted_tiles_win(const tile_t *standing_tiles, intptr_t standing_cnt, tile_t test_tile) {
    useful_table_t useful_table;
    if (0 == honors_and_knitted_tiles_shanten(standing_tiles, standing_cnt, &useful_table)) {
        return useful_table[test_tile];
    }
    return false;
}

//-------------------------------- ö�ٴ��� --------------------------------

// ö�ٴ�������1��
static bool enum_discard_tile_1(const hand_tiles_t *hand_tiles, tile_t discard_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    enum_result_t result;
    result.discard_tile = discard_tile;
    result.form_flag = FORM_FLAG_BASIC_FORM;
    result.shanten = basic_form_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
    if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0���������Ҵ����������Ч�ƣ�������Ϊ����
        result.shanten = -1;
    }
    if (!enum_callback(context, &result)) {
        return false;
    }

    // ������13��ʱ������Ҫ�����������
    if (hand_tiles->tile_count == 13) {
        if (form_flag | FORM_FLAG_SEVEN_PAIRS) {
            result.form_flag = FORM_FLAG_SEVEN_PAIRS;
            result.shanten = seven_pairs_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0���������Ҵ����������Ч�ƣ�������Ϊ����
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_THIRTEEN_ORPHANS) {
            result.form_flag = FORM_FLAG_THIRTEEN_ORPHANS;
            result.shanten = thirteen_orphans_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0���������Ҵ����������Ч�ƣ�������Ϊ����
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }

        if (form_flag | FORM_FLAG_HONORS_AND_KNITTED_TILES) {
            result.form_flag = FORM_FLAG_HONORS_AND_KNITTED_TILES;
            result.shanten = honors_and_knitted_tiles_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0���������Ҵ����������Ч�ƣ�������Ϊ����
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    // ������13�Ż���10��ʱ������Ҫ���������
    if (hand_tiles->tile_count == 13 || hand_tiles->tile_count == 10) {
        if (form_flag | FORM_FLAG_KNITTED_STRAIGHT) {
            result.form_flag = FORM_FLAG_KNITTED_STRAIGHT;
            result.shanten = knitted_straight_shanten(hand_tiles->standing_tiles, hand_tiles->tile_count, &result.useful_table);
            if (result.shanten == 0 && result.useful_table[discard_tile]) {  // 0���������Ҵ����������Ч�ƣ�������Ϊ����
                result.shanten = -1;
            }
            if (!enum_callback(context, &result)) {
                return false;
            }
        }
    }

    return true;
}

// ö�ٴ�������
void enum_discard_tile(const hand_tiles_t *hand_tiles, tile_t serving_tile, uint8_t form_flag,
    void *context, enum_callback_t enum_callback) {
    // �ȼ������е�
    if (!enum_discard_tile_1(hand_tiles, serving_tile, form_flag, context, enum_callback)) {
        return;
    }

    if (serving_tile == 0) {
        return;
    }

    // �����ƴ��
    tile_table_t cnt_table;
    map_tiles(hand_tiles->standing_tiles, hand_tiles->tile_count, &cnt_table);

    // ����һ������
    hand_tiles_t temp;
    memcpy(&temp, hand_tiles, sizeof(temp));

    // ���γ��Դ����е�����
    for (int i = 0; i < 34; ++i) {
        tile_t t = all_tiles[i];
        if (cnt_table[t] && t != serving_tile && cnt_table[serving_tile] < 4) {
            --cnt_table[t];  // ��������
            ++cnt_table[serving_tile];  // ��������

            // ��tableת������
            table_to_tiles(cnt_table, temp.standing_tiles, temp.tile_count);

            // ����
            if (!enum_discard_tile_1(&temp, t, form_flag, context, enum_callback)) {
                return;
            }

            // ��ԭ
            --cnt_table[serving_tile];
            ++cnt_table[t];
        }
    }
}

}
