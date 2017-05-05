/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common/base64.h"


namespace pebble {


static char base64_code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @Function: 根据在Base64编码表中的序号求得某个字符
 *            0-63 : A-Z(25) a-z(51), 0-9(61), +(62), /(63)
 * @Param:    unsigned char n
 * @Return:   字符
 */
char Base64::Base2Chr(unsigned char n)
{
    n &= 0x3F;
    if (n < 26)
        return static_cast<char>(n + 'A');
    else if (n < 52)
        return static_cast<char>(n - 26 + 'a');
    else if (n < 62)
        return static_cast<char>(n - 52 + '0');
    else if (n == 62)
        return '+';
    else
        return '/';
}

/**
 * @Function: 求得某个字符在Base64编码表中的序号
 * @Param:    char c   字符
 * @Return:   序号值
 */
unsigned char Base64::Chr2Base(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (unsigned char)(c - 'A');
    else if (c >= 'a' && c <= 'z')
        return (unsigned char )(c - 'a' + 26);
    else if (c >= '0' && c <= '9')
        return ( unsigned char )(c - '0' + 52);
    else if (c == '+')
        return 62;
    else if (c == '/')
        return 63;
    else
        return 64;  //  无效字符
}

bool Base64::Encode(const std::string& src, std::string* dst)
{
    if (0 == src.size() || NULL == dst)
    {
        return false;
    }

    dst->resize(Base64EncodeLen(src.size()));

    int c = -1;

    unsigned char* p = reinterpret_cast<unsigned char*>(&(*dst)[0]);
    unsigned char* s = p;
    unsigned char* q = reinterpret_cast<unsigned char*>(const_cast<char*>(&src[0]));

    for (size_t i = 0; i < src.size();)
    {
        // 处理的时候，都是把24bit当作一个单位，因为3*8=4*6
        c = q[i++];
        c *= 256;
        if (i < src.size())
            c += q[i];
        i++;
        c *= 256;
        if (i < src.size())
            c += q[i];
        i++;

        // 每次取6bit当作一个8bit的char放在p中
        p[0] = base64_code[(c & 0x00fc0000) >> 18];
        p[1] = base64_code[(c & 0x0003f000) >> 12];
        p[2] = base64_code[(c & 0x00000fc0) >> 6];
        p[3] = base64_code[(c & 0x0000003f) >> 0];

        // 这里是处理结尾情况
        if (i > src.size())
            p[3] = '=';

        if (i > src.size() + 1)
            p[2] = '=';

        p += 4; // 编码后的数据指针相应的移动
    }

    *p = 0;   // 防野指针
    dst->resize(p - s);

    return true;
}

bool Base64::Decode(const std::string& src, std::string* dst)
{
    if (0 == src.size() || NULL == dst)
    {
        return false;
    }

    dst->resize(Base64DecodeLen(src.size()));

    unsigned char* p = reinterpret_cast<unsigned char*>(&(*dst)[0]);
    unsigned char* q = p;
    unsigned char c = 0;
    unsigned char t = 0;

    for (size_t i = 0; i < src.size(); i++)
    {
        if (src[i] == '=')
            break;
        do
        {
            if (src[i])
                c = Chr2Base(src[i]);
            else
                c = 65;  //  字符串结束
        } while (c == 64);  //  跳过无效字符，如回车等

        if (c == 65)
            break;
        switch ( i % 4 )
        {
        case 0 :
            t = c << 2;
            break;
        case 1 :
            *p = (unsigned char)(t | (c >> 4));
            p++;
            t = ( unsigned char )( c << 4 );
            break;
        case 2 :
            *p = (unsigned char)(t | (c >> 2));
            p++;
            t = ( unsigned char )(c << 6);
            break;
        case 3 :
            *p = ( unsigned char )(t | c);
            p++;
            break;
        }
    }

    dst->resize(p - q);

    return true;
}

int Base64::Base64EncodeLen(int n)
{
    return (n + 2) / 3 * 4 + 1;
}

int Base64::Base64DecodeLen(int n)
{
    return n / 4 * 3 + 2;
}

} // namespace pebble

