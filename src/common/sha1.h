/*
 *    sha1.h
 *
 *    Copyright (C) 1998
 *    Paul E. Jones <paulej@arid.us>
 *    All Rights Reserved.
 *
 *****************************************************************************
 *    $Id: sha1.h,v 1.6 2004/03/27 18:02:26 paulej Exp $
 *****************************************************************************
 *
 *    Description:
 *         This class implements the Secure Hashing Standard as defined
 *         in FIPS PUB 180-1 published April 17, 1995.
 *
 *        Many of the variable names in this class, especially the single
 *        character names, were used because those were the names used
 *        in the publication.
 *
 *         Please read the file sha1.cpp for more information.
 *
 *         This is the same as MD5.
 *
 *  Created on: 2009-3-26
 *      Author: root
 *      usage    :    char SHABuffer[41];
 *                    SHA1 SHA;
 *                    SHA.SHA_GO("gfc",SHABuffer);
 *                    执行完成之后SHABuffer中即存储了由"a string"计算得到的SHA值
         算法描述：
                    1.填充形成整512 bits(64字节), (N+1)*512;
                                N*512 + 448位                         64位
                        真实数据 + 填充(一个1和无数个0)                 保存真实数据的长度
                        —————————————————————— ----------------------
                    2.四个32位被称为连接变量的整数参数：
                        A=，B=，C=，D=,E=
                    3.开始算法的四轮循环运算。
                        循环的次数(N+1)是信息中512位信息分组的数目。
                        for(N+1, 每次取512 bits(64字节)来进行处理：)
                        {
                            512bits 分成16个分组(每个分组4字节。int data[16])
                            用16个分组中的每一个分组和A，B，C，D,E通过4个函数进行运算，
                            得到新的A，B，C，D,E
                        }
                    4.最后，将A，B，C，D,E拼接起来就是SHA值
                        将拼接后的值用字符串的方式表示出来，就是最终的结果。
                        (比如 0x234233F, 用串 “234233F”表示出来，生成的串的空间将是值的空间的2倍。)
 *        调试情况: 该代码已经调试通过。
 */

#ifndef _SHA1_H_
#define _SHA1_H_

#include<stdio.h>
#include<string.h>


namespace pebble {


class SHA1
{
    public:
        SHA1();
        virtual ~SHA1();

        bool Encode2Hex(const char *lpData_Input, char *lpSHACode_Output);

        bool Encode2Ascii(const char *lpData_Input, char *lpSHACode_Output);
    private:
        unsigned int H[5];                        // Message digest buffers
        unsigned int Length_Low;                // Message length in bits
        unsigned int Length_High;                // Message length in bits
        unsigned char Message_Block[64];        // 512-bit message blocks
        int Message_Block_Index;                // Index into message block array
    private:
        void SHAInit();

        void AddDataLen(int nDealDataLen);

        // Process the next 512 bits of the message
        void ProcessMessageBlock();

        // Pads the current message block to 512 bits
        void PadMessage();

        // Performs a circular left shift operation
        inline unsigned CircularShift(int bits, unsigned word);
};

} // namespace pebble

#endif
