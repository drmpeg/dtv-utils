/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "mpeg4ip_bitstream.h"
#include <math.h>
#include "mp4av_h264.h"

#define H264_START_CODE 0x000001
#define H264_PREVENT_3_BYTE 0x000003


static uint8_t exp_golomb_bits[256] = {
8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 
};

static const uint8_t trailing_bits[9] = 
  { 0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

uint32_t h264_ue (CBitstream *bs)
{
  uint32_t bits, read;
  int bits_left;
  uint8_t coded;
  bool done = false;
  bits = 0;
  // we want to read 8 bits at a time - if we don't have 8 bits, 
  // read what's left, and shift.  The exp_golomb_bits calc remains the
  // same.
  while (done == false) {
    bits_left = bs->bits_remain();
    if (bits_left < 8) {
      read = bs->PeekBits(bits_left) << (8 - bits_left);
      done = true;
    } else {
      read = bs->PeekBits(8);
      if (read == 0) {
	bs->GetBits(8);
	bits += 8;
      } else {
	done = true;
      }
    }
  }
  coded = exp_golomb_bits[read];
  bs->GetBits(coded);
  bits += coded;

  //  printf("ue - bits %d\n", bits);
  return bs->GetBits(bits + 1) - 1;
}

int32_t h264_se (CBitstream *bs) 
{
  uint32_t ret;
  ret = h264_ue(bs);
  if ((ret & 0x1) == 0) {
    ret >>= 1;
    int32_t temp = 0 - ret;
    return temp;
  } 
  return (ret + 1) >> 1;
}

void h264_check_0s (CBitstream *bs, int count)
{
  uint32_t val;
  val = bs->GetBits(count);
  if (val != 0) {
    printf("field error - %d bits should be 0 is %x\n", 
	   count, val);
  }
}

void h264_hrd_parameters (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t cpb_cnt;
  dec->cpb_cnt_minus1 = cpb_cnt = h264_ue(bs);
  uint32_t temp;
  printf("     cpb_cnt_minus1: %u\n", cpb_cnt);
  printf("     bit_rate_scale: %u\n", bs->GetBits(4));
  printf("     cpb_size_scale: %u\n", bs->GetBits(4));
  for (uint32_t ix = 0; ix <= cpb_cnt; ix++) {
    printf("      bit_rate_value_minus1[%u]: %u\n", ix, h264_ue(bs));
    printf("      cpb_size_value_minus1[%u]: %u\n", ix, h264_ue(bs));
    printf("      cbr_flag[%u]: %u\n", ix, bs->GetBits(1));
  }
  temp = dec->initial_cpb_removal_delay_length_minus1 = bs->GetBits(5);
  printf("     initial_cpb_removal_delay_length_minus1: %u\n", temp);

  dec->cpb_removal_delay_length_minus1 = temp = bs->GetBits(5);
  printf("     cpb_removal_delay_length_minus1: %u\n", temp);
  dec->dpb_output_delay_length_minus1 = temp = bs->GetBits(5);
  printf("     dpb_output_delay_length_minus1: %u\n", temp);
  dec->time_offset_length = temp = bs->GetBits(5);  
  printf("     time_offset_length: %u\n", temp);
}

void h264_vui_parameters (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp, temp2;
  temp = bs->GetBits(1);
  printf("    aspect_ratio_info_present_flag: %u\n", temp);
  if (temp) {
    temp = bs->GetBits(8);
    printf("     aspect_ratio_idc:%u\n", temp);
    if (temp == 0xff) { // extended_SAR
      printf("      sar_width: %u\n", bs->GetBits(16));
      printf("      sar_height: %u\n", bs->GetBits(16));
    }
  }
  temp = bs->GetBits(1);
  printf("    overscan_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     overscan_appropriate_flag: %u\n", bs->GetBits(1));
  }
  temp = bs->GetBits(1);
  printf("    video_signal_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     video_format: %u\n", bs->GetBits(3));
    printf("     video_full_range_flag: %u\n", bs->GetBits(1));
    temp = bs->GetBits(1);
    printf("     colour_description_present_flag: %u\n", temp);
    if (temp) {
      printf("      colour_primaries: %u\n", bs->GetBits(8));
      printf("      transfer_characteristics: %u\n", bs->GetBits(8));
      printf("      matrix_coefficients: %u\n", bs->GetBits(8));
    }
  }
    
  temp = bs->GetBits(1);
  printf("    chroma_loc_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     chroma_sample_loc_type_top_field: %u\n", h264_ue(bs));
    printf("     chroma_sample_loc_type_bottom_field: %u\n", h264_ue(bs));
  }
  temp = bs->GetBits(1);
  printf("    timing_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     num_units_in_tick: %u\n", bs->GetBits(32));
    printf("     time_scale: %u\n", bs->GetBits(32));
    printf("     fixed_frame_scale: %u\n", bs->GetBits(1));
  }
  temp = bs->GetBits(1);
  printf("    nal_hrd_parameters_present_flag: %u\n", temp);
  if (temp) {
    dec->NalHrdBpPresentFlag = 1;
    dec->CpbDpbDelaysPresentFlag = 1;
    h264_hrd_parameters(dec, bs);
  }
  temp2 = bs->GetBits(1);
  printf("    vcl_hrd_parameters_present_flag: %u\n", temp2);
  if (temp2) {
    dec->VclHrdBpPresentFlag = 1;
    dec->CpbDpbDelaysPresentFlag = 1;
    h264_hrd_parameters(dec, bs);
  }
  if (temp || temp2) {
    printf("    low_delay_hrd_flag: %u\n", bs->GetBits(1));
  }
  dec->pic_struct_present_flag = temp = bs->GetBits(1);
  printf("    pic_struct_present_flag: %u\n", temp);
  temp = bs->GetBits(1);
  if (temp) {
    printf("    motion_vectors_over_pic_boundaries_flag: %u\n", bs->GetBits(1));
    printf("    max_bytes_per_pic_denom: %u\n", h264_ue(bs));
    printf("    max_bits_per_mb_denom: %u\n", h264_ue(bs));
    printf("    log2_max_mv_length_horizontal: %u\n", h264_ue(bs));
    printf("    log2_max_mv_length_vertical: %u\n", h264_ue(bs));
    printf("    num_reorder_frames: %u\n", h264_ue(bs));
    printf("     max_dec_frame_buffering: %u\n", h264_ue(bs));
  }
}
    
static uint32_t calc_ceil_log2 (uint32_t val)
{
  uint32_t ix, cval;

  ix = 0; cval = 1;
  while (ix < 32) {
    if (cval >= val) return ix;
    cval <<= 1;
    ix++;
  }
  return ix;
}

static void scaling_list (uint ix, uint sizeOfScalingList, CBitstream *bs)
{
  uint lastScale = 8, nextScale = 8;
  uint jx;
  int deltaScale;

  for (jx = 0; jx < sizeOfScalingList; jx++) {
    if (nextScale != 0) {
      deltaScale = h264_se(bs);
      nextScale = (lastScale + deltaScale + 256) % 256;
      printf("     delta: %d\n", deltaScale);
    }
    if (nextScale == 0) {
      lastScale = lastScale;
    } else {
      lastScale = nextScale;
    }
    printf("     scaling list[%u][%u]: %u\n", ix, jx, lastScale);

  }
}

void h264_parse_sequence_parameter_set (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp;
  dec->profile = bs->GetBits(8);
  printf("   profile: %u\n", dec->profile);
  printf("   constaint_set0_flag: %d\n", bs->GetBits(1));
  printf("   constaint_set1_flag: %d\n", bs->GetBits(1));
  printf("   constaint_set2_flag: %d\n", bs->GetBits(1));
  printf("   constaint_set3_flag: %d\n", bs->GetBits(1));
  h264_check_0s(bs, 4);
  printf("   level_idc: %u\n", bs->GetBits(8));
  printf("   seq parameter set id: %u\n", h264_ue(bs));
  if (dec->profile == 100 || dec->profile == 110 ||
      dec->profile == 122 || dec->profile == 144) {
    dec->chroma_format_idc = h264_ue(bs);
    printf("   chroma format idx: %u\n", dec->chroma_format_idc);

    if (dec->chroma_format_idc == 3) {
      dec->residual_colour_transform_flag = bs->GetBits(1);
      printf("    resigual colour transform flag: %u\n", dec->residual_colour_transform_flag);
    }
    dec->bit_depth_luma_minus8 = h264_ue(bs);
    printf("   bit depth luma minus8: %u\n", dec->bit_depth_luma_minus8);
    dec->bit_depth_chroma_minus8 = h264_ue(bs);
    printf("   bit depth chroma minus8: %u\n", dec->bit_depth_luma_minus8);
    dec->qpprime_y_zero_transform_bypass_flag = bs->GetBits(1);
    printf("   Qpprime Y Zero Transform Bypass flag: %u\n", 
	   dec->qpprime_y_zero_transform_bypass_flag);
    dec->seq_scaling_matrix_present_flag = bs->GetBits(1);
    printf("   Seq Scaling Matrix Present Flag: %u\n", 
	   dec->seq_scaling_matrix_present_flag);
    if (dec->seq_scaling_matrix_present_flag) {
      for (uint ix = 0; ix < 8; ix++) {
	temp = bs->GetBits(1);
	printf("   Seq Scaling List[%u] Present Flag: %u\n", ix, temp); 
	if (temp) {
	  scaling_list(ix, ix < 6 ? 16 : 64, bs);
	}
      }
    }
    
  }

  dec->log2_max_frame_num_minus4 = h264_ue(bs);
  printf("   log2_max_frame_num_minus4: %u\n", dec->log2_max_frame_num_minus4);
  dec->pic_order_cnt_type = h264_ue(bs);
  printf("   pic_order_cnt_type: %u\n", dec->pic_order_cnt_type);
  if (dec->pic_order_cnt_type == 0) {
    dec->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(bs);
    printf("    log2_max_pic_order_cnt_lsb_minus4: %u\n", 
	   dec->log2_max_pic_order_cnt_lsb_minus4);
  } else if (dec->pic_order_cnt_type == 1) {
    dec->delta_pic_order_always_zero_flag = bs->GetBits(1);
    printf("    delta_pic_order_always_zero_flag: %u\n", 
	   dec->delta_pic_order_always_zero_flag);
    printf("    offset_for_non_ref_pic: %d\n", h264_se(bs));
    printf("    offset_for_top_to_bottom_field: %d\n", h264_se(bs));
    temp = h264_ue(bs);
    for (uint32_t ix = 0; ix < temp; ix++) {
      printf("      offset_for_ref_frame[%u]: %d\n",
	     ix, h264_se(bs));
    }
  }
  printf("   num_ref_frames: %u\n", h264_ue(bs));
  printf("   gaps_in_frame_num_value_allowed_flag: %u\n", bs->GetBits(1));
  uint32_t PicWidthInMbs = h264_ue(bs) + 1;
    
  printf("   pic_width_in_mbs_minus1: %u (%u)\n", PicWidthInMbs - 1, 
	 PicWidthInMbs * 16);
  uint32_t PicHeightInMapUnits = h264_ue(bs) + 1;

  printf("   pic_height_in_map_minus1: %u\n", 
	 PicHeightInMapUnits - 1);
  dec->frame_mbs_only_flag = bs->GetBits(1);
  printf("   frame_mbs_only_flag: %u\n", dec->frame_mbs_only_flag);
  printf("     derived height: %u\n", (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16);
  if (!dec->frame_mbs_only_flag) {
    printf("    mb_adaptive_frame_field_flag: %u\n", bs->GetBits(1));
  }
  printf("   direct_8x8_inference_flag: %u\n", bs->GetBits(1));
  temp = bs->GetBits(1);
  printf("   frame_cropping_flag: %u\n", temp);
  if (temp) {
    printf("     frame_crop_left_offset: %u\n", h264_ue(bs));
    printf("     frame_crop_right_offset: %u\n", h264_ue(bs));
    printf("     frame_crop_top_offset: %u\n", h264_ue(bs));
    printf("     frame_crop_bottom_offset: %u\n", h264_ue(bs));
  }
  temp = bs->GetBits(1);
  printf("   vui_parameters_present_flag: %u\n", temp);
  if (temp) {
    h264_vui_parameters(dec, bs);
  }
}

static void h264_parse_seq_ext (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp;
  printf("   seq_parameter_set_id: %u\n", h264_ue(bs));
  temp = h264_ue(bs);
  printf("   aux format idc: %u\n", temp);
  if (temp != 0) {
    temp = h264_ue(bs);
    printf("    bit depth aux minus8:%u\n", temp);
    printf("    alpha incr flag:%u\n", bs->GetBits(1));
    printf("    alpha opaque value: %u\n", bs->GetBits(temp + 9));
    printf("    alpha transparent value: %u\n", bs->GetBits(temp + 9));
  }
  printf("   additional extension flag: %u\n", bs->GetBits(1));
}
    
  
void h264_parse_pic_parameter_set (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t num_slice_groups, temp, iGroup;
    printf("   pic_parameter_set_id: %u\n", h264_ue(bs));
    printf("   seq_parameter_set_id: %u\n", h264_ue(bs));
    printf("   entropy_coding_mode_flag: %u\n", bs->GetBits(1));
    dec->pic_order_present_flag = bs->GetBits(1);
    printf("   pic_order_present_flag: %u\n", dec->pic_order_present_flag);
    num_slice_groups = h264_ue(bs);
    printf("   num_slice_groups_minus1: %u\n", num_slice_groups);
    if (num_slice_groups > 0) {
      temp = h264_ue(bs);
      printf("    slice_group_map_type: %u\n", temp);
      if (temp == 0) {
	for (iGroup = 0; iGroup <= num_slice_groups; iGroup++) {
	  printf("     run_length_minus1[%u]: %u\n", iGroup, h264_ue(bs));
	}
      } else if (temp == 2) {
	for (iGroup = 0; iGroup < num_slice_groups; iGroup++) {
	  printf("     top_left[%u]: %u\n", iGroup, h264_ue(bs));
	  printf("     bottom_right[%u]: %u\n", iGroup, h264_ue(bs));
	}
      } else if (temp < 6) { // 3, 4, 5
	printf("     slice_group_change_direction_flag: %u\n", bs->GetBits(1));
	printf("     slice_group_change_rate_minus1: %u\n", h264_ue(bs));
      } else if (temp == 6) {
	temp = h264_ue(bs);
	printf("     pic_size_in_map_units_minus1: %u\n", temp);
	uint32_t bits = calc_ceil_log2(num_slice_groups + 1);
	printf("     bits - %u\n", bits);
	for (iGroup = 0; iGroup <= temp; iGroup++) {
	  printf("      slice_group_id[%u]: %u\n", iGroup, bs->GetBits(bits));
	}
      }
    }
    printf("   num_ref_idx_l0_active_minus1: %u\n", h264_ue(bs));
    printf("   num_ref_idx_l1_active_minus1: %u\n", h264_ue(bs));
    printf("   weighted_pred_flag: %u\n", bs->GetBits(1));
    printf("   weighted_bipred_idc: %u\n", bs->GetBits(2));
    printf("   pic_init_qp_minus26: %d\n", h264_se(bs));
    printf("   pic_init_qs_minus26: %d\n", h264_se(bs));
    printf("   chroma_qp_index_offset: %d\n", h264_se(bs));
    printf("   deblocking_filter_control_present_flag: %u\n", bs->GetBits(1));
    printf("   constrained_intra_pred_flag: %u\n", bs->GetBits(1));
    printf("   redundant_pic_cnt_present_flag: %u\n", bs->GetBits(1));
    int bits = bs->bits_remain();
    if (bits == 0) return;
    if (bits <= 8) {
      uint8_t trail_check = bs->PeekBits(bits);
      if (trail_check == trailing_bits[bits]) return;
    }
    // we have the extensions
    uint8_t transform_8x8_mode_flag = bs->GetBits(1);
    printf("   transform_8x8_mode_flag: %u\n", transform_8x8_mode_flag);
    temp = bs->GetBits(1);
    printf("   pic_scaling_matrix_present_flag: %u\n", temp);
    if (temp) {
      uint max_count = 6 + (2 * transform_8x8_mode_flag);
      for (uint ix = 0; ix < max_count; ix++) {
	temp = bs->GetBits(1);
	printf("   Pic Scaling List[%u] Present Flag: %u\n", ix, temp); 
	if (temp) {
	  scaling_list(ix, ix < 6 ? 16 : 64, bs);
	}
      }
    }
    printf("   second_chroma_qp_index_offset: %u\n", h264_se(bs));
}

static const char *sei[19] = {
  "buffering_period",
  "pic_timing", 
  "pan_scan_rect", 
  "filler_payload",
  "user_data_registered_itu_t_t35",
  "user_data_unregistered",
  "recovery_point",
  "dec_ref_pic_marking_repetition",
  "spare_pic",
  "scene_info",
  "sub_seq_info",
  "sub_seq-layer_characteristics",
  "full_frame_freeze",
  "full_frame_freeze_release",
  "full_frame_snapshot",
  "progressive_refinement_segment_start",
  "progressive_refinement_segment_end",
  "motioned_constrained_slice_group_set",
};

static void h264_parse_sei (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t payload_type;
  uint32_t payload_size, count;
  uint32_t read_val;
  const char *sei_type;
  char *buffer = NULL;
  uint8_t *payload_buffer = NULL;
  uint32_t bufsize = 0;
  bool is_printable;
  uint32_t temp;

  while (bs->bits_remain() >= 16) {
    payload_type = 0;
    while ((read_val = bs->GetBits(8)) == 0xff) {
      payload_type += 255;
    }
    payload_type += read_val;
    payload_size = 0;
    while ((read_val = bs->GetBits(8)) == 0xff) {
      payload_size += 255;
    }
    payload_size += read_val;

    sei_type = payload_type <= 18 ? sei[payload_type] : "unknown value";
    printf("   payload_type: %u %s\n", payload_type, sei_type);
    printf("   payload_size: %u", payload_size);
    if (payload_size + 1 > bufsize) {
      buffer = (char *)realloc(buffer, payload_size + 1);
      payload_buffer = (uint8_t *)realloc(payload_buffer, payload_size + 1);
      bufsize = payload_size + 1;
    }
    uint ix = 0;
    count = payload_size;
    if (payload_size > 8) {
      printf("\n   ");
    }
    is_printable = true;
    while (count > 0) {
      uint8_t bits = bs->GetBits(8);
      payload_buffer[ix] = bits;
      if (isprint(bits)) {
	buffer[ix++] = bits;
      } else {
	buffer[ix++] = '.';
	is_printable = false;
      }
      printf(" 0x%x", bits);
      if ((ix % 8) == 0) printf("\n   ");
      count--;
    }
    printf("\n");
    if (is_printable || payload_type == 4 || payload_type == 5) {
      buffer[ix] = '\0';
      printf("    string is \"%s\"\n", buffer);
    }
    try {
      CBitstream payload_bs;
      payload_bs.init(payload_buffer, payload_size * 8);
      switch (payload_type) {
      case 0: // buffering period
	printf("    seq_parameter_set_id: %u\n", h264_ue(&payload_bs));
	if (dec->NalHrdBpPresentFlag) {
	  for (ix = 0; ix <= dec->cpb_cnt_minus1; ix++) {
	    printf("    initial_cpb_removal_delay[%u]: %u\n", 
		   ix, 
		   payload_bs.GetBits(dec->initial_cpb_removal_delay_length_minus1 + 1));
	    printf("    initial_cpb_removal_delay_offset[%u]: %u\n", 
		   ix, 
		   payload_bs.GetBits(dec->initial_cpb_removal_delay_length_minus1 + 1));
	  }
	    
	}
	if (dec->VclHrdBpPresentFlag) {
	  for (ix = 0; ix <= dec->cpb_cnt_minus1; ix++) {
	    printf("    initial_cpb_removal_delay[%u]: %u\n", 
		   ix, 
		   payload_bs.GetBits(dec->initial_cpb_removal_delay_length_minus1 + 1));
	    printf("    initial_cpb_removal_delay_offset[%u]: %u\n", 
		   ix, 
		   payload_bs.GetBits(dec->initial_cpb_removal_delay_length_minus1 + 1));
	  }
	}
	break;
      case 1: // picture timing
	if (dec->CpbDpbDelaysPresentFlag) {
	  printf("    cpb_removal_delay: %u\n", 
		 payload_bs.GetBits(dec->cpb_removal_delay_length_minus1 + 1));
	  printf("    dpb_output_delay: %u\n", 
		 payload_bs.GetBits(dec->dpb_output_delay_length_minus1 + 1));
	}
	if (dec->pic_struct_present_flag) {
	  temp = payload_bs.GetBits(4);
	  printf("    pict_struct: %u\n", temp);
	  uint NumClockTS = 0;
	  if (temp < 3) NumClockTS = 1;
	  else if (temp < 5 || temp == 7) NumClockTS = 2;
	  else if (temp < 9) NumClockTS = 3;
	  for (ix = 0; ix < NumClockTS; ix++) {
	    temp = payload_bs.GetBits(1);
	    printf("    clock_timestamp_flag[%u]: %u\n", ix, temp);
	    if (temp) {
	      printf("     ct_type: %u\n", payload_bs.GetBits(2));
	      printf("     nuit_field_base_flag: %u\n", payload_bs.GetBits(1));
	      printf("     counting_type: %u\n", payload_bs.GetBits(5));
	      temp = payload_bs.GetBits(1);
	      printf("     full_timestamp_flag: %u\n", temp);
	      printf("     discontinuity_flag: %u\n", payload_bs.GetBits(1));
	      printf("     cnt_dropped_flag: %u\n", payload_bs.GetBits(1));
	      printf("     n_frame: %u\n", payload_bs.GetBits(8));
	      if (temp) {
		printf("     seconds_value: %u\n", payload_bs.GetBits(6));
		printf("     minutes_value: %u\n", payload_bs.GetBits(6));
		printf("     hours_value: %u\n", payload_bs.GetBits(5));
	      } else {
		temp = payload_bs.GetBits(1);
		printf("     seconds_flag: %u\n", temp);
		if (temp) {
		  printf("     seconds_value: %u\n", payload_bs.GetBits(6));
		  temp = payload_bs.GetBits(1);
		  printf("     minutes_flag: %u\n", temp);
		  if (temp) {
		    printf("     minutes_value: %u\n", payload_bs.GetBits(6));
		    temp = payload_bs.GetBits(1);
		    printf("     hours_flag: %u\n", temp);
		    if (temp) {
		      printf("     hours_value: %u\n", payload_bs.GetBits(5));
		    }
		  }
		}
	      }
	      if (dec->time_offset_length > 0) {
		printf("     time_offset: %d\n", 
		       payload_bs.GetBits(dec->time_offset_length));
	      }
	    }
	  }
	}
	break;
      case 2: //pan scan rectangle
	printf("    pan_scan_rect_id: %u\n", h264_ue(&payload_bs));
	temp = payload_bs.GetBits(1);
	printf("    pan_scan_rect_cancel_flag: %u\n", temp);
	if (!temp) {
	  temp = h264_ue(&payload_bs);
	  printf("     pan_scan_cnd_minus1: %u\n", temp);
	  for (ix = 0; ix <= temp; ix++) {
	    printf("      pan_scan_rect_left_offset[%u]: %u\n", 
		   ix,h264_se(&payload_bs));
	    printf("      pan_scan_rect_right_offset[%u]: %u\n", 
		   ix,h264_se(&payload_bs));
	    printf("      pan_scan_rect_top_offset[%u]: %u\n", 
		   ix,h264_se(&payload_bs));
	    printf("      pan_scan_rect_bottom_offset[%u]: %u\n", 
		   ix,h264_se(&payload_bs));
	  }
	  printf("      pan_scan_rect_repitition_period: %u\n", 
		 h264_ue(&payload_bs));
	}
	break;

      case 6: // recover point
	printf("    recovery_frame_cnt: %u\n", h264_ue(&payload_bs));
	printf("    exact_match_flag: %u\n", payload_bs.GetBits(1));
	printf("    broken_link_flag: %u\n", payload_bs.GetBits(1));
	printf("    changing_slice_group_idc: %u\n",  payload_bs.GetBits(2));
	break;
      case 7: // decoded reference picture marking repetition
	printf("    original_idr_flag: %u\n", payload_bs.GetBits(1));
	printf("    original_frame_num: %u\n", h264_ue(&payload_bs));
	if (!dec->frame_mbs_only_flag) {
	  temp = payload_bs.GetBits(1);
	  printf("    original_field_pic_flag: %u\n", temp);
	  if (temp) {
	    printf("     original_bottom_field_flag: %u\n", 
		   payload_bs.GetBits(1));
	  }
	}
	break;
      case 8: { // spare pic 
	uint32_t spare_field_flag;
	printf("    target_frame_num: %u\n", h264_ue(&payload_bs));
	spare_field_flag = payload_bs.GetBits(1);
	printf("    spare_field_flag: %u\n", spare_field_flag);
	if (spare_field_flag) 
	  printf("     target_bottom_field_flag: %u\n", payload_bs.GetBits(1));
	temp = h264_ue(&payload_bs);
	printf("    num_spare_pics_minus1: %u\n", temp);
#if 0
	for (ix = 0; ix <= temp; ix++) {
	  printf("    delta_spare_frame_num[%u]: %u\n", ix, h264_ue(&payload_bs));
	  if (spare_field_flag) {
	    printf("    spare_bottom_field_flag[%u]: %u\n", 
		   ix, payload_bs.GetBits(1));
	  }
	  uint32_t spare_area_idc = h264_ue(&payload_bs);
	  printf("    spare_area_idc[%u]: %u", ix, spare_area_idc);
	  if (spare_area_idc == 1) {
	  }
	}
#endif
	break;
      }
      case 9: // scene information
	temp = payload_bs.GetBits(1);
	printf("    scene_info_present_flag: %u\n", temp);
	if (temp) {
	  printf("     scene_id: %u\n", h264_ue(&payload_bs));
	  temp = h264_ue(&payload_bs);
	  printf("     scene_transition_type: %u\n", temp);
	  if (temp > 3) {
	    printf("      second_scene_id: %u\n", h264_ue(&payload_bs));
	  }
	}
	break;
	
      }
      
    } catch (BitstreamErr_t err) {
      printf("\nERROR reading bitstream %s\n\n", err == BITSTREAM_PAST_END ?
	     "read past payload end" : "too many bits requested");
    }
  }
  CHECK_AND_FREE(buffer);
}

uint32_t h264_find_next_start_code (uint8_t *pBuf, 
				    uint32_t bufLen)
{
  uint32_t val;
  uint32_t offset;

  offset = 0;
  if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 0 && pBuf[3] == 1) {
    pBuf += 4;
    offset = 4;
  } else if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    val |= *pBuf++;
    offset++;
    if (val == H264_START_CODE) {
      return offset - 4;
    }
    if ((val & 0x00ffffff) == H264_START_CODE) {
      return offset - 3;
    }
  }
  return 0;
}

static uint32_t remove_03 (uint8_t *bptr, uint32_t len)
{
  uint32_t nal_len = 0;

  while (nal_len + 2 < len) {
    if (bptr[0] == 0 && bptr[1] == 0 && bptr[2] == 3) {
      bptr += 2;
      nal_len += 2;
      len--;
      memmove(bptr, bptr + 1, len - nal_len);
    } else {
      bptr++;
      nal_len++;
    }
  }
  return len;
}
static const char *nal[] = {
  "Coded slice of non-IDR picture", // 1
  "Coded slice data partition A",   // 2
  "Coded slice data partition B",   // 3
  "Coded slice data partition C",   // 4
  "Coded slice of an IDR picture",  // 5
  "SEI",                            // 6
  "Sequence parameter set",         // 7
  "Picture parameter set",          // 8
  "Access unit delimeter",          // 9
  "End of Sequence",                // 10
  "end of stream",                  // 11
  "filler data",                    // 12
  "Sequence parameter set extension", // 13
};

static const char *nal_unit_type (uint8_t type)
{
  if (type == 0 || type >= 24) {
    return "unspecified";
  }
  if (type < 14) {
    return nal[type - 1];
  }
  return "reserved";
}
const char *slice_type[] = {
  "P", 
  "B", 
  "I", 
  "SP", 
  "SI", 
  "P", 
  "B", 
  "I",
  "SP",
  "SI",
};
void h264_slice_header (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp;
  printf("   first_mb_in_slice: %u\n", h264_ue(bs));
  temp = h264_ue(bs);
  printf("   slice_type: %u (%s)\n", temp, temp < 10 ? slice_type[temp] : "invalid");
  printf("   pic_parameter_set_id: %u\n", h264_ue(bs));
  dec->frame_num = bs->GetBits(dec->log2_max_frame_num_minus4 + 4);
  printf("   frame_num: %u (%u bits)\n", 
	 dec->frame_num, 
	 dec->log2_max_frame_num_minus4 + 4);
  // these are defaults
  dec->field_pic_flag = 0;
  dec->bottom_field_flag = 0;
  dec->delta_pic_order_cnt[0] = 0;
  dec->delta_pic_order_cnt[1] = 0;
  if (!dec->frame_mbs_only_flag) {
    dec->field_pic_flag = bs->GetBits(1);
    printf("   field_pic_flag: %u\n", dec->field_pic_flag);
    if (dec->field_pic_flag) {
      dec->bottom_field_flag = bs->GetBits(1);
      printf("    bottom_field_flag: %u\n", dec->bottom_field_flag);
    }
  }
  if (dec->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
    dec->idr_pic_id = h264_ue(bs);
    printf("   idr_pic_id: %u\n", dec->idr_pic_id);
  }
  switch (dec->pic_order_cnt_type) {
  case 0:
    dec->pic_order_cnt_lsb = bs->GetBits(dec->log2_max_pic_order_cnt_lsb_minus4 + 4);
    printf("   pic_order_cnt_lsb: %u\n", dec->pic_order_cnt_lsb);
    if (dec->pic_order_present_flag && !dec->field_pic_flag) {
      dec->delta_pic_order_cnt_bottom = h264_se(bs);
      printf("   delta_pic_order_cnt_bottom: %d\n", 
	     dec->delta_pic_order_cnt_bottom);
    }
    break;
  case 1:
    if (!dec->delta_pic_order_always_zero_flag) {
      dec->delta_pic_order_cnt[0] = h264_se(bs);
      printf("   delta_pic_order_cnt[0]: %d\n", 
	     dec->delta_pic_order_cnt[0]);
    }
    if (dec->pic_order_present_flag && !dec->field_pic_flag) {
      dec->delta_pic_order_cnt[1] = h264_se(bs);
      printf("   delta_pic_order_cnt[1]: %d\n", 
	     dec->delta_pic_order_cnt[1]);
    }
    break;
  }
  
}    

void h264_slice_layer_without_partitioning (h264_decode_t *dec, 
					    CBitstream *bs)
{
  h264_slice_header(dec, bs);
}
uint8_t h264_parse_nal (h264_decode_t *dec, CBitstream *bs)
{
  uint8_t type = 0;

  try {
    if (bs->GetBits(24) == 0) bs->GetBits(8);
    h264_check_0s(bs, 1);
    dec->nal_ref_idc = bs->GetBits(2);
    dec->nal_unit_type = type = bs->GetBits(5);
    printf(" ref %u type %u %s\n", dec->nal_ref_idc, type, nal_unit_type(type));
    switch (type) {
    case H264_NAL_TYPE_NON_IDR_SLICE:
    case H264_NAL_TYPE_IDR_SLICE:
      h264_slice_layer_without_partitioning(dec, bs);
      break;
    case H264_NAL_TYPE_SEQ_PARAM:
      h264_parse_sequence_parameter_set(dec, bs);
      break;
    case H264_NAL_TYPE_PIC_PARAM:
      h264_parse_pic_parameter_set(dec, bs);
      break;
    case H264_NAL_TYPE_SEI:
      h264_parse_sei(dec, bs);
      break;
    case H264_NAL_TYPE_ACCESS_UNIT:
      printf("   primary_pic_type: %u\n", bs->GetBits(3));
      break;
    case H264_NAL_TYPE_SEQ_EXTENSION:
      h264_parse_seq_ext(dec, bs);
      break;
    }
  } catch (BitstreamErr_t err) {
    printf("\nERROR reading bitstream %s\n\n", err == BITSTREAM_PAST_END ?
	   "read past NAL end" : "too many bits requested");
  }
  return type;
}
  
// false if different picture, true if nal is the same picture
bool compare_boundary (h264_decode_t *prev, h264_decode_t *on)
{
  if (prev->frame_num != on->frame_num) {
    return false;
  }
  if (prev->field_pic_flag != on->field_pic_flag) {
    return false;
  }
  if (prev->nal_ref_idc != on->nal_ref_idc &&
      (prev->nal_ref_idc == 0 ||
       on->nal_ref_idc == 0)) {
    return false;
  }
  
  if (prev->frame_num == on->frame_num &&
      prev->pic_order_cnt_type == on->pic_order_cnt_type) {
    if (prev->pic_order_cnt_type == 0) {
      if (prev->pic_order_cnt_lsb != on->pic_order_cnt_lsb) {
	return false;
      }
      if (prev->delta_pic_order_cnt_bottom != on->delta_pic_order_cnt_bottom) {
	return false;
      }
    } else if (prev->pic_order_cnt_type == 1) {
      if (prev->delta_pic_order_cnt[0] != on->delta_pic_order_cnt[0]) {
	return false;
      }
      if (prev->delta_pic_order_cnt[1] != on->delta_pic_order_cnt[1]) {
	return false;
      }
    }
  }

  if (prev->nal_unit_type == H264_NAL_TYPE_IDR_SLICE &&
      on->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
    if (prev->idr_pic_id != on->idr_pic_id) {
      return false;
    }
  }

  return true;
  
}
int main (int argc, char **argv)
{
#define MAX_BUFFER 65536 * 8
  const char* usageString = 
    "[-version] <file-name>\n";
  const char *ProgName = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "version", 0, 0, 'v' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long(argc, argv, "v",
			 long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case '?':
      fprintf(stderr, "usage: %s %s", ProgName, usageString);
      exit(0);
    case 'v':
      fprintf(stderr, "%s - %s version %s\n", 
	      ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      ProgName, c);
    }
  }

  /* check that we have at least one non-option argument */
  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s", ProgName, usageString);
    exit(1);
  }

  uint8_t buffer[MAX_BUFFER];
  uint32_t buffer_on, buffer_size;
  uint64_t bytes = 0;
  FILE *m_file;
  h264_decode_t dec, prevdec;
  bool have_prevdec = false;
  memset(&dec, 0, sizeof(dec));
#if 0
  uint8_t count = 0;
  // this prints out the 8-bit to # of zero bit array that we use
  // to decode ue(v)
  for (uint32_t ix = 0; ix <= 255; ix++) {
    uint8_t ij;
    uint8_t bit = 0x80;
    for (ij = 0;
	 (bit & ix) == 0 && ij < 8; 
	 ij++, bit >>= 1);
    printf("%d, ", ij);
    count++;
    if (count > 16) {
      printf("\n");
      count = 0;
    }
  }
  printf("\n");
#endif

  fprintf(stdout, "%s - %s version %s\n", 
	  ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
  m_file = fopen(argv[optind], FOPEN_READ_BINARY);

  if (m_file == NULL) {
    fprintf(stderr, "file %s not found\n", *argv);
    exit(-1);
  }

  buffer_on = buffer_size = 0;
  while (!feof(m_file)) {
    bytes += buffer_on;
    if (buffer_on != 0) {
      buffer_on = buffer_size - buffer_on;
      memmove(buffer, &buffer[buffer_size - buffer_on], buffer_on);
    }
    buffer_size = fread(buffer + buffer_on, 
			1, 
			MAX_BUFFER - buffer_on, 
			m_file);
    buffer_size += buffer_on;
    buffer_on = 0;

    bool done = false;
    CBitstream ourbs;
    do {
      uint32_t ret;
      ret = h264_find_next_start_code(buffer + buffer_on, 
				      buffer_size - buffer_on);
      if (ret == 0) {
	done = true;
	if (buffer_on == 0) {
	  fprintf(stderr, "couldn't find start code in buffer from 0\n");
	  exit(-1);
	}
      } else {
	// have a complete NAL from buffer_on to end
	if (ret > 3) {
	  uint32_t nal_len;

	  nal_len = remove_03(buffer + buffer_on, ret);

#if 0
	  printf("Nal length %u start code %u bytes "U64"\n", nal_len, 
		 buffer[buffer_on + 2] == 1 ? 3 : 4, bytes + buffer_on);
#else
	  printf("Nal length %u start code %u bytes \n", nal_len, 
		 buffer[buffer_on + 2] == 1 ? 3 : 4);
#endif
	  ourbs.init(buffer + buffer_on, nal_len * 8);
	  uint8_t type;
	  type = h264_parse_nal(&dec, &ourbs);
	  if (type >= 1 && type <= 5) {
	    if (have_prevdec) {
	      // compare the 2
	      bool bound;
	      bound = compare_boundary(&prevdec, &dec);
	      printf("Nal is %s\n", bound ? "part of last picture" : "new picture");
	    }
	    memcpy(&prevdec, &dec, sizeof(dec));
	    have_prevdec = true;
	  } else if (type >= 9 && type <= 11) {
	    have_prevdec = false; // don't need to check
	  }
	}
#if 0
	printf("buffer on "X64" "X64" %u len %u %02x %02x %02x %02x\n",
	       bytes + buffer_on, 
	       bytes + buffer_on + ret,
	       buffer_on, 
	       ret,
	       buffer[buffer_on],
	       buffer[buffer_on+1],
	       buffer[buffer_on+2],
	       buffer[buffer_on+3]);
#endif
	buffer_on += ret; // buffer_on points to next code
      }
    } while (done == false);
  }
  fclose(m_file);
  return 0;
}
