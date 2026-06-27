/*
H.264 access unit delimiter utility
Should be compiled with the following command line:
gcc -O2 -D_FILE_OFFSET_BITS=64 flags264.c -o flags264
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define    TRUE        1
#define    FALSE       0

unsigned int read_bits(unsigned char **param_ptr, unsigned int num_bits)
{
    int i;
    unsigned char *bit_ptr = *param_ptr;
    unsigned int value = 0;

    for (i = num_bits - 1; i >= 0; i--) {
        value |= *bit_ptr++ << i;
    }
    *param_ptr = bit_ptr;
    return value;
}

unsigned int read_ue(unsigned char **param_ptr)
{
    int b, leadingZeroBits = -1;
    unsigned char *bit_ptr = *param_ptr;
    unsigned int value = 0;

    for (b = 0; !b; leadingZeroBits++) {
        b = read_bits(&bit_ptr, 1);
    }
    value = ((1 << leadingZeroBits) - 1) + read_bits(&bit_ptr, leadingZeroBits);
    *param_ptr = bit_ptr;
    return value;
}

unsigned int next_bits(unsigned char **param_ptr, unsigned int num_bits)
{
    int i;
    unsigned char *bit_ptr = *param_ptr;
    unsigned int value = 0;

    for (i = num_bits - 1; i >= 0; i--) {
        value |= *bit_ptr++ << i;
    }
    return value;
}

int main(int argc, char **argv)
{
    FILE *fp;
    static unsigned char buffer[16384];
    int i, j, bits, length;
    unsigned long long aud_offset, offset = 0;
    unsigned long long prev_aud_offset = 0;
    unsigned int temp;
    int first_sequence_dump = FALSE;
    int first_picture_dump = FALSE;
    int emulation_flag = FALSE;
    unsigned int parse = 0;
    unsigned int parsed = 0;
    unsigned int nal_ref_idc;
    unsigned int sequence_parameter_set_parse = 0;
    unsigned int sequence_parameter_set_index = 0;
    unsigned char sequence_parameter_set[256 * 8];
    unsigned char *sequence_parameter_ptr;
    unsigned int coded_slice_parse = 0;
    unsigned int coded_slice_index = 0;
    unsigned char coded_slice[256 * 8];
    unsigned char *coded_slice_ptr;
    unsigned int sei_parse = 0;
    unsigned int sei_index = 0;
    unsigned char sei[256 * 8];
    unsigned char *sei_ptr;
    unsigned char *last_sei_ptr;
    unsigned int profile_idc;
    unsigned int constraint_set3_flag;
    unsigned int level_idc;
    unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    unsigned int pic_width_in_mbs_minus1;
    unsigned int pic_height_in_map_units_minus1;
    unsigned int frame_mbs_only_flag = 0;
    unsigned int mb_adaptive_frame_field_flag = 0;
    unsigned int aspect_ratio_idc = 0;
    unsigned int num_units_in_tick = 0;
    unsigned int time_scale = 0;
    unsigned int cpb_cnt_minus1 = 0;
    unsigned int bit_rate_scale = 0;
    unsigned int bit_rate_value_minus1;
    unsigned int nal_hrd_parameters_present_flag = 0;
    unsigned int vcl_hrd_parameters_present_flag = 0;
    unsigned int nal_initial_cpb_removal_delay_length_minus1 = 0;
    unsigned int nal_cpb_removal_delay_length_minus1 = 0;
    unsigned int nal_dpb_output_delay_length_minus1 = 0;
    unsigned int vcl_initial_cpb_removal_delay_length_minus1 = 0;
    unsigned int vcl_cpb_removal_delay_length_minus1 = 0;
    unsigned int vcl_dpb_output_delay_length_minus1 = 0;
    unsigned int pic_struct_present_flag = 0;
    unsigned int payloadType;
    unsigned int last_payload_type_byte;
    unsigned int payloadSize;
    unsigned int last_payload_size_byte;
    unsigned int pic_struct;
    unsigned int separate_colour_plane_flag;
    unsigned int pic_order_cnt_type;
    unsigned int log2_max_frame_num_minus4;
    unsigned int log2_max_pic_order_cnt_lsb_minus4;
    unsigned int pic_order_cnt_lsb;
    unsigned int slice_type;
    unsigned int frame_num;
    unsigned int field_pic_flag;
    unsigned int bottom_field_flag;
    unsigned int IdrPicFlag;
    unsigned int video_fields = 0;
    long double frame_rate = 1.0;

    if (argc != 2) {
        fprintf(stderr, "usage: flags264 <infile>\n");
        exit(-1);
    }

    /*--- open binary file (for parsing) ---*/
    fp = fopen(argv[1], "rb");
    if (fp == 0) {
        fprintf(stderr, "Cannot open input file <%s>\n", argv[1]);
        exit(-1);
    }

    while(!feof(fp)) {
        length = fread(&buffer[0], 1, 16384, fp);
        for (i = 0; i < length; i++) {
            parsed = parse;
            parse = (parse << 8) + buffer[i];
            if ((parse & 0xffffff00) == 0x00000100) {
                if (sequence_parameter_set_parse != 0) {
                    sequence_parameter_ptr = &sequence_parameter_set[0];
                    profile_idc = read_bits(&sequence_parameter_ptr, 8);
                    sequence_parameter_ptr += 3;
                    constraint_set3_flag = read_bits(&sequence_parameter_ptr, 1);
                    sequence_parameter_ptr += 4;
                    level_idc = read_bits(&sequence_parameter_ptr, 8);
                    temp = read_ue(&sequence_parameter_ptr);          /* seq_parameter_set_id */
                    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128 || profile_idc == 138 || profile_idc == 139 || profile_idc == 134 || profile_idc == 135) {
                        temp = read_ue(&sequence_parameter_ptr);          /* chroma_format_idc */
                        if (temp == 3) {
                            separate_colour_plane_flag = read_bits(&sequence_parameter_ptr, 1); /* separate_colour_plane_flag */
                        }
                        temp = read_ue(&sequence_parameter_ptr);          /* bit_depth_luma_minus8 */
                        temp = read_ue(&sequence_parameter_ptr);          /* bit_depth_chroma_minus8 */
                        temp = read_bits(&sequence_parameter_ptr, 1);     /* qpprime_y_zero_transform_bypass_flag */
                        temp = read_bits(&sequence_parameter_ptr, 1);     /* seq_scaling_matrix_present_flag */
                        if (temp) {
                            /* fix me */
                        }
                    }
                    log2_max_frame_num_minus4 = read_ue(&sequence_parameter_ptr);   /* log2_max_frame_num_minus4 */
                    pic_order_cnt_type = read_ue(&sequence_parameter_ptr);          /* pic_order_cnt_type */
                    if (pic_order_cnt_type == 0) {
                        log2_max_pic_order_cnt_lsb_minus4 = read_ue(&sequence_parameter_ptr);      /* log2_max_pic_order_cnt_lsb_minus4 */
                    }
                    else if (temp == 1) {
                        temp = read_bits(&sequence_parameter_ptr, 1); /* delta_pic_order_always_zero_flag */
                        temp = read_ue(&sequence_parameter_ptr);      /* offset_for_non_ref_pic */
                        temp = read_ue(&sequence_parameter_ptr);      /* offset_for_top_to_bottom_field */
                        num_ref_frames_in_pic_order_cnt_cycle = read_ue(&sequence_parameter_ptr);
                        for (j = 0; j < num_ref_frames_in_pic_order_cnt_cycle; j++) {
                            temp = read_ue(&sequence_parameter_ptr);  /* offset_for_ref_frame */
                        }
                    }
                    temp = read_ue(&sequence_parameter_ptr);          /* max_num_ref_frames */
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* gaps_in_frame_num_value_allowed_flag */
                    pic_width_in_mbs_minus1 = read_ue(&sequence_parameter_ptr);
                    pic_height_in_map_units_minus1 = read_ue(&sequence_parameter_ptr);
                    frame_mbs_only_flag = read_bits(&sequence_parameter_ptr, 1);
                    if (!frame_mbs_only_flag) {
                        mb_adaptive_frame_field_flag = read_bits(&sequence_parameter_ptr, 1);
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* direct_8x8_inference_flag */
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* frame_cropping_flag */
                    if (temp) {
                        temp = read_ue(&sequence_parameter_ptr);      /* frame_crop_left_offset */
                        temp = read_ue(&sequence_parameter_ptr);      /* frame_crop_right_offset */
                        temp = read_ue(&sequence_parameter_ptr);      /* frame_crop_top_offset */
                        temp = read_ue(&sequence_parameter_ptr);      /* frame_crop_bottom_offset */
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* vui_parameters_present_flag */
                    if (temp) {
                        temp = read_bits(&sequence_parameter_ptr, 1); /* aspect_ratio_info_present_flag */
                        if (temp) {
                            aspect_ratio_idc = read_bits(&sequence_parameter_ptr, 8);
                            if (aspect_ratio_idc == 255) {
                                temp = read_bits(&sequence_parameter_ptr, 16);    /* sar_width */
                                temp = read_bits(&sequence_parameter_ptr, 16);    /* sar_hight */
                            }
                        }
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* overscan_info_present_flag */
                    if (temp) {
                        temp = read_bits(&sequence_parameter_ptr, 1); /* overscan_appropriate_flag */
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* video_signal_type_present_flag */
                    if (temp) {
                        temp = read_bits(&sequence_parameter_ptr, 3); /* video_format */
                        temp = read_bits(&sequence_parameter_ptr, 1); /* video_full_range_flag */
                        temp = read_bits(&sequence_parameter_ptr, 1); /* colour_description_present_flag */
                        if (temp) {
                            temp = read_bits(&sequence_parameter_ptr, 8); /* colour_primaries */
                            temp = read_bits(&sequence_parameter_ptr, 8); /* transfer_characteristics */
                            temp = read_bits(&sequence_parameter_ptr, 8); /* matrix_coefficients */
                        }
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* chroma_loc_info_present_flag */
                    if (temp) {
                        temp = read_ue(&sequence_parameter_ptr);      /* chroma_sample_loc_type_top_field */
                        temp = read_ue(&sequence_parameter_ptr);      /* chroma_sample_loc_type_bottom_field */
                    }
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* timing_info_present_flag */
                    if (temp) {
                        num_units_in_tick = read_bits(&sequence_parameter_ptr, 32); /* num_units_in_tick */
                        time_scale = read_bits(&sequence_parameter_ptr, 32); /* time_scale */
                        temp = read_bits(&sequence_parameter_ptr, 1); /* fixed_frame_rate_flag */
                    }
                    nal_hrd_parameters_present_flag = read_bits(&sequence_parameter_ptr, 1);     /* nal_hrd_parameters_present_flag */
                    if (nal_hrd_parameters_present_flag) {
                        cpb_cnt_minus1 = read_ue(&sequence_parameter_ptr);    /* cpb_cnt_minus1 */
                        bit_rate_scale = read_bits(&sequence_parameter_ptr, 4); /* bit_rate_scale */
                        temp = read_bits(&sequence_parameter_ptr, 4); /* cpb_size_scale */
                        for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                            bit_rate_value_minus1 = read_ue(&sequence_parameter_ptr);  /* bit_rate_value_minus1 */
                            temp = read_ue(&sequence_parameter_ptr);  /* cpb_size_value_minus1 */
                            temp = read_bits(&sequence_parameter_ptr, 1);     /* cbr_flag */
                        }
                        nal_initial_cpb_removal_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        nal_cpb_removal_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        nal_dpb_output_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        temp = read_bits(&sequence_parameter_ptr, 5); /* time_offset_length */
                    }
                    vcl_hrd_parameters_present_flag = read_bits(&sequence_parameter_ptr, 1);     /* vcl_hrd_parameters_present_flag */
                    if (vcl_hrd_parameters_present_flag) {
                        cpb_cnt_minus1 = read_ue(&sequence_parameter_ptr);    /* cpb_cnt_minus1 */
                        temp = read_bits(&sequence_parameter_ptr, 4); /* bit_rate_scale */
                        temp = read_bits(&sequence_parameter_ptr, 4); /* cpb_size_scale */
                        for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                            bit_rate_value_minus1 = read_ue(&sequence_parameter_ptr);  /* bit_rate_value_minus1 */
                            temp = read_ue(&sequence_parameter_ptr);  /* cpb_size_value_minus1 */
                            temp = read_bits(&sequence_parameter_ptr, 1);     /* cbr_flag */
                        }
                        vcl_initial_cpb_removal_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        vcl_cpb_removal_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        vcl_dpb_output_delay_length_minus1 = read_bits(&sequence_parameter_ptr, 5);
                        temp = read_bits(&sequence_parameter_ptr, 5); /* time_offset_length */
                    }
                    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                        temp = read_bits(&sequence_parameter_ptr, 1); /* low_delay_hrd_flag */
                    }
                    pic_struct_present_flag = read_bits(&sequence_parameter_ptr, 1);
                    if (first_sequence_dump == FALSE) {
                        switch (profile_idc) {
                            case 66:
                                printf("Baseline Profile, ");
                                break;
                            case 77:
                                printf("Main Profile, ");
                                break;
                            case 88:
                                printf("Extended Profile, ");
                                break;
                            case 100:
                                printf("High Profile, ");
                                break;
                            case 110:
                                printf("High 10 Profile, ");
                                break;
                            case 122:
                                printf("High 4:2:2 Profile, ");
                                break;
                            case 144:
                                printf("High 4:4:4 Profile, ");
                                break;
                            default:
                                printf("Unknown Profile, \n");
                                break;
                        }
                        if ((level_idc == 11) && (constraint_set3_flag == 1)) {
                            printf("Level = 1.b\n");
                        }
                        else {
                            printf("Level = %d.%d\n", level_idc / 10, (level_idc - ((level_idc / 10) * 10)));
                        }
                        if (frame_mbs_only_flag == 0) {
                            printf("Horizontal Size = %d\n", (pic_width_in_mbs_minus1 + 1) * 16);
                            printf("Vertical Size = %d\n", (pic_height_in_map_units_minus1 + 1) * 32);
                        }
                        else {
                            printf("Horizontal Size = %d\n", (pic_width_in_mbs_minus1 + 1) * 16);
                            printf("Vertical Size = %d\n", (pic_height_in_map_units_minus1 + 1) * 16);
                        }
                        switch (aspect_ratio_idc) {
                            case 0:
                                printf("Aspect ratio = Unspecified\n");
                                break;
                            case 1:
                                printf("Aspect ratio = 1:1 (square)\n");
                                break;
                            case 2:
                                printf("Aspect ratio = 12:11\n");
                                break;
                            case 3:
                                printf("Aspect ratio = 10:11\n");
                                break;
                            case 4:
                                printf("Aspect ratio = 16:11\n");
                                break;
                            case 5:
                                printf("Aspect ratio = 40:33\n");
                                break;
                            case 6:
                                printf("Aspect ratio = 24:11\n");
                                break;
                            case 7:
                                printf("Aspect ratio = 20:11\n");
                                break;
                            case 8:
                                printf("Aspect ratio = 32:11\n");
                                break;
                            case 9:
                                printf("Aspect ratio = 80:33\n");
                                break;
                            case 10:
                                printf("Aspect ratio = 18:11\n");
                                break;
                            case 11:
                                printf("Aspect ratio = 15:11\n");
                                break;
                            case 12:
                                printf("Aspect ratio = 64:33\n");
                                break;
                            case 13:
                                printf("Aspect ratio = 160:99\n");
                                break;
                            case 14:
                                printf("Aspect ratio = 4:3\n");
                                break;
                            case 15:
                                printf("Aspect ratio = 3:2\n");
                                break;
                            case 16:
                                printf("Aspect ratio = 2:1\n");
                                break;
                            case 255:
                                printf("Aspect ratio = Extended_SAR\n");
                                break;
                            default:
                                printf("Aspect ratio = Reserved\n");
                                break;
                        }
                        if (frame_mbs_only_flag == 0) {
                            frame_rate = (long double)time_scale / (long double)num_units_in_tick;
                            printf("Field rate = %2.3f\n", (double)frame_rate);
                        }
                        else {
                            frame_rate = ((long double)time_scale / (long double)num_units_in_tick) / 2.0;
                            printf("Frame rate = %2.3f\n", (double)frame_rate);
                        }
                        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                            printf("Bit rate = %d\n", (bit_rate_value_minus1 + 1) << (6 + bit_rate_scale));
                        }
                    }
                    first_sequence_dump = TRUE;
                    sequence_parameter_set_parse = 0;
                }
                if (coded_slice_parse != 0) {
                    coded_slice_ptr = &coded_slice[0];
                    temp = read_ue(&coded_slice_ptr);             /* first_mb_in_slice */
                    slice_type = read_ue(&coded_slice_ptr);       /* slice_type */
                    temp = read_ue(&coded_slice_ptr);             /* pic_parameter_set_id */
                    if (separate_colour_plane_flag == 1)
                    {
                        temp = read_bits(&coded_slice_ptr, 2);    /* colour_plane_id */
                    }
                    frame_num = read_bits(&coded_slice_ptr, log2_max_frame_num_minus4 + 4);
                    if (!frame_mbs_only_flag)
                    {
                        field_pic_flag = read_bits(&coded_slice_ptr, 1);    /* field_pic_flag */
                        if (field_pic_flag)
                        {
                            bottom_field_flag = read_bits(&coded_slice_ptr, 1);    /* bottom_field_flag */
                        }
                    }
                    if (IdrPicFlag)
                    {
                        temp = read_ue(&coded_slice_ptr);         /* idr_pic_id */
                    }
                    if (pic_order_cnt_type == 0)
                    {
                        pic_order_cnt_lsb = read_bits(&coded_slice_ptr, log2_max_pic_order_cnt_lsb_minus4 + 4);
                    }
                    coded_slice_parse = 0;
                }
                if (sei_parse != 0) {
                    sei_ptr = &sei[0];
                    if ((parsed & 0xff000000) == 0) {
                        sei_index -= 40;
                    }
                    else {
                        sei_index -= 32;
                    }
                    do {
                        payloadType = 0;
                        while (next_bits(&sei_ptr, 8) == 0xff) {
                            temp = read_bits(&sei_ptr, 8);
                            sei_index -= 8;
                            payloadType += 255;
                        }
                        last_payload_type_byte = read_bits(&sei_ptr, 8);
                        sei_index -= 8;
                        payloadType += last_payload_type_byte;
                        payloadSize = 0;
                        while (next_bits(&sei_ptr, 8) == 0xff) {
                            temp = read_bits(&sei_ptr, 8);
                            sei_index -= 8;
                            payloadSize += 255;
                        }
                        last_payload_size_byte = read_bits(&sei_ptr, 8);
                        sei_index -= 8;
                        payloadSize += last_payload_size_byte;
                        payloadSize *= 8;
                        switch (payloadType) {
                            case 0:
                                last_sei_ptr = sei_ptr;
                                temp = read_ue(&sei_ptr);    /* seq_parameter_set_id */
                                sei_index -= sei_ptr - last_sei_ptr;
                                payloadSize -= sei_ptr - last_sei_ptr;
                                if (nal_hrd_parameters_present_flag) {
                                    for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                                        temp = read_bits(&sei_ptr, nal_initial_cpb_removal_delay_length_minus1 + 1);
                                        sei_index -= nal_initial_cpb_removal_delay_length_minus1 + 1;
                                        payloadSize -= nal_initial_cpb_removal_delay_length_minus1 + 1;
                                        temp = read_bits(&sei_ptr, nal_initial_cpb_removal_delay_length_minus1 + 1);
                                        sei_index -= nal_initial_cpb_removal_delay_length_minus1 + 1;
                                        payloadSize -= nal_initial_cpb_removal_delay_length_minus1 + 1;
                                    }
                                }
                                if (vcl_hrd_parameters_present_flag) {
                                    for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                                        temp = read_bits(&sei_ptr, vcl_initial_cpb_removal_delay_length_minus1 + 1);
                                        sei_index -= vcl_initial_cpb_removal_delay_length_minus1 + 1;
                                        payloadSize -= vcl_initial_cpb_removal_delay_length_minus1 + 1;
                                        temp = read_bits(&sei_ptr, vcl_initial_cpb_removal_delay_length_minus1 + 1);
                                        sei_index -= vcl_initial_cpb_removal_delay_length_minus1 + 1;
                                        payloadSize -= vcl_initial_cpb_removal_delay_length_minus1 + 1;
                                    }
                                }
                                temp = read_bits(&sei_ptr, payloadSize);
                                sei_index -= payloadSize;
                                break;
                            case 1:
                                if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                                    temp = read_bits(&sei_ptr, nal_cpb_removal_delay_length_minus1 + 1);
                                    sei_index -= nal_cpb_removal_delay_length_minus1 + 1;
                                    payloadSize -= nal_cpb_removal_delay_length_minus1 + 1;
                                    temp = read_bits(&sei_ptr, nal_dpb_output_delay_length_minus1 + 1);
                                    sei_index -= nal_dpb_output_delay_length_minus1 + 1;
                                    payloadSize -= nal_dpb_output_delay_length_minus1 + 1;
                                }
                                if (pic_struct_present_flag) {
                                    pic_struct = read_bits(&sei_ptr, 4);
                                    if (pic_struct == 0 || pic_struct == 1 || pic_struct == 2) {
                                        video_fields += 1;
                                    }
                                    else if (pic_struct == 3 || pic_struct == 4 || pic_struct == 7) {
                                        video_fields += 2;
                                    }
                                    else if (pic_struct == 5 || pic_struct == 6 || pic_struct == 8) {
                                        video_fields += 3;
                                    }
                                    sei_index -= 4;
                                    payloadSize -= 4;
                                }
                                temp = read_bits(&sei_ptr, payloadSize);
                                sei_index -= payloadSize;
                                break;
                            case 4:
                                temp = read_bits(&sei_ptr, 8);     /* itu_t_t35_country_code */
                                if (temp != 0xff) {
                                    j = 1;
                                }
                                else {
                                    temp = read_bits(&sei_ptr, 8);
                                    j = 2;
                                }
                                do {
                                    temp = read_bits(&sei_ptr, 8);
                                    j++;
                                } while (j < payloadSize / 8);
                                sei_index -= j * 8;
                                payloadSize -= j * 8;
                                temp = read_bits(&sei_ptr, payloadSize);
                                sei_index -= payloadSize;
                                break;
                            case 5:
                                temp = read_bits(&sei_ptr, 32);    /* uuid_iso_iec_11578 */
                                temp = read_bits(&sei_ptr, 32);
                                temp = read_bits(&sei_ptr, 32);
                                temp = read_bits(&sei_ptr, 32);
                                for (j = 16; j < payloadSize / 8; j++) {
                                    temp = read_bits(&sei_ptr, 8);
                                }
                                sei_index -= payloadSize;
                                break;
                            case 6:
                                last_sei_ptr = sei_ptr;
                                temp = read_ue(&sei_ptr);          /* recovery_frame_cnt */
                                sei_index -= sei_ptr - last_sei_ptr;
                                payloadSize -= sei_ptr - last_sei_ptr;
                                temp = read_bits(&sei_ptr, 4);
                                sei_index -= 4;
                                payloadSize -= 4;
                                temp = read_bits(&sei_ptr, payloadSize);
                                sei_index -= payloadSize;
                                break;
                            default:
                                temp = read_bits(&sei_ptr, payloadSize);
                                sei_index -= payloadSize;
                                break;
                        }
                    } while (sei_index);
                    sei_parse = 0;
                }
            }
            if (parse == 0x00000127 || parse == 0x00000147 || parse == 0x00000167) {
                sequence_parameter_set_parse = 256;
                sequence_parameter_set_index = 0;
            }
            else if (sequence_parameter_set_parse != 0) {
                --sequence_parameter_set_parse;
                if ((parse & 0xffffff) == 0x000003) {
                    emulation_flag = TRUE;
                }
                if ((parse == 0x00000300 || parse == 0x00000301 || parse == 0x00000302 || parse == 0x00000303) && (emulation_flag == TRUE)) {
                    sequence_parameter_set_index -= 8;
                    emulation_flag = FALSE;
                }
                for (bits = 7; bits >= 0; bits--) {
                    sequence_parameter_set[sequence_parameter_set_index++] = (parse >> bits) & 0x1;
                }
            }
            else if (parse == 0x00000106 && first_sequence_dump == TRUE) {
                sei_parse = 256;
                sei_index = 0;
            }
            else if (sei_parse != 0) {
                --sei_parse;
                if ((parse & 0xffffff) == 0x000003) {
                    emulation_flag = TRUE;
                }
                if ((parse == 0x00000300 || parse == 0x00000301 || parse == 0x00000302 || parse == 0x00000303) && (emulation_flag == TRUE)) {
                    sei_index -= 8;
                    emulation_flag = FALSE;
                }
                for (bits = 7; bits >= 0; bits--) {
                    sei[sei_index++] = (parse >> bits) & 0x1;
                }
            }
            else if (parse == 0x00000101 || parse == 0x00000121 || parse == 0x00000141 || parse == 0x00000161 || parse == 0x00000125 || parse == 0x00000145 || parse == 0x00000165) {
                nal_ref_idc = (parse & 0x60) >> 5;
                IdrPicFlag = (((parse & 0x1f) == 5) ? 1 : 0);
                coded_slice_parse = 256;
                coded_slice_index = 0;
            }
            else if (coded_slice_parse != 0) {
                --coded_slice_parse;
                if (coded_slice_parse == 0) {
                      coded_slice_parse = 1;
                }
                else {
                    if ((parse & 0xffffff) == 0x000003) {
                        emulation_flag = TRUE;
                    }
                    if ((parse == 0x00000300 || parse == 0x00000301 || parse == 0x00000302 || parse == 0x00000303) && (emulation_flag == TRUE)) {
                        coded_slice_index -= 8;
                        emulation_flag = FALSE;
                    }
                    for (bits = 7; bits >= 0; bits--) {
                        coded_slice[coded_slice_index++] = (parse >> bits) & 0x1;
                    }
                }
            }
            else if (parse == 0x00000109) {
                if ((parsed & 0xff000000) == 0) {
                    aud_offset = offset - 4;
                }
                else {
                    aud_offset = offset - 3;
                }
                if (first_picture_dump == FALSE) {
                    first_picture_dump = TRUE;
                }
                else {
                    switch (slice_type) {
                        case 0:
                        case 5:
                            if (frame_mbs_only_flag == 0) {
                                if (field_pic_flag) {
                                    if (bottom_field_flag) {
                                        printf("P bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                    else {
                                        printf("P top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                }
                                else {
                                    switch (pic_struct) {
                                        case 3:
                                            printf("P tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 4:
                                            printf("P bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 5:
                                            printf("P tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 6:
                                            printf("P btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                    }
                                }
                            }
                            else {
                                printf("P frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                            }
                            break;
                        case 1:
                        case 6:
                            if (frame_mbs_only_flag == 0) {
                                if (field_pic_flag) {
                                    if (bottom_field_flag) {
                                        if (nal_ref_idc == 0) {
                                            printf("b bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                        else {
                                            printf("B bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                    }
                                    else {
                                        if (nal_ref_idc == 0) {
                                            printf("b top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                        else {
                                            printf("B top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                    }
                                }
                                else {
                                    if (nal_ref_idc == 0) {
                                        switch (pic_struct) {
                                            case 3:
                                                printf("b tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 4:
                                                printf("b bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 5:
                                                printf("b tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 6:
                                                printf("b btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                        }
                                    }
                                    else {
                                        switch (pic_struct) {
                                            case 3:
                                                printf("B tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 4:
                                                printf("B bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 5:
                                                printf("B tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 6:
                                                printf("B btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                        }
                                    }
                                }
                            }
                            else {
                                if (nal_ref_idc == 0) {
                                    printf("b frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                }
                                else {
                                    printf("B frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                }
                            }
                            break;
                        case 2:
                        case 7:
                            if (frame_mbs_only_flag == 0) {
                                if (field_pic_flag) {
                                    if (bottom_field_flag) {
                                        if (IdrPicFlag) {
                                            printf("IDR bot field POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                        else {
                                            printf("I bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                    }
                                    else {
                                        if (IdrPicFlag) {
                                            printf("IDR top field POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                        else {
                                            printf("I top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                        }
                                    }
                                }
                                else {
                                    if (IdrPicFlag) {
                                        switch (pic_struct) {
                                            case 3:
                                                printf("IDR tb  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 4:
                                                printf("IDR bt  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 5:
                                                printf("IDR tbt frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 6:
                                                printf("IDR btb frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                        }
                                    }
                                    else {
                                        switch (pic_struct) {
                                            case 3:
                                                printf("I tb  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 4:
                                                printf("I bt  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 5:
                                                printf("I tbt frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                            case 6:
                                                printf("I btb frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                                break;
                                        }
                                    }
                                }
                            }
                            else {
                                if (IdrPicFlag) {
                                    printf("IDR frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                }
                                else {
                                    printf("I frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                }
                            }
                            break;
                        case 3:
                        case 8:
                            if (frame_mbs_only_flag == 0) {
                                if (field_pic_flag) {
                                    if (bottom_field_flag) {
                                        printf("SP bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                    else {
                                        printf("SP top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                }
                                else {
                                    switch (pic_struct) {
                                        case 3:
                                            printf("SP tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 4:
                                            printf("SP bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 5:
                                            printf("SP tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 6:
                                            printf("SP btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                    }
                                }
                            }
                            else {
                                printf("SP frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                            }
                            break;
                        case 4:
                        case 9:
                            if (frame_mbs_only_flag == 0) {
                                if (field_pic_flag) {
                                    if (bottom_field_flag) {
                                        printf("SI bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                    else {
                                        printf("SI top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                    }
                                }
                                else {
                                    switch (pic_struct) {
                                        case 3:
                                            printf("SI tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 4:
                                            printf("SI bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 5:
                                            printf("SI tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                        case 6:
                                            printf("SI btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                                            break;
                                    }
                                }
                            }
                            else {
                                printf("SI frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (aud_offset - prev_aud_offset) * 8);
                            }
                            break;
                    }
                    prev_aud_offset = aud_offset;
                }
            }
            offset++;
        }
    }
    if (coded_slice_parse != 0) {
        coded_slice_ptr = &coded_slice[0];
        temp = read_ue(&coded_slice_ptr);             /* first_mb_in_slice */
        slice_type = read_ue(&coded_slice_ptr);       /* slice_type */
        temp = read_ue(&coded_slice_ptr);             /* pic_parameter_set_id */
        if (separate_colour_plane_flag == 1)
        {
            temp = read_bits(&coded_slice_ptr, 2);    /* colour_plane_id */
        }
        frame_num = read_bits(&coded_slice_ptr, log2_max_frame_num_minus4 + 4);
        if (!frame_mbs_only_flag)
        {
            field_pic_flag = read_bits(&coded_slice_ptr, 1);    /* field_pic_flag */
            if (field_pic_flag)
            {
                bottom_field_flag = read_bits(&coded_slice_ptr, 1);    /* bottom_field_flag */
            }
        }
        if (IdrPicFlag)
        {
            temp = read_ue(&coded_slice_ptr);         /* idr_pic_id */
        }
        if (pic_order_cnt_type == 0)
        {
            pic_order_cnt_lsb = read_bits(&coded_slice_ptr, log2_max_pic_order_cnt_lsb_minus4 + 4);
        }
        coded_slice_parse = 0;
    }
    switch (slice_type) {
        case 0:
        case 5:
            if (frame_mbs_only_flag == 0) {
                if (field_pic_flag) {
                    if (bottom_field_flag) {
                        printf("P bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                    else {
                        printf("P top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                }
                else {
                    switch (pic_struct) {
                        case 3:
                            printf("P tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 4:
                            printf("P bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 5:
                            printf("P tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 6:
                            printf("P btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                    }
                }
            }
            else {
                printf("P frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
            }
            break;
        case 1:
        case 6:
            if (frame_mbs_only_flag == 0) {
                if (field_pic_flag) {
                    if (bottom_field_flag) {
                        if (nal_ref_idc == 0) {
                            printf("b bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                        else {
                            printf("B bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                    }
                    else {
                        if (nal_ref_idc == 0) {
                            printf("b top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                        else {
                            printf("B top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                    }
                }
                else {
                    if (nal_ref_idc == 0) {
                        switch (pic_struct) {
                            case 3:
                                printf("b tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 4:
                                printf("b bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 5:
                                printf("b tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 6:
                                printf("b btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                        }
                    }
                    else {
                        switch (pic_struct) {
                            case 3:
                                printf("B tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 4:
                                printf("B bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 5:
                                printf("B tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 6:
                                printf("B btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                        }
                    }
                }
            }
            else {
                if (nal_ref_idc == 0) {
                    printf("b frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                }
                else {
                    printf("B frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                }
            }
            break;
        case 2:
        case 7:
            if (frame_mbs_only_flag == 0) {
                if (field_pic_flag) {
                    if (bottom_field_flag) {
                        if (IdrPicFlag) {
                            printf("IDR bot field POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                        else {
                            printf("I bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                    }
                    else {
                        if (IdrPicFlag) {
                            printf("IDR top field POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                        else {
                            printf("I top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                        }
                    }
                }
                else {
                    if (IdrPicFlag) {
                        switch (pic_struct) {
                            case 3:
                                printf("IDR tb  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 4:
                                printf("IDR bt  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 5:
                                printf("IDR tbt frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 6:
                                printf("IDR btb frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                        }
                    }
                    else {
                        switch (pic_struct) {
                            case 3:
                                printf("I tb  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 4:
                                printf("I bt  frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 5:
                                printf("I tbt frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                            case 6:
                                printf("I btb frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                                break;
                        }
                    }
                }
            }
            else {
                if (IdrPicFlag) {
                    printf("IDR frame POC = %d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                }
                else {
                    printf("I frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                }
            }
            break;
        case 3:
        case 8:
            if (frame_mbs_only_flag == 0) {
                if (field_pic_flag) {
                    if (bottom_field_flag) {
                        printf("SP bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                    else {
                        printf("SP top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                }
                else {
                    switch (pic_struct) {
                        case 3:
                            printf("SP tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 4:
                            printf("SP bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 5:
                            printf("SP tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 6:
                            printf("SP btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                    }
                }
            }
            else {
                printf("SP frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
            }
            break;
        case 4:
        case 9:
            if (frame_mbs_only_flag == 0) {
                if (field_pic_flag) {
                    if (bottom_field_flag) {
                        printf("SI bot field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                    else {
                        printf("SI top field POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                    }
                }
                else {
                    switch (pic_struct) {
                        case 3:
                            printf("SI tb  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 4:
                            printf("SI bt  frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 5:
                            printf("SI tbt frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                        case 6:
                            printf("SI btb frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
                            break;
                    }
                }
            }
            else {
                printf("SI frame POC = %3d, Pic# = %3d, position = %llu, bits = %llu\n", pic_order_cnt_lsb, frame_num, prev_aud_offset, (offset - prev_aud_offset) * 8);
            }
            break;
    }
    fclose(fp);
    return 0;
}
