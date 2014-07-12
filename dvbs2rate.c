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

/* DVB-S2 useful bitrate */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

double    rate_qpsk[11][3] = {{1.0, 4.0, 12.0}, {1.0, 3.0, 12.0}, {2.0, 5.0, 12.0}, {1.0, 2.0, 12.0}, {3.0, 5.0, 12.0}, {2.0, 3.0, 10.0}, {3.0, 4.0, 12.0}, {4.0, 5.0, 12.0}, {5.0, 6.0, 10.0}, {8.0, 9.0, 8.0}, {9.0, 10.0, 8.0}};
double    rate_8psk[6][3] = {{3.0, 5.0, 12.0}, {2.0, 3.0, 10.0}, {3.0, 4.0, 12.0}, {5.0, 6.0, 10.0}, {8.0, 9.0, 8.0}, {9.0, 10.0, 8.0}};
double    rate_16apsk[6][3] = {{2.0, 3.0, 10.0}, {3.0, 4.0, 12.0}, {4.0, 5.0, 12.0}, {5.0, 6.0, 10.0}, {8.0, 9.0, 8.0}, {9.0, 10.0, 8.0}};
double    rate_32apsk[5][3] = {{3.0, 4.0, 12.0}, {4.0, 5.0, 12.0}, {5.0, 6.0, 10.0}, {8.0, 9.0, 8.0}, {9.0, 10.0, 8.0}};

int main(int argc, char **argv)
{
    int    i;
    double    rate;
    double    symbol_rate = 27500000.0;
    double    fec_frame = 64800.0;
    double    q;

    if (argc != 2) {
        fprintf(stderr, "usage: dvbs2rate <symbol rate>\n");
        exit(-1);
    }

    symbol_rate = atof(argv[1]);
    q = 2.0;
    printf("QPSK, pilots off\n");
    for (i = 0; i < 11; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 0) * (fec_frame * (rate_qpsk[i][0] / rate_qpsk[i][1]) - (16 * rate_qpsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_qpsk[i][0], (int)rate_qpsk[i][1], (int)rate_qpsk[i][2], rate);
    }
    printf("QPSK, pilots on\n");
    for (i = 0; i < 11; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 36) * (fec_frame * (rate_qpsk[i][0] / rate_qpsk[i][1]) - (16 * rate_qpsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_qpsk[i][0], (int)rate_qpsk[i][1], (int)rate_qpsk[i][2], rate);
    }

    q = 3.0;
    printf("8PSK, pilots off\n");
    for (i = 0; i < 6; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 0) * (fec_frame * (rate_8psk[i][0] / rate_8psk[i][1]) - (16 * rate_8psk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_8psk[i][0], (int)rate_8psk[i][1], (int)rate_8psk[i][2], rate);
    }
    printf("8PSK, pilots on\n");
    for (i = 0; i < 6; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 36) * (fec_frame * (rate_8psk[i][0] / rate_8psk[i][1]) - (16 * rate_8psk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_8psk[i][0], (int)rate_8psk[i][1], (int)rate_8psk[i][2], rate);
    }

    q = 4.0;
    printf("16APSK, pilots off\n");
    for (i = 0; i < 6; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 0) * (fec_frame * (rate_16apsk[i][0] / rate_16apsk[i][1]) - (16 * rate_16apsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_16apsk[i][0], (int)rate_16apsk[i][1], (int)rate_16apsk[i][2], rate);
    }
    printf("16APSK, pilots on\n");
    for (i = 0; i < 6; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 36) * (fec_frame * (rate_16apsk[i][0] / rate_16apsk[i][1]) - (16 * rate_16apsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_16apsk[i][0], (int)rate_16apsk[i][1], (int)rate_16apsk[i][2], rate);
    }

    q = 5.0;
    printf("32APSK, pilots off\n");
    for (i = 0; i < 5; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 0) * (fec_frame * (rate_32apsk[i][0] / rate_32apsk[i][1]) - (16 * rate_32apsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_32apsk[i][0], (int)rate_32apsk[i][1], (int)rate_32apsk[i][2], rate);
    }
    printf("32APSK, pilots on\n");
    for (i = 0; i < 5; i++)
    {
        rate = symbol_rate / (fec_frame / q + 90 + ceil((fec_frame/ q / 90 / 16 - 1)) * 36) * (fec_frame * (rate_32apsk[i][0] / rate_32apsk[i][1]) - (16 * rate_32apsk[i][2]) - 80);
        printf("coderate = %d/%d, BCH rate = %d, ts rate = %f\n", (int)rate_32apsk[i][0], (int)rate_32apsk[i][1], (int)rate_32apsk[i][2], rate);
    }

    return 0;
}

