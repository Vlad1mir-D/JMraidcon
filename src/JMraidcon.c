/*
 * JMraidcon - A console interface to the JMicron JMB394 H/W RAID controller
 * Copyright (C) 2010 Werner Johansson, <wj@xnk.nu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <scsi/sg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "jm_crc.h"
#include "sata_xor.h"
#include <asm/byteorder.h> // For __le32_to_cpu etc

#define SECTORSIZE (512)
#define READ_CMD (0x28)
#define WRITE_CMD (0x2a)
#define RW_CMD_LEN (10)

// First 4 bytes are always the same for all the scrambled commands, next 4 bytes forms an incrementing command id
const uint8_t probe6[]={ 0x22, 0x03, 0x7b, 0x19, 0x06,0x00,0x00,0x00, 0x00, 0x01, 0x02, 0xff, 0x01 }; // This returns very little info (at the end)?
const uint8_t probe7[]={ 0x22, 0x03, 0x7b, 0x19, 0x0b,0x00,0x00,0x00, 0x00, 0x01, 0x01, 0xff }; // This cmd returns the "RAID Manager" name
const uint8_t probe8[]={ 0x22, 0x03, 0x7b, 0x19, 0x0c,0x00,0x00,0x00, 0x00, 0x01, 0x02, 0xff, 0x0a };
const uint8_t probe9[]={ 0x22, 0x03, 0x7b, 0x19, 0x0e,0x00,0x00,0x00, 0x00, 0x02, 0x01, 0xff }; // This returns the names of disks attached (or in a specific RAID volume?)
const uint8_t probe11[]={ 0x22, 0x03, 0x7b, 0x19, 0x10,0x00,0x00,0x00, 0x00, 0x02, 0x02, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Identify disk0?
const uint8_t probe12[]={ 0x22, 0x03, 0x7b, 0x19, 0x11,0x00,0x00,0x00, 0x00, 0x02, 0x02, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01 }; // Identify disk1?
const uint8_t probe13[]={ 0x22, 0x03, 0x7b, 0x19, 0x12,0x00,0x00,0x00, 0x00, 0x02, 0x02, 0xff, 0x02, 0x00, 0x00, 0x00, 0x02 }; // Identify disk2?
const uint8_t probe14[]={ 0x22, 0x03, 0x7b, 0x19, 0x13,0x00,0x00,0x00, 0x00, 0x02, 0x02, 0xff, 0x03, 0x00, 0x00, 0x00, 0x03 }; // Identify disk3?
const uint8_t probe15[]={ 0x22, 0x03, 0x7b, 0x19, 0x14,0x00,0x00,0x00, 0x00, 0x02, 0x02, 0xff, 0x04, 0x00, 0x00, 0x00, 0x04 }; // Identify disk4?
const uint8_t probe16[]={ 0x22, 0x03, 0x7b, 0x19, 0x15,0x00,0x00,0x00, 0x00, 0x03, 0x02, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 }; // AWARD I5, wtf??
const uint8_t probe17[]={ 0x22, 0x03, 0x7b, 0x19, 0x16,0x00,0x00,0x00, 0x00, 0x03, 0x02, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01 }; // ??
const uint8_t probe18[]={ 0x22, 0x03, 0x7b, 0x19, 0x17,0x00,0x00,0x00, 0x00, 0x03, 0x02, 0xff, 0x02, 0x00, 0x00, 0x00, 0x02 }; // ??
const uint8_t probe19[]={ 0x22, 0x03, 0x7b, 0x19, 0x18,0x00,0x00,0x00, 0x00, 0x03, 0x02, 0xff, 0x03, 0x00, 0x00, 0x00, 0x03 }; // ??
const uint8_t probe20[]={ 0x22, 0x03, 0x7b, 0x19, 0x19,0x00,0x00,0x00, 0x00, 0x03, 0x02, 0xff, 0x04, 0x00, 0x00, 0x00, 0x04 }; // ??
const uint8_t probe21[]={ 0x22, 0x03, 0x7b, 0x19, 0x1a,0x00,0x00,0x00, 0x00, 0x01, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }; // Returns nothing?
const uint8_t probe23[]={ 0x22, 0x03, 0x7b, 0x19, 0x1c,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x00, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk0?
const uint8_t probe24[]={ 0x22, 0x03, 0x7b, 0x19, 0x1d,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x00, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk0 part2?
const uint8_t probe25[]={ 0x22, 0x03, 0x7b, 0x19, 0x1e,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x01, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk1?
const uint8_t probe26[]={ 0x22, 0x03, 0x7b, 0x19, 0x1f,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x01, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk1 part2?
const uint8_t probe27[]={ 0x22, 0x03, 0x7b, 0x19, 0x20,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x02, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk2?
const uint8_t probe28[]={ 0x22, 0x03, 0x7b, 0x19, 0x21,0x00,0x00,0x00, 0x00, 0x02, 0x03, 0xff, 0x02, 0x02, 0x00, 0xe0, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0xc2, 0x00, 0xa0, 0x00, 0xb0 }; // SMART data disk2 part2?

    sg_io_hdr_t io_hdr;
#warning FIXME: Should not use a hard-coded sector number (0x21), even though it is backed up and restored afterwards
    uint8_t rwCmdBlk[RW_CMD_LEN] =
                    {READ_CMD, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x01, 0x00}; // SECTOR NUMBER 0x21!!!!!!

uint32_t Do_JM_Cmd( int theFD, uint32_t* theCmd, uint32_t* theResp ) {
    uint32_t retval=0;

    // Calculate CRC for the request
    uint32_t myCRC = JM_CRC( theCmd, 0x7f );

    // Stash the CRC at the end
    theCmd[0x7f] = __cpu_to_le32( myCRC );
//    printf("Command CRC: 0x%08x\n", myCRC);

    // Make the data look really 31337 (or not)
    SATA_XOR( theCmd );

    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    rwCmdBlk[0] = WRITE_CMD;
    io_hdr.dxferp = theCmd;
    ioctl(theFD, SG_IO, &io_hdr);

    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    rwCmdBlk[0] = READ_CMD;
    io_hdr.dxferp = theResp;
    ioctl(theFD, SG_IO, &io_hdr);

    // Make the 31337-looking response sane
    SATA_XOR( theResp );

    myCRC = JM_CRC( theResp, 0x7f);
    if( myCRC != __le32_to_cpu( theResp[0x7f] ) ) {
        printf( "Warning: Response CRC 0x%08x does not match the calculated 0x%08x!!\n", __le32_to_cpu( theResp[0x7f] ), myCRC );
        retval=1;
    }
    return retval;
}

static void hexdump(uint8_t* thePtr, uint32_t theLen) {
    int looper;
    for(looper=0; looper<theLen; looper++) {
        printf("0x%02x, ", thePtr[looper]);
        if((looper&0x0f)==0x0f) {
            int asc;
            for(asc=looper-0x0f; asc<looper; asc++) {
                uint8_t theOut=thePtr[asc];
                if(theOut<0x20 || theOut>0x7f) theOut='.';
                printf("%c",theOut);
            }
            printf("\n");
        }
    }
    printf("\n");
}

static void TestCmd( int theFD, uint8_t* theCmd, uint32_t theLen) {
    uint8_t tempBuf1[SECTORSIZE];
    uint8_t tempBuf2[SECTORSIZE];

    // Entire sector is always sent, so zero fill cmd
    memset( tempBuf1, 0, SECTORSIZE );
    memcpy( tempBuf1, theCmd, theLen );

    printf( "Sending command:\n");
    hexdump( tempBuf1, (theLen+0x0f)&0x1f0 );
    Do_JM_Cmd( theFD, (uint32_t*)tempBuf1, (uint32_t*)tempBuf2 );
    printf( "Response:\n");
    hexdump(tempBuf2, SECTORSIZE);
}

int main(int argc, char * argv[])
{
    int sg_fd, k;
    uint8_t saveBuf[SECTORSIZE];
    uint8_t probeBuf[SECTORSIZE];
    uint8_t sense_buffer[32];

    printf("JMraidcon version x, Copyright (C) 2010 Werner Johansson\n" \
        "JMraidcon comes with ABSOLUTELY NO WARRANTY.\n" \
        "This is free software, and you are welcome\n" \
        "to redistribute it under certain conditions.\n\n" );

    if (2 != argc) {
        printf("Usage : JMraidcon /dev/sd<X>\n");
        return 1;
    }

    if ((sg_fd = open(argv[1], O_RDWR)) < 0) {
        printf("Cannot open device");
        return 1;
    }

    // Check if the opened device looks like a sg one.
    // Inspired by the sg_simple0 example
    if ((ioctl(sg_fd, SG_GET_VERSION_NUM, &k) < 0) || (k < 30000)) {
        printf("%s is not an sg device, or old sg driver\n", argv[1]);
        return 1;
    }

    // Setup the ioctl struct
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(rwCmdBlk);
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = SECTORSIZE;
    io_hdr.dxferp = saveBuf;
    io_hdr.cmdp = rwCmdBlk;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 3000;

    // Add more error handling like this later
    if( ioctl( sg_fd, SG_IO, &io_hdr ) < 0 ) {
        printf("ioctl SG_IO failed");
        return 1;
    }

    // Generate and send the initial "wakeup" data
    // No idea what the second dword represents at this point
    // Note that these (and all other writes) should be directed to an unused sector!!
    memset( probeBuf, 0, SECTORSIZE );

    // For wide access
    uint32_t* probeBuf32 = (uint32_t*)probeBuf;

    // Populate with the static data
    probeBuf32[0 >> 2] = __cpu_to_le32( 0x197b0325 );
    probeBuf32[0x1f8 >> 2] = __cpu_to_le32( 0x10eca1db );
    for( uint32_t i=0x10; i<0x1f8; i++ ) {
        probeBuf[i] = i&0xff;
    }

    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    rwCmdBlk[0] = WRITE_CMD;
    io_hdr.dxferp = probeBuf;

    // The only value (except the CRC at the end) that changes between the 4 wakeup sectors
    probeBuf32[4 >> 2] = __cpu_to_le32( 0x3c75a80b );
    uint32_t myCRC = JM_CRC( probeBuf32, 0x1fc >> 2 );
    probeBuf32[0x1fc >> 2] = __cpu_to_le32( myCRC );
    ioctl(sg_fd, SG_IO, &io_hdr);

    probeBuf32[4 >> 2] = __cpu_to_le32( 0x0388e337 );
    myCRC = JM_CRC( probeBuf32, 0x1fc >> 2 );
    probeBuf32[0x1fc >> 2] = __cpu_to_le32( myCRC );
    ioctl(sg_fd, SG_IO, &io_hdr);

    probeBuf32[4 >> 2] = __cpu_to_le32( 0x689705f3 );
    myCRC = JM_CRC( probeBuf32, 0x1fc >> 2 );
    probeBuf32[0x1fc >> 2] = __cpu_to_le32( myCRC );
    ioctl(sg_fd, SG_IO, &io_hdr);

    probeBuf32[4 >> 2] = __cpu_to_le32( 0xe00c523a );
    myCRC = JM_CRC( probeBuf32, 0x1fc >> 2 );
    probeBuf32[0x1fc >> 2] = __cpu_to_le32( myCRC );
    ioctl(sg_fd, SG_IO, &io_hdr);

    // Initial probe complete, now send scrambled commands to the same sector

    TestCmd( sg_fd, (uint8_t*)probe6, sizeof(probe6) );
    TestCmd( sg_fd, (uint8_t*)probe7, sizeof(probe7) );
    TestCmd( sg_fd, (uint8_t*)probe8, sizeof(probe8) );
    TestCmd( sg_fd, (uint8_t*)probe9, sizeof(probe9) );
    TestCmd( sg_fd, (uint8_t*)probe11, sizeof(probe11) );
    TestCmd( sg_fd, (uint8_t*)probe12, sizeof(probe12) );
    TestCmd( sg_fd, (uint8_t*)probe13, sizeof(probe13) );
    TestCmd( sg_fd, (uint8_t*)probe14, sizeof(probe14) );
    TestCmd( sg_fd, (uint8_t*)probe15, sizeof(probe15) );
    TestCmd( sg_fd, (uint8_t*)probe16, sizeof(probe16) );
    TestCmd( sg_fd, (uint8_t*)probe17, sizeof(probe17) );
    TestCmd( sg_fd, (uint8_t*)probe18, sizeof(probe18) );
    TestCmd( sg_fd, (uint8_t*)probe19, sizeof(probe19) );
    TestCmd( sg_fd, (uint8_t*)probe20, sizeof(probe20) );
    TestCmd( sg_fd, (uint8_t*)probe21, sizeof(probe21) );
    TestCmd( sg_fd, (uint8_t*)probe23, sizeof(probe23) );
    TestCmd( sg_fd, (uint8_t*)probe24, sizeof(probe24) );
    TestCmd( sg_fd, (uint8_t*)probe25, sizeof(probe25) );
    TestCmd( sg_fd, (uint8_t*)probe26, sizeof(probe26) );
    TestCmd( sg_fd, (uint8_t*)probe27, sizeof(probe27) );
    TestCmd( sg_fd, (uint8_t*)probe28, sizeof(probe28) );

    // Restore the original data to the sector
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    rwCmdBlk[0] = WRITE_CMD;
    io_hdr.dxferp = saveBuf;
    ioctl(sg_fd, SG_IO, &io_hdr);

    close(sg_fd);
    return 0;
}

