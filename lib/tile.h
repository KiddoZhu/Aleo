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

#ifndef __MAHJONG_ALGORITHM__TILE_H__
#define __MAHJONG_ALGORITHM__TILE_H__

#include <stddef.h>
#include <stdint.h>

#ifdef _MSC_VER  // for MSVC
#define forceinline __forceinline
#elif defined __GNUC__  // for gcc on Linux/Apple OS X
#define forceinline __inline__ __attribute__((always_inline))
#else
#define forceinline inline
#endif

namespace mahjong {

/**
 * @brief ����ע�����õ���������
 * - ˳�ӣ������У���ɫ��ͬ����������3���ơ�
 * - ���ӣ�������ͬ���ơ�������Ϊ���̣�δ������Ϊ���̡��׳ƿ�����Ҳ����ӣ����������̣������㰵�̡�
 * - ���ӣ�˳�ӺͿ��ӵ�ͳ�ơ��׳�һ�仰��һ���ơ�
 * - ȸͷ������������ʽ�У�������ϵĶ��ӣ�Ҳ�н����ۡ�
 * - �������ͣ�4����1ȸͷ�ĺ�����ʽ��
 * - ������ͣ���4����1ȸͷ�ĺ�����ʽ���ڹ�������У����߶ԡ�ʮ���ۡ�ȫ������������͡�
 * - ���壺Ҳ����ǰ�壬ָ���ԡ������������ܵ�״̬��������ͱ�Ȼ������״̬��������Ȼ�������壬���ᱩ¶�����Ʋ���������͵���Ϣ��
 * - ��¶�����ơ����ơ����Ƶ�ͳ�ƣ�����������ѡ�ִ����������Լ��������ӵ���Ϊ��һ�㲻�������ܣ�Ҳ�����ƣ��׳ƶ��ơ�
 *     ��¶��ʱ��Ҳ�������ܣ���ʱ�����ܳ�Ϊ֮����¶�����ԡ��������ܳ�Ϊ����¶��
 * - ���ƣ��������Ƴ�ȥ�ԡ�������֮����ơ�
 * - ���ƣ��������ƺͳԡ������ܵ��ƣ���ʱ��ָ���ơ�
 * - ���ƣ�ֻ������Ҫ��һ���Ƽ��ܺ��Ƶ�״̬���׳��½С���С��кͣ�������
 * - һ������ָ��һ�ž������Ƶ�״̬��Ҳ��һ������һ�������Դ������ж���������������N������
 * - ���������ﵽ����״̬��Ҫ�Ƶ�������
 * - ��Ч�ƣ���ʹ���������ٵ��ƣ�Ҳ�ƽ����ơ������ơ�
 * - �����ƣ���ʹ��Ч�����ӵ��ơ�ͨ����˵������ʹ����������ơ�
 * - ���ӣ�������ͬ���ơ�ȸͷһ���Ƕ��ӣ������Ӳ�һ����ȸͷ��
 * - ���棺�����У���ɫ��ͬ�������ڵ������ƣ���45m����������ƶ�����˳�ӡ�Ҳ����ͷ��
 * - Ƕ�ţ������У���ɫ��ͬ�������1�������ƣ���57s��ֻ�����м���ƹ���˳�ӣ��м�������Ƴ�ΪǶ�š�
 * - ���ţ�Ҳ���������ڵ������ƣ������ڴ��ڱ߽�λ�ã�ֻ����һ������ܹ���˳�ӣ���12ֻ����3����˳�ӡ�89ֻ����7����˳�ӣ�����3����7���Ϊ���š�
 * - ���ӣ�ָ��һ���ƾ��ܹ���1�����ӵ������ơ�����̬�п��Ӵ��ӣ������ӣ���������ӡ�Ƕ�Ŵ��ӡ����Ŵ��ӡ�
 * - ���ϴ��ӣ������ƹ��ɵĴ��ӡ��������У���Ƕ�š���������ӡ�Ƕ�Ŵ����ӡ����Ŵ����ӵȵ���̬��
 * - �Ե�������ʱ�������ƶ��Ѿ��������ӣ�ʣ�����ԣ�ֻ������һ�Գɿ̼��ɺ��ƣ���ʱ��һ�Գ䵱ȸͷ������������̬�жԵ���Ҳ��˫�������������ơ�
 */


/**
 * @addtogroup tile
 * @{
 */

/**
 * @brief ��ɫ
 */
typedef uint8_t suit_t;

/**
 * @brief ����
 */
typedef uint8_t rank_t;

#define TILE_SUIT_NONE          0  ///< ��Ч
#define TILE_SUIT_CHARACTERS    1  ///< ���ӣ�CHARACTERS��
#define TILE_SUIT_BAMBOO        2  ///< ���ӣ�BAMBOO��
#define TILE_SUIT_DOTS          3  ///< ���ӣ�DOTS��
#define TILE_SUIT_HONORS        4  ///< ���ƣ�HONORS��

/**
 * @brief ��\n
 * �ڴ�ṹ��
 * - 0-3 4bit �Ƶĵ���
 * - 4-7 4bit �ƵĻ�ɫ
 * �Ϸ�����Ϊ��
 * - 0x11 - 0x19 ���ӣ�CHARACTERS��
 * - 0x21 - 0x29 ���ӣ�BAMBOO��
 * - 0x31 - 0x39 ���ӣ�DOTS��
 * - 0x41 - 0x47 ���ƣ�HONORS��
 */
typedef uint8_t tile_t;

/**
 * @brief ����һ����
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] suit ��ɫ
 * @param [in] rank ����
 * @return tile_t ��
 */
static forceinline tile_t make_tile(suit_t suit, rank_t rank) {
    return (((suit & 0xF) << 4) | (rank & 0xF));
}

/**
 * @brief ��ȡ�ƵĻ�ɫ
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile ��
 * @return suit_t ��ɫ
 */
static forceinline suit_t tile_get_suit(tile_t tile) {
    return ((tile >> 4) & 0xF);
}

/**
 * @brief ��ȡ�Ƶĵ���
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile ��
 * @return rank_t ����
 */
static forceinline rank_t tile_get_rank(tile_t tile) {
    return (tile & 0xF);
}

/**
 * @brief �����Ƶ�ֵ
 */
enum tile_value_t {
    TILE_1m = 0x11, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s = 0x21, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p = 0x31, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E  = 0x41, TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P ,
    TILE_TABLE_SIZE
};

/**
 * @brief ���кϷ�����
 */
static const tile_t all_tiles[] = {
    TILE_1m, TILE_2m, TILE_3m, TILE_4m, TILE_5m, TILE_6m, TILE_7m, TILE_8m, TILE_9m,
    TILE_1s, TILE_2s, TILE_3s, TILE_4s, TILE_5s, TILE_6s, TILE_7s, TILE_8s, TILE_9s,
    TILE_1p, TILE_2p, TILE_3p, TILE_4p, TILE_5p, TILE_6p, TILE_7p, TILE_8p, TILE_9p,
    TILE_E , TILE_S , TILE_W , TILE_N , TILE_C , TILE_F , TILE_P
};

/**
 * @brief �Ʊ�����
 *
 * ˵�������ж����ơ��������������㷨�У������Ķ����������ִ洢��ʽ��
 * - һ�������Ʊ���������ʾ������ӵ�е�ö�������ִ洢��ʽ���ŵ����ڵݹ����ʱ��������ֻ��Ҫ�޸ı�����Ӧ�±��ֵ��ȱ����һ���Ƶ�����������ȷ��
 * - ��һ����ֱ�����Ƶ����飬���ִ洢��ʽ���ŵ��Ǻ�����ȷ��һ���Ƶ�������ȱ�����ڵݹ����ʱ�������Ӳ����㣬��Ҫ��������ɾ��Ԫ�ز���
 */
typedef uint16_t tile_table_t[TILE_TABLE_SIZE];

#define PACK_TYPE_NONE 0  ///< ��Ч
#define PACK_TYPE_CHOW 1  ///< ˳��
#define PACK_TYPE_PUNG 2  ///< ����
#define PACK_TYPE_KONG 3  ///< ��
#define PACK_TYPE_PAIR 4  ///< ȸͷ

/**
 * @brief ����
 *  ���ڱ�ʾһ�����ӻ���ȸͷ
 *
 * �ڴ�ṹ��
 * - 0-7 8bit tile �ƣ�����˳�ӣ����ʾ�м������ƣ�����234p����ô��Ϊ3p��
 * - 8-11 4bit type �������ͣ�ʹ��PACK_TYPE_xxx��
 * - 12-15 4bit offer ������Ϣ��ȡֵ��ΧΪ0123\n
 *       0��ʾ���֣���˳�����̡����ܣ�����0��ʾ���֣���˳�����̡����ܣ�
 *
 *       ���������ǿ��Ӻ͸�ʱ��123�ֱ�����ʾ���ϼ�/�Լ�/�¼ҹ���\n
 *       ��������Ϊ˳��ʱ�����ڳ���ֻ�����ϼҹ���������123�ֱ�����ʾ�ڼ������ϼҹ���
 */
typedef uint16_t pack_t;

/**
 * @brief ����һ������
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] offer ������Ϣ
 * @param [in] type ��������
 * @param [in] tile �ƣ�����˳�ӣ�Ϊ�м������ƣ�
 */
static forceinline pack_t make_pack(uint8_t offer, uint8_t type, tile_t tile) {
    return (offer << 12 | (type << 8) | tile);
}

/**
 * @brief �����Ƿ�Ϊ����
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] pack ����
 * @return bool
 */
static forceinline bool is_pack_melded(pack_t pack) {
    return !!((pack >> 12) & 0xF);
}

/**
 * @brief ����Ĺ�����Ϣ
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] pack ����
 * @return uint8_t
 */
static forceinline uint8_t pack_get_offer(pack_t pack) {
    return ((pack >> 12) & 0xF);
}

/**
 * @brief ��ȡ���������
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] pack ����
 * @return uint8_t ��������
 */
static forceinline uint8_t pack_get_type(pack_t pack) {
    return ((pack >> 8) & 0xF);
}

/**
 * @brief ��ȡ�Ƶĵ���
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] pack ����
 * @return tile_t �ƣ�����˳�ӣ�Ϊ�м������ƣ�
 */
static forceinline tile_t pack_get_tile(pack_t pack) {
    return (pack & 0xFF);
}

/**
 * @brief ���ƽṹ
 *  ���ƽṹһ�������ʽ��3*��¶��������+������=13
 */
struct hand_tiles_t {
    pack_t fixed_packs[5];      ///< ��¶�����飨���ӣ�����������
    intptr_t pack_count;        ///< ��¶�����飨���ӣ�������������
    tile_t standing_tiles[13];  ///< ����
    intptr_t tile_count;        ///< ������
};


/**
 * @brief �ж��Ƿ�Ϊ��һɫ������
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_green(tile_t tile) {
    // �����������жϣ�23468s������Ϊ��һɫ������
    //return (tile == TILE_2s || tile == TILE_3s || tile == TILE_4s || tile == TILE_6s || tile == TILE_8s || tile == TILE_F);

    // �㷨ԭ��
    // 0x48-0x11=0x37=55�պ���һ��64λ���͵ķ�Χ�ڣ�
    // ��uint64_t��ÿһλ��ʾһ���Ƶı�ǣ����ȵõ�һ��ħ����
    // Ȼ��ÿ�β�����Ӧλ����
    return !!(0x0020000000AE0000ULL & (1ULL << (tile - TILE_1m)));
}

/**
 * @brief �ж��Ƿ�Ϊ�Ʋ���������
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_reversible(tile_t tile) {
    // �����������жϣ�245689s��1234589p���װ�Ϊ�Ʋ���������
    //return (tile == TILE_2s || tile == TILE_4s || tile == TILE_5s || tile == TILE_6s || tile == TILE_8s || tile == TILE_9s ||
    //    tile == TILE_1p || tile == TILE_2p || tile == TILE_3p || tile == TILE_4p || tile == TILE_5p || tile == TILE_8p || tile == TILE_9p ||
    //    tile == TILE_P);

    // �㷨ԭ��ͬ��һɫ�������жϺ���
    return !!(0x0040019F01BA0000ULL & (1ULL << (tile - TILE_1m)));
}

/**
 * @brief �ж��Ƿ�Ϊ�����۾ţ���ͷ�ƣ�
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_terminal(tile_t tile) {
    // �����������ж�
    //return (tile == TILE_1m || tile == TILE_9m || tile == TILE_1s || tile == TILE_9s || tile == TILE_1p || tile == TILE_9p);

    // �㷨ԭ���۲������۾ŵĶ�����λ��
    // 0x11��0001 0001
    // 0x19��0001 1001
    // 0x21��0010 0001
    // 0x29��0010 1001
    // 0x31��0011 0001
    // 0x39��0011 1001
    // �����Ƶĵ�4bitֻ�������0001��1001֮�䣬��0111λ�룬ֻ��0001��1001�Ľ��Ϊ1
    // �������Ƶĸ�4bitֻ�������0001��0011֮�䣬��1100λ�룬��ȻΪ0
    // ���ǹ���ħ��0xC7��1100 0111������λ�룬���Ϊ1�ģ���Ϊ�����۾�
    // ȱ�ݣ���4bit�Ĳ������0xB��0xD��0xF�������У���4bit�Ĳ������0x01��0x09��������
    return ((tile & 0xC7) == 1);
}

/**
 * @brief �ж��Ƿ�Ϊ����
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_winds(tile_t tile) {
    return (tile > 0x40 && tile < 0x45);
}

/**
 * @brief �ж��Ƿ�Ϊ���ƣ���Ԫ�ƣ�
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_dragons(tile_t tile) {
    return (tile > 0x44 && tile < 0x48);
}

/**
 * @brief �ж��Ƿ�Ϊ����
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_honor(tile_t tile) {
    return (tile > 0x40 && tile < 0x48);
}

/**
 * @brief �ж��Ƿ�Ϊ����
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_numbered_suit(tile_t tile) {
    if (tile < 0x1A) return (tile > 0x10);
    if (tile < 0x2A) return (tile > 0x20);
    if (tile < 0x3A) return (tile > 0x30);
    return false;
}

/**
 * @brief �ж��Ƿ�Ϊ���ƣ����죩
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @see is_numbered_suit
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_numbered_suit_quick(tile_t tile) {
    // �㷨ԭ������Ϊ0x11-0x19��0x21-0x29��0x31-0x39����0xC0λ�룬���Ϊ0
    return !(tile & 0xC0);
}

/**
 * @brief �ж��Ƿ�Ϊ�۾��ƣ����������۾ź����ƣ�
 * @param [in] tile ��
 * @return bool
 */
static forceinline bool is_terminal_or_honor(tile_t tile) {
    return is_terminal(tile) || is_honor(tile);
}

/**
 * @brief �ж������ƻ�ɫ�Ƿ���ͬ�����죩
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile0 ��0
 * @param [in] tile1 ��1
 * @return bool
 */
static forceinline bool is_suit_equal_quick(tile_t tile0, tile_t tile1) {
    // �㷨ԭ����4bit��ʾ��ɫ
    return ((tile0 & 0xF0) == (tile1 & 0xF0));
}

/**
 * @brief �ж������Ƶ����Ƿ���ͬ�����죩
 *  �������������ĺϷ��ԡ�������벻�Ϸ���ֵ�����޷���֤�Ϸ�����ֵ�ĺϷ���
 * @param [in] tile0 ��0
 * @param [in] tile1 ��1
 * @return bool
 */
static forceinline bool is_rank_equal_quick(tile_t tile0, tile_t tile1) {
    // �㷨ԭ����4bit��ʾ��ɫ����4bit����ΪC��Ϊ�˹��˵�����
    return ((tile0 & 0xCF) == (tile1 & 0xCF));
}

/**
 * end group
 * @}
 */

}

#endif
