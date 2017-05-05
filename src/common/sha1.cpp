/*
 *    sha1.cpp
 *
 *    Copyright (C) 1998
 *    Paul E. Jones <paulej@arid.us>
 *    All Rights Reserved.
 *
 *****************************************************************************
 *    $Id: sha1.cpp,v 1.9 2004/03/27 18:02:20 paulej Exp $
 *****************************************************************************
 *
 *    Description:
 *         This class implements the Secure Hashing Standard as defined
 *         in FIPS PUB 180-1 published April 17, 1995.
 *
 *         The Secure Hashing Standard, which uses the Secure Hashing
 *         Algorithm (SHA), produces a 160-bit message digest for a
 *         given data stream.  In theory, it is highly improbable that
 *         two messages will produce the same message digest.  Therefore,
 *         this algorithm can serve as a means of providing a "fingerprint"
 *         for a message.
 *
 *    Portability Issues:
 *         SHA-1 is defined in terms of 32-bit "words".  This code was
 *         written with the expectation that the processor has at least
 *         a 32-bit machine word size.  If the machine word size is larger,
 *         the code should still function properly.  One caveat to that
 *        is that the input functions taking characters and character arrays
 *        assume that only 8 bits of information are stored in each character.
 *
 *    Caveats:
 *         SHA-1 is designed to work with messages less than 2^64 bits long.
 *         Although SHA-1 allows a message digest to be generated for
 *         messages of any number of bits less than 2^64, this implementation
 *         only works with messages with a length that is a multiple of 8
 *         bits.
 *
 */

#include <stdio.h>

#include "common/sha1.h"

namespace pebble {


SHA1::SHA1()
{
    SHAInit();
}

SHA1::~SHA1()
{
}

void SHA1::SHAInit()
{
    Length_Low                = 0;
    Length_High                = 0;
    Message_Block_Index        = 0;

    H[0]        = 0x67452301;
    H[1]        = 0xEFCDAB89;
    H[2]        = 0x98BADCFE;
    H[3]        = 0x10325476;
    H[4]        = 0xC3D2E1F0;
}

// space of lpSHACode_Output must be >= 20 bytes;
bool SHA1::Encode2Hex(const char *lpData_Input, char *lpSHACode_Output)
{
    if (lpData_Input == NULL || lpSHACode_Output == NULL)
        return false;

    SHAInit();

    // One times analyse 64Bytes, 512 bits.
    int nInputLen = strlen(lpData_Input);
    int nDealDataLen = 0;                            //    the length of can-deal-data, this times;
    for (int pos = 0 ; pos <= nInputLen ; pos += 64)
    {
        if (nInputLen - pos >= 64)
        {
            nDealDataLen = 64;                        // input-data is enough fill 64bytes,
            memset(Message_Block, 0, sizeof(Message_Block));
            memcpy(Message_Block, lpData_Input + pos, nDealDataLen);

            AddDataLen(nDealDataLen);
            ProcessMessageBlock();
            AddDataLen(0);
        }
        else
        {
            nDealDataLen = nInputLen - pos;
            // input-data isn't enough fill 64bytes,need fill 0x8000000000 and lenth of real-data.
            memset(Message_Block, 0, sizeof(Message_Block));
            memcpy(Message_Block, lpData_Input + pos, nDealDataLen);

            AddDataLen(nDealDataLen);
            PadMessage();
        }
    }

    // copy result to output
    for (int i = 0; i < 5; i++)
    {
        sprintf(&(lpSHACode_Output[8*i]), "%08x", H[i]); // NOLINT
    }

    return true;
}

bool SHA1::Encode2Ascii(const char *lpData_Input, char *lpSHACode_Output)
{
    if (lpData_Input == NULL || lpSHACode_Output == NULL)
        return false;

    SHAInit();

    // One times analyse 64Bytes, 512 bits.
    int nInputLen = strlen(lpData_Input);
    int nDealDataLen = 0;                            //    the length of can-deal-data, this times;
    for (int pos = 0 ; pos <= nInputLen; pos += 64)
    {
        if (nInputLen - pos >= 64)
        {
            nDealDataLen = 64;                        // input-data is enough fill 64bytes,
            memset(Message_Block, 0, sizeof(Message_Block));
            memcpy(Message_Block, lpData_Input + pos, nDealDataLen);

            AddDataLen(nDealDataLen);
            ProcessMessageBlock();
            AddDataLen(0);
        }
        else
        {
            nDealDataLen = nInputLen - pos;
            // input-data isn't enough fill 64bytes,need fill 0x8000000000 and lenth of real-data.
            memset(Message_Block, 0, sizeof(Message_Block));
            memcpy(Message_Block, lpData_Input + pos, nDealDataLen);

            AddDataLen(nDealDataLen);
            PadMessage();
        }
    }

    // copy result to output
    for (int i = 0; i < 5; i++)
    {
        memcpy(lpSHACode_Output + i * 4, (char*)&H[i] + 3, 1); // NOLINT
        memcpy(lpSHACode_Output + i * 4 + 1, (char*)&H[i] + 2, 1); // NOLINT
        memcpy(lpSHACode_Output + i * 4 + 2, (char*)&H[i] + 1, 1); // NOLINT
        memcpy(lpSHACode_Output + i * 4 + 3, (char*)&H[i] + 0, 1); // NOLINT
    }

    return true;
}

void SHA1::AddDataLen(int nDealDataLen)
{
    Message_Block_Index = nDealDataLen;

    if ((Length_Low += ((unsigned int)nDealDataLen << 3)) < ((unsigned int)nDealDataLen << 3))
    {
        Length_High++;
    }
    Length_High += ((unsigned int)nDealDataLen >> 29);
}


/*
 *    ProcessMessageBlock
 *
 *    Description:
 *        This function will process the next 512 bits of the message
 *        stored in the Message_Block array.
 *
 *    Parameters:
 *        None.
 *
 *    Returns:
 *        Nothing.
 *
 *    Comments:
 *        Many of the variable names in this function, especially the single
 *         character names, were used because those were the names used
 *          in the publication.
 *
 */
void SHA1::ProcessMessageBlock()
{
    const unsigned K[] = {                 // Constants defined for SHA-1
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
    };
    int         t;                            // Loop counter
    unsigned     temp;                        // Temporary word value
    unsigned    W[80];                        // Word sequence
    unsigned    A, B, C, D, E;                // Word buffers

    /*
     *    Initialize the first 16 words in the array W
     */
    for (t = 0 ; t < 16 ; t++)
    {
        W[t] = ((unsigned) Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) Message_Block[t * 4 + 3]);
    }

    for (t = 16 ; t < 80 ; t++)
    {
        W[t] = CircularShift(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = H[0];
    B = H[1];
    C = H[2];
    D = H[3];
    E = H[4];

    for (t = 0 ; t < 20 ; t++)
    {
        temp = CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = CircularShift(30, B);
        B = A;
        A = temp;
    }

    for (t = 20 ; t < 40 ; t++)
    {
        temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = CircularShift(30, B);
        B = A;
        A = temp;
    }

    for (t = 40 ; t < 60 ; t++)
    {
        temp = CircularShift(5, A) +
                ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = CircularShift(30, B);
        B = A;
        A = temp;
    }

    for (t = 60 ; t < 80; t++)
    {
        temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = CircularShift(30, B);
        B = A;
        A = temp;
    }

    H[0] = (H[0] + A) & 0xFFFFFFFF;
    H[1] = (H[1] + B) & 0xFFFFFFFF;
    H[2] = (H[2] + C) & 0xFFFFFFFF;
    H[3] = (H[3] + D) & 0xFFFFFFFF;
    H[4] = (H[4] + E) & 0xFFFFFFFF;
}

/*
 *    PadMessage
 *
 *    Description:
 *        According to the standard, the message must be padded to an even
 *        512 bits.  The first padding bit must be a '1'.  The last 64 bits
 *        represent the length of the original message.  All bits in between
 *        should be 0.  This function will pad the message according to those
 *        rules by filling the message_block array accordingly.  It will also
 *        call ProcessMessageBlock() appropriately.  When it returns, it
 *        can be assumed that the message digest has been computed.
 *
 *    Parameters:
 *        None.
 *
 *    Returns:
 *        Nothing.
 *
 *    Comments:
 *
 */
void SHA1::PadMessage()
{
    /*
     *    Check to see if the current message block is too small to hold
     *    the initial padding bits and length.  If so, we will pad the
     *    block, process it, and then continue padding into a second block.
     */
    if (Message_Block_Index > 55)
    {
        Message_Block[Message_Block_Index++] = 0x80;
        while (Message_Block_Index < 64)
        {
            Message_Block[Message_Block_Index++] = 0;
        }

        ProcessMessageBlock();

        while (Message_Block_Index < 56)
        {
            Message_Block[Message_Block_Index++] = 0;
        }
    }
    else
    {
        Message_Block[Message_Block_Index++] = 0x80;
        while (Message_Block_Index < 56)
        {
            Message_Block[Message_Block_Index++] = 0;
        }
    }

    /*
     *    Store the message length as the last 8 octets
     */
    Message_Block[56] = (Length_High >> 24) & 0xFF;
    Message_Block[57] = (Length_High >> 16) & 0xFF;
    Message_Block[58] = (Length_High >> 8) & 0xFF;
    Message_Block[59] = (Length_High) & 0xFF;
    Message_Block[60] = (Length_Low >> 24) & 0xFF;
    Message_Block[61] = (Length_Low >> 16) & 0xFF;
    Message_Block[62] = (Length_Low >> 8) & 0xFF;
    Message_Block[63] = (Length_Low) & 0xFF;

    ProcessMessageBlock();
}


/*
 *    CircularShift
 *
 *    Description:
 *        This member function will perform a circular shifting operation.
 *
 *    Parameters:
 *        bits: [in]
 *            The number of bits to shift (1-31)
 *        word: [in]
 *            The value to shift (assumes a 32-bit integer)
 *
 *    Returns:
 *        The shifted value.
 *
 *    Comments:
 *
 */
unsigned SHA1::CircularShift(int bits, unsigned word)
{
    return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32-bits));
}

} // namespace pebble


