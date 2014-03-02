/*
 * Copyright 2014 Ron Economos (w6rz@comcast.net)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* DVB-T useful bitrate */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int    gi, cr, bandwidth;
    double    bitrate;
    long long    clock_num = 6 * 8000000;
    long long    clock_den = 7;
    long long    carriers_num = 6048;
    long long    packet_num = 188;
    long long    packet_den = 204;
    long long    guard_den;
    long long    bitrate_num, bitrate_den;

    if (argc != 2) {
        fprintf(stderr, "usage: dvbtrate <channel bandwidth>\n");
        exit(-1);
    }

    bandwidth = atoi(argv[1]);
    clock_num = (long long)(bandwidth * 8000000);
    printf("QPSK\n");
    for (cr = 1; cr <=7; cr++)  {
        if (cr != 4 && cr != 6)  {
            printf("coderate = %d/%d", cr, cr + 1);
            for (gi = 4; gi <=32; gi*=2)  {
                bitrate_num = clock_num * carriers_num * 2 * packet_num * cr;
                guard_den = 8192 + (8192 / gi);
                bitrate_den = packet_den * guard_den * (cr + 1) * clock_den;
                bitrate = (double)bitrate_num / (double)bitrate_den;
                printf(" %f", bitrate);
            }
            printf("\n");
        }
    }
    printf("QAM-16\n");
    for (cr = 1; cr <=7; cr++)  {
        if (cr != 4 && cr != 6)  {
            printf("coderate = %d/%d", cr, cr + 1);
            for (gi = 4; gi <=32; gi*=2)  {
                bitrate_num = clock_num * carriers_num * 4 * packet_num * cr;
                guard_den = 8192 + (8192 / gi);
                bitrate_den = packet_den * guard_den * (cr + 1) * clock_den;
                bitrate = (double)bitrate_num / (double)bitrate_den;
                printf(" %f", bitrate);
            }
            printf("\n");
        }
    }
    printf("QAM-64\n");
    for (cr = 1; cr <=7; cr++)  {
        if (cr != 4 && cr != 6)  {
            printf("coderate = %d/%d", cr, cr + 1);
            for (gi = 4; gi <=32; gi*=2)  {
                bitrate_num = clock_num * carriers_num * 6 * packet_num * cr;
                guard_den = 8192 + (8192 / gi);
                bitrate_den = packet_den * guard_den * (cr + 1) * clock_den;
                bitrate = (double)bitrate_num / (double)bitrate_den;
                printf(" %f", bitrate);
            }
            printf("\n");
        }
    }

    return 0;
}

