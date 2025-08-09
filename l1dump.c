#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

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
    int l1b_num_subframes;
    int l1b_preamble_num_symbols;
    int l1b_l1_detail_size_bytes;
    int l1b_first_sub_mimo;
    int l1b_first_sub_sbs_first;
    int l1b_first_sub_sbs_last;
    int l1b_first_sub_mimo_mixed = 0;
    int l1d_version;
    int l1d_num_rf;
    int l1d_mimo = 0;
    int l1d_sbs_first = 0;
    int l1d_sbs_last = 0;
    int l1d_num_plp = 0;
    int l1d_plp_layer;
    int l1d_plp_mod = 0;
    int l1d_plp_TI_mode;
    int l1d_plp_num_channel_bonded;
    int l1d_plp_TI_extended_interleaving = 0;
    int l1d_plp_HTI_inter_subframe;
    int l1d_plp_HTI_num_ti_blocks;
    int l1d_mimo_mixed;

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
    value = get_bits(1);
    if (value == 0) {
        printf("L1B_frame_length_mode = time-aligned\n");
    }
    else {
        printf("L1B_frame_length_mode = symbol-aligned\n");
    }
    if (value == 0) {
        value = get_bits(10);
        printf("L1B_frame_length = %ld\n", value);
        value = get_bits(13);
        printf("L1B_excess_samples_per_symbol = %ld\n", value);
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
            case 0:
                printf("L1B_preamble_reduced_carriers = 5.832 MHz\n");
                break;
            case 1:
                printf("L1B_preamble_reduced_carriers = 5.751 MHz\n");
                break;
            case 2:
                printf("L1B_preamble_reduced_carriers = 5.670 MHz\n");
                break;
            case 3:
                printf("L1B_preamble_reduced_carriers = 5.589 MHz\n");
                break;
            case 4:
                printf("L1B_preamble_reduced_carriers = 5.508 MHz\n");
                break;
            case 5:
                printf("L1B_preamble_reduced_carriers = Reserved\n");
                break;
            case 6:
                printf("L1B_preamble_reduced_carriers = Reserved\n");
                break;
            case 7:
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
        case 0:
            printf("L1B_L1_Detail_fec_type = Mode 1\n");
            break;
        case 1:
            printf("L1B_L1_Detail_fec_type = Mode 2\n");
            break;
        case 2:
            printf("L1B_L1_Detail_fec_type = Mode 3\n");
            break;
        case 3:
            printf("L1B_L1_Detail_fec_type = Mode 4\n");
            break;
        case 4:
            printf("L1B_L1_Detail_fec_type = Mode 5\n");
            break;
        case 5:
            printf("L1B_L1_Detail_fec_type = Mode 6\n");
            break;
        case 6:
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
        case 0:
            printf("L1B_first_sub_fft_size = 8K\n");
            break;
        case 1:
            printf("L1B_first_sub_fft_size = 16K\n");
            break;
        case 2:
            printf("L1B_first_sub_fft_size = 32K\n");
            break;
        case 3:
            printf("L1B_first_sub_fft_size = Reserved\n");
            break;
    }
    value = get_bits(3);
    switch (value) {
        case 0:
            printf("L1B_first_sub_reduced_carriers = 5.832 MHz\n");
            break;
        case 1:
            printf("L1B_first_sub_reduced_carriers = 5.751 MHz\n");
            break;
        case 2:
            printf("L1B_first_sub_reduced_carriers = 5.670 MHz\n");
            break;
        case 3:
            printf("L1B_first_sub_reduced_carriers = 5.589 MHz\n");
            break;
        case 4:
            printf("L1B_first_sub_reduced_carriers = 5.508 MHz\n");
            break;
        case 5:
            printf("L1B_first_sub_reduced_carriers = Reserved\n");
            break;
        case 6:
            printf("L1B_first_sub_reduced_carriers = Reserved\n");
            break;
        case 7:
            printf("L1B_first_sub_reduced_carriers = Reserved\n");
            break;
    }
    value = get_bits(4);
    switch (value) {
        case 0:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
        case 1:
            printf("L1B_first_sub_guard_interval = GI1_192\n");
            break;
        case 2:
            printf("L1B_first_sub_guard_interval = GI2_384\n");
            break;
        case 3:
            printf("L1B_first_sub_guard_interval = GI3_512\n");
            break;
        case 4:
            printf("L1B_first_sub_guard_interval = GI4_768\n");
            break;
        case 5:
            printf("L1B_first_sub_guard_interval = GI5_1024\n");
            break;
        case 6:
            printf("L1B_first_sub_guard_interval = GI6_1536\n");
            break;
        case 7:
            printf("L1B_first_sub_guard_interval = GI7_2048\n");
            break;
        case 8:
            printf("L1B_first_sub_guard_interval = GI8_2432\n");
            break;
        case 9:
            printf("L1B_first_sub_guard_interval = GI9_3072\n");
            break;
        case 10:
            printf("L1B_first_sub_guard_interval = GI10_3648\n");
            break;
        case 11:
            printf("L1B_first_sub_guard_interval = GI11_4096\n");
            break;
        case 12:
            printf("L1B_first_sub_guard_interval = GI12_4864\n");
            break;
        case 13:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
        case 14:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
        case 15:
            printf("L1B_first_sub_guard_interval = Reserved\n");
            break;
    }
    value = get_bits(11);
    printf("L1B_first_sub_num_ofdm_symbols = %ld\n", value + 1);
    value = get_bits(5);
    switch (value) {
        case 0:
            printf("L1B_first_sub_scattered_pilot_pattern = SP3_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.175\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.288\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.396\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 1:
            printf("L1B_first_sub_scattered_pilot_pattern = SP3_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.175\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.396\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.660\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 2:
            printf("L1B_first_sub_scattered_pilot_pattern = SP4_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.072\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.274\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.413\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 3:
            printf("L1B_first_sub_scattered_pilot_pattern = SP4_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.274\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.660\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.799\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 4:
            printf("L1B_first_sub_scattered_pilot_pattern = SP6_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.202\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.429\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.585\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.698\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 5:
            printf("L1B_first_sub_scattered_pilot_pattern = SP6_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.413\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.679\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.862\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 6:
            printf("L1B_first_sub_scattered_pilot_pattern = SP8_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.288\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.698\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.841\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 7:
            printf("L1B_first_sub_scattered_pilot_pattern = SP8_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.514\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.799\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.138\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 8:
            printf("L1B_first_sub_scattered_pilot_pattern = SP12_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.445\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.718\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.905\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 9:
            printf("L1B_first_sub_scattered_pilot_pattern = SP12_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.679\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.995\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.213\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.371\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 10:
            printf("L1B_first_sub_scattered_pilot_pattern = SP16_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.549\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.841\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.188\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 11:
            printf("L1B_first_sub_scattered_pilot_pattern = SP16_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.820\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.163\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.399\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.570\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 12:
            printf("L1B_first_sub_scattered_pilot_pattern = SP24_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.718\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.042\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.265\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.427\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 13:
            printf("L1B_first_sub_scattered_pilot_pattern = SP24_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.018\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.399\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.661\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.851\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 14:
            printf("L1B_first_sub_scattered_pilot_pattern = SP32_2\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.862\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.213\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.427\n");
                    break;
                case 4:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.630\n");
                    break;
                default:
                    printf("L1B_first_sub_scattered_pilot_boost = Reserved\n");
                    break;
            }
            break;
        case 15:
            printf("L1B_first_sub_scattered_pilot_pattern = SP32_4\n");
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1B_first_sub_scattered_pilot_boost = 1.000\n");
                    break;
                case 1:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.163\n");
                    break;
                case 2:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.570\n");
                    break;
                case 3:
                    printf("L1B_first_sub_scattered_pilot_boost = 2.851\n");
                    break;
                case 4:
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
                case 0:
                    printf("L1D_fft_size = 8K\n");
                    break;
                case 1:
                    printf("L1D_fft_size = 16K\n");
                    break;
                case 2:
                    printf("L1D_fft_size = 32K\n");
                    break;
                case 3:
                    printf("L1D_fft_size = Reserved\n");
                    break;
            }
            value = get_bits(3);
            switch (value) {
                case 0:
                    printf("L1D_reduced_carriers = 5.832 MHz\n");
                    break;
                case 1:
                    printf("L1D_reduced_carriers = 5.751 MHz\n");
                    break;
                case 2:
                    printf("L1D_reduced_carriers = 5.670 MHz\n");
                    break;
                case 3:
                    printf("L1D_reduced_carriers = 5.589 MHz\n");
                    break;
                case 4:
                    printf("L1D_reduced_carriers = 5.508 MHz\n");
                    break;
                case 5:
                    printf("L1D_reduced_carriers = Reserved\n");
                    break;
                case 6:
                    printf("L1D_reduced_carriers = Reserved\n");
                    break;
                case 7:
                    printf("L1D_reduced_carriers = Reserved\n");
                    break;
            }
            value = get_bits(4);
            switch (value) {
                case 0:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
                case 1:
                    printf("L1D_guard_interval = GI1_192\n");
                    break;
                case 2:
                    printf("L1D_guard_interval = GI2_384\n");
                    break;
                case 3:
                    printf("L1D_guard_interval = GI3_512\n");
                    break;
                case 4:
                    printf("L1D_guard_interval = GI4_768\n");
                    break;
                case 5:
                    printf("L1D_guard_interval = GI5_1024\n");
                    break;
                case 6:
                    printf("L1D_guard_interval = GI6_1536\n");
                    break;
                case 7:
                    printf("L1D_guard_interval = GI7_2048\n");
                    break;
                case 8:
                    printf("L1D_guard_interval = GI8_2432\n");
                    break;
                case 9:
                    printf("L1D_guard_interval = GI9_3072\n");
                    break;
                case 10:
                    printf("L1D_guard_interval = GI10_3648\n");
                    break;
                case 11:
                    printf("L1D_guard_interval = GI11_4096\n");
                    break;
                case 12:
                    printf("L1D_guard_interval = GI12_4864\n");
                    break;
                case 13:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
                case 14:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
                case 15:
                    printf("L1D_guard_interval = Reserved\n");
                    break;
            }
            value = get_bits(11);
            printf("L1D_num_ofdm_symbols = %ld\n", value + 1);
            value = get_bits(5);
            switch (value) {
                case 0:
                    printf("L1D_scattered_pilot_pattern = SP3_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.175\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.288\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.396\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 1:
                    printf("L1D_scattered_pilot_pattern = SP3_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.175\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.396\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.660\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 2:
                    printf("L1D_scattered_pilot_pattern = SP4_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.072\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.274\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.413\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 3:
                    printf("L1D_scattered_pilot_pattern = SP4_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.274\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.660\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.799\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 4:
                    printf("L1D_scattered_pilot_pattern = SP6_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.202\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.429\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.585\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.698\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 5:
                    printf("L1D_scattered_pilot_pattern = SP6_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.413\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.679\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.862\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 6:
                    printf("L1D_scattered_pilot_pattern = SP8_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.288\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.698\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 1.841\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 7:
                    printf("L1D_scattered_pilot_pattern = SP8_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.514\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.799\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.138\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 8:
                    printf("L1D_scattered_pilot_pattern = SP12_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.445\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.718\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 1.905\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 9:
                    printf("L1D_scattered_pilot_pattern = SP12_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.679\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.995\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.213\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.371\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 10:
                    printf("L1D_scattered_pilot_pattern = SP16_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.549\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 1.841\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.188\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 11:
                    printf("L1D_scattered_pilot_pattern = SP16_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.820\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 2.163\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.399\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.570\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 12:
                    printf("L1D_scattered_pilot_pattern = SP24_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.718\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 2.042\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.265\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.427\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 13:
                    printf("L1D_scattered_pilot_pattern = SP24_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 2.018\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 2.399\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.661\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.851\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 14:
                    printf("L1D_scattered_pilot_pattern = SP32_2\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 1.862\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 2.213\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.427\n");
                            break;
                        case 4:
                            printf("L1D_scattered_pilot_boost = 2.630\n");
                            break;
                        default:
                            printf("L1D_scattered_pilot_boost = Reserved\n");
                            break;
                    }
                    break;
                case 15:
                    printf("L1D_scattered_pilot_pattern = SP32_4\n");
                    value = get_bits(3);
                    switch (value) {
                        case 0:
                            printf("L1D_scattered_pilot_boost = 1.000\n");
                            break;
                        case 1:
                            printf("L1D_scattered_pilot_boost = 2.163\n");
                            break;
                        case 2:
                            printf("L1D_scattered_pilot_boost = 2.570\n");
                            break;
                        case 3:
                            printf("L1D_scattered_pilot_boost = 2.851\n");
                            break;
                        case 4:
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
            if (value <= 5) {
                value = get_bits(4);
                switch (value) {
                    case 0:
                        printf("L1D_plp%d_mod = QPSK\n", j);
                        break;
                    case 1:
                        printf("L1D_plp%d_mod = 16QAM\n", j);
                        break;
                    case 2:
                        printf("L1D_plp%d_mod = 64QAM\n", j);
                        break;
                    case 3:
                        printf("L1D_plp%d_mod = 256QAM\n", j);
                        break;
                    case 4:
                        printf("L1D_plp%d_mod = 1024QAM\n", j);
                        break;
                    case 5:
                        printf("L1D_plp%d_mod = 4096QAM\n", j);
                        break;
                    default:
                        printf("L1D_plp%d_mod = Reserved\n", j);
                        break;
                }
                l1d_plp_mod = value;
                value = get_bits(4);
                switch (value) {
                    case 0:
                        printf("L1D_plp%d_cod = 2/15\n", j);
                        break;
                    case 1:
                        printf("L1D_plp%d_cod = 3/15\n", j);
                        break;
                    case 2:
                        printf("L1D_plp%d_cod = 4/15\n", j);
                        break;
                    case 3:
                        printf("L1D_plp%d_cod = 5/15\n", j);
                        break;
                    case 4:
                        printf("L1D_plp%d_cod = 6/15\n", j);
                        break;
                    case 5:
                        printf("L1D_plp%d_cod = 7/15\n", j);
                        break;
                    case 6:
                        printf("L1D_plp%d_cod = 8/15\n", j);
                        break;
                    case 7:
                        printf("L1D_plp%d_cod = 9/15\n", j);
                        break;
                    case 8:
                        printf("L1D_plp%d_cod = 10/15\n", j);
                        break;
                    case 9:
                        printf("L1D_plp%d_cod = 11/15\n", j);
                        break;
                    case 10:
                        printf("L1D_plp%d_cod = 12/15\n", j);
                        break;
                    case 11:
                        printf("L1D_plp%d_cod = 13/15\n", j);
                        break;
                    default:
                        printf("L1D_plp%d_cod = Reserved\n", j);
                        break;
                }
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
