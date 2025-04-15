/*
 * Copyright 2021-2025 Ron Economos (w6rz@comcast.net)
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

/* ATSC 3.0 useful bitrate */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "atsc3rate.h"

enum atsc3_fftsize_t {
  FFTSIZE_8K = 0,
  FFTSIZE_16K,
  FFTSIZE_32K,
};

enum atsc3_code_rate_t {
  C2_15 = 0,
  C3_15,
  C4_15,
  C5_15,
  C6_15,
  C7_15,
  C8_15,
  C9_15,
  C10_15,
  C11_15,
  C12_15,
  C13_15,
};

enum atsc3_framesize_t {
  FECFRAME_NORMAL = 0,
  FECFRAME_SHORT,
};

enum atsc3_constellation_t {
  MOD_QPSK = 0,
  MOD_16QAM,
  MOD_64QAM,
  MOD_256QAM,
  MOD_1024QAM,
  MOD_4096QAM,
};

enum atsc3_guardinterval_t {
  GI_RESERVED = 0,
  GI_1_192,
  GI_2_384,
  GI_3_512,
  GI_4_768,
  GI_5_1024,
  GI_6_1536,
  GI_7_2048,
  GI_8_2432,
  GI_9_3072,
  GI_10_3648,
  GI_11_4096,
  GI_12_4864,
};

enum atsc3_pilotpattern_t {
  PILOT_SP3_2 = 0,
  PILOT_SP3_4,
  PILOT_SP4_2,
  PILOT_SP4_4,
  PILOT_SP6_2,
  PILOT_SP6_4,
  PILOT_SP8_2,
  PILOT_SP8_4,
  PILOT_SP12_2,
  PILOT_SP12_4,
  PILOT_SP16_2,
  PILOT_SP16_4,
  PILOT_SP24_2,
  PILOT_SP24_4,
  PILOT_SP32_2,
  PILOT_SP32_4,
};

enum atsc3_l1_fec_mode_t {
  L1_FEC_MODE_1 = 0,
  L1_FEC_MODE_2,
  L1_FEC_MODE_3,
  L1_FEC_MODE_4,
  L1_FEC_MODE_5,
  L1_FEC_MODE_6,
  L1_FEC_MODE_7,
};

enum atsc3_reduced_carriers_t {
  CRED_0 = 0,
  CRED_1,
  CRED_2,
  CRED_3,
  CRED_4,
};

enum atsc3_scattered_pilot_boost_t {
  SPB_0 = 0,
  SPB_1,
  SPB_2,
  SPB_3,
  SPB_4,
};

#define TI_MEMORY (1<<19)

int main(int argc, char **argv)
{
  int fftsize, numpayloadsyms, numpreamblesyms, rate;
  int constellation, mod;
  int framesize, plpsize;
  int fft_size;
  int guardinterval;
  int pilotpattern;
  int l1cells, totalcells;
  int total_preamble_cells;
  int first_preamble_cells;
  int preamble_cells;
  int data_cells;
  int sbs_cells;
  int sbs_data_cells;
  int sbsnullcells;
  int papr_cells;
  int fec_cells;
  int fec_blocks;
  int ti_blocks;
  int hti_plpsize;
  int firstsbs;
  int cred = 0;
  int pilotboost = 4;
  int paprmode;
  int gisamples;
  double clock_num, clock_den = 1.0;
  double bitrate, T, TS, TF, TB, symbols, kbch, fecsize, fecrate;
  float plp_ratio;

  if (argc != 15 && argc != 16) {
    fprintf(stderr, "usage: atsc3rate <fft size> <guard interval> <number of data symbols> <number of preamble symbols> <code rate> <modulation> <frame size> <pilot pattern> <first SBS> <L1 Basic mode> <L1 Detail mode> <reduced carriers> <pilot boost> <PAPR mode> <optional HTI blocks>\n");
    fprintf(stderr, "\nfft size = 8, 16, 32\n");
    fprintf(stderr, "\nguard interval = 1/192, 2/384, 3/512, 4/768, 5/1024, 6/1536, 7/2048, 8/2432, 9/3072, 10/3648, 11/4096, 12/3864\n");
    fprintf(stderr, "\nmodulation 0/QPSK, 1/16QAM, 2/64QAM, 3/256QAM\n");
    fprintf(stderr, "\nframe size = 0/normal, 1/short\n");
    fprintf(stderr, "\npilot pattern = 0/SP3_2, 1/SP3_4, 2/SP4_2, 3/SP4_4, 4/SP6_2, 5/SP6_4, 6/SP8_2, 7/SP8_4, 8/SP12_2, 9/SP12_4, 10/SP16_2, 11/SP16_4, 12/SP24_2, 13/SP24_4, 14/SP32_2, 15/SP32_4\n");
    exit(-1);
  }

  clock_num = 384000.0 * (16.0 + 2.0);
  fftsize = atoi(argv[1]);
  fftsize *= 1024;
  switch (fftsize)
  {
    case 8192:
      fft_size = FFTSIZE_8K;
      break;
    case 16384:
      fft_size = FFTSIZE_16K;
      break;
    case 32768:
      fft_size = FFTSIZE_32K;
      break;
    default:
      fft_size = FFTSIZE_8K;
      break;
    }
  guardinterval = atoi(argv[2]);
  numpayloadsyms = atoi(argv[3]);
  numpreamblesyms = atoi(argv[4]);
  rate = atoi(argv[5]);
  rate -= 2;
  constellation = atoi(argv[6]);
  framesize = atoi(argv[7]);
  pilotpattern = atoi(argv[8]);
  firstsbs = atoi(argv[9]);
  cred = atoi(argv[12]);
  pilotboost = atoi(argv[13]);
  paprmode = atoi(argv[14]);
  switch (atoi(argv[10]) - 1)
  {
    case L1_FEC_MODE_1:
      l1cells = 3820;
      break;
    case L1_FEC_MODE_2:
      l1cells = 934;
      break;
    case L1_FEC_MODE_3:
      l1cells = 484;
      break;
    case L1_FEC_MODE_4:
      l1cells = 259;
      break;
    case L1_FEC_MODE_5:
      l1cells = 163;
      break;
    default:
      l1cells = 3820;
      break;
  }
  switch (atoi(argv[11]) - 1)
  {
    case L1_FEC_MODE_1:
      l1cells += 2787;
      break;
    case L1_FEC_MODE_2:
      l1cells += 774;
      break;
    case L1_FEC_MODE_3:
      l1cells += 617;
      break;
    case L1_FEC_MODE_4:
      l1cells += 338;
      break;
    case L1_FEC_MODE_5:
      l1cells += 204;
      break;
    case L1_FEC_MODE_6:
      l1cells += 124;
      break;
    case L1_FEC_MODE_7:
      l1cells += 85;
      break;
    default:
      l1cells += 3820;
      break;
  }
  if (framesize == FECFRAME_NORMAL) {
    switch (rate) {
      case C2_15:
        kbch = 8448;
        break;
      case C3_15:
        kbch = 12768;
        break;
      case C4_15:
        kbch = 17088;
        break;
      case C5_15:
        kbch = 21408;
        break;
      case C6_15:
        kbch = 25728;
        break;
      case C7_15:
        kbch = 30048;
        break;
      case C8_15:
        kbch = 34368;
        break;
      case C9_15:
        kbch = 38688;
        break;
      case C10_15:
        kbch = 43008;
        break;
      case C11_15:
        kbch = 47328;
        break;
      case C12_15:
        kbch = 51648;
        break;
      case C13_15:
        kbch = 55968;
        break;
      default:
        kbch = 0;
        break;
    }
  }
  else if (framesize == FECFRAME_SHORT) {
    switch (rate) {
      case C2_15:
        kbch = 1992;
        break;
      case C3_15:
        kbch = 3072;
        break;
      case C4_15:
        kbch = 4152;
        break;
      case C5_15:
        kbch = 5232;
        break;
      case C6_15:
        kbch = 6312;
        break;
      case C7_15:
        kbch = 7392;
        break;
      case C8_15:
        kbch = 8472;
        break;
      case C9_15:
        kbch = 9552;
        break;
      case C10_15:
        kbch = 10632;
        break;
      case C11_15:
        kbch = 11712;
        break;
      case C12_15:
        kbch = 12792;
        break;
      case C13_15:
        kbch = 13872;
        break;
      default:
        kbch = 0;
        break;
    }
  }
  else {
    kbch = 0;
  }
  switch (constellation) {
    case MOD_QPSK:
      mod = 2;
      break;
    case MOD_16QAM:
      mod = 4;
      break;
    case MOD_64QAM:
      mod = 6;
      break;
    case MOD_256QAM:
      mod = 8;
      break;
    case MOD_1024QAM:
      mod = 10;
      break;
    case MOD_4096QAM:
      mod = 12;
      break;
    default:
      mod = 2;
      break;
  }
  switch (fft_size) {
    case FFTSIZE_8K:
      papr_cells = 72;
      switch (guardinterval) {
        case GI_1_192:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
        case GI_2_384:
          gisamples = 384;
          first_preamble_cells = preamble_cells_table[1][4];
          preamble_cells = preamble_cells_table[1][cred];
          break;
        case GI_3_512:
          gisamples = 512;
          first_preamble_cells = preamble_cells_table[2][4];
          preamble_cells = preamble_cells_table[2][cred];
          break;
        case GI_4_768:
          gisamples = 768;
          first_preamble_cells = preamble_cells_table[3][4];
          preamble_cells = preamble_cells_table[3][cred];
          break;
        case GI_5_1024:
          gisamples = 1024;
          first_preamble_cells = preamble_cells_table[4][4];
          preamble_cells = preamble_cells_table[4][cred];
          break;
        case GI_6_1536:
          gisamples = 1536;
          first_preamble_cells = preamble_cells_table[5][4];
          preamble_cells = preamble_cells_table[5][cred];
          break;
        case GI_7_2048:
          gisamples = 2048;
          first_preamble_cells = preamble_cells_table[6][4];
          preamble_cells = preamble_cells_table[6][cred];
          break;
        default:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
      }
      switch (pilotpattern) {
        case PILOT_SP3_2:
          data_cells = data_cells_table_8K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_2][cred][pilotboost];
          break;
        case PILOT_SP3_4:
          data_cells = data_cells_table_8K[PILOT_SP3_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_4][cred][pilotboost];
          break;
        case PILOT_SP4_2:
          data_cells = data_cells_table_8K[PILOT_SP4_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP4_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP4_2][cred][pilotboost];
          break;
        case PILOT_SP4_4:
          data_cells = data_cells_table_8K[PILOT_SP4_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP4_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP4_4][cred][pilotboost];
          break;
        case PILOT_SP6_2:
          data_cells = data_cells_table_8K[PILOT_SP6_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP6_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP6_2][cred][pilotboost];
          break;
        case PILOT_SP6_4:
          data_cells = data_cells_table_8K[PILOT_SP6_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP6_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP6_4][cred][pilotboost];
          break;
        case PILOT_SP8_2:
          data_cells = data_cells_table_8K[PILOT_SP8_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP8_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP8_2][cred][pilotboost];
          break;
        case PILOT_SP8_4:
          data_cells = data_cells_table_8K[PILOT_SP8_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP8_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP8_4][cred][pilotboost];
          break;
        case PILOT_SP12_2:
          data_cells = data_cells_table_8K[PILOT_SP12_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP12_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP12_2][cred][pilotboost];
          break;
        case PILOT_SP12_4:
          data_cells = data_cells_table_8K[PILOT_SP12_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP12_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP12_4][cred][pilotboost];
          break;
        case PILOT_SP16_2:
          data_cells = data_cells_table_8K[PILOT_SP16_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP16_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP16_2][cred][pilotboost];
          break;
        case PILOT_SP16_4:
          data_cells = data_cells_table_8K[PILOT_SP16_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP16_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP16_4][cred][pilotboost];
          break;
        case PILOT_SP24_2:
          data_cells = data_cells_table_8K[PILOT_SP24_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP24_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP24_2][cred][pilotboost];
          break;
        case PILOT_SP24_4:
          data_cells = data_cells_table_8K[PILOT_SP24_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP24_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP24_4][cred][pilotboost];
          break;
        case PILOT_SP32_2:
          data_cells = data_cells_table_8K[PILOT_SP32_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP32_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP32_2][cred][pilotboost];
          break;
        case PILOT_SP32_4:
          data_cells = data_cells_table_8K[PILOT_SP32_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP32_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP32_4][cred][pilotboost];
          break;
        default:
          data_cells = data_cells_table_8K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_2][cred][pilotboost];
          break;
      }
      break;
    case FFTSIZE_16K:
      papr_cells = 144;
      switch (guardinterval) {
        case GI_1_192:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[7][4];
          preamble_cells = preamble_cells_table[7][cred];
          break;
        case GI_2_384:
          gisamples = 384;
          first_preamble_cells = preamble_cells_table[8][4];
          preamble_cells = preamble_cells_table[8][cred];
          break;
        case GI_3_512:
          gisamples = 512;
          first_preamble_cells = preamble_cells_table[9][4];
          preamble_cells = preamble_cells_table[9][cred];
          break;
        case GI_4_768:
          gisamples = 768;
          first_preamble_cells = preamble_cells_table[10][4];
          preamble_cells = preamble_cells_table[10][cred];
          break;
        case GI_5_1024:
          gisamples = 1024;
          first_preamble_cells = preamble_cells_table[11][4];
          preamble_cells = preamble_cells_table[11][cred];
          break;
        case GI_6_1536:
          gisamples = 1536;
          first_preamble_cells = preamble_cells_table[12][4];
          preamble_cells = preamble_cells_table[12][cred];
          break;
        case GI_7_2048:
          gisamples = 2048;
          first_preamble_cells = preamble_cells_table[13][4];
          preamble_cells = preamble_cells_table[13][cred];
          break;
        case GI_8_2432:
          gisamples = 2432;
          first_preamble_cells = preamble_cells_table[14][4];
          preamble_cells = preamble_cells_table[14][cred];
          break;
        case GI_9_3072:
          gisamples = 3072;
          first_preamble_cells = preamble_cells_table[15][4];
          preamble_cells = preamble_cells_table[15][cred];
          break;
        case GI_10_3648:
          gisamples = 3648;
          first_preamble_cells = preamble_cells_table[16][4];
          preamble_cells = preamble_cells_table[16][cred];
          break;
        case GI_11_4096:
          gisamples = 4096;
          first_preamble_cells = preamble_cells_table[17][4];
          preamble_cells = preamble_cells_table[17][cred];
          break;
        default:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[7][4];
          preamble_cells = preamble_cells_table[7][cred];
          break;
      }
      switch (pilotpattern) {
        case PILOT_SP3_2:
          data_cells = data_cells_table_16K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP3_2][cred][pilotboost];
          break;
        case PILOT_SP3_4:
          data_cells = data_cells_table_16K[PILOT_SP3_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP3_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP3_4][cred][pilotboost];
          break;
        case PILOT_SP4_2:
          data_cells = data_cells_table_16K[PILOT_SP4_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP4_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP4_2][cred][pilotboost];
          break;
        case PILOT_SP4_4:
          data_cells = data_cells_table_16K[PILOT_SP4_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP4_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP4_4][cred][pilotboost];
          break;
        case PILOT_SP6_2:
          data_cells = data_cells_table_16K[PILOT_SP6_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP6_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP6_2][cred][pilotboost];
          break;
        case PILOT_SP6_4:
          data_cells = data_cells_table_16K[PILOT_SP6_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP6_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP6_4][cred][pilotboost];
          break;
        case PILOT_SP8_2:
          data_cells = data_cells_table_16K[PILOT_SP8_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP8_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP8_2][cred][pilotboost];
          break;
        case PILOT_SP8_4:
          data_cells = data_cells_table_16K[PILOT_SP8_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP8_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP8_4][cred][pilotboost];
          break;
        case PILOT_SP12_2:
          data_cells = data_cells_table_16K[PILOT_SP12_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP12_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP12_2][cred][pilotboost];
          break;
        case PILOT_SP12_4:
          data_cells = data_cells_table_16K[PILOT_SP12_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP12_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP12_4][cred][pilotboost];
          break;
        case PILOT_SP16_2:
          data_cells = data_cells_table_16K[PILOT_SP16_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP16_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP16_2][cred][pilotboost];
          break;
        case PILOT_SP16_4:
          data_cells = data_cells_table_16K[PILOT_SP16_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP16_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP16_4][cred][pilotboost];
          break;
        case PILOT_SP24_2:
          data_cells = data_cells_table_16K[PILOT_SP24_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP24_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP24_2][cred][pilotboost];
          break;
        case PILOT_SP24_4:
          data_cells = data_cells_table_16K[PILOT_SP24_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP24_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP24_4][cred][pilotboost];
          break;
        case PILOT_SP32_2:
          data_cells = data_cells_table_16K[PILOT_SP32_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP32_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP32_2][cred][pilotboost];
          break;
        case PILOT_SP32_4:
          data_cells = data_cells_table_16K[PILOT_SP32_4][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP32_4][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP32_4][cred][pilotboost];
          break;
        default:
          data_cells = data_cells_table_16K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_16K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_16K[PILOT_SP3_2][cred][pilotboost];
          break;
      }
      break;
    case FFTSIZE_32K:
      papr_cells = 288;
      switch (guardinterval) {
        case GI_1_192:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[18][4];
          preamble_cells = preamble_cells_table[18][cred];
          break;
        case GI_2_384:
          gisamples = 384;
          first_preamble_cells = preamble_cells_table[19][4];
          preamble_cells = preamble_cells_table[19][cred];
          break;
        case GI_3_512:
          gisamples = 512;
          first_preamble_cells = preamble_cells_table[20][4];
          preamble_cells = preamble_cells_table[20][cred];
          break;
        case GI_4_768:
          gisamples = 768;
          first_preamble_cells = preamble_cells_table[21][4];
          preamble_cells = preamble_cells_table[21][cred];
          break;
        case GI_5_1024:
          gisamples = 1024;
          first_preamble_cells = preamble_cells_table[22][4];
          preamble_cells = preamble_cells_table[22][cred];
          break;
        case GI_6_1536:
          gisamples = 1536;
          first_preamble_cells = preamble_cells_table[23][4];
          preamble_cells = preamble_cells_table[23][cred];
          break;
        case GI_7_2048:
          gisamples = 2048;
          first_preamble_cells = preamble_cells_table[24][4];
          preamble_cells = preamble_cells_table[24][cred];
          break;
        case GI_8_2432:
          gisamples = 2432;
          first_preamble_cells = preamble_cells_table[25][4];
          preamble_cells = preamble_cells_table[25][cred];
          break;
        case GI_9_3072:
          gisamples = 3072;
          if (pilotpattern == PILOT_SP8_2 || pilotpattern == PILOT_SP8_4) {
            first_preamble_cells = preamble_cells_table[26][4];
            preamble_cells = preamble_cells_table[26][cred];
          }
          else {
            first_preamble_cells = preamble_cells_table[27][4];
            preamble_cells = preamble_cells_table[27][cred];
          }
          break;
        case GI_10_3648:
          gisamples = 3648;
          if (pilotpattern == PILOT_SP8_2 || pilotpattern == PILOT_SP8_4) {
            first_preamble_cells = preamble_cells_table[28][4];
            preamble_cells = preamble_cells_table[28][cred];
          }
          else {
            first_preamble_cells = preamble_cells_table[29][4];
            preamble_cells = preamble_cells_table[29][cred];
          }
          break;
        case GI_11_4096:
          gisamples = 4096;
          first_preamble_cells = preamble_cells_table[30][4];
          preamble_cells = preamble_cells_table[30][cred];
          break;
        case GI_12_4864:
          gisamples = 4864;
          first_preamble_cells = preamble_cells_table[31][4];
          preamble_cells = preamble_cells_table[31][cred];
          break;
        default:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[18][4];
          preamble_cells = preamble_cells_table[18][cred];
          break;
      }
      switch (pilotpattern) {
        case PILOT_SP3_2:
          data_cells = data_cells_table_32K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP3_2][cred][pilotboost];
          break;
        case PILOT_SP3_4:
          data_cells = data_cells_table_32K[PILOT_SP3_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP3_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP3_4][cred][pilotboost];
          break;
        case PILOT_SP4_2:
          data_cells = data_cells_table_32K[PILOT_SP4_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP4_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP4_2][cred][pilotboost];
          break;
        case PILOT_SP4_4:
          data_cells = data_cells_table_32K[PILOT_SP4_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP4_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP4_4][cred][pilotboost];
          break;
        case PILOT_SP6_2:
          data_cells = data_cells_table_32K[PILOT_SP6_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP6_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP6_2][cred][pilotboost];
          break;
        case PILOT_SP6_4:
          data_cells = data_cells_table_32K[PILOT_SP6_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP6_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP6_4][cred][pilotboost];
          break;
        case PILOT_SP8_2:
          data_cells = data_cells_table_32K[PILOT_SP8_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP8_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP8_2][cred][pilotboost];
          break;
        case PILOT_SP8_4:
          data_cells = data_cells_table_32K[PILOT_SP8_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP8_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP8_4][cred][pilotboost];
          break;
        case PILOT_SP12_2:
          data_cells = data_cells_table_32K[PILOT_SP12_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP12_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP12_2][cred][pilotboost];
          break;
        case PILOT_SP12_4:
          data_cells = data_cells_table_32K[PILOT_SP12_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP12_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP12_4][cred][pilotboost];
          break;
        case PILOT_SP16_2:
          data_cells = data_cells_table_32K[PILOT_SP16_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP16_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP16_2][cred][pilotboost];
          break;
        case PILOT_SP16_4:
          data_cells = data_cells_table_32K[PILOT_SP16_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP16_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP16_4][cred][pilotboost];
          break;
        case PILOT_SP24_2:
          data_cells = data_cells_table_32K[PILOT_SP24_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP24_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP24_2][cred][pilotboost];
          break;
        case PILOT_SP24_4:
          data_cells = data_cells_table_32K[PILOT_SP24_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP24_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP24_4][cred][pilotboost];
          break;
        case PILOT_SP32_2:
          data_cells = data_cells_table_32K[PILOT_SP32_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP32_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP32_2][cred][pilotboost];
          break;
        case PILOT_SP32_4:
          data_cells = data_cells_table_32K[PILOT_SP32_4][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP32_4][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP32_4][cred][pilotboost];
          break;
        default:
          data_cells = data_cells_table_32K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_32K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_32K[PILOT_SP3_2][cred][pilotboost];
          break;
      }
      break;
    default:
      papr_cells = 72;
      switch (guardinterval) {
        case GI_1_192:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
        case GI_2_384:
          gisamples = 384;
          first_preamble_cells = preamble_cells_table[1][4];
          preamble_cells = preamble_cells_table[1][cred];
          break;
        case GI_3_512:
          gisamples = 512;
          first_preamble_cells = preamble_cells_table[2][4];
          preamble_cells = preamble_cells_table[2][cred];
          break;
        case GI_4_768:
          gisamples = 768;
          first_preamble_cells = preamble_cells_table[3][4];
          preamble_cells = preamble_cells_table[3][cred];
          break;
        case GI_5_1024:
          gisamples = 1024;
          first_preamble_cells = preamble_cells_table[4][4];
          preamble_cells = preamble_cells_table[4][cred];
          break;
        case GI_6_1536:
          gisamples = 1536;
          first_preamble_cells = preamble_cells_table[5][4];
          preamble_cells = preamble_cells_table[5][cred];
          break;
        case GI_7_2048:
          gisamples = 2048;
          first_preamble_cells = preamble_cells_table[6][4];
          preamble_cells = preamble_cells_table[6][cred];
          break;
        default:
          gisamples = 192;
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
      }
      switch (pilotpattern) {
        case PILOT_SP3_2:
          data_cells = data_cells_table_8K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_2][cred][pilotboost];
          break;
        case PILOT_SP3_4:
          data_cells = data_cells_table_8K[PILOT_SP3_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_4][cred][pilotboost];
          break;
        case PILOT_SP4_2:
          data_cells = data_cells_table_8K[PILOT_SP4_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP4_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP4_2][cred][pilotboost];
          break;
        case PILOT_SP4_4:
          data_cells = data_cells_table_8K[PILOT_SP4_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP4_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP4_4][cred][pilotboost];
          break;
        case PILOT_SP6_2:
          data_cells = data_cells_table_8K[PILOT_SP6_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP6_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP6_2][cred][pilotboost];
          break;
        case PILOT_SP6_4:
          data_cells = data_cells_table_8K[PILOT_SP6_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP6_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP6_4][cred][pilotboost];
          break;
        case PILOT_SP8_2:
          data_cells = data_cells_table_8K[PILOT_SP8_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP8_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP8_2][cred][pilotboost];
          break;
        case PILOT_SP8_4:
          data_cells = data_cells_table_8K[PILOT_SP8_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP8_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP8_4][cred][pilotboost];
          break;
        case PILOT_SP12_2:
          data_cells = data_cells_table_8K[PILOT_SP12_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP12_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP12_2][cred][pilotboost];
          break;
        case PILOT_SP12_4:
          data_cells = data_cells_table_8K[PILOT_SP12_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP12_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP12_4][cred][pilotboost];
          break;
        case PILOT_SP16_2:
          data_cells = data_cells_table_8K[PILOT_SP16_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP16_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP16_2][cred][pilotboost];
          break;
        case PILOT_SP16_4:
          data_cells = data_cells_table_8K[PILOT_SP16_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP16_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP16_4][cred][pilotboost];
          break;
        case PILOT_SP24_2:
          data_cells = data_cells_table_8K[PILOT_SP24_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP24_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP24_2][cred][pilotboost];
          break;
        case PILOT_SP24_4:
          data_cells = data_cells_table_8K[PILOT_SP24_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP24_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP24_4][cred][pilotboost];
          break;
        case PILOT_SP32_2:
          data_cells = data_cells_table_8K[PILOT_SP32_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP32_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP32_2][cred][pilotboost];
          break;
        case PILOT_SP32_4:
          data_cells = data_cells_table_8K[PILOT_SP32_4][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP32_4][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP32_4][cred][pilotboost];
          break;
        default:
          data_cells = data_cells_table_8K[PILOT_SP3_2][cred];
          sbs_cells = sbs_cells_table_8K[PILOT_SP3_2][cred];
          sbs_data_cells = sbs_data_cells_table_8K[PILOT_SP3_2][cred][pilotboost];
          break;
      }
  }
  if (framesize == FECFRAME_NORMAL)
  {
    fecsize = 64800.0;
    printf("frame size = normal\n");
    switch (constellation) {
      case MOD_QPSK:
        fec_cells = 32400;
        break;
      case MOD_16QAM:
        fec_cells = 16200;
        break;
      case MOD_64QAM:
        fec_cells = 10800;
        break;
      case MOD_256QAM:
        fec_cells = 8100;
        break;
      case MOD_1024QAM:
        fec_cells = 6480;
        break;
      case MOD_4096QAM:
        fec_cells = 5400;
        break;
      default:
        fec_cells = 0;
        break;
    }
  }
  else if (framesize == FECFRAME_SHORT)
  {
    fecsize = 16200.0;
    printf("frame size = short\n");
    switch (constellation) {
      case MOD_QPSK:
        fec_cells = 8100;
        break;
      case MOD_16QAM:
        fec_cells = 4050;
        break;
      case MOD_64QAM:
        fec_cells = 2700;
        break;
      case MOD_256QAM:
        fec_cells = 2025;
        break;
      default:
        fec_cells = 0;
        break;
    }
  }
  else
  {
    fecsize = 0.0;
    printf("frame size = invalid\n");
  }
  switch (rate)
  {
    case C2_15:
      printf("code rate = 2/15\n");
      break;
    case C3_15:
      printf("code rate = 3/15\n");
      break;
    case C4_15:
      printf("code rate = 4/15\n");
      break;
    case C5_15:
      printf("code rate = 5/15\n");
      break;
    case C6_15:
      printf("code rate = 6/15\n");
      break;
    case C7_15:
      printf("code rate = 7/15\n");
      break;
    case C8_15:
      printf("code rate = 8/15\n");
      break;
    case C9_15:
      printf("code rate = 9/15\n");
      break;
    case C10_15:
      printf("code rate = 10/15\n");
      break;
    case C11_15:
      printf("code rate = 11/15\n");
      break;
    case C12_15:
      printf("code rate = 12/15\n");
      break;
    case C13_15:
      printf("code rate = 13/15\n");
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
  printf("FFT size = %d\n", fftsize);
  printf("number of data symbols = %d\n", numpayloadsyms);
  printf("number of preamble symbols = %d\n", numpreamblesyms);
  printf("guard interval samples = %d\n", gisamples);
  switch (pilotpattern)
  {
    case PILOT_SP3_2:
      printf("pilot pattern = SP3_2\n");
      break;
    case PILOT_SP3_4:
      printf("pilot pattern = SP3_4\n");
      break;
    case PILOT_SP4_2:
      printf("pilot pattern = SP4_2\n");
      break;
    case PILOT_SP4_4:
      printf("pilot pattern = SP4_4\n");
      break;
    case PILOT_SP6_2:
      printf("pilot pattern = SP6_2\n");
      break;
    case PILOT_SP6_4:
      printf("pilot pattern = SP6_4\n");
      break;
    case PILOT_SP8_2:
      printf("pilot pattern = SP8_2\n");
      break;
    case PILOT_SP8_4:
      printf("pilot pattern = SP8_4\n");
      break;
    case PILOT_SP12_2:
      printf("pilot pattern = SP12_2\n");
      break;
    case PILOT_SP12_4:
      printf("pilot pattern = SP12_4\n");
      break;
    case PILOT_SP16_2:
      printf("pilot pattern = SP16_2\n");
      break;
    case PILOT_SP16_4:
      printf("pilot pattern = SP16_4\n");
      break;
    case PILOT_SP24_2:
      printf("pilot pattern = SP24_2\n");
      break;
    case PILOT_SP24_4:
      printf("pilot pattern = SP24_4\n");
      break;
    case PILOT_SP32_2:
      printf("pilot pattern = SP32_2\n");
      break;
    case PILOT_SP32_4:
      printf("pilot pattern = SP32_4\n");
      break;
    default:
      printf("pilot pattern = invalid\n");
      break;
  }
  if (firstsbs) {
    printf("first SBS insertion enabled\n");
  }
  else {
    printf("first SBS insertion disabled\n");
  }
  printf("L1 Basic mode = %d\n", atoi(argv[10]));
  printf("L1 Detail mode = %d\n", atoi(argv[11]));
  switch (cred)
  {
    case CRED_0:
      printf("bandwidth = 5.833 MHz\n");
      break;
    case CRED_1:
      printf("bandwidth = 5.752 MHz\n");
      break;
    case CRED_2:
      printf("bandwidth = 5.671 MHz\n");
      break;
    case CRED_3:
      printf("bandwidth = 5.590 MHz\n");
      break;
    case CRED_4:
      printf("bandwidth = 5.509 MHz\n");
      break;
    default:
      printf("bandwidth = invalid\n");
      break;
  }
  switch (pilotpattern)
  {
    case PILOT_SP3_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.175\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.288\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.396\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP3_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.175\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.396\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.549\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.660\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP4_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.072\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.274\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.413\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.514\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP4_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.274\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.514\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.660\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.799\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP6_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.202\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.429\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.585\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.698\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP6_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.413\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.679\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.862\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.995\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP8_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.288\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.549\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.698\n");
          break;
        case SPB_4:
          printf("pilot boost = 1.841\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP8_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.514\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.799\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.995\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.138\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP12_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.445\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.718\n");
          break;
        case SPB_3:
          printf("pilot boost = 1.905\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.042\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP12_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.679\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.995\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.213\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.371\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP16_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.549\n");
          break;
        case SPB_2:
          printf("pilot boost = 1.841\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.042\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.188\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP16_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.820\n");
          break;
        case SPB_2:
          printf("pilot boost = 2.163\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.399\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.570\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP24_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.718\n");
          break;
        case SPB_2:
          printf("pilot boost = 2.042\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.265\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.427\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP24_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 2.018\n");
          break;
        case SPB_2:
          printf("pilot boost = 2.399\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.661\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.851\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP32_2:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 1.862\n");
          break;
        case SPB_2:
          printf("pilot boost = 2.213\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.427\n");
          break;
        case SPB_4:
          printf("pilot boost = 2.630\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    case PILOT_SP32_4:
      switch (pilotboost) {
        case SPB_0:
          printf("pilot boost = 1.000\n");
          break;
        case SPB_1:
          printf("pilot boost = 2.163\n");
          break;
        case SPB_2:
          printf("pilot boost = 2.570\n");
          break;
        case SPB_3:
          printf("pilot boost = 2.851\n");
          break;
        case SPB_4:
          printf("pilot boost = 3.055\n");
          break;
        default:
          printf("pilot boost = invalid\n");
          break;
      }
      break;
    default:
      printf("pilot boost = invalid\n");
      break;
  }
  printf("\n");

  if (paprmode != 1) {
    papr_cells = 0;
  }
  symbols = numpayloadsyms + numpreamblesyms;
  T = 1.0 / (clock_num / clock_den);
  TB = 1.0 / 6144000.0;
  TS = (T * (fftsize + gisamples)) * 1000.0;
  TF = (symbols * TS) + (3072.0 * 4 * TB * 1000.0);
  printf("clock rate = %f Msps, symbol time = %f ms\n", (clock_num / clock_den) / 1000000.0, TS);
  printf("frame time = %f ms\n", TF);
  total_preamble_cells = 0;
  for (int n = 1; n < numpreamblesyms; n++) {
    total_preamble_cells += preamble_cells - papr_cells;
  }
  if (numpreamblesyms == 0) {
    first_preamble_cells = 0;
    l1cells = 0;
  }
  if (firstsbs) {
    totalcells = first_preamble_cells + total_preamble_cells + ((numpayloadsyms - 2) * (data_cells - papr_cells)) + ((sbs_cells - papr_cells) * 2);
  }
  else {
    totalcells = first_preamble_cells + total_preamble_cells + ((numpayloadsyms - 1) * (data_cells - papr_cells)) + (sbs_cells - papr_cells);
  }
  printf("total cells = %d\n", totalcells);
  sbsnullcells = sbs_cells - sbs_data_cells;
  printf("L1 cells = %d\n", l1cells);
  printf("1st preamble cells = %d\n", first_preamble_cells);
  if (numpreamblesyms != 0) {
    if (l1cells > first_preamble_cells) {
      if (numpreamblesyms != 2) {
        printf("**** warning, two preamble symbols required ****\n");
      }
    }
    else {
      if (numpreamblesyms != 1) {
        printf("**** warning, one preamble symbol required ****\n");
      }
    }
  }
  if (firstsbs) {
    plpsize = totalcells - l1cells - (sbsnullcells * 2);
    printf("SBS null cells = %d\n", sbsnullcells * 2);
  }
  else {
    plpsize = totalcells - l1cells - sbsnullcells;
    printf("SBS null cells = %d\n", sbsnullcells);
  }
  if (argc == 16) {
    fec_blocks = atoi(argv[15]);
    hti_plpsize = fec_blocks * fec_cells;
    if (hti_plpsize % TI_MEMORY) {
      ti_blocks = (hti_plpsize / TI_MEMORY) + 1;
    }
    else {
      ti_blocks = hti_plpsize / TI_MEMORY;
    }
    plp_ratio = (float)hti_plpsize / (float)plpsize;
    if (plp_ratio > 0.9) {
      printf("PLP size = %d, unused cells = %d, minimum TI blocks = %d\n", hti_plpsize, plpsize - hti_plpsize, ti_blocks);
    }
    else {
      printf("PLP size = %d, unused cells = %d\n", hti_plpsize, plpsize - hti_plpsize);
    }
    plpsize = hti_plpsize;
  }
  else {
    printf("PLP size = %d\n", plpsize);
  }
  fecrate = ((kbch - 16) / fecsize); /* 1 TS packet per ALP packet and MODE always = 1 */
  bitrate = (1000.0 / TF) * (plpsize * mod * fecrate);
  printf("TS bitrate = %.03f\n", bitrate);
  fecrate = kbch / fecsize;
  bitrate = (1000.0 / TF) * (plpsize * mod * fecrate);
  printf("PLP bitrate = %.03f\n", bitrate);
  return 0;
}
