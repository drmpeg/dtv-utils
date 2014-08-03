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
#include <string.h>
#include <math.h>

#define TRUE            1
#define FALSE           0
#define OPTION_CHAR    '-'

double    rate_qpsk[11][4] = {{1.0, 4.0, 12.0, 2}, {1.0, 3.0, 12.0, 2}, {2.0, 5.0, 12.0, 2}, {1.0, 2.0, 12.0, 2}, {3.0, 5.0, 12.0, 2}, {2.0, 3.0, 10.0, 2}, {3.0, 4.0, 12.0, 2}, {4.0, 5.0, 12.0, 2}, {5.0, 6.0, 10.0, 2}, {8.0, 9.0, 8.0, 2}, {9.0, 10.0, 8.0, 1}};
double    rate_8psk[6][4] = {{3.0, 5.0, 12.0, 2}, {2.0, 3.0, 10.0, 2}, {3.0, 4.0, 12.0, 2}, {5.0, 6.0, 10.0, 2}, {8.0, 9.0, 8.0, 2}, {9.0, 10.0, 8.0, 1}};
double    rate_16apsk[6][4] = {{2.0, 3.0, 10.0, 2}, {3.0, 4.0, 12.0, 2}, {4.0, 5.0, 12.0, 2}, {5.0, 6.0, 10.0, 2}, {8.0, 9.0, 8.0, 2}, {9.0, 10.0, 8.0, 1}};
double    rate_32apsk[5][4] = {{3.0, 4.0, 12.0, 2}, {4.0, 5.0, 12.0, 2}, {5.0, 6.0, 10.0, 2}, {8.0, 9.0, 8.0, 2}, {9.0, 10.0, 8.0, 1}};

double    rate_qpsk_short[10][6] = {{1.0, 4.0, 12.0, 2, 1.0, 5.0}, {1.0, 3.0, 12.0, 2, 1.0, 3.0}, {2.0, 5.0, 12.0, 2, 2.0, 5.0}, {1.0, 2.0, 12.0, 2, 4.0, 9.0}, {3.0, 5.0, 12.0, 2, 3.0, 5.0}, {2.0, 3.0, 12.0, 2, 2.0, 3.0}, {3.0, 4.0, 12.0, 2, 11.0, 15.0}, {4.0, 5.0, 12.0, 2, 7.0, 9.0}, {5.0, 6.0, 12.0, 2, 37.0, 45.0}, {8.0, 9.0, 12.0, 2, 8.0, 9.0}};
double    rate_8psk_short[5][6] = {{3.0, 5.0, 12.0, 2, 3.0, 5.0}, {2.0, 3.0, 12.0, 2, 2.0, 3.0}, {3.0, 4.0, 12.0, 2, 11.0, 15.0}, {5.0, 6.0, 12.0, 2, 37.0, 45.0}, {8.0, 9.0, 12.0, 2, 8.0, 9.0}};
double    rate_16apsk_short[5][6] = {{2.0, 3.0, 12.0, 2, 2.0, 3.0}, {3.0, 4.0, 12.0, 2, 11.0, 15.0}, {4.0, 5.0, 12.0, 2, 7.0, 9.0}, {5.0, 6.0, 12.0, 2, 37.0, 45.0}, {8.0, 9.0, 12.0, 2, 8.0, 9.0}};
double    rate_32apsk_short[4][6] = {{3.0, 4.0, 12.0, 2, 11.0, 15.0}, {4.0, 5.0, 12.0, 2, 7.0, 9.0}, {5.0, 6.0, 12.0, 2, 37.0, 45.0}, {8.0, 9.0, 12.0, 2, 8.0, 9.0}};

double    rate_qpskx[3][4] = {{13.0, 45.0, 12.0, 3}, {9.0, 20.0, 12.0, 4}, {11.0, 20.0, 12.0, 3}};
double    rate_8apsk[2][4] = {{100.0, 180.0, 12.0, 1}, {104.0, 180.0, 12.0, 1}};
double    rate_8pskx[3][4] = {{23.0, 36.0, 12.0, 3}, {25.0, 36.0, 12.0, 3}, {13.0, 18.0, 12.0, 3}};
double    rate_16apskx[8][4] = {{26.0, 45.0, 12.0, 3}, {3.0, 5.0, 12.0, 5}, {28.0, 45.0, 12.0, 3}, {23.0, 36.0, 12.0, 3}, {25.0, 36.0, 12.0, 3}, {13.0, 18.0, 12.0, 3}, {140.0, 180.0, 12.0, 1}, {154.0, 180.0, 12.0, 1}};
double    rate_8_8apsk[5][4] = {{90.0, 180.0, 12.0, 2}, {96.0, 180.0, 12.0, 2}, {100.0, 180.0, 12.0, 1}, {18.0, 30.0, 12.0, 3}, {20.0, 30.0, 12.0, 3}};
double    rate_32_16rbapsk[1][4] = {{2.0, 3.0, 12.0, 5}};
double    rate_32_16apsk[3][4] = {{128.0, 180.0, 12.0, 1}, {132.0, 180.0, 12.0, 1}, {140.0, 180.0, 12.0, 1}};
double    rate_64apsk[1][4] = {{128.0, 180.0, 12.0, 1}};
double    rate_64_28apsk[1][4] = {{132.0, 180.0, 12.0, 1}};
double    rate_64_20apsk[3][4] = {{7.0, 9.0, 12.0, 5}, {4.0, 5.0, 12.0, 5}, {5.0, 6.0, 12.0, 5}};
double    rate_128apsk[2][4] = {{135.0, 180.0, 12.0, 1}, {140.0, 180.0, 12.0, 1}};
double    rate_256apsk[6][4] = {{20.0, 30.0, 12.0, 3}, {22.0, 30.0, 12.0, 3}, {116.0, 180.0, 12.0, 1}, {124.0, 180.0, 12.0, 1}, {128.0, 180.0, 12.0, 1}, {135.0, 180.0, 12.0, 1}};

double    rate_qpskx_short[6][4] = {{11.0, 45.0, 12.0, 3}, {4.0, 15.0, 12.0, 4}, {14.0, 45.0, 12.0, 3}, {7.0, 15.0, 12.0, 4}, {8.0, 15.0, 12.0, 4}, {32.0, 45.0, 12.0, 3}};
double    rate_8pskx_short[4][4] = {{7.0, 15.0, 12.0, 4}, {8.0, 15.0, 12.0, 4}, {26.0, 45.0, 12.0, 3}, {32.0, 45.0, 12.0, 3}};
double    rate_16apskx_short[5][4] = {{7.0, 15.0, 12.0, 4}, {8.0, 15.0, 12.0, 4}, {26.0, 45.0, 12.0, 3}, {3.0, 5.0, 12.0, 5}, {32.0, 45.0, 12.0, 3}};
double    rate_32_16rbapsk_short[2][4] = {{2.0, 3.0, 12.0, 5}, {32.0, 45.0, 12.0, 3}};

double calc(double symbols, double mod, double num, double den, double bch, double pilots)
{
    double    fec_frame = 64800.0;
    double    tsrate;

    tsrate = symbols / (fec_frame / mod + 90 + ceil((fec_frame/ mod / 90 / 16 - 1)) * pilots) * (fec_frame * (num / den) - (16 * bch) - 80);
    return (tsrate);
}

double calc_short(double symbols, double mod, double num, double den, double bch, double pilots)
{
    double    fec_frame = 16200.0;
    double    tsrate;

    tsrate = symbols / (fec_frame / mod + 90 + ceil((fec_frame/ mod / 90 / 16 - 1)) * pilots) * (fec_frame * (num / den) - (16 * bch) - 80);
    return (tsrate);
}

void dump(double rate, double num, double den, double bch, double spaces)
{
    int    i;
    char   s[10];

    for (i = 0; i < (int)spaces; i++)
    {
        s[i] = 0x20;
    }
    s[i] = 0x0;
    printf("coderate = %d/%d,%sBCH rate = %2d, ts rate = %f\n", (int)num, (int)den, s, (int)bch, rate);
}

int main(int argc, char **argv)
{
    int       i;
    int       short_frame = FALSE;
    int       dvb_s2x = FALSE;
    double    rate;
    double    symbol_rate = 27500000.0;
    double    q;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: dvbs2rate -sx <symbol rate>\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\ts = short FECFRAME rates\n");
        fprintf(stderr, "\tx = DVB-S2X rates\n");
        exit(-1);
    }
    if (argc == 2)
    {
        symbol_rate = atof(argv[1]);
    }
    else
    {
        if(*argv[1] == OPTION_CHAR)
        {
            for(i = 1; i < strlen(argv[1]); i++)
            {
                switch (argv[1][i])
                {
                    case 's':
                    case 'S':
                        short_frame = TRUE;
                        break;
                    case 'x':
                    case 'X':
                        dvb_s2x = TRUE;
                        break;
                    default:
                        fprintf(stderr, "Unsupported Option: %c\n", argv[1][i]);
                }
            }
        }
        else
        {
            fprintf(stderr, "usage: dvbs2rate -sx <symbol rate>\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "\ts = short FECFRAME rates\n");
            fprintf(stderr, "\tx = DVB-S2X rates\n");
            exit(-1);
        }
        symbol_rate = atof(argv[2]);
    }

    if (dvb_s2x == FALSE)
    {
        if (short_frame == FALSE)
        {
            printf("DVB-S2 normal FECFRAME\n");
            q = 2.0;
            printf("QPSK, pilots off\n");
            for (i = 0; i < 11; i++)
            {
                rate = calc(symbol_rate, q, rate_qpsk[i][0], rate_qpsk[i][1], rate_qpsk[i][2], 0.0);
                dump(rate, rate_qpsk[i][0], rate_qpsk[i][1], rate_qpsk[i][2], rate_qpsk[i][3]);
            }
            printf("QPSK, pilots on\n");
            for (i = 0; i < 11; i++)
            {
                rate = calc(symbol_rate, q, rate_qpsk[i][0], rate_qpsk[i][1], rate_qpsk[i][2], 36.0);
                dump(rate, rate_qpsk[i][0], rate_qpsk[i][1], rate_qpsk[i][2], rate_qpsk[i][3]);
            }

            q = 3.0;
            printf("8PSK, pilots off\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_8psk[i][0], rate_8psk[i][1], rate_8psk[i][2], 0.0);
                dump(rate, rate_8psk[i][0], rate_8psk[i][1], rate_8psk[i][2], rate_8psk[i][3]);
            }
            printf("8PSK, pilots on\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_8psk[i][0], rate_8psk[i][1], rate_8psk[i][2], 36.0);
                dump(rate, rate_8psk[i][0], rate_8psk[i][1], rate_8psk[i][2], rate_8psk[i][3]);
            }

            q = 4.0;
            printf("16APSK, pilots off\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_16apsk[i][0], rate_16apsk[i][1], rate_16apsk[i][2], 0.0);
                dump(rate, rate_16apsk[i][0], rate_16apsk[i][1], rate_16apsk[i][2], rate_16apsk[i][3]);
            }
            printf("16APSK, pilots on\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_16apsk[i][0], rate_16apsk[i][1], rate_16apsk[i][2], 36.0);
                dump(rate, rate_16apsk[i][0], rate_16apsk[i][1], rate_16apsk[i][2], rate_16apsk[i][3]);
            }

            q = 5.0;
            printf("32APSK, pilots off\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc(symbol_rate, q, rate_32apsk[i][0], rate_32apsk[i][1], rate_32apsk[i][2], 0.0);
                dump(rate, rate_32apsk[i][0], rate_32apsk[i][1], rate_32apsk[i][2], rate_32apsk[i][3]);
            }
            printf("32APSK, pilots on\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc(symbol_rate, q, rate_32apsk[i][0], rate_32apsk[i][1], rate_32apsk[i][2], 36.0);
                dump(rate, rate_32apsk[i][0], rate_32apsk[i][1], rate_32apsk[i][2], rate_32apsk[i][3]);
            }
        }
        else
        {
            printf("DVB-S2 short FECFRAME\n");
            q = 2.0;
            printf("QPSK, pilots off\n");
            for (i = 0; i < 10; i++)
            {
                rate = calc_short(symbol_rate, q, rate_qpsk_short[i][4], rate_qpsk_short[i][5], rate_qpsk_short[i][2], 0.0);
                dump(rate, rate_qpsk_short[i][0], rate_qpsk_short[i][1], rate_qpsk_short[i][2], rate_qpsk_short[i][3]);
            }
            printf("QPSK, pilots on\n");
            for (i = 0; i < 10; i++)
            {
                rate = calc_short(symbol_rate, q, rate_qpsk_short[i][4], rate_qpsk_short[i][5], rate_qpsk_short[i][2], 36.0);
                dump(rate, rate_qpsk_short[i][0], rate_qpsk_short[i][1], rate_qpsk_short[i][2], rate_qpsk_short[i][3]);
            }

            q = 3.0;
            printf("8PSK, pilots off\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_8psk_short[i][4], rate_8psk_short[i][5], rate_8psk_short[i][2], 0.0);
                dump(rate, rate_8psk_short[i][0], rate_8psk_short[i][1], rate_8psk_short[i][2], rate_8psk_short[i][3]);
            }
            printf("8PSK, pilots on\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_8psk_short[i][4], rate_8psk_short[i][5], rate_8psk_short[i][2], 36.0);
                dump(rate, rate_8psk_short[i][0], rate_8psk_short[i][1], rate_8psk_short[i][2], rate_8psk_short[i][3]);
            }

            q = 4.0;
            printf("16APSK, pilots off\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_16apsk_short[i][4], rate_16apsk_short[i][5], rate_16apsk_short[i][2], 0.0);
                dump(rate, rate_16apsk_short[i][0], rate_16apsk_short[i][1], rate_16apsk_short[i][2], rate_16apsk_short[i][3]);
            }
            printf("16APSK, pilots on\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_16apsk_short[i][4], rate_16apsk_short[i][5], rate_16apsk_short[i][2], 36.0);
                dump(rate, rate_16apsk_short[i][0], rate_16apsk_short[i][1], rate_16apsk_short[i][2], rate_16apsk_short[i][3]);
            }

            q = 5.0;
            printf("32APSK, pilots off\n");
            for (i = 0; i < 4; i++)
            {
                rate = calc_short(symbol_rate, q, rate_32apsk_short[i][4], rate_32apsk_short[i][5], rate_32apsk_short[i][2], 0.0);
                dump(rate, rate_32apsk_short[i][0], rate_32apsk_short[i][1], rate_32apsk_short[i][2], rate_32apsk_short[i][3]);
            }
            printf("32APSK, pilots on\n");
            for (i = 0; i < 4; i++)
            {
                rate = calc_short(symbol_rate, q, rate_32apsk_short[i][4], rate_32apsk_short[i][5], rate_32apsk_short[i][2], 36.0);
                dump(rate, rate_32apsk_short[i][0], rate_32apsk_short[i][1], rate_32apsk_short[i][2], rate_32apsk_short[i][3]);
            }
        }
    }
    else
    {
        if (short_frame == FALSE)
        {
            printf("DVB-S2X normal FECFRAME\n");
            q = 2.0;
            printf("QPSK, pilots off\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_qpskx[i][0], rate_qpskx[i][1], rate_qpskx[i][2], 0.0);
                dump(rate, rate_qpskx[i][0], rate_qpskx[i][1], rate_qpskx[i][2], rate_qpskx[i][3]);
            }
            printf("QPSK, pilots on\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_qpskx[i][0], rate_qpskx[i][1], rate_qpskx[i][2], 36.0);
                dump(rate, rate_qpskx[i][0], rate_qpskx[i][1], rate_qpskx[i][2], rate_qpskx[i][3]);
            }

            q = 3.0;
            printf("8APSK, pilots off\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc(symbol_rate, q, rate_8apsk[i][0], rate_8apsk[i][1], rate_8apsk[i][2], 0.0);
                dump(rate, rate_8apsk[i][0], rate_8apsk[i][1], rate_8apsk[i][2], rate_8apsk[i][3]);
            }
            printf("8APSK, pilots on\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc(symbol_rate, q, rate_8apsk[i][0], rate_8apsk[i][1], rate_8apsk[i][2], 36.0);
                dump(rate, rate_8apsk[i][0], rate_8apsk[i][1], rate_8apsk[i][2], rate_8apsk[i][3]);
            }

            q = 3.0;
            printf("8PSK, pilots off\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_8pskx[i][0], rate_8pskx[i][1], rate_8pskx[i][2], 0.0);
                dump(rate, rate_8pskx[i][0], rate_8pskx[i][1], rate_8pskx[i][2], rate_8pskx[i][3]);
            }
            printf("8PSK, pilots on\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_8pskx[i][0], rate_8pskx[i][1], rate_8pskx[i][2], 36.0);
                dump(rate, rate_8pskx[i][0], rate_8pskx[i][1], rate_8pskx[i][2], rate_8pskx[i][3]);
            }

            q = 4.0;
            printf("16APSK, pilots off\n");
            for (i = 0; i < 8; i++)
            {
                rate = calc(symbol_rate, q, rate_16apskx[i][0], rate_16apskx[i][1], rate_16apskx[i][2], 0.0);
                dump(rate, rate_16apskx[i][0], rate_16apskx[i][1], rate_16apskx[i][2], rate_16apskx[i][3]);
            }
            printf("16APSK, pilots on\n");
            for (i = 0; i < 8; i++)
            {
                rate = calc(symbol_rate, q, rate_16apskx[i][0], rate_16apskx[i][1], rate_16apskx[i][2], 36.0);
                dump(rate, rate_16apskx[i][0], rate_16apskx[i][1], rate_16apskx[i][2], rate_16apskx[i][3]);
            }

            q = 4.0;
            printf("8+8APSK, pilots off\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc(symbol_rate, q, rate_8_8apsk[i][0], rate_8_8apsk[i][1], rate_8_8apsk[i][2], 0.0);
                dump(rate, rate_8_8apsk[i][0], rate_8_8apsk[i][1], rate_8_8apsk[i][2], rate_8_8apsk[i][3]);
            }
            printf("8+8APSK, pilots on\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc(symbol_rate, q, rate_8_8apsk[i][0], rate_8_8apsk[i][1], rate_8_8apsk[i][2], 36.0);
                dump(rate, rate_8_8apsk[i][0], rate_8_8apsk[i][1], rate_8_8apsk[i][2], rate_8_8apsk[i][3]);
            }

            q = 5.0;
            printf("4+12+16rbAPSK, pilots off\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_32_16rbapsk[i][0], rate_32_16rbapsk[i][1], rate_32_16rbapsk[i][2], 0.0);
                dump(rate, rate_32_16rbapsk[i][0], rate_32_16rbapsk[i][1], rate_32_16rbapsk[i][2], rate_32_16rbapsk[i][3]);
            }
            printf("4+12+16rbAPSK, pilots on\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_32_16rbapsk[i][0], rate_32_16rbapsk[i][1], rate_32_16rbapsk[i][2], 36.0);
                dump(rate, rate_32_16rbapsk[i][0], rate_32_16rbapsk[i][1], rate_32_16rbapsk[i][2], rate_32_16rbapsk[i][3]);
            }

            q = 5.0;
            printf("4+8+4+16APSK, pilots off\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_32_16apsk[i][0], rate_32_16apsk[i][1], rate_32_16apsk[i][2], 0.0);
                dump(rate, rate_32_16apsk[i][0], rate_32_16apsk[i][1], rate_32_16apsk[i][2], rate_32_16apsk[i][3]);
            }
            printf("4+8+4+16APSK, pilots on\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_32_16apsk[i][0], rate_32_16apsk[i][1], rate_32_16apsk[i][2], 36.0);
                dump(rate, rate_32_16apsk[i][0], rate_32_16apsk[i][1], rate_32_16apsk[i][2], rate_32_16apsk[i][3]);
            }

            q = 6.0;
            printf("64APSK, pilots off\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_64apsk[i][0], rate_64apsk[i][1], rate_64apsk[i][2], 0.0);
                dump(rate, rate_64apsk[i][0], rate_64apsk[i][1], rate_64apsk[i][2], rate_64apsk[i][3]);
            }
            printf("64APSK, pilots on\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_64apsk[i][0], rate_64apsk[i][1], rate_64apsk[i][2], 36.0);
                dump(rate, rate_64apsk[i][0], rate_64apsk[i][1], rate_64apsk[i][2], rate_64apsk[i][3]);
            }

            q = 6.0;
            printf("4+12+20+28APSK, pilots off\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_64_28apsk[i][0], rate_64_28apsk[i][1], rate_64_28apsk[i][2], 0.0);
                dump(rate, rate_64_28apsk[i][0], rate_64_28apsk[i][1], rate_64_28apsk[i][2], rate_64_28apsk[i][3]);
            }
            printf("4+12+20+28APSK, pilots on\n");
            for (i = 0; i < 1; i++)
            {
                rate = calc(symbol_rate, q, rate_64_28apsk[i][0], rate_64_28apsk[i][1], rate_64_28apsk[i][2], 36.0);
                dump(rate, rate_64_28apsk[i][0], rate_64_28apsk[i][1], rate_64_28apsk[i][2], rate_64_28apsk[i][3]);
            }

            q = 6.0;
            printf("8+16+20+20APSK, pilots off\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_64_20apsk[i][0], rate_64_20apsk[i][1], rate_64_20apsk[i][2], 0.0);
                dump(rate, rate_64_20apsk[i][0], rate_64_20apsk[i][1], rate_64_20apsk[i][2], rate_64_20apsk[i][3]);
            }
            printf("8+16+20+20APSK, pilots on\n");
            for (i = 0; i < 3; i++)
            {
                rate = calc(symbol_rate, q, rate_64_20apsk[i][0], rate_64_20apsk[i][1], rate_64_20apsk[i][2], 36.0);
                dump(rate, rate_64_20apsk[i][0], rate_64_20apsk[i][1], rate_64_20apsk[i][2], rate_64_20apsk[i][3]);
            }

            q = 7.0;
            printf("128APSK, pilots off\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc(symbol_rate, q, rate_128apsk[i][0], rate_128apsk[i][1], rate_128apsk[i][2], 0.0);
                dump(rate, rate_128apsk[i][0], rate_128apsk[i][1], rate_128apsk[i][2], rate_128apsk[i][3]);
            }
            printf("128APSK, pilots on\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc(symbol_rate, q, rate_128apsk[i][0], rate_128apsk[i][1], rate_128apsk[i][2], 36.0);
                dump(rate, rate_128apsk[i][0], rate_128apsk[i][1], rate_128apsk[i][2], rate_128apsk[i][3]);
            }

            q = 8.0;
            printf("256APSK, pilots off\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_256apsk[i][0], rate_256apsk[i][1], rate_256apsk[i][2], 0.0);
                dump(rate, rate_256apsk[i][0], rate_256apsk[i][1], rate_256apsk[i][2], rate_256apsk[i][3]);
            }
            printf("256APSK, pilots on\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc(symbol_rate, q, rate_256apsk[i][0], rate_256apsk[i][1], rate_256apsk[i][2], 36.0);
                dump(rate, rate_256apsk[i][0], rate_256apsk[i][1], rate_256apsk[i][2], rate_256apsk[i][3]);
            }
        }
        else
        {
            printf("DVB-S2X short FECFRAME\n");
            q = 2.0;
            printf("QPSK, pilots off\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc_short(symbol_rate, q, rate_qpskx_short[i][0], rate_qpskx_short[i][1], rate_qpskx_short[i][2], 0.0);
                dump(rate, rate_qpskx_short[i][0], rate_qpskx_short[i][1], rate_qpskx_short[i][2], rate_qpskx_short[i][3]);
            }
            printf("QPSK, pilots on\n");
            for (i = 0; i < 6; i++)
            {
                rate = calc_short(symbol_rate, q, rate_qpskx_short[i][0], rate_qpskx_short[i][1], rate_qpskx_short[i][2], 36.0);
                dump(rate, rate_qpskx_short[i][0], rate_qpskx_short[i][1], rate_qpskx_short[i][2], rate_qpskx_short[i][3]);
            }

            q = 3.0;
            printf("8PSK, pilots off\n");
            for (i = 0; i < 4; i++)
            {
                rate = calc_short(symbol_rate, q, rate_8pskx_short[i][0], rate_8pskx_short[i][1], rate_8pskx_short[i][2], 0.0);
                dump(rate, rate_8pskx_short[i][0], rate_8pskx_short[i][1], rate_8pskx_short[i][2], rate_8pskx_short[i][3]);
            }
            printf("8PSK, pilots on\n");
            for (i = 0; i < 4; i++)
            {
                rate = calc_short(symbol_rate, q, rate_8pskx_short[i][0], rate_8pskx_short[i][1], rate_8pskx_short[i][2], 36.0);
                dump(rate, rate_8pskx_short[i][0], rate_8pskx_short[i][1], rate_8pskx_short[i][2], rate_8pskx_short[i][3]);
            }

            q = 4.0;
            printf("16APSK, pilots off\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_16apskx_short[i][0], rate_16apskx_short[i][1], rate_16apskx_short[i][2], 0.0);
                dump(rate, rate_16apskx_short[i][0], rate_16apskx_short[i][1], rate_16apskx_short[i][2], rate_16apskx_short[i][3]);
            }
            printf("16APSK, pilots on\n");
            for (i = 0; i < 5; i++)
            {
                rate = calc_short(symbol_rate, q, rate_16apskx_short[i][0], rate_16apskx_short[i][1], rate_16apskx_short[i][2], 36.0);
                dump(rate, rate_16apskx_short[i][0], rate_16apskx_short[i][1], rate_16apskx_short[i][2], rate_16apskx_short[i][3]);
            }

            q = 5.0;
            printf("4+12+16rbAPSK, pilots off\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc_short(symbol_rate, q, rate_32_16rbapsk_short[i][0], rate_32_16rbapsk_short[i][1], rate_32_16rbapsk_short[i][2], 0.0);
                dump(rate, rate_32_16rbapsk_short[i][0], rate_32_16rbapsk_short[i][1], rate_32_16rbapsk_short[i][2], rate_32_16rbapsk_short[i][3]);
            }
            printf("4+12+16rbAPSK, pilots on\n");
            for (i = 0; i < 2; i++)
            {
                rate = calc_short(symbol_rate, q, rate_32_16rbapsk_short[i][0], rate_32_16rbapsk_short[i][1], rate_32_16rbapsk_short[i][2], 36.0);
                dump(rate, rate_32_16rbapsk_short[i][0], rate_32_16rbapsk_short[i][1], rate_32_16rbapsk_short[i][2], rate_32_16rbapsk_short[i][3]);
            }
        }
    }

    return 0;
}

