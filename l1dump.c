#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "l1dump.h"

/* base64 decoder from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/ */

int b64invs[] = { 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51 };

size_t b64_decoded_size(const char *in)
{
    size_t len;
    size_t ret;
    size_t i;

    if (in == NULL)
        return 0;

    len = strlen(in);
    ret = len / 4 * 3;

    for (i=len; i-->0; ) {
        if (in[i] == '=') {
            ret--;
        } else {
            break;
        }
    }

    return ret;
}

int b64_isvalidchar(char c)
{
    if (c >= '0' && c <= '9')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c == '+' || c == '/' || c == '=')
        return 1;
    return 0;
}

int b64_decode(const char *in, unsigned char *out, size_t outlen)
{
    size_t len;
    size_t i;
    size_t j;
    int    v;

    if (in == NULL || out == NULL)
        return 0;

    len = strlen(in);
    if (outlen < b64_decoded_size(in) || len % 4 != 0)
        return 0;

    for (i=0; i<len; i++) {
        if (!b64_isvalidchar(in[i])) {
            return 0;
        }
    }

    for (i=0, j=0; i<len; i+=4, j+=3) {
        v = b64invs[in[i]-43];
        v = (v << 6) | b64invs[in[i+1]-43];
        v = in[i+2]=='=' ? v << 6 : (v << 6) | b64invs[in[i+2]-43];
        v = in[i+3]=='=' ? v << 6 : (v << 6) | b64invs[in[i+3]-43];

        out[j] = (v >> 16) & 0xFF;
        if (in[i+2] != '=')
            out[j+1] = (v >> 8) & 0xFF;
        if (in[i+3] != '=')
            out[j+2] = v & 0xFF;
    }

    return 1;
}

#define BUFFER_SIZE 512

char bits[BUFFER_SIZE * 8];
int bits_index = 0;

struct subframe_info_t {
    int num_preamble_symbols;
    int num_ofdm_symbols;
    int fft_size;
    int guard_interval;
};

void atsc3rate(int fft_size, int guardinterval, int numpayloadsyms, int numpreamblesyms, int rate, int constellation, int framesize, int pilotpattern, int firstsbs, int cred, int pilotboost, int paprmode, int ti_mode, int fec_blocks, int l1_detail_cells, int subframe, int num_subframes, struct subframe_info_t *subframe_info, int frame_length_mode, int frame_length, int excess_samples);

int get_bits(int count)
{
    int i;
    long value = 0;

    for (i = count; i > 0; i--) {
        value |= bits[bits_index++] << (i - 1);
    }
    return value;
}

int main(int argc, char **argv)
{
    FILE *fp;
    long size, value;
    int i, j, k, length, index = 0;
    static char buffer[BUFFER_SIZE];
    char *out;
    size_t out_len;
    int l1b_version;
    int l1b_time_info_flag;
    int l1b_papr_reduction;
    int l1b_frame_length_mode;
    int l1b_frame_length = 0;
    int l1b_excess_samples_per_symbol = 0;
    int l1b_num_subframes;
    int l1b_preamble_num_symbols;
    int l1b_l1_detail_size_bytes;
    int l1b_l1_detail_total_cells;
    int l1b_first_sub_mimo;
    int l1b_first_sub_fft_size;
    int l1b_first_sub_reduced_carriers;
    int l1b_first_sub_guard_interval;
    int l1b_first_sub_num_ofdm_symbols;
    int l1b_first_sub_scattered_pilot_pattern;
    int l1b_first_sub_scattered_pilot_boost;
    int l1b_first_sub_sbs_first;
    int l1b_first_sub_sbs_last;
    int l1b_first_sub_mimo_mixed = 0;
    int l1d_version;
    int l1d_num_rf;
    int l1d_mimo = 0;
    int l1d_fft_size;
    int l1d_reduced_carriers;
    int l1d_guard_interval;
    int l1d_num_ofdm_symbols;
    int l1d_scattered_pilot_pattern;
    int l1d_scattered_pilot_boost;
    int l1d_sbs_first = 0;
    int l1d_sbs_last = 0;
    int l1d_num_plp = 0;
    int l1d_plp_layer;
    int l1d_plp_fec_type = 0;
    int l1d_plp_mod = 0;
    int l1d_plp_cod = 0;
    int l1d_plp_TI_mode = 0;
    int l1d_plp_num_channel_bonded;
    int l1d_plp_TI_extended_interleaving = 0;
    int l1d_plp_HTI_inter_subframe;
    int l1d_plp_HTI_num_ti_blocks = 0;
    int l1d_plp_HTI_num_fec_blocks = 0;
    int l1d_mimo_mixed;
    struct subframe_info_t subframe_info[257];

    if (argc != 2) {
        fprintf(stderr, "usage: l1dump <filename>\n");
        exit(-1);
    }

    /*--- open binary file (for parsing) ---*/
    fp = fopen(argv[1], "rb");
    if (fp == 0) {
        fprintf(stderr, "Cannot open input file <%s>\n", argv[1]);
        exit(-1);
    }
    if (fseek(fp, 0, SEEK_END) < 0) {
        fprintf(stderr, "Cannot seek input file <%s>\n", argv[1]);
        fclose(fp);
        exit(-1);
    }
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    length = fread(&buffer[0], 1, size, fp);
    for (i = 0; i < length; i++) {
        if (buffer[i] == 0x0a || buffer[i] == 0x0d) {
            break;
        }
    }
    buffer[i] = 0;
    out_len = b64_decoded_size(buffer);
    out = malloc(out_len);

    if (!b64_decode(buffer, (unsigned char *)out, out_len)) {
        fprintf(stderr, "Decode Failure\n");
        free(out);
        fclose(fp);
        exit(-1);
    }

    for (i = 0; i < out_len; i++) {
      for (int n = 7; n >= 0; n--) {
        bits[index++] = out[i] & (1 << n) ? 1 : 0;
      }
    }

    value = get_bits(3);
    l1b_version = value;
    value = get_bits(1);
    value = get_bits(1);
    value = get_bits(2);
    l1b_time_info_flag = value;
    value = get_bits(1);
    value = get_bits(2);
    value = get_bits(1);
    if (value == 0) {
        value = get_bits(10);
        value = get_bits(13);
    }
    else {
        value = get_bits(16);
        value = get_bits(7);
    }
    value = get_bits(8);
    l1b_num_subframes = value;
    value = get_bits(3);
    subframe_info[0].num_preamble_symbols = value + 1;
    value = get_bits(3);
    value = get_bits(2);
    value = get_bits(13);
    l1b_l1_detail_size_bytes = value;
    value = get_bits(3);
    value = get_bits(2);
    value = get_bits(19);
    value = get_bits(1);
    l1b_first_sub_mimo = value;
    value = get_bits(2);
    value = get_bits(2);
    switch (value) {
        case FFTSIZE_8K:
            value = 8192;
            break;
        case FFTSIZE_16K:
            value = 16384;
            break;
        case FFTSIZE_32K:
            value = 32768;
            break;
        default:
            value = 8192;
            break;
    }
    subframe_info[0].fft_size = value;
    value = get_bits(3);
    value = get_bits(4);
    switch (value) {
        case GI_RESERVED:
            value = 0;
            break;
        case GI_1_192:
            value = 192;
            break;
        case GI_2_384:
            value = 384;
            break;
        case GI_3_512:
            value = 512;
            break;
        case GI_4_768:
            value = 768;
            break;
        case GI_5_1024:
            value = 1024;
            break;
        case GI_6_1536:
            value = 1536;
            break;
        case GI_7_2048:
            value = 2048;
            break;
        case GI_8_2432:
            value = 2432;
            break;
        case GI_9_3072:
            value = 3072;
            break;
        case GI_10_3648:
            value = 3648;
            break;
        case GI_11_4096:
            value = 4096;
            break;
        case GI_12_4864:
            value = 4864;
            break;
        default:
            value = 0;
            break;
    }
    subframe_info[0].guard_interval = value;
    value = get_bits(11);
    subframe_info[0].num_ofdm_symbols = value + 1;
    value = get_bits(5);
    value = get_bits(3);
    value = get_bits(1);
    l1b_first_sub_sbs_first = value;
    value = get_bits(1);
    l1b_first_sub_sbs_last = value;
    if (l1b_version == 0) {
        value = get_bits(48);
    }
    else if (l1b_version >= 1) {
        value = get_bits(1);
        value = get_bits(47);
    }
    value = get_bits(32);

    value = get_bits(4);
    l1d_version = value;
    value = get_bits(3);
    l1d_num_rf = value;
    for (i = 1; i <= l1d_num_rf; i++) {
        value = get_bits(16);
        get_bits(3);
    }
    if (l1b_time_info_flag != 0) {
        value = get_bits(32);
        value = get_bits(10);
        if (l1b_time_info_flag != 1) {
            value = get_bits(10);
            if (l1b_time_info_flag != 2) {
                value = get_bits(10);
            }
        }
    }
    for (i = 0; i <= l1b_num_subframes; i++) {
        if (i > 0) {
            value = get_bits(1);
            l1d_mimo = value;
            value = get_bits(2);
            value = get_bits(2);
            switch (value) {
                case FFTSIZE_8K:
                    value = 8192;
                    break;
                case FFTSIZE_16K:
                    value = 16384;
                    break;
                case FFTSIZE_32K:
                    value = 32768;
                    break;
                default:
                    value = 8192;
                    break;
            }
            subframe_info[i].fft_size = value;
            value = get_bits(3);
            value = get_bits(4);
            switch (value) {
                case GI_RESERVED:
                    value = 0;
                    break;
                case GI_1_192:
                    value = 192;
                    break;
                case GI_2_384:
                    value = 384;
                    break;
                case GI_3_512:
                    value = 512;
                    break;
                case GI_4_768:
                    value = 768;
                    break;
                case GI_5_1024:
                    value = 1024;
                    break;
                case GI_6_1536:
                    value = 1536;
                    break;
                case GI_7_2048:
                    value = 2048;
                    break;
                case GI_8_2432:
                    value = 2432;
                    break;
                case GI_9_3072:
                    value = 3072;
                    break;
                case GI_10_3648:
                    value = 3648;
                    break;
                case GI_11_4096:
                    value = 4096;
                    break;
                case GI_12_4864:
                    value = 4864;
                    break;
                default:
                    value = 0;
                   break;
            }
            subframe_info[i].guard_interval = value;
            value = get_bits(11);
            subframe_info[i].num_ofdm_symbols = value + 1;
            value = get_bits(5);
            value = get_bits(3);
            value = get_bits(1);
            l1d_sbs_first = value;
            value = get_bits(1);
            l1d_sbs_first = value;
        }
        if (l1b_num_subframes > 0) {
            value = get_bits(1);
        }
        value = get_bits(1);
        if (i == 0) {
            if (l1b_first_sub_sbs_first == 1 || l1b_first_sub_sbs_last == 1) {
                value = get_bits(13);
            }
        }
        if (i > 0) {
            if (l1d_sbs_first == 1 || l1d_sbs_last == 1) {
                value = get_bits(13);
            }
        }
        value = get_bits(6);
        l1d_num_plp = value;
        for (j = 0; j <= l1d_num_plp; j++) {
            value = get_bits(6);
            value = get_bits(1);
            value = get_bits(2);
            l1d_plp_layer = value;
            value = get_bits(24);
            value = get_bits(24);
            value = get_bits(2);
            value = get_bits(4);
            if (value <= 5) {
                value = get_bits(4);
                l1d_plp_mod = value;
                value = get_bits(4);
                l1d_plp_cod = value;
            }
            value = get_bits(2);
            l1d_plp_TI_mode = value;
            if (l1d_plp_TI_mode == 0) {
                value = get_bits(15);
            }
            else if (l1d_plp_TI_mode == 1) {
                value = get_bits(22);
            }
            if (l1d_num_rf > 0) {
                value = get_bits(3);
                l1d_plp_num_channel_bonded = value;
                if (l1d_plp_num_channel_bonded > 0) {
                    value = get_bits(2);
                    for (k = 0; k < l1d_plp_num_channel_bonded; k++) {
                        value = get_bits(3);
                    }
                }
            }
            if ((i == 0 && l1b_first_sub_mimo == 1) || (i > 0 && l1d_mimo)) {
                value = get_bits(1);
                value = get_bits(1);
                value = get_bits(1);
            }
            if (l1d_plp_layer == 0) {
                value = get_bits(1);
                if (value == 0) {
                }
                else {
                    value = get_bits(14);
                    value = get_bits(24);
                }
                if (l1d_plp_TI_mode == 1 || l1d_plp_TI_mode == 2) {
                    if (l1d_plp_mod == 0) {
                        value = get_bits(1);
                        l1d_plp_TI_extended_interleaving = value;
                    }
                }
                if (l1d_plp_TI_mode == 1) {
                    value = get_bits(3);
                    value = get_bits(11);
                }
                else if (l1d_plp_TI_mode == 2) {
                    value = get_bits(1);
                    l1d_plp_HTI_inter_subframe = value;
                    value = get_bits(4);
                    l1d_plp_HTI_num_ti_blocks = value;
                    value = get_bits(12);
                    if (l1d_plp_HTI_inter_subframe == 0) {
                        value = get_bits(12);
                    }
                    else {
                        for (k = 0; k <= l1d_plp_HTI_num_ti_blocks; k++) {
                            value = get_bits(12);
                        }
                    }
                    value = get_bits(1);
                }
            }
            else {
                value = get_bits(5);
            }
        }
    }
    if (l1d_version >= 1) {
        value = get_bits(16);
    }
    if (l1d_version >= 2) {
        for (i = 0; i <= l1b_num_subframes; i++) {
            if (i > 0) {
                value = get_bits(1);
                l1d_mimo_mixed = value;
                value = value | (l1d_mimo << 1);
            }
            if ((i == 0 && l1b_first_sub_mimo_mixed) == 1 || (i > 0 && l1d_mimo_mixed == 1)) {
                for (j = 0; j <= l1d_num_plp; j++) {
                    value = get_bits(1);
                    if (value == 1) {
                        value = get_bits(1);
                        value = get_bits(1);
                        value = get_bits(1);
                    }
                }
            }
        }
    }
    if ((((l1b_l1_detail_size_bytes * 8) - 32) - (bits_index - 200)) > 0) {
        get_bits(((l1b_l1_detail_size_bytes * 8) - 32) - (bits_index - 200));
    }
    value = get_bits(32);

    bits_index = 0;
    value = get_bits(3);
    printf("L1B_version = %ld\n", value);
    l1b_version = value;
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_mimo_scattered_pilot_encoding = Walsh-Hadamard pilots or no MIMO subframes\n");
    }
    else {
        printf("L1B_mimo_scattered_pilot_encoding = Null pilots\n");
    }
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_lls_flag = No LLS in current frame\n");
    }
    else {
        printf("L1B_lls_flag = LLS in current frame\n");
    }
    value = get_bits(2);
    switch (value) {
        case 0:
            printf("L1B_time_info_flag = Time information is not included in the current frame\n");
            break;
        case 1:
            printf("L1B_time_info_flag = Time information is included in the current frame and signaled to ms precision\n");
            break;
        case 2:
            printf("L1B_time_info_flag = Time information is included in the current frame and signaled to µs precision\n");
            break;
        case 3:
            printf("L1B_time_info_flag = Time information is included in the current frame and signaled to ns precision\n");
            break;
    }
    l1b_time_info_flag = value;
    value = get_bits(1);
    printf("L1B_return_channel_flag = %ld\n", value);
    value = get_bits(2);
    switch (value) {
        case 0:
            printf("L1B_papr_reduction = No PAPR reduction used\n");
            break;
        case 1:
            printf("L1B_papr_reduction = Tone reservation only\n");
            break;
        case 2:
            printf("L1B_papr_reduction = ACE only\n");
            break;
        case 3:
            printf("L1B_papr_reduction = Both TR and ACE\n");
            break;
    }
    l1b_papr_reduction = value & 1;
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_frame_length_mode = time-aligned\n");
    }
    else {
        printf("L1B_frame_length_mode = symbol-aligned\n");
    }
    l1b_frame_length_mode = value;
    if (value == 0) {
        value = get_bits(10);
        printf("L1B_frame_length = %ld\n", value);
        l1b_frame_length = value;
        value = get_bits(13);
        printf("L1B_excess_samples_per_symbol = %ld\n", value);
        l1b_excess_samples_per_symbol = value;
    }
    else {
        value = get_bits(16);
        printf("L1B_time_offset = %ld\n", value);
        value = get_bits(7);
        printf("L1B_additional samples = %ld\n", value);
    }
    value = get_bits(8);
    printf("L1B_num_subframes = %ld\n", value + 1);
    l1b_num_subframes = value;
    value = get_bits(3);
    printf("L1B_preamble_num_symbols = %ld\n", value + 1);
    l1b_preamble_num_symbols = value + 1;
    value = get_bits(3);
    if (l1b_preamble_num_symbols > 1) {
        switch (value) {
            case CRED_0:
                printf("L1B_preamble_reduced_carriers = 5.832 MHz\n");
                break;
            case CRED_1:
                printf("L1B_preamble_reduced_carriers = 5.751 MHz\n");
                break;
            case CRED_2:
                printf("L1B_preamble_reduced_carriers = 5.670 MHz\n");
                break;
            case CRED_3:
                printf("L1B_preamble_reduced_carriers = 5.589 MHz\n");
                break;
            case CRED_4:
                printf("L1B_preamble_reduced_carriers = 5.508 MHz\n");
                break;
            default:
                printf("L1B_preamble_reduced_carriers = Reserved\n");
                break;
        }
    }
    value = get_bits(2);
    printf("L1B_L1_Detail_content_tag = %ld\n", value);
    value = get_bits(13);
    printf("L1B_L1_Detail_size_bytes = %ld\n", value);
    l1b_l1_detail_size_bytes = value;
    value = get_bits(3);
    switch (value) {
        case L1_FEC_MODE_1:
            printf("L1B_L1_Detail_fec_type = Mode 1\n");
            break;
        case L1_FEC_MODE_2:
            printf("L1B_L1_Detail_fec_type = Mode 2\n");
            break;
        case L1_FEC_MODE_3:
            printf("L1B_L1_Detail_fec_type = Mode 3\n");
            break;
        case L1_FEC_MODE_4:
            printf("L1B_L1_Detail_fec_type = Mode 4\n");
            break;
        case L1_FEC_MODE_5:
            printf("L1B_L1_Detail_fec_type = Mode 5\n");
            break;
        case L1_FEC_MODE_6:
            printf("L1B_L1_Detail_fec_type = Mode 6\n");
            break;
        case L1_FEC_MODE_7:
            printf("L1B_L1_Detail_fec_type = Mode 7\n");
            break;
        case 7:
            printf("L1B_L1_Detail_fec_type = Reserved\n");
            break;
    }
    value = get_bits(2);
    switch (value) {
        case 0:
            printf("L1B_L1_additional_parity_mode = K=0 (No additional parity used)\n");
            break;
        case 1:
            printf("L1B_L1_additional_parity_mode = K=1)\n");
            break;
        case 2:
            printf("L1B_L1_additional_parity_mode = K=2\n");
            break;
        case 3:
            printf("L1B_L1_additional_parity_mode = Reserved for future use\n");
            break;
    }
    value = get_bits(19);
    printf("L1B_L1_Detail_total_cells = %ld\n", value);
    l1b_l1_detail_total_cells = value;
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_first_sub_mimo = No MIMO\n");
    }
    else {
        printf("L1B_first_sub_mimo = MIMO\n");
    }
    l1b_first_sub_mimo = value;
    value = get_bits(2);
    switch (value) {
        case 0:
            printf("L1B_first_sub_miso = No MISO\n");
            break;
        case 1:
            printf("L1B_first_sub_miso = MISO with 64 coefficients\n");
            break;
        case 2:
            printf("L1B_first_sub_miso = MISO with 256 coefficients\n");
            break;
        case 3:
            printf("L1B_first_sub_miso = Reserved\n");
            break;
    }
    value = get_bits(2);
    switch (value) {
        case FFTSIZE_8K:
            printf("L1B_first_sub_fft_size = 8K\n");
            break;
        case FFTSIZE_16K:
            printf("L1B_first_sub_fft_size = 16K\n");
            break;
        case FFTSIZE_32K:
            printf("L1B_first_sub_fft_size = 32K\n");
            break;
        default:
            printf("L1B_first_sub_fft_size = Reserved\n");
            break;
    }
    l1b_first_sub_fft_size = value;
    value = get_bits(3);
    switch (value) {
        case CRED_0:
            printf("L1B_first_sub_reduced_carriers = 5.832 MHz\n");
            break;
        case CRED_1:
            printf("L1B_first_sub_reduced_carriers = 5.751 MHz\n");
            break;
        case CRED_2:
            printf("L1B_first_sub_reduced_carriers = 5.670 MHz\n");
            break;
        case CRED_3:
            printf("L1B_first_sub_reduced_carriers = 5.589 MHz\n");
            break;
        case CRED_4:
            printf("L1B_first_sub_reduced_carriers = 5.508 MHz\n");
            break;
        default:
            printf("L1B_first_sub_reduced_carriers = Reserved\n");
            break;
    }
    l1b_first_sub_reduced_carriers = value;
    value = get_bits(4);
    switch (value) {
        case GI_RESERVED:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
        case GI_1_192:
            printf("L1B_first_sub_guard_interval = GI1_192\n");
            break;
        case GI_2_384:
            printf("L1B_first_sub_guard_interval = GI2_384\n");
            break;
        case GI_3_512:
            printf("L1B_first_sub_guard_interval = GI3_512\n");
            break;
        case GI_4_768:
            printf("L1B_first_sub_guard_interval = GI4_768\n");
            break;
        case GI_5_1024:
            printf("L1B_first_sub_guard_interval = GI5_1024\n");
            break;
        case GI_6_1536:
            printf("L1B_first_sub_guard_interval = GI6_1536\n");
            break;
        case GI_7_2048:
            printf("L1B_first_sub_guard_interval = GI7_2048\n");
            break;
        case GI_8_2432:
            printf("L1B_first_sub_guard_interval = GI8_2432\n");
            break;
        case GI_9_3072:
            printf("L1B_first_sub_guard_interval = GI9_3072\n");
            break;
        case GI_10_3648:
            printf("L1B_first_sub_guard_interval = GI10_3648\n");
            break;
        case GI_11_4096:
            printf("L1B_first_sub_guard_interval = GI11_4096\n");
            break;
        case GI_12_4864:
            printf("L1B_first_sub_guard_interval = GI12_4864\n");
            break;
        default:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
    }
    l1b_first_sub_guard_interval = value;
    value = get_bits(11);
    printf("L1B_first_sub_num_ofdm_symbols = %ld\n", value + 1);
    l1b_first_sub_num_ofdm_symbols = value + 1;
    value = get_bits(5);
    l1b_first_sub_scattered_pilot_pattern = value;
    switch (value) {
        case PILOT_SP3_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP3_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.175\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.288\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.396\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP3_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP3_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.175\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.396\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.660\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP4_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP4_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.072\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.274\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.413\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP4_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP4_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.274\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.660\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.799\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP6_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP6_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.202\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.429\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.585\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.698\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP6_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP6_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.413\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.679\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.862\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP8_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP8_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.288\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.698\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.841\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP8_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP8_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.799\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.138\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP12_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP12_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.445\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.718\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.905\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP12_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP12_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.679\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.213\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.371\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP16_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP16_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.841\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.188\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP16_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP16_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.820\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.163\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.399\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.570\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP24_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP24_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.718\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.265\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.427\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP24_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP24_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.018\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.399\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.661\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.851\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP32_2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP32_2\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.862\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.213\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.427\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.630\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case PILOT_SP32_4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP32_4\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            switch (value) {
                case SPB_0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case SPB_1:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.163\n");
                    break;
                case SPB_2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.570\n");
                    break;
                case SPB_3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.851\n");
                    break;
                case SPB_4:
                    printf("L1B_first_sub_scattered_pilot_boost = 3.055\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        default:
            printf("L1B_first_sub_scattered_pilot_pattern = Reserved\n");
            value = get_bits(3);
            l1b_first_sub_scattered_pilot_boost = value;
            printf("L1B_first_sub_scattered_pilot_boost = Undefined\n");
            break;
    }
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_first_sub_sbs_first = not present\n");
    }
    else {
        printf("L1B_first_sub_sbs_first = present\n");
    }
    l1b_first_sub_sbs_first = value;
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_first_sub_sbs_last = not present\n");
    }
    else {
        printf("L1B_first_sub_sbs_last = present\n");
    }
    l1b_first_sub_sbs_last = value;
    if (l1b_version == 0) {
        value = get_bits(48);
    }
    else if (l1b_version >= 1) {
        value = get_bits(1);
        l1b_first_sub_mimo_mixed = value;
        value = value | (l1b_first_sub_mimo << 1);
        switch (value) {
            case 0:
                printf("L1B_first_sub_mimo_mixed = All PLPs in first subframe use SISO\n");
                break;
            case 1:
                printf("L1B_first_sub_mimo_mixed = PLPs of both types in first subframe\n");
                break;
            case 2:
                printf("L1B_first_sub_mimo_mixed = All PLPs in first subframe use MIMO\n");
                break;
            case 3:
                printf("L1B_first_sub_mimo_mixed = Invalid Combination\n");
                break;
        }
        value = get_bits(47);
    }
    value = get_bits(32);
    printf("L1B_crc = 0x%08x\n", (unsigned int)value);

    value = get_bits(4);
    printf("L1D_version = %ld\n", value);
    l1d_version = value;
    value = get_bits(3);
    if (value == 0) {
        printf("L1D_num_rf = No Channel Bonding\n");
    }
    else {
        printf("L1D_num_rf = Channel Bonding, %ld channel(s)\n", value);
    }
    l1d_num_rf = value;
    for (i = 1; i <= l1d_num_rf; i++) {
        value = get_bits(16);
        printf("L1D_bonded_bsid = 0x%04x\n", (unsigned int)value);
        get_bits(3);
    }
    if (l1b_time_info_flag != 0) {
        value = get_bits(32);
        printf("L1D_time_sec = %ld\n", value);
        value = get_bits(10);
        printf("L1D_time_msec = %ld\n", value);
        if (l1b_time_info_flag != 1) {
            value = get_bits(10);
            printf("L1D_time_usec = %ld\n", value);
            if (l1b_time_info_flag != 2) {
                value = get_bits(10);
                printf("L1D_time_nsec = %ld\n", value);
            }
        }
    }
    for (i = 0; i <= l1b_num_subframes; i++) {
        printf("******** subframe = %d ********\n", i);
        if (i > 0) {
            value = get_bits(1);
            if (value == 0) {
                printf("L1D_mimo = No MIMO\n");
            }
            else {
                printf("L1D_mimo = MIMO\n");
            }
            l1d_mimo = value;
            value = get_bits(2);
            switch (value) {
                case 0:
                    printf("L1D_miso = No MISO\n");
                    break;
                case 1:
                    printf("L1D_miso = MISO with 64 coefficients\n");
                    break;
                case 2:
                    printf("L1D_miso = MISO with 256 coefficients\n");
                    break;
                case 3:
                    printf("L1D_miso = Reserved\n");
                    break;
            }
            value = get_bits(2);
            switch (value) {
                case FFTSIZE_8K:
                    printf("L1D_fft_size = 8K\n");
                    break;
                case FFTSIZE_16K:
                    printf("L1D_fft_size = 16K\n");
                    break;
                case FFTSIZE_32K:
                    printf("L1D_fft_size = 32K\n");
                    break;
                default:
                    printf("L1D_fft_size = Reserved\n");
                    break;
            }
            l1d_fft_size = value;
            value = get_bits(3);
            switch (value) {
                case CRED_0:
                    printf("L1D_reduced_carriers = 5.832 MHz\n");
                    break;
                case CRED_1:
                    printf("L1D_reduced_carriers = 5.751 MHz\n");
                    break;
                case CRED_2:
                    printf("L1D_reduced_carriers = 5.670 MHz\n");
                    break;
                case CRED_3:
                    printf("L1D_reduced_carriers = 5.589 MHz\n");
                    break;
                case CRED_4:
                    printf("L1D_reduced_carriers = 5.508 MHz\n");
                    break;
                default:
                    printf("L1D_reduced_carriers = Reserved\n");
                    break;
            }
            l1d_reduced_carriers = value;
            value = get_bits(4);
            switch (value) {
                case GI_RESERVED:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
                case GI_1_192:
                    printf("L1D_guard_interval = GI1_192\n");
                    break;
                case GI_2_384:
                    printf("L1D_guard_interval = GI2_384\n");
                    break;
                case GI_3_512:
                    printf("L1D_guard_interval = GI3_512\n");
                    break;
                case GI_4_768:
                    printf("L1D_guard_interval = GI4_768\n");
                    break;
                case GI_5_1024:
                    printf("L1D_guard_interval = GI5_1024\n");
                    break;
                case GI_6_1536:
                    printf("L1D_guard_interval = GI6_1536\n");
                    break;
                case GI_7_2048:
                    printf("L1D_guard_interval = GI7_2048\n");
                    break;
                case GI_8_2432:
                    printf("L1D_guard_interval = GI8_2432\n");
                    break;
                case GI_9_3072:
                    printf("L1D_guard_interval = GI9_3072\n");
                    break;
                case GI_10_3648:
                    printf("L1D_guard_interval = GI10_3648\n");
                    break;
                case GI_11_4096:
                    printf("L1D_guard_interval = GI11_4096\n");
                    break;
                case GI_12_4864:
                    printf("L1D_guard_interval = GI12_4864\n");
                    break;
                default:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
            }
            l1d_guard_interval = value;
            value = get_bits(11);
            printf("L1D_num_ofdm_symbols = %ld\n", value + 1);
            l1d_num_ofdm_symbols = value + 1;
            value = get_bits(5);
            l1d_scattered_pilot_pattern = value;
            switch (value) {
                case PILOT_SP3_2:
                    printf("L1D_scattered_pilot_pattern = SP3_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.175\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.288\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.396\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP3_4:
                    printf("L1D_scattered_pilot_pattern = SP3_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.175\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.396\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.660\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP4_2:
                    printf("L1D_scattered_pilot_pattern = SP4_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.072\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.274\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.413\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP4_4:
                    printf("L1D_scattered_pilot_pattern = SP4_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.274\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.660\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.799\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP6_2:
                    printf("L1D_scattered_pilot_pattern = SP6_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.202\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.429\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.585\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.698\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP6_4:
                    printf("L1D_scattered_pilot_pattern = SP6_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.413\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.679\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.862\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP8_2:
                    printf("L1D_scattered_pilot_pattern = SP8_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.288\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.698\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 1.841\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP8_4:
                    printf("L1D_scattered_pilot_pattern = SP8_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.799\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.138\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP12_2:
                    printf("L1D_scattered_pilot_pattern = SP12_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.445\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.718\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 1.905\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP12_4:
                    printf("L1D_scattered_pilot_pattern = SP12_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.679\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.213\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.371\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP16_2:
                    printf("L1D_scattered_pilot_pattern = SP16_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 1.841\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.188\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP16_4:
                    printf("L1D_scattered_pilot_pattern = SP16_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.820\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 2.163\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.399\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.570\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP24_2:
                    printf("L1D_scattered_pilot_pattern = SP24_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.718\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.265\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.427\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP24_4:
                    printf("L1D_scattered_pilot_pattern = SP24_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 2.018\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 2.399\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.661\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.851\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP32_2:
                    printf("L1D_scattered_pilot_pattern = SP32_2\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 1.862\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 2.213\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.427\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 2.630\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case PILOT_SP32_4:
                    printf("L1D_scattered_pilot_pattern = SP32_4\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    switch (value) {
                        case SPB_0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case SPB_1:
                            printf("L1D_scattered_pilot_boost = 2.163\n");
                            break;
                        case SPB_2:
                            printf("L1D_scattered_pilot_boost = 2.570\n");
                            break;
                        case SPB_3:
                            printf("L1D_scattered_pilot_boost = 2.851\n");
                            break;
                        case SPB_4:
                            printf("L1D_scattered_pilot_boost = 3.055\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                default:
                    printf("L1D_scattered_pilot_pattern = Reserved\n");
                    value = get_bits(3);
                    l1d_scattered_pilot_boost = value;
                    printf("L1D_scattered_pilot_boost = Undefined\n");
                    break;
            }
            value = get_bits(1);
            if (value == 0) {
                printf("L1D_sbs_first = not present\n");
            }
            else {
                printf("L1D_sbs_first = present\n");
            }
            l1d_sbs_first = value;
            value = get_bits(1);
            if (value == 0) {
                printf("L1D_sbs_last = not present\n");
            }
            else {
                printf("L1D_sbs_last = present\n");
            }
            l1d_sbs_first = value;
        }
        if (l1b_num_subframes > 0) {
            value = get_bits(1);
            printf("L1D_subframe_multiplex = %ld\n", value);
        }
        value = get_bits(1);
        if (value == 0) {
            printf("L1D_frequency_interleaver = Preamble Only\n");
        }
        else {
            printf("L1D_frequency_interleaver = All Symbols\n");
        }
        if (i == 0) {
            if (l1b_first_sub_sbs_first == 1 || l1b_first_sub_sbs_last == 1) {
                value = get_bits(13);
                printf("L1D_sbs_null_cells = %ld\n", value);
            }
        }
        if (i > 0) {
            if (l1d_sbs_first == 1 || l1d_sbs_last == 1) {
                value = get_bits(13);
                printf("L1D_sbs_null_cells = %ld\n", value);
            }
        }
        value = get_bits(6);
        printf("L1D_num_plp = %ld\n", value + 1);
        l1d_num_plp = value;
        for (j = 0; j <= l1d_num_plp; j++) {
            printf("********** plp = %d **********\n", j);
            value = get_bits(6);
            printf("L1D_plp%d_id = %ld\n", j, value);
            value = get_bits(1);
            printf("L1D_plp%d_lls_flag = %ld\n", j, value);
            value = get_bits(2);
            switch (value) {
                case 0:
                    printf("L1D_plp%d_layer = Core\n", j);
                    break;
                case 1:
                    printf("L1D_plp%d_layer = Enhanced\n", j);
                    break;
                default:
                    printf("L1D_plp%d_layer = Reserved\n", j);
                    break;
            }
            l1d_plp_layer = value;
            value = get_bits(24);
            printf("L1D_plp%d_start = %ld\n", j, value);
            value = get_bits(24);
            printf("L1D_plp%d_size = %ld\n", j, value);
            value = get_bits(2);
            switch (value) {
                case 0:
                    printf("L1D_plp%d_scrambler_type = PRBS\n", j);
                    break;
                default:
                    printf("L1D_plp%d_scrambler_type = Reserved\n", j);
                    break;
            }
            value = get_bits(4);
            switch (value) {
                case 0:
                    printf("L1D_plp%d_fec_type = BCH + 16K LDPC\n", j);
                    break;
                case 1:
                    printf("L1D_plp%d_fec_type = BCH + 64K LDPC\n", j);
                    break;
                case 2:
                    printf("L1D_plp%d_fec_type = CRC + 16K LDPC\n", j);
                    break;
                case 3:
                    printf("L1D_plp%d_fec_type = CRC + 64K LDPC\n", j);
                    break;
                case 4:
                    printf("L1D_plp%d_fec_type = 16K LDPC only\n", j);
                    break;
                case 5:
                    printf("L1D_plp%d_fec_type = 64K LDPC only\n", j);
                    break;
                default:
                    printf("L1D_plp%d_fec_type = Reserved\n", j);
                    break;
            }
            l1d_plp_fec_type = !(value & 1);
            if (value <= 5) {
                value = get_bits(4);
                switch (value) {
                    case MOD_QPSK:
                        printf("L1D_plp%d_mod = QPSK\n", j);
                        break;
                    case MOD_16QAM:
                        printf("L1D_plp%d_mod = 16QAM\n", j);
                        break;
                    case MOD_64QAM:
                        printf("L1D_plp%d_mod = 64QAM\n", j);
                        break;
                    case MOD_256QAM:
                        printf("L1D_plp%d_mod = 256QAM\n", j);
                        break;
                    case MOD_1024QAM:
                        printf("L1D_plp%d_mod = 1024QAM\n", j);
                        break;
                    case MOD_4096QAM:
                        printf("L1D_plp%d_mod = 4096QAM\n", j);
                        break;
                    default:
                        printf("L1D_plp%d_mod = Reserved\n", j);
                        break;
                }
                l1d_plp_mod = value;
                value = get_bits(4);
                switch (value) {
                    case C2_15:
                        printf("L1D_plp%d_cod = 2/15\n", j);
                        break;
                    case C3_15:
                        printf("L1D_plp%d_cod = 3/15\n", j);
                        break;
                    case C4_15:
                        printf("L1D_plp%d_cod = 4/15\n", j);
                        break;
                    case C5_15:
                        printf("L1D_plp%d_cod = 5/15\n", j);
                        break;
                    case C6_15:
                        printf("L1D_plp%d_cod = 6/15\n", j);
                        break;
                    case C7_15:
                        printf("L1D_plp%d_cod = 7/15\n", j);
                        break;
                    case C8_15:
                        printf("L1D_plp%d_cod = 8/15\n", j);
                        break;
                    case C9_15:
                        printf("L1D_plp%d_cod = 9/15\n", j);
                        break;
                    case C10_15:
                        printf("L1D_plp%d_cod = 10/15\n", j);
                        break;
                    case C11_15:
                        printf("L1D_plp%d_cod = 11/15\n", j);
                        break;
                    case C12_15:
                        printf("L1D_plp%d_cod = 12/15\n", j);
                        break;
                    case C13_15:
                        printf("L1D_plp%d_cod = 13/15\n", j);
                        break;
                    default:
                        printf("L1D_plp%d_cod = Reserved\n", j);
                        break;
                }
                l1d_plp_cod = value;
            }
            value = get_bits(2);
            switch (value) {
                case 0:
                    printf("L1D_plp%d_TI_mode = No time interleaving\n", j);
                    break;
                case 1:
                    printf("L1D_plp%d_TI_mode = Convolutional time interleaving\n", j);
                    break;
                case 2:
                    printf("L1D_plp%d_TI_mode = Hybrid time interleaving\n", j);
                    break;
                case 3:
                    printf("L1D_plp%d_TI_mode = Reserved\n", j);
                    break;
            }
            l1d_plp_TI_mode = value;
            if (l1d_plp_TI_mode == 0) {
                value = get_bits(15);
                printf("L1D_plp%d_fec_block_start = %ld\n", j, value);
            }
            else if (l1d_plp_TI_mode == 1) {
                value = get_bits(22);
                printf("L1D_plp%d_CTI_fec_block_start = %ld\n", j, value);
            }
            if (l1d_num_rf > 0) {
                value = get_bits(3);
                printf("L1D_plp%d_num_channel_bonded = %ld\n", j, value);
                l1d_plp_num_channel_bonded = value;
                if (l1d_plp_num_channel_bonded > 0) {
                    value = get_bits(2);
                    switch (value) {
                        case 0:
                            printf("L1D_plp%d_channel_bonding_format = Plain channel bonding\n", j);
                            break;
                        case 1:
                            printf("L1D_plp%d_channel_bonding_format = SNR averaged channel bonding\n", j);
                            break;
                        default:
                            printf("L1D_plp%d_channel_bonding_format = Reserved\n", j);
                            break;
                    }
                    for (k = 0; k < l1d_plp_num_channel_bonded; k++) {
                        value = get_bits(3);
                        printf("L1D_plp%d_bonded_rf_id = %ld\n", j, value);
                    }
                }
            }
            if ((i == 0 && l1b_first_sub_mimo == 1) || (i > 0 && l1d_mimo)) {
                value = get_bits(1);
                printf("L1D_plp%d_mimo_stream_combining = %ld\n", j, value);
                value = get_bits(1);
                printf("L1D_plp%d_mimo_IQ_interleaving = %ld\n", j, value);
                value = get_bits(1);
                printf("L1D_plp%d_mimo_PH = %ld\n", j, value);
            }
            if (l1d_plp_layer == 0) {
                value = get_bits(1);
                if (value == 0) {
                    printf("L1D_plp%d_type = non-dispersed\n", j);
                }
                else {
                    printf("L1D_plp%d_type = dispersed\n", j);
                    value = get_bits(14);
                    printf("L1D_plp%d_num_subslices = %ld\n", j, value + 1);
                    value = get_bits(24);
                    printf("L1D_plp%d_subslice_interval = %ld\n", j, value);
                }
                if (l1d_plp_TI_mode == 1 || l1d_plp_TI_mode == 2) {
                    if (l1d_plp_mod == 0) {
                        value = get_bits(1);
                        if (value == 0) {
                            printf("L1D_plp%d_TI_extended_interleaving = disabled\n", j);
                        }
                        else {
                            printf("L1D_plp%d_TI_extended_interleaving = enabled\n", j);
                        }
                        l1d_plp_TI_extended_interleaving = value;
                    }
                }
                if (l1d_plp_TI_mode == 1) {
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_plp%d_CTI_depth = 512\n", j);
                            break;
                        case 1:
                            printf("L1D_plp%d_CTI_depth = 724\n", j);
                            break;
                        case 2:
                            if (l1d_plp_TI_extended_interleaving == 0) {
                                printf("L1D_plp%d_CTI_depth = 887\n", j);
                            }
                            else {
                                printf("L1D_plp%d_CTI_depth = 1254\n", j);
                            }
                            break;
                        case 3:
                            if (l1d_plp_TI_extended_interleaving == 0) {
                                printf("L1D_plp%d_CTI_depth = 1024\n", j);
                            }
                            else {
                                printf("L1D_plp%d_CTI_depth = 1448\n", j);
                            }
                            break;
                        default:
                            printf("L1D_plp%d_CTI_depth = Reserved\n", j);
                            break;
                    }
                    value = get_bits(11);
                    printf("L1D_plp%d_CTI_start_row = %ld\n", j, value);
                }
                else if (l1d_plp_TI_mode == 2) {
                    value = get_bits(1);
                    printf("L1D_plp%d_HTI_inter_subframe = %ld\n", j, value);
                    l1d_plp_HTI_inter_subframe = value;
                    value = get_bits(4);
                    printf("L1D_plp%d_HTI_num_ti_blocks = %ld\n", j, value + 1);
                    l1d_plp_HTI_num_ti_blocks = value;
                    value = get_bits(12);
                    printf("L1D_plp%d_HTI_num_fec_blocks_max = %ld\n", j, value + 1);
                    if (l1d_plp_HTI_inter_subframe == 0) {
                        value = get_bits(12);
                        printf("L1D_plp%d_HTI_num_fec_blocks = %ld\n", j, value + 1);
                        l1d_plp_HTI_num_fec_blocks = value;
                    }
                    else {
                        for (k = 0; k <= l1d_plp_HTI_num_ti_blocks; k++) {
                            value = get_bits(12);
                            printf("L1D_plp%d_HTI_num_fec_blocks = %ld\n", j, value + 1);
                        }
                    }
                    value = get_bits(1);
                    if (value == 0) {
                        printf("L1D_plp%d_HTI_cell_interleaver = disabled\n", j);
                    }
                    else {
                        printf("L1D_plp%d_HTI_cell_interleaver = enabled\n", j);
                    }
                }
            }
            else {
                value = get_bits(5);
                switch (value) {
                    case 0:
                        printf("L1D_plp%d_ldm_injection_level = 0.0 dB\n", j);
                        break;
                    case 1:
                        printf("L1D_plp%d_ldm_injection_level = 0.05 dB\n", j);
                        break;
                    case 2:
                        printf("L1D_plp%d_ldm_injection_level = 1.0 dB\n", j);
                        break;
                    case 3:
                        printf("L1D_plp%d_ldm_injection_level = 1.5 dB\n", j);
                        break;
                    case 4:
                        printf("L1D_plp%d_ldm_injection_level = 2.0 dB\n", j);
                        break;
                    case 5:
                        printf("L1D_plp%d_ldm_injection_level = 2.5 dB\n", j);
                        break;
                    case 6:
                        printf("L1D_plp%d_ldm_injection_level = 3.0 dB\n", j);
                        break;
                    case 7:
                        printf("L1D_plp%d_ldm_injection_level = 3.5 dB\n", j);
                        break;
                    case 8:
                        printf("L1D_plp%d_ldm_injection_level = 4.0 dB\n", j);
                        break;
                    case 9:
                        printf("L1D_plp%d_ldm_injection_level = 4.5 dB\n", j);
                        break;
                    case 10:
                        printf("L1D_plp%d_ldm_injection_level = 5.0 dB\n", j);
                        break;
                    case 11:
                        printf("L1D_plp%d_ldm_injection_level = 6.0 dB\n", j);
                        break;
                    case 12:
                        printf("L1D_plp%d_ldm_injection_level = 7.0 dB\n", j);
                        break;
                    case 13:
                        printf("L1D_plp%d_ldm_injection_level = 8.0 dB\n", j);
                        break;
                    case 14:
                        printf("L1D_plp%d_ldm_injection_level = 9.0 dB\n", j);
                        break;
                    case 15:
                        printf("L1D_plp%d_ldm_injection_level = 10.0 dB\n", j);
                        break;
                    case 16:
                        printf("L1D_plp%d_ldm_injection_level = 11.0 dB\n", j);
                        break;
                    case 17:
                        printf("L1D_plp%d_ldm_injection_level = 12.0 dB\n", j);
                        break;
                    case 18:
                        printf("L1D_plp%d_ldm_injection_level = 13.0 dB\n", j);
                        break;
                    case 19:
                        printf("L1D_plp%d_ldm_injection_level = 14.0 dB\n", j);
                        break;
                    case 20:
                        printf("L1D_plp%d_ldm_injection_level = 15.0 dB\n", j);
                        break;
                    case 21:
                        printf("L1D_plp%d_ldm_injection_level = 16.0 dB\n", j);
                        break;
                    case 22:
                        printf("L1D_plp%d_ldm_injection_level = 17.0 dB\n", j);
                        break;
                    case 23:
                        printf("L1D_plp%d_ldm_injection_level = 18.0 dB\n", j);
                        break;
                    case 24:
                        printf("L1D_plp%d_ldm_injection_level = 19.0 dB\n", j);
                        break;
                    case 25:
                        printf("L1D_plp%d_ldm_injection_level = 20.0 dB\n", j);
                        break;
                    case 26:
                        printf("L1D_plp%d_ldm_injection_level = 21.0 dB\n", j);
                        break;
                    case 27:
                        printf("L1D_plp%d_ldm_injection_level = 22.0 dB\n", j);
                        break;
                    case 28:
                        printf("L1D_plp%d_ldm_injection_level = 23.0 dB\n", j);
                        break;
                    case 29:
                        printf("L1D_plp%d_ldm_injection_level = 24.0 dB\n", j);
                        break;
                    case 30:
                        printf("L1D_plp%d_ldm_injection_level = 25.0 dB\n", j);
                        break;
                    case 31:
                        printf("L1D_plp%d_ldm_injection_level = Reserved\n", j);
                        break;
                }
            }
            if (i == 0) {
                atsc3rate(l1b_first_sub_fft_size, l1b_first_sub_guard_interval, l1b_first_sub_num_ofdm_symbols, l1b_preamble_num_symbols, l1d_plp_cod, l1d_plp_mod, l1d_plp_fec_type, l1b_first_sub_scattered_pilot_pattern, l1b_first_sub_sbs_first, l1b_first_sub_reduced_carriers, l1b_first_sub_scattered_pilot_boost, l1b_papr_reduction, l1d_plp_TI_mode, l1d_plp_HTI_num_fec_blocks + 1, l1b_l1_detail_total_cells, i, l1b_num_subframes + 1, &subframe_info[0], l1b_frame_length_mode, l1b_frame_length, l1b_excess_samples_per_symbol);
            }
            else if (i > 0) {
                atsc3rate(l1d_fft_size, l1d_guard_interval, l1d_num_ofdm_symbols, 0, l1d_plp_cod, l1d_plp_mod, l1d_plp_fec_type, l1d_scattered_pilot_pattern, l1d_sbs_first, l1d_reduced_carriers, l1d_scattered_pilot_boost, l1b_papr_reduction, l1d_plp_TI_mode, l1d_plp_HTI_num_fec_blocks + 1, l1b_l1_detail_total_cells, i, l1b_num_subframes + 1, &subframe_info[0], l1b_frame_length_mode, l1b_frame_length, l1b_excess_samples_per_symbol);
            }
        }
    }
    if (l1d_version >= 1) {
        value = get_bits(16);
        printf("L1D_bsid = 0x%04x\n", (unsigned int)value);
    }
    if (l1d_version >= 2) {
        for (i = 0; i <= l1b_num_subframes; i++) {
            if (i > 0) {
                value = get_bits(1);
                l1d_mimo_mixed = value;
                value = value | (l1d_mimo << 1);
                switch (value) {
                    case 0:
                        printf("L1D_mimo_mixed = All PLPs in subframe use SISO\n");
                        break;
                    case 1:
                        printf("L1D_mimo_mixed = PLPs of both types in subframe\n");
                        break;
                    case 2:
                        printf("L1D_mimo_mixed = All PLPs in subframe use MIMO\n");
                        break;
                    case 3:
                        printf("L1D_mimo_mixed = Invalid Combination\n");
                        break;
                }
            }
            if ((i == 0 && l1b_first_sub_mimo_mixed) == 1 || (i > 0 && l1d_mimo_mixed == 1)) {
                for (j = 0; j <= l1d_num_plp; j++) {
                    value = get_bits(1);
                    if (value == 0) {
                        printf("L1D_plp%d_mimo = No MIMO\n", j);
                    }
                    else {
                        printf("L1D_plp%d_mimo = MIMO\n", j);
                    }
                    if (value == 1) {
                        value = get_bits(1);
                        printf("L1D_plp%d_mimo_stream_combining = %ld\n", j, value);
                        value = get_bits(1);
                        printf("L1D_plp%d_mimo_IQ_interleaving = %ld\n", j, value);
                        value = get_bits(1);
                        printf("L1D_plp%d_mimo_PH = %ld\n", j, value);
                    }
                }
            }
        }
    }
    if ((((l1b_l1_detail_size_bytes * 8) - 32) - (bits_index - 200)) > 0) {
        get_bits(((l1b_l1_detail_size_bytes * 8) - 32) - (bits_index - 200));
    }
    value = get_bits(32);
    printf("L1D_crc = 0x%08x\n", (unsigned int)value);

    free(out);
    fclose(fp);
    return 0;
}

void atsc3rate(int fft_size, int guardinterval, int numpayloadsyms, int numpreamblesyms, int rate, int constellation, int framesize, int pilotpattern, int firstsbs, int cred, int pilotboost, int paprmode, int ti_mode, int fec_blocks, int l1_detail_cells, int subframe, int num_subframes, struct subframe_info_t *subframe_info, int frame_length_mode, int frame_length, int excess_samples)
{
  int mod, plpsize;
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
  double clock_num, clock_den = 1.0;
  double bitrate, T, TS, TF, TB, TSX, symbols, kbch, fecsize, fecrate;

  clock_num = 384000.0 * (16.0 + 2.0);
  switch (0)
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
  l1cells += l1_detail_cells;

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
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
        case GI_2_384:
          first_preamble_cells = preamble_cells_table[1][4];
          preamble_cells = preamble_cells_table[1][cred];
          break;
        case GI_3_512:
          first_preamble_cells = preamble_cells_table[2][4];
          preamble_cells = preamble_cells_table[2][cred];
          break;
        case GI_4_768:
          first_preamble_cells = preamble_cells_table[3][4];
          preamble_cells = preamble_cells_table[3][cred];
          break;
        case GI_5_1024:
          first_preamble_cells = preamble_cells_table[4][4];
          preamble_cells = preamble_cells_table[4][cred];
          break;
        case GI_6_1536:
          first_preamble_cells = preamble_cells_table[5][4];
          preamble_cells = preamble_cells_table[5][cred];
          break;
        case GI_7_2048:
          first_preamble_cells = preamble_cells_table[6][4];
          preamble_cells = preamble_cells_table[6][cred];
          break;
        default:
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
          first_preamble_cells = preamble_cells_table[7][4];
          preamble_cells = preamble_cells_table[7][cred];
          break;
        case GI_2_384:
          first_preamble_cells = preamble_cells_table[8][4];
          preamble_cells = preamble_cells_table[8][cred];
          break;
        case GI_3_512:
          first_preamble_cells = preamble_cells_table[9][4];
          preamble_cells = preamble_cells_table[9][cred];
          break;
        case GI_4_768:
          first_preamble_cells = preamble_cells_table[10][4];
          preamble_cells = preamble_cells_table[10][cred];
          break;
        case GI_5_1024:
          first_preamble_cells = preamble_cells_table[11][4];
          preamble_cells = preamble_cells_table[11][cred];
          break;
        case GI_6_1536:
          first_preamble_cells = preamble_cells_table[12][4];
          preamble_cells = preamble_cells_table[12][cred];
          break;
        case GI_7_2048:
          first_preamble_cells = preamble_cells_table[13][4];
          preamble_cells = preamble_cells_table[13][cred];
          break;
        case GI_8_2432:
          first_preamble_cells = preamble_cells_table[14][4];
          preamble_cells = preamble_cells_table[14][cred];
          break;
        case GI_9_3072:
          first_preamble_cells = preamble_cells_table[15][4];
          preamble_cells = preamble_cells_table[15][cred];
          break;
        case GI_10_3648:
          first_preamble_cells = preamble_cells_table[16][4];
          preamble_cells = preamble_cells_table[16][cred];
          break;
        case GI_11_4096:
          first_preamble_cells = preamble_cells_table[17][4];
          preamble_cells = preamble_cells_table[17][cred];
          break;
        default:
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
          first_preamble_cells = preamble_cells_table[18][4];
          preamble_cells = preamble_cells_table[18][cred];
          break;
        case GI_2_384:
          first_preamble_cells = preamble_cells_table[19][4];
          preamble_cells = preamble_cells_table[19][cred];
          break;
        case GI_3_512:
          first_preamble_cells = preamble_cells_table[20][4];
          preamble_cells = preamble_cells_table[20][cred];
          break;
        case GI_4_768:
          first_preamble_cells = preamble_cells_table[21][4];
          preamble_cells = preamble_cells_table[21][cred];
          break;
        case GI_5_1024:
          first_preamble_cells = preamble_cells_table[22][4];
          preamble_cells = preamble_cells_table[22][cred];
          break;
        case GI_6_1536:
          first_preamble_cells = preamble_cells_table[23][4];
          preamble_cells = preamble_cells_table[23][cred];
          break;
        case GI_7_2048:
          first_preamble_cells = preamble_cells_table[24][4];
          preamble_cells = preamble_cells_table[24][cred];
          break;
        case GI_8_2432:
          first_preamble_cells = preamble_cells_table[25][4];
          preamble_cells = preamble_cells_table[25][cred];
          break;
        case GI_9_3072:
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
          first_preamble_cells = preamble_cells_table[30][4];
          preamble_cells = preamble_cells_table[30][cred];
          break;
        case GI_12_4864:
          first_preamble_cells = preamble_cells_table[31][4];
          preamble_cells = preamble_cells_table[31][cred];
          break;
        default:
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
          first_preamble_cells = preamble_cells_table[0][4];
          preamble_cells = preamble_cells_table[0][cred];
          break;
        case GI_2_384:
          first_preamble_cells = preamble_cells_table[1][4];
          preamble_cells = preamble_cells_table[1][cred];
          break;
        case GI_3_512:
          first_preamble_cells = preamble_cells_table[2][4];
          preamble_cells = preamble_cells_table[2][cred];
          break;
        case GI_4_768:
          first_preamble_cells = preamble_cells_table[3][4];
          preamble_cells = preamble_cells_table[3][cred];
          break;
        case GI_5_1024:
          first_preamble_cells = preamble_cells_table[4][4];
          preamble_cells = preamble_cells_table[4][cred];
          break;
        case GI_6_1536:
          first_preamble_cells = preamble_cells_table[5][4];
          preamble_cells = preamble_cells_table[5][cred];
          break;
        case GI_7_2048:
          first_preamble_cells = preamble_cells_table[6][4];
          preamble_cells = preamble_cells_table[6][cred];
          break;
        default:
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
    fec_cells = 0;
  }

  if (paprmode != 1) {
    papr_cells = 0;
  }
  TF = 0.0;
  T = 1.0 / (clock_num / clock_den);
  TB = 1.0 / 6144000.0;
  if (frame_length_mode == 0) {
    for (int n = 0; n < num_subframes; n++) {
      if (n == 0) {
        TS = (T * (subframe_info[n].fft_size + subframe_info[n].guard_interval)) * 1000.0;
        TSX = (T * (subframe_info[n].fft_size + subframe_info[n].guard_interval + excess_samples)) * 1000.0;
        TF += (subframe_info[n].num_ofdm_symbols * TSX) + (subframe_info[n].num_preamble_symbols * TS) + (3072.0 * 4 * TB * 1000.0);
        if (subframe == n && num_subframes > 1) {
          if ((num_subframes - 1) == n) { 
            printf("sub-frame time = %f ms\n", (subframe_info[n].num_ofdm_symbols * TSX) + (subframe_info[n].num_preamble_symbols * TS) + (3072.0 * 4 * TB * 1000.0) + ((frame_length * 5.0) - TF));
          }
          else {
            printf("sub-frame time = %f ms\n", (subframe_info[n].num_ofdm_symbols * TSX) + (subframe_info[n].num_preamble_symbols * TS) + (3072.0 * 4 * TB * 1000.0));
          }
        }
      }
      else {
        symbols = subframe_info[n].num_ofdm_symbols;
        TS = (T * (subframe_info[n].fft_size + subframe_info[n].guard_interval + excess_samples)) * 1000.0;
        TF += (symbols * TS);
        if (subframe == n && num_subframes > 1) {
          if ((num_subframes - 1) == n) { 
            printf("sub-frame time = %f ms\n", (symbols * TS) + ((frame_length * 5.0) - TF));
          }
          else {
            printf("sub-frame time = %f ms\n", (symbols * TS));
          }
        }
      }
    }
    TF = frame_length * 5.0;
  }
  else {
    for (int n = 0; n < num_subframes; n++) {
      if (n == 0) {
        symbols = subframe_info[n].num_ofdm_symbols + subframe_info[n].num_preamble_symbols;
        TS = (T * (subframe_info[n].fft_size + subframe_info[n].guard_interval)) * 1000.0;
        TF += (symbols * TS) + (3072.0 * 4 * TB * 1000.0);
        if (subframe == n && num_subframes > 1) {
          printf("sub-frame time = %f ms\n", (symbols * TS) + (3072.0 * 4 * TB * 1000.0));
        }
      }
      else {
        symbols = subframe_info[n].num_ofdm_symbols;
        TS = (T * (subframe_info[n].fft_size + subframe_info[n].guard_interval)) * 1000.0;
        TF += (symbols * TS);
        if (subframe == n && num_subframes > 1) {
          printf("sub-frame time = %f ms\n", (symbols * TS));
        }
      }
    }
  }
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
  sbsnullcells = sbs_cells - sbs_data_cells;
  if (firstsbs) {
    plpsize = totalcells - l1cells - (sbsnullcells * 2);
  }
  else {
    plpsize = totalcells - l1cells - sbsnullcells;
  }
  if (ti_mode == 2) {
    plpsize = fec_blocks * fec_cells;
  }
  fecrate = ((kbch - 16) / fecsize); /* 1 TS packet per ALP packet and MODE always = 1 */
  bitrate = (1000.0 / TF) * (plpsize * mod * fecrate);
  printf("TS bitrate = %.03f\n", bitrate);
  fecrate = kbch / fecsize;
  bitrate = (1000.0 / TF) * (plpsize * mod * fecrate);
  printf("PLP bitrate = %.03f\n", bitrate);
}
