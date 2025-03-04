#ifndef SOCKET_UTIL
#define SOCKET_UTIL

#include <stdio.h>

namespace fz
{
    namespace util
    {
        void byteorder()
        {
            union
            {
                short value;
                char union_bytes[sizeof(value)];
            } test;

            test.value = 0x0102;
            if ((test.union_bytes[0] == 0x01) && (test.union_bytes[1] = 0x02))
            {
                printf("big endian");
            }
            else
            if ((test.union_bytes[0] == 0x02) && (test.union_bytes[1] = 0x01))
            {
                printf("little endian\n");
            }
            else
            {
                printf("unknow ...");
            }
        }

    } // namespace util
    
} // namespace fz



#endif