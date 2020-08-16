/*
 * Copyright 2017 Ron Economos (w6rz@comcast.net)
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

/* DVB-T2 useful bitrate */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KBCH_1_2 7032
#define KSIG_POST 350
#define NBCH_PARITY 168
#undef MISO

    enum dvbt2_code_rate_t {
      C1_2 = 1,
      C3_5,
      C2_3,
      C3_4,
      C4_5,
      C5_6,
      C1_3,
      C2_5,
    };

    enum dvbt2_constellation_t {
      MOD_BPSK = 0,
      MOD_QPSK,
      MOD_16QAM,
      MOD_64QAM,
      MOD_256QAM,
      MOD_1024QAM,
      MOD_4096QAM,
    };

    enum dvbt2_framesize_t {
      FECFRAME_NORMAL = 0,
      FECFRAME_SHORT,
    };

    enum dvbt2_extended_carrier_t {
      CARRIERS_NORMAL = 0,
      CARRIERS_EXTENDED,
    };

    enum dvbt2_fftsize_t {
      FFTSIZE_2K = 0,
      FFTSIZE_8K_NORM,
      FFTSIZE_4K,
      FFTSIZE_1K,
      FFTSIZE_16K,
      FFTSIZE_32K_NORM,
      FFTSIZE_8K_SGI,
      FFTSIZE_32K_SGI,
    };

    enum dvbt2_pilotpattern_t {
      PILOT_PP1 = 1,
      PILOT_PP2,
      PILOT_PP3,
      PILOT_PP4,
      PILOT_PP5,
      PILOT_PP6,
      PILOT_PP7,
      PILOT_PP8,
    };

    enum dvbt2_guardinterval_t {
      GI_1_32 = 0,
      GI_1_16,
      GI_1_8,
      GI_1_4,
      GI_1_128,
      GI_19_128,
      GI_19_256,
    };

int main(int argc, char **argv)
{
    int    fftsize, numsymbols, rate, N_P2, C_P2, C_DATA, N_FC, C_FC;
    int    cells, N_punc_temp, N_post_temp, N_post, D_L1, cell_size, eta_mod;
    int    constellation;
    int    framesize;
    int    fft_size;
    int    guardinterval;
    int    pilotpattern;
    int    carriermode;
    int    bandwidth;
    double    clock_num, clock_den = 7.0;
    double    bitrate, T, TU, TS, TF, GI, symbols, fecblocks, kbch, max_symbols;

    if (argc != 12) {
        fprintf(stderr, "usage: dvbt2rate <channel bandwidth> <fft size> <guard interval> <number of data symbols> <number of FEC blocks> <code rate> <modulation> <frame size> <extended carrier> <pilot pattern> <L1 modulation>\n");
        exit(-1);
    }

    bandwidth = atoi(argv[1]);
    if (bandwidth == 0)
    {
        clock_num = 131000000.0;
        clock_den = 71.0;
    }
    else
    {
        clock_num = (bandwidth * 8000000.0);
    }
    fftsize = atoi(argv[2]);
    fftsize *= 1024;
    switch (fftsize)
    {
        case 1024:
            N_P2 = 16;
            fft_size = FFTSIZE_1K;
            break;
        case 2048:
            N_P2 = 8;
            fft_size = FFTSIZE_2K;
            break;
        case 4096:
            N_P2 = 4;
            fft_size = FFTSIZE_4K;
            break;
        case 8192:
            N_P2 = 2;
            fft_size = FFTSIZE_8K_NORM;
            break;
        case 16384:
            N_P2 = 1;
            fft_size = FFTSIZE_16K;
            break;
        case 32768:
            N_P2 = 1;
            fft_size = FFTSIZE_32K_NORM;
            break;
        default:
            N_P2 = 0;
            fft_size = FFTSIZE_1K;
            break;
    }
    guardinterval = atoi(argv[3]);
    switch (guardinterval)
    {
        case GI_1_32:
            GI = 1.0 / 32.0;
            break;
        case GI_1_16:
            GI = 1.0 / 16.0;
            break;
        case GI_1_8:
            GI = 1.0 / 8.0;
            break;
        case GI_1_4:
            GI = 1.0 / 4.0;
            break;
        case GI_1_128:
            GI = 1.0 / 128.0;
            break;
        case GI_19_128:
            GI = 19.0 / 128.0;
            break;
        case GI_19_256:
            GI = 19.0 / 256.0;
            break;
    }
    numsymbols = atoi(argv[4]);
    fecblocks = atof(argv[5]);
    rate = atoi(argv[6]);
    constellation = atoi(argv[7]);
    framesize = atoi(argv[8]);
    carriermode = atoi(argv[9]);
    pilotpattern = atoi(argv[10]);
    switch (atoi(argv[11]))
    {
        case MOD_BPSK:
            eta_mod = 1;
            break;
        case MOD_QPSK:
            eta_mod = 2;
            break;
        case MOD_16QAM:
            eta_mod = 4;
            break;
        case MOD_64QAM:
            eta_mod = 6;
            break;
    }
    if (framesize == FECFRAME_NORMAL)
    {
        switch (rate)
        {
            case C1_2:
                kbch = 32208.0;
                break;
            case C3_5:
                kbch = 38688.0;
                break;
            case C2_3:
                kbch = 43040.0;
                break;
            case C3_4:
                kbch = 48408.0;
                break;
            case C4_5:
                kbch = 51648.0;
                break;
            case C5_6:
                kbch = 53840.0;
                break;
            default:
                kbch = 0.0;
                break;
        }
    }
    else
    {
        switch (rate)
        {
            case C1_3:
                kbch = 5232.0;
                break;
            case C2_5:
                kbch = 6312.0;
                break;
            case C1_2:
                kbch = 7032.0;
                break;
            case C3_5:
                kbch = 9552.0;
                break;
            case C2_3:
                kbch = 10632.0;
                break;
            case C3_4:
                kbch = 11712.0;
                break;
            case C4_5:
                kbch = 12432.0;
                break;
            case C5_6:
                kbch = 13152.0;
                break;
            default:
                kbch = 0;
                break;
        }
    }

    printf("FFT size = %d\n", fftsize);
    switch (guardinterval)
    {
        case GI_1_32:
            printf("guard interval = 1/32\n");
            break;
        case GI_1_16:
            printf("guard interval = 1/16\n");
            break;
        case GI_1_8:
            printf("guard interval = 1/8\n");
            break;
        case GI_1_4:
            printf("guard interval = 1/4\n");
            break;
        case GI_1_128:
            printf("guard interval = 1/128\n");
            break;
        case GI_19_128:
            printf("guard interval = 19/128\n");
            break;
        case GI_19_256:
            printf("guard interval = 19/256\n");
            break;
        default:
            printf("guard interval = invalid\n");
            break;
    }
    printf("number of data symbols = %d\n", numsymbols);
    printf("number of FEC blocks = %d\n", (int)fecblocks);
    switch (rate)
    {
        case C1_2:
            printf("code rate = 1/2\n");
            break;
        case C3_5:
            printf("code rate = 3/5\n");
            break;
        case C2_3:
            printf("code rate = 2/3\n");
            break;
        case C3_4:
            printf("code rate = 3/4\n");
            break;
        case C4_5:
            printf("code rate = 4/5\n");
            break;
        case C5_6:
            printf("code rate = 5/6\n");
            break;
        case C1_3:
            printf("code rate = 1/3\n");
            break;
        case C2_5:
            printf("code rate = 2/5\n");
            break;
        default:
            printf("code rate = invalid\n");
            break;
    }
    switch (constellation)
    {
        case MOD_QPSK:
            printf("constellation = QPSK\n");
            break;
        case MOD_16QAM:
            printf("constellation = 16QAM\n");
            break;
        case MOD_64QAM:
            printf("constellation = 64QAM\n");
            break;
        case MOD_256QAM:
            printf("constellation = 256QAM\n");
            break;
        case MOD_1024QAM:
            printf("constellation = 1024QAM\n");
            break;
        case MOD_4096QAM:
            printf("constellation = 4096QAM\n");
            break;
        default:
            printf("constellation = invalid\n");
            break;
    }
    if (framesize == FECFRAME_NORMAL)
    {
        printf("frame size = normal\n");
    }
    else if (framesize == FECFRAME_SHORT)
    {
        printf("frame size = short\n");
    }
    else
    {
        printf("frame size = invalid\n");
    }
    if (carriermode == CARRIERS_NORMAL)
    {
        printf("carrier mode = normal\n");
    }
    else if (carriermode == CARRIERS_EXTENDED)
    {
        printf("carrier mode = extended\n");
    }
    else
    {
        printf("carrier mode = invalid\n");
    }
    switch (pilotpattern)
    {
        case PILOT_PP1:
            printf("pilot pattern = PP1\n");
            break;
        case PILOT_PP2:
            printf("pilot pattern = PP2\n");
            break;
        case PILOT_PP3:
            printf("pilot pattern = PP3\n");
            break;
        case PILOT_PP4:
            printf("pilot pattern = PP4\n");
            break;
        case PILOT_PP5:
            printf("pilot pattern = PP5\n");
            break;
        case PILOT_PP6:
            printf("pilot pattern = PP6\n");
            break;
        case PILOT_PP7:
            printf("pilot pattern = PP7\n");
            break;
        case PILOT_PP8:
            printf("pilot pattern = PP8\n");
            break;
        default:
            printf("pilot pattern = invalid\n");
            break;
    }
    switch (atoi(argv[11]))
    {
        case MOD_BPSK:
            printf("L1 constellation = BPSK\n");
            break;
        case MOD_QPSK:
            printf("L1 constellation = QPSK\n");
            break;
        case MOD_16QAM:
            printf("L1 constellation = 16QAM\n");
            break;
        case MOD_64QAM:
            printf("L1 constellation = 64QAM\n");
            break;
        default:
            printf("L1 constellation = invalid\n");
            break;
    }
    printf("\n");

    symbols = numsymbols + N_P2;
    T = 1.0 / (clock_num / clock_den);
    TU = T * fftsize;
    TS = TU * (1.0 + GI);
    TF = (symbols * TS) + (2048.0 * T);
    max_symbols = floor(0.25 / TS);
    if (fft_size == FFTSIZE_32K_NORM || fft_size == FFTSIZE_32K_SGI)
    {
        max_symbols = ((int)max_symbols / 2) * 2;
    }
    printf("clock rate = %f, TF = %f ms\n", clock_num / clock_den, TF * 1000.0);

    bitrate = (1.0 / TF) * (188.0 / 188.0) * ((fecblocks * (kbch - 80.0)) - 0.0);
    printf("Normal mode bitrate = %f\n", bitrate);
    bitrate = (1.0 / TF) * (188.0 / 187.0) * ((fecblocks * (kbch - 80.0)) - 0.0);
    printf("High Efficiency mode bitrate = %f\n\n", bitrate);

    if (framesize == FECFRAME_NORMAL)
    {
        switch (constellation)
        {
            case MOD_QPSK:
                cell_size = 32400;
                break;
            case MOD_16QAM:
                cell_size = 16200;
                break;
            case MOD_64QAM:
                cell_size = 10800;
                break;
            case MOD_256QAM:
                cell_size = 8100;
                break;
            case MOD_1024QAM:
                cell_size = 6480;
                break;
            case MOD_4096QAM:
                cell_size = 5400;
                break;
            default:
                cell_size = 0;
                break;
        }
    }
    else
    {
        switch (constellation)
        {
            case MOD_QPSK:
                cell_size = 8100;
                break;
            case MOD_16QAM:
                cell_size = 4050;
                break;
            case MOD_64QAM:
                cell_size = 2700;
                break;
            case MOD_256QAM:
                cell_size = 2025;
                break;
            case MOD_1024QAM:
                cell_size = 1620;
                break;
            case MOD_4096QAM:
                cell_size = 1350;
                break;
            default:
                cell_size = 0;
                break;
        }
    }
#ifdef MISO
    switch (fft_size)
    {
        case FFTSIZE_1K:
            N_P2 = 16;
            C_P2 = 546;
            break;
        case FFTSIZE_2K:
            N_P2 = 8;
            C_P2 = 1098;
            break;
        case FFTSIZE_4K:
            N_P2 = 4;
            C_P2 = 2198;
            break;
        case FFTSIZE_8K_NORM:
        case FFTSIZE_8K_SGI:
            N_P2 = 2;
            C_P2 = 4398;
            break;
        case FFTSIZE_16K:
            N_P2 = 1;
            C_P2 = 8814;
            break;
        case FFTSIZE_32K_NORM:
        case FFTSIZE_32K_SGI:
            N_P2 = 1;
            C_P2 = 17612;
            break;
        default:
            N_P2 = 0;
            C_P2 = 0;
            break;
    }
#else
    switch (fft_size)
    {
        case FFTSIZE_1K:
            N_P2 = 16;
            C_P2 = 558;
            break;
        case FFTSIZE_2K:
            N_P2 = 8;
            C_P2 = 1118;
            break;
        case FFTSIZE_4K:
            N_P2 = 4;
            C_P2 = 2236;
            break;
        case FFTSIZE_8K_NORM:
        case FFTSIZE_8K_SGI:
            N_P2 = 2;
            C_P2 = 4472;
            break;
        case FFTSIZE_16K:
            N_P2 = 1;
            C_P2 = 8944;
            break;
        case FFTSIZE_32K_NORM:
        case FFTSIZE_32K_SGI:
            N_P2 = 1;
            C_P2 = 22432;
            break;
        default:
            N_P2 = 0;
            C_P2 = 0;
            break;
    }
#endif
    switch (fft_size)
    {
        case FFTSIZE_1K:
            switch (pilotpattern)
            {
                case PILOT_PP1:
                    C_DATA = 764;
                    N_FC = 568;
                    C_FC = 402;
                    break;
                case PILOT_PP2:
                    C_DATA = 768;
                    N_FC = 710;
                    C_FC = 654;
                    break;
                case PILOT_PP3:
                    C_DATA = 798;
                    N_FC = 710;
                    C_FC = 490;
                    break;
                case PILOT_PP4:
                    C_DATA = 804;
                    N_FC = 780;
                    C_FC = 707;
                    break;
                case PILOT_PP5:
                    C_DATA = 818;
                    N_FC = 780;
                    C_FC = 544;
                    break;
                case PILOT_PP6:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                case PILOT_PP7:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                case PILOT_PP8:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                default:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
            }
            break;
        case FFTSIZE_2K:
            switch (pilotpattern)
            {
                case PILOT_PP1:
                    C_DATA = 1522;
                    N_FC = 1136;
                    C_FC = 804;
                    break;
                case PILOT_PP2:
                    C_DATA = 1532;
                    N_FC = 1420;
                    C_FC = 1309;
                    break;
                case PILOT_PP3:
                    C_DATA = 1596;
                    N_FC = 1420;
                    C_FC = 980;
                    break;
                case PILOT_PP4:
                    C_DATA = 1602;
                    N_FC = 1562;
                    C_FC = 1415;
                    break;
                case PILOT_PP5:
                    C_DATA = 1632;
                    N_FC = 1562;
                    C_FC = 1088;
                    break;
                case PILOT_PP6:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                case PILOT_PP7:
                    C_DATA = 1646;
                    N_FC = 1632;
                    C_FC = 1396;
                    break;
                case PILOT_PP8:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                default:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
            }
            break;
        case FFTSIZE_4K:
            switch (pilotpattern)
            {
                case PILOT_PP1:
                    C_DATA = 3084;
                    N_FC = 2272;
                    C_FC = 1609;
                    break;
                case PILOT_PP2:
                    C_DATA = 3092;
                    N_FC = 2840;
                    C_FC = 2619;
                    break;
                case PILOT_PP3:
                    C_DATA = 3228;
                    N_FC = 2840;
                    C_FC = 1961;
                    break;
                case PILOT_PP4:
                    C_DATA = 3234;
                    N_FC = 3124;
                    C_FC = 2831;
                    break;
                case PILOT_PP5:
                    C_DATA = 3298;
                    N_FC = 3124;
                    C_FC = 2177;
                    break;
                case PILOT_PP6:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                case PILOT_PP7:
                    C_DATA = 3328;
                    N_FC = 3266;
                    C_FC = 2792;
                    break;
                case PILOT_PP8:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
                default:
                    C_DATA = 0;
                    N_FC = 0;
                    C_FC = 0;
                    break;
            }
            break;
        case FFTSIZE_8K_NORM:
        case FFTSIZE_8K_SGI:
            if (carriermode == CARRIERS_NORMAL)
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 6208;
                        N_FC = 4544;
                        C_FC = 3218;
                        break;
                    case PILOT_PP2:
                        C_DATA = 6214;
                        N_FC = 5680;
                        C_FC = 5238;
                        break;
                    case PILOT_PP3:
                        C_DATA = 6494;
                        N_FC = 5680;
                        C_FC = 3922;
                        break;
                    case PILOT_PP4:
                        C_DATA = 6498;
                        N_FC = 6248;
                        C_FC = 5662;
                        break;
                    case PILOT_PP5:
                        C_DATA = 6634;
                        N_FC = 6248;
                        C_FC = 4354;
                        break;
                    case PILOT_PP6:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP7:
                        C_DATA = 6698;
                        N_FC = 6532;
                        C_FC = 5585;
                        break;
                    case PILOT_PP8:
                        C_DATA = 6698;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            else
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 6296;
                        N_FC = 4608;
                        C_FC = 3264;
                        break;
                    case PILOT_PP2:
                        C_DATA = 6298;
                        N_FC = 5760;
                        C_FC = 5312;
                        break;
                    case PILOT_PP3:
                        C_DATA = 6584;
                        N_FC = 5760;
                        C_FC = 3978;
                        break;
                    case PILOT_PP4:
                        C_DATA = 6588;
                        N_FC = 6336;
                        C_FC = 5742;
                        break;
                    case PILOT_PP5:
                        C_DATA = 6728;
                        N_FC = 6336;
                        C_FC = 4416;
                        break;
                    case PILOT_PP6:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP7:
                        C_DATA = 6788;
                        N_FC = 6624;
                        C_FC = 5664;
                        break;
                    case PILOT_PP8:
                        C_DATA = 6788;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            break;
        case FFTSIZE_16K:
            if (carriermode == CARRIERS_NORMAL)
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 12418;
                        N_FC = 9088;
                        C_FC = 6437;
                        break;
                    case PILOT_PP2:
                        C_DATA = 12436;
                        N_FC = 11360;
                        C_FC = 10476;
                        break;
                    case PILOT_PP3:
                        C_DATA = 12988;
                        N_FC = 11360;
                        C_FC = 7845;
                        break;
                    case PILOT_PP4:
                        C_DATA = 13002;
                        N_FC = 12496;
                        C_FC = 11324;
                        break;
                    case PILOT_PP5:
                        C_DATA = 13272;
                        N_FC = 12496;
                        C_FC = 8709;
                        break;
                    case PILOT_PP6:
                        C_DATA = 13288;
                        N_FC = 13064;
                        C_FC = 11801;
                        break;
                    case PILOT_PP7:
                        C_DATA = 13416;
                        N_FC = 13064;
                        C_FC = 11170;
                        break;
                    case PILOT_PP8:
                        C_DATA = 13406;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            else
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 12678;
                        N_FC = 9280;
                        C_FC = 6573;
                        break;
                    case PILOT_PP2:
                        C_DATA = 12698;
                        N_FC = 11600;
                        C_FC = 10697;
                        break;
                    case PILOT_PP3:
                        C_DATA = 13262;
                        N_FC = 11600;
                        C_FC = 8011;
                        break;
                    case PILOT_PP4:
                        C_DATA = 13276;
                        N_FC = 12760;
                        C_FC = 11563;
                        break;
                    case PILOT_PP5:
                        C_DATA = 13552;
                        N_FC = 12760;
                        C_FC = 8893;
                        break;
                    case PILOT_PP6:
                        C_DATA = 13568;
                        N_FC = 13340;
                        C_FC = 12051;
                        break;
                    case PILOT_PP7:
                        C_DATA = 13698;
                        N_FC = 13340;
                        C_FC = 11406;
                        break;
                    case PILOT_PP8:
                        C_DATA = 13688;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            break;
        case FFTSIZE_32K_NORM:
        case FFTSIZE_32K_SGI:
            if (carriermode == CARRIERS_NORMAL)
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP2:
                        C_DATA = 24886;
                        N_FC = 22720;
                        C_FC = 20952;
                        break;
                    case PILOT_PP3:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP4:
                        C_DATA = 26022;
                        N_FC = 24992;
                        C_FC = 22649;
                        break;
                    case PILOT_PP5:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP6:
                        C_DATA = 26592;
                        N_FC = 26128;
                        C_FC = 23603;
                        break;
                    case PILOT_PP7:
                        C_DATA = 26836;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP8:
                        C_DATA = 26812;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            else
            {
                switch (pilotpattern)
                {
                    case PILOT_PP1:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP2:
                        C_DATA = 25412;
                        N_FC = 23200;
                        C_FC = 21395;
                        break;
                    case PILOT_PP3:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP4:
                        C_DATA = 26572;
                        N_FC = 25520;
                        C_FC = 23127;
                        break;
                    case PILOT_PP5:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP6:
                        C_DATA = 27152;
                        N_FC = 26680;
                        C_FC = 24102;
                        break;
                    case PILOT_PP7:
                        C_DATA = 27404;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    case PILOT_PP8:
                        C_DATA = 27376;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                    default:
                        C_DATA = 0;
                        N_FC = 0;
                        C_FC = 0;
                        break;
                }
            }
            break;
        default:
            C_DATA = 0;
            N_FC = 0;
            C_FC = 0;
            break;
    }
#ifndef MISO
    if (guardinterval == GI_1_128 && pilotpattern == PILOT_PP7)
    {
        N_FC = 0;
        C_FC = 0;
    }
    if (guardinterval == GI_1_32 && pilotpattern == PILOT_PP4)
    {
        N_FC = 0;
        C_FC = 0;
    }
    if (guardinterval == GI_1_16 && pilotpattern == PILOT_PP2)
    {
        N_FC = 0;
        C_FC = 0;
    }
    if (guardinterval == GI_19_256 && pilotpattern == PILOT_PP2)
    {
        N_FC = 0;
        C_FC = 0;
    }
#endif
    numsymbols = (int)max_symbols - N_P2;
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + C_FC;
    }
    N_punc_temp = (6 * (KBCH_1_2 - KSIG_POST)) / 5;
    N_post_temp = KSIG_POST + NBCH_PARITY + 9000 - N_punc_temp;
    if (N_P2 == 1)
    {
        N_post = ceil((float)N_post_temp / (2 * (float)eta_mod)) * 2 * eta_mod;
    }
    else
    {
        N_post = ceil((float)N_post_temp / ((float)eta_mod * (float)N_P2)) * eta_mod * N_P2;
    }
    D_L1 = (N_post / eta_mod) + 1840;
    printf("max symbols = %d, max blocks = %d\n", (int)max_symbols, (cells - D_L1) / cell_size);

    numsymbols = (int)symbols - N_P2;
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + C_FC;
    }
    N_punc_temp = (6 * (KBCH_1_2 - KSIG_POST)) / 5;
    N_post_temp = KSIG_POST + NBCH_PARITY + 9000 - N_punc_temp;
    if (N_P2 == 1)
    {
        N_post = ceil((float)N_post_temp / (2 * (float)eta_mod)) * 2 * eta_mod;
    }
    else
    {
        N_post = ceil((float)N_post_temp / ((float)eta_mod * (float)N_P2)) * eta_mod * N_P2;
    }
    D_L1 = (N_post / eta_mod) + 1840;
    printf("symbols = %d, max blocks = %d\n", (int)symbols, (cells - D_L1) / cell_size);
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + N_FC;
    }
    printf("cells = %d, stream = %d, L1 = %d, dummy = %d, unmodulated = %d\n\n", cells, cell_size * (int)fecblocks, D_L1, cells - (cell_size * (int)fecblocks) - 1840 - (N_post / eta_mod) - (N_FC - C_FC), N_FC - C_FC);

    switch (fft_size)
    {
        case FFTSIZE_1K:
            if (C_DATA != 0)
            {
                C_DATA -= 10;
            }
            if (N_FC != 0)
            {
                N_FC -= 10;
            }
            if (C_FC != 0)
            {
                C_FC -= 10;
            }
            break;
        case FFTSIZE_2K:
            if (C_DATA != 0)
            {
                C_DATA -= 18;
            }
            if (N_FC != 0)
            {
                N_FC -= 18;
            }
            if (C_FC != 0)
            {
                C_FC -= 18;
            }
            break;
        case FFTSIZE_4K:
            if (C_DATA != 0)
            {
                C_DATA -= 36;
            }
            if (N_FC != 0)
            {
                N_FC -= 36;
            }
            if (C_FC != 0)
            {
                C_FC -= 36;
            }
            break;
        case FFTSIZE_8K_NORM:
        case FFTSIZE_8K_SGI:
            if (C_DATA != 0)
            {
                C_DATA -= 72;
            }
            if (N_FC != 0)
            {
                N_FC -= 72;
            }
            if (C_FC != 0)
            {
                C_FC -= 72;
            }
            break;
        case FFTSIZE_16K:
            if (C_DATA != 0)
            {
                C_DATA -= 144;
            }
            if (N_FC != 0)
            {
                N_FC -= 144;
            }
            if (C_FC != 0)
            {
                C_FC -= 144;
            }
            break;
        case FFTSIZE_32K_NORM:
        case FFTSIZE_32K_SGI:
            if (C_DATA != 0)
            {
                C_DATA -= 288;
            }
            if (N_FC != 0)
            {
                N_FC -= 288;
            }
            if (C_FC != 0)
            {
                C_FC -= 288;
            }
            break;
    }
    numsymbols = (int)max_symbols - N_P2;
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + C_FC;
    }
    N_punc_temp = (6 * (KBCH_1_2 - KSIG_POST)) / 5;
    N_post_temp = KSIG_POST + NBCH_PARITY + 9000 - N_punc_temp;
    if (N_P2 == 1)
    {
        N_post = ceil((float)N_post_temp / (2 * (float)eta_mod)) * 2 * eta_mod;
    }
    else
    {
        N_post = ceil((float)N_post_temp / ((float)eta_mod * (float)N_P2)) * eta_mod * N_P2;
    }
    D_L1 = (N_post / eta_mod) + 1840;
    printf("PAPR max symbols = %d, max blocks = %d\n", (int)max_symbols, (cells - D_L1) / cell_size);

    numsymbols = (int)symbols - N_P2;
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + C_FC;
    }
    N_punc_temp = (6 * (KBCH_1_2 - KSIG_POST)) / 5;
    N_post_temp = KSIG_POST + NBCH_PARITY + 9000 - N_punc_temp;
    if (N_P2 == 1)
    {
        N_post = ceil((float)N_post_temp / (2 * (float)eta_mod)) * 2 * eta_mod;
    }
    else
    {
        N_post = ceil((float)N_post_temp / ((float)eta_mod * (float)N_P2)) * eta_mod * N_P2;
    }
    D_L1 = (N_post / eta_mod) + 1840;
    printf("symbols = %d, max blocks = %d\n", (int)symbols, (cells - D_L1) / cell_size);
    if (N_FC == 0)
    {
        cells = (N_P2 * C_P2) + (numsymbols * C_DATA);
    }
    else
    {
        cells = (N_P2 * C_P2) + ((numsymbols - 1) * C_DATA) + N_FC;
    }
    printf("cells = %d, stream = %d, L1 = %d, dummy = %d, unmodulated = %d\n", cells, cell_size * (int)fecblocks, D_L1, cells - (cell_size * (int)fecblocks) - 1840 - (N_post / eta_mod) - (N_FC - C_FC), N_FC - C_FC);

    return 0;
}

