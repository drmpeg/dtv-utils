/*
Transport Stream Demultiplexer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE        1
#define FALSE       0

#define VIDEO       0
#define AUDIO       1
#define PADDING     2

#define I           0
#define P           1
#define B           2
#define BI          3
#define SKIPPED     4

#define SEQUENCE_HEADER_CODE 0x000001b3

#define OPTION_CHAR    '-'

void demux_mpeg2_transport_init(void);
void demux_mpeg2_transport(unsigned int, unsigned char *);

FILE *fpoutvideo, *fpoutaudio;
unsigned int program = 1;
unsigned int video_channel = 1;
unsigned int audio_channel = 1;
unsigned int pid_counter[0x2000];
unsigned long long packet_counter = 0;
unsigned long long pid_first_packet[0x2000];
unsigned long long pid_last_packet[0x2000];
unsigned long long pts_aligned = 0xffffffffffffffffLL;
unsigned int parse_only = FALSE;
unsigned int dump_audio_pts = FALSE;
unsigned int dump_video_pts = FALSE;
unsigned int timecode_mode = FALSE;
unsigned int dump_pids = FALSE;
unsigned int suppress_tsrate = FALSE;
unsigned int pes_streams = FALSE;
unsigned int dump_psip = FALSE;
unsigned int hdmv_mode = FALSE;
unsigned int dump_extra = FALSE;
unsigned int dump_pcr = FALSE;
unsigned int lpcm_mode = FALSE;
unsigned int force_mode = FALSE;
unsigned int dump_index = FALSE;
unsigned int running_average_bitrate = 0;
unsigned int running_average_bitrate_peak = 0;
unsigned int coded_frames = 0;
unsigned int video_fields = 0;
unsigned int video_progressive = 0;
unsigned long long last_video_pts = 0;
unsigned long long last_audio_pts = 0;
unsigned long long last_video_pts_diff = 0;
unsigned long long last_audio_pts_diff = 0;
unsigned short pcr_pid = 0xffff;
unsigned short video_pid = 0xffff;
unsigned short audio_pid = 0xffff;
unsigned char audio_stream_type;
unsigned char video_stream_type;

int main(int argc, char **argv)
{
    FILE *fp;
    static unsigned char buffer[16384];
    unsigned int i, length;
    int temp;
    char videofilename[] = {"bits0001.mpv"};
    char audiofilename[] = {"bits0001.mpa"};

    if (argc != 5 && argc != 6) {
        fprintf(stderr, "xport Transport Stream Demuxer 1.1\n\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\txport <-pavtdszgher2fi> <infile> <program number> <video stream number> <audio stream number>\n\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\tp = parse only, do not demux to video and audio files\n");
        fprintf(stderr, "\ta = dump audio PTS\n");
        fprintf(stderr, "\tv = dump video PTS\n");
        fprintf(stderr, "\tt = GOP timecode mode, count repeated fields/frames\n");
        fprintf(stderr, "\td = dump all PID's (useful for debugging muxers, but tons of output)\n");
        fprintf(stderr, "\ts = suppress TS rate dumping (useful when piping output to a file)\n");
        fprintf(stderr, "\tz = demux to PES streams (instead of elementary streams)\n");
        fprintf(stderr, "\tg = dump ATSC PSIP information\n");
        fprintf(stderr, "\th = input file is in HDMV (AVCHD and Blu-ray) format (192 byte packets)\n");
        fprintf(stderr, "\te = dump HDMV arrival_time_stamp difference\n");
        fprintf(stderr, "\tr = dump PCR\n");
        fprintf(stderr, "\t2 = only extract 2 channels of HDMV LPCM audio from multi-channel tracks\n");
        fprintf(stderr, "\tf = force PID's and video stream type\n");
        fprintf(stderr, "\ti = dump index info\n");
        exit(-1);
    }

    if (argc == 5) {
        /*--- open binary file (for parsing) ---*/
        fp = fopen(argv[1], "rb");
        if (fp == 0) {
            fprintf(stderr, "Cannot open bitstream file <%s>\n", argv[1]);
            exit(-1);
        }
    }
    else {
        if (*argv[1] == OPTION_CHAR) {
            for (i = 1; i < strlen(argv[1]); i++) {
                switch (argv[1][i]) {
                    case 'p':
                    case 'P':
                        parse_only = TRUE;
                        break;
                    case 'a':
                    case 'A':
                        dump_audio_pts = TRUE;
                        break;
                    case 'v':
                    case 'V':
                        dump_video_pts = TRUE;
                        break;
                    case 't':
                    case 'T':
                        timecode_mode = TRUE;
                        break;
                    case 'd':
                    case 'D':
                        dump_pids = TRUE;
                        break;
                    case 's':
                    case 'S':
                        suppress_tsrate = TRUE;
                        break;
                    case 'z':
                    case 'Z':
                        pes_streams = TRUE;
                        break;
                    case 'g':
                    case 'G':
                        dump_psip = TRUE;
                        break;
                    case 'h':
                    case 'H':
                        hdmv_mode = TRUE;
                        break;
                    case 'e':
                    case 'E':
                        dump_extra = TRUE;
                        break;
                    case 'r':
                    case 'R':
                        dump_pcr = TRUE;
                        break;
                    case '2':
                        lpcm_mode = TRUE;
                        break;
                    case 'f':
                    case 'F':
                        force_mode = TRUE;
                        break;
                    case 'i':
                    case 'I':
                        dump_index = TRUE;
                        break;
                    default:
                        fprintf(stderr, "Unsupported Option: %c\n", argv[1][i]);
                }
            }
        }
        else {
            fprintf(stderr, "xport Transport Stream Demuxer 1.1\n\n");
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "\txport <-pavtdszgher2fi> <infile> <program number> <video stream number> <audio stream number>\n\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "\tp = parse only, do not demux to video and audio files\n");
            fprintf(stderr, "\ta = dump audio PTS\n");
            fprintf(stderr, "\tv = dump video PTS\n");
            fprintf(stderr, "\tt = GOP timecode mode, count repeated fields/frames\n");
            fprintf(stderr, "\td = dump all PID's (useful for debugging muxers, but tons of output)\n");
            fprintf(stderr, "\ts = suppress TS rate dumping (useful when piping output to a file)\n");
            fprintf(stderr, "\tz = demux to PES streams (instead of elementary streams)\n");
            fprintf(stderr, "\tg = dump ATSC PSIP information\n");
            fprintf(stderr, "\th = input file is in HDMV (AVCHD and Blu-ray) format (192 byte packets)\n");
            fprintf(stderr, "\te = dump HDMV arrival_time_stamp difference\n");
            fprintf(stderr, "\tr = dump PCR\n");
            fprintf(stderr, "\t2 = only extract 2 channels of HDMV LPCM audio from multi-channel tracks\n");
            fprintf(stderr, "\tf = force PID's and video stream type\n");
            fprintf(stderr, "\ti = dump index info\n");
            exit(-1);
        }
        /*--- open binary file (for parsing) ---*/
        fp = fopen(argv[2], "rb");
        if (fp == 0) {
            fprintf(stderr, "Cannot open bitstream file <%s>\n", argv[2]);
            exit(-1);
        }
    }

    if (parse_only == FALSE) {
        /*--- open binary file (for video output) ---*/
        fpoutvideo = fopen(&videofilename[0], "wb");
        if (fpoutvideo == 0) {
            fprintf(stderr, "Cannot open video output file <%s>\n", &videofilename[0]);
            exit(-1);
        }

        /*--- open binary file (for audio output) ---*/
        fpoutaudio = fopen(&audiofilename[0], "wb");
        if (fpoutaudio == 0) {
            fprintf(stderr, "Cannot open audio output file <%s>\n", &audiofilename[0]);
            exit(-1);
        }
    }

    if (argc == 5) {
        program = atoi(argv[2]);
        video_channel = atoi(argv[3]);
        audio_channel = atoi(argv[4]);
    }
    else {
        if (force_mode == TRUE) {
            video_pid = (unsigned short)strtoul(argv[3], NULL, 16);
            if (video_pid == 0) {
                video_channel = 0;
            }
            audio_pid = (unsigned short)strtoul(argv[4], NULL, 16);
            video_stream_type = (unsigned char)strtoul(argv[5], NULL, 16);
            audio_stream_type = 0x81;
            pcr_pid = video_pid;
        }
        else {
            program = atoi(argv[3]);
            video_channel = atoi(argv[4]);
            audio_channel = atoi(argv[5]);
        }
    }
    printf("xport Transport Stream Demuxer 1.1\n");
    printf("program = %d, video channel = %d, audio channel = %d\n", program, video_channel, audio_channel);
    demux_mpeg2_transport_init();

    while (!feof(fp)) {
        length = fread(&buffer[0], 1, 16384, fp);
        demux_mpeg2_transport(length, &buffer[0]);
    }
    printf("\n");
    for (i = 0; i < 0x2000; i++) {
        if (pid_counter[i] != 0) {
            printf("packets for pid %4d <0x%04x> = %d, first = %lld, last = %lld\n", i, i, pid_counter[i], pid_first_packet[i], pid_last_packet[i]); 
        }
    }
    if (video_progressive == 0) {
        printf("coded pictures = %d, video fields = %d\n", coded_frames, video_fields);
    }
    else {
        printf("coded pictures = %d, video frames = %d\n", coded_frames, video_fields);
    }
    temp = (int)((last_audio_pts + last_audio_pts_diff) - (last_video_pts + last_video_pts_diff));
    printf("Ending audio to video PTS difference = %d ticks, %f milliseconds\n", temp, (double)temp / 90.0);
    fclose(fp);
    if (parse_only == FALSE) {
        fclose(fpoutvideo);
        fclose(fpoutaudio);
    }
    return 0;
}

void parse_ac3_audio(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int first_access_unit)
{
    unsigned int i, j;
    static int first_header = FALSE;
    static int second_header = FALSE;
    static int audio_synced = FALSE;
    static int first_synced = FALSE;
    static unsigned int parse = 0;
    static unsigned int header_parse = 0;
    static unsigned int frame_size = 0;
    static unsigned int frame_size_check = 0;
    static unsigned char frame_buffer[128][3840 + 8];
    static unsigned char frame_buffer_start = 0x0b;
    static unsigned int frame_buffer_index = 0;
    static unsigned int frame_buffer_count = 0;
    static unsigned int frame_buffer_length[128];
    static unsigned long long frame_buffer_pts[128];
    static unsigned long long current_pts;
    static unsigned long long current_pts_saved;
    static unsigned int current_pts_valid = FALSE;
    static unsigned int audio_sampling_rate;
    static unsigned int audio_bitrate;
    static unsigned int audio_bsid;
    static unsigned int audio_bsmod;
    static unsigned int audio_acmod;

    if (parse_only == FALSE) {
        if (audio_synced == TRUE) {
            fwrite(es_ptr, 1, length, fpoutaudio);
        }
    }
    if (audio_synced == FALSE) {
        if (first_access_unit == TRUE) {
            current_pts_saved = pts;
            current_pts_valid = TRUE;
        }
        for (i = 0; i < length; i++) {
            parse = (parse << 8) + *es_ptr++;
            if ((parse & 0xffff) == 0x00000b77) {
                if (current_pts_valid == TRUE) {
                    current_pts = current_pts_saved;
                    current_pts_valid = FALSE;
                }
                else {
                    if (frame_size_check != 0) {
                        if (frame_buffer_index == frame_size_check) {
                            current_pts += ((1536 * 90000) / audio_sampling_rate);
                        }
                    }
                }
                if (first_header == FALSE) {
                    header_parse = 5;
                    first_header = TRUE;
                    frame_buffer_pts[frame_buffer_count] = current_pts;
                }
                else if (second_header == FALSE) {
                    if (frame_size == 5) {
                        second_header = TRUE;
                        printf("Audio Bitrate = %d, Audio Sampling Rate = %d\n", audio_bitrate, audio_sampling_rate);
                        switch (audio_acmod & 0x7) {
                            case 7:
                                printf("Audio Mode = 3/2, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 6:
                                printf("Audio Mode = 2/2, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 5:
                                printf("Audio Mode = 3/1, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 4:
                                printf("Audio Mode = 2/1, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 3:
                                printf("Audio Mode = 3/0, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 2:
                                printf("Audio Mode = 2/0, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 1:
                                printf("Audio Mode = 1/0, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                            case 0:
                                printf("Audio Mode = 1+1, bsid = %d, bsmod = %d\n", audio_bsid, audio_bsmod);
                                break;
                        }
                        if (audio_synced == FALSE) {
                            frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                            frame_buffer_index = 0;
                            frame_buffer_count++;
                            frame_buffer_count &= 0x7f;
                            frame_buffer_pts[frame_buffer_count] = current_pts;
                        }
                    }
                    else {
                        printf("sync word mismatch!\n");
                        first_header = FALSE;
                        frame_buffer_count = 0;
                        frame_buffer_index = 0;
                    }
                }
                else {
                    if (audio_synced == FALSE) {
                        if (frame_buffer_index == frame_size_check) {
                            frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                            frame_buffer_index = 0;
                            frame_buffer_count++;
                            frame_buffer_count &= 0x7f;
                            frame_buffer_pts[frame_buffer_count] = current_pts;
                        }
                    }
                }
            }
            else if (header_parse != 0) {
                --header_parse;
                if (header_parse == 2) {
                    switch ((parse & 0xc0) >> 6) {
                        case 3:
                            audio_sampling_rate = 0;
                            break;
                        case 2:
                            audio_sampling_rate = 32000;
                            break;
                        case 1:
                            audio_sampling_rate = 44100;
                            break;
                        case 0:
                            audio_sampling_rate = 48000;
                            break;
                    }
                    switch ((parse & 0x3f) >> 1) {
                        case 18:
                            audio_bitrate = 640000;
                            break;
                        case 17:
                            audio_bitrate = 576000;
                            break;
                        case 16:
                            audio_bitrate = 512000;
                            break;
                        case 15:
                            audio_bitrate = 448000;
                            break;
                        case 14:
                            audio_bitrate = 384000;
                            break;
                        case 13:
                            audio_bitrate = 320000;
                            break;
                        case 12:
                            audio_bitrate = 256000;
                            break;
                        case 11:
                            audio_bitrate = 224000;
                            break;
                        case 10:
                            audio_bitrate = 192000;
                            break;
                        case 9:
                            audio_bitrate = 160000;
                            break;
                        case 8:
                            audio_bitrate = 128000;
                            break;
                        case 7:
                            audio_bitrate = 112000;
                            break;
                        case 6:
                            audio_bitrate = 96000;
                            break;
                        case 5:
                            audio_bitrate = 80000;
                            break;
                        case 4:
                            audio_bitrate = 64000;
                            break;
                        case 3:
                            audio_bitrate = 56000;
                            break;
                        case 2:
                            audio_bitrate = 48000;
                            break;
                        case 1:
                            audio_bitrate = 40000;
                            break;
                        case 0:
                            audio_bitrate = 32000;
                            break;
                        default:
                            audio_bitrate = 0;
                    }
                }
                else if (header_parse == 1) {
                    audio_bsid = (parse & 0xf8) >> 3;
                    audio_bsmod = parse & 0x7;
                }
                else if (header_parse == 0) {
                    audio_acmod = (parse & 0xe0) >> 5;
                    if (audio_sampling_rate == 0 || audio_bitrate == 0) {
                        first_header = FALSE;
                    }
                    else {
                        frame_size = audio_bitrate * 192 / audio_sampling_rate;
                        frame_size_check = audio_bitrate * 192 / audio_sampling_rate;
                    }
                }
            }
            if (audio_synced == FALSE && first_header == TRUE && second_header == TRUE) {
                if (pts_aligned != 0xffffffffffffffffLL || video_channel == 0) {
                    if (current_pts >= pts_aligned || video_channel == 0) {
                        audio_synced = TRUE;
                        frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                        for (j = 0; j <= frame_buffer_count; j++) {
                            if ((frame_buffer_pts[j] + 2800) > pts_aligned || video_channel == 0) {
#if 0
                                printf("j = %d, pts = 0x%08x, length = %d\n", j, (unsigned int)frame_buffer_pts[j], frame_buffer_length[j]);
#endif
                                if (first_synced == FALSE) {
                                    first_synced = TRUE;
                                    if (video_channel == 0) {
                                        printf("First Audio PTS = 0x%08x\n", (unsigned int)frame_buffer_pts[j]);
                                    }
                                    else {
                                        printf("First Audio PTS = 0x%08x, %d\n", (unsigned int)frame_buffer_pts[j], (unsigned int)(frame_buffer_pts[j] - pts_aligned));
                                    }
                                    if (parse_only == FALSE) {
                                        fwrite(&frame_buffer_start, 1, 1, fpoutaudio);
                                    }
                                }
                                if (parse_only == FALSE) {
                                    fwrite(&frame_buffer[j][0], 1, frame_buffer_length[j], fpoutaudio);
                                }
                            }
                        }
                        if (parse_only == FALSE) {
                            fwrite(es_ptr - 1, 1, length - i, fpoutaudio);
                        }
                    }
                    else {
                        --frame_size;
                        frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                        if (frame_buffer_index == (3840 + 8)) {
                            --frame_buffer_index;
                        }
                    }
                }
                else {
                    --frame_size;
                    frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                    if (frame_buffer_index == (3840 + 8)) {
                        --frame_buffer_index;
                    }
                }
            }
            else if (first_header == TRUE) {
                --frame_size;
                frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                if (frame_buffer_index == (3840 + 8)) {
                    --frame_buffer_index;
                }
            }
        }
    }
}

void parse_mp2_audio(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int first_access_unit)
{
    unsigned int i, j;
    static int first_header = FALSE;
    static int second_header = FALSE;
    static int audio_synced = FALSE;
    static int first_synced = FALSE;
    static unsigned int parse = 0;
    static unsigned int header_parse = 0;
    static unsigned int frame_size = 0;
    static unsigned int frame_size_check = 0;
    static unsigned char frame_buffer[128][3840 + 8];
    static unsigned char frame_buffer_start = 0xff;
    static unsigned int frame_buffer_index = 0;
    static unsigned int frame_buffer_count = 0;
    static unsigned int frame_buffer_length[128];
    static unsigned long long frame_buffer_pts[128];
    static unsigned long long current_pts;
    static unsigned long long current_pts_saved;
    static unsigned int current_pts_valid = FALSE;
    static unsigned int audio_sampling_rate;
    static unsigned int audio_bitrate;
    static unsigned int audio_mode;
    static unsigned int audio_mode_ext;
    static unsigned int audio_copyright;
    static unsigned int audio_original;
    static unsigned int audio_emphasis;

    if (parse_only == FALSE) {
        if (audio_synced == TRUE) {
            fwrite(es_ptr, 1, length, fpoutaudio);
        }
    }
    if (audio_synced == FALSE) {
        if (first_access_unit == TRUE) {
            current_pts_saved = pts;
            current_pts_valid = TRUE;
        }
        for (i = 0; i < length; i++) {
            parse = (parse << 8) + *es_ptr++;
#if 1
            if ((parse & 0xffff) == 0x0000fffc || (parse & 0xffff) == 0x0000fffd) {
#else
            if ((parse & 0xffff) == 0x0000fffc) {
#endif
                if (current_pts_valid == TRUE) {
                    current_pts = current_pts_saved;
                    current_pts_valid = FALSE;
                }
                else {
                    if (frame_size_check != 0) {
                        if (frame_buffer_index == frame_size_check) {
                            current_pts += ((1152 * 90000) / audio_sampling_rate);
                        }
                    }
                }
                if (first_header == FALSE) {
                    header_parse = 2;
                    first_header = TRUE;
                    frame_buffer_pts[frame_buffer_count] = current_pts;
                }
                else if (second_header == FALSE) {
                    if (frame_size == 2) {
                        second_header = TRUE;
                        printf("Audio Bitrate = %d, Audio Sampling Rate = %d\n", audio_bitrate, audio_sampling_rate);
                        switch (audio_mode & 0x3) {
                            case 3:
                                printf("Audio Mode = Single Channel, mode_extension = %d\n", audio_mode_ext);
                                break;
                            case 2:
                                printf("Audio Mode = Dual Channel, mode_extension = %d\n", audio_mode_ext);
                                break;
                            case 1:
                                printf("Audio Mode = Joint Stereo, mode_extension = %d\n", audio_mode_ext);
                                break;
                            case 0:
                                printf("Audio Mode = Stereo, mode_extension = %d\n", audio_mode_ext);
                                break;
                        }
                        switch (audio_emphasis & 0x3) {
                            case 3:
                                printf("Audio Emphasis = CCITT J.17, copyright = %d, original = %d\n", audio_copyright, audio_original);
                                break;
                            case 2:
                                printf("Audio Emphasis = Reserved, copyright = %d, original = %d\n", audio_copyright, audio_original);
                                break;
                            case 1:
                                printf("Audio Emphasis = 50/15 usec, copyright = %d, original = %d\n", audio_copyright, audio_original);
                                break;
                            case 0:
                                printf("Audio Emphasis = None, copyright = %d, original = %d\n", audio_copyright, audio_original);
                                break;
                        }
                        if (audio_synced == FALSE) {
                            frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                            frame_buffer_index = 0;
                            frame_buffer_count++;
                            frame_buffer_count &= 0x7f;
                            frame_buffer_pts[frame_buffer_count] = current_pts;
                        }
                    }
                    else {
                        first_header = FALSE;
                        frame_buffer_count = 0;
                        frame_buffer_index = 0;
                    }
                }
                else {
                    if (audio_synced == FALSE) {
                        if (frame_buffer_index == frame_size_check) {
                            frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                            frame_buffer_index = 0;
                            frame_buffer_count++;
                            frame_buffer_count &= 0x7f;
                            frame_buffer_pts[frame_buffer_count] = current_pts;
                        }
                    }
                }
            }
            else if (header_parse != 0) {
                --header_parse;
                if (header_parse == 1) {
                    switch ((parse & 0xc) >> 2) {
                        case 3:
                            audio_sampling_rate = 0;
                            break;
                        case 2:
                            audio_sampling_rate = 32000;
                            break;
                        case 1:
                            audio_sampling_rate = 48000;
                            break;
                        case 0:
                            audio_sampling_rate = 44100;
                            break;
                    }
                    switch ((parse & 0xf0) >> 4) {
                        case 14:
                            audio_bitrate = 384000;
                            break;
                        case 13:
                            audio_bitrate = 320000;
                            break;
                        case 12:
                            audio_bitrate = 256000;
                            break;
                        case 11:
                            audio_bitrate = 224000;
                            break;
                        case 10:
                            audio_bitrate = 192000;
                            break;
                        case 9:
                            audio_bitrate = 160000;
                            break;
                        case 8:
                            audio_bitrate = 128000;
                            break;
                        case 7:
                            audio_bitrate = 112000;
                            break;
                        case 6:
                            audio_bitrate = 96000;
                            break;
                        case 5:
                            audio_bitrate = 80000;
                            break;
                        case 4:
                            audio_bitrate = 64000;
                            break;
                        case 3:
                            audio_bitrate = 56000;
                            break;
                        case 2:
                            audio_bitrate = 48000;
                            break;
                        case 1:
                            audio_bitrate = 32000;
                            break;
                        case 0:
                            audio_bitrate = 0;
                            break;
                        default:
                            audio_bitrate = 0;
                    }
                }
                else if (header_parse == 0) {
                    audio_mode = (parse & 0xc0) >> 6;
                    audio_mode_ext = (parse & 0x30) >> 4;
                    audio_copyright = (parse & 0x8) >> 3;
                    audio_original = (parse & 0x4) >> 2;
                    audio_emphasis = parse & 0x3;
                    if (audio_sampling_rate == 0 || audio_bitrate == 0) {
                        first_header = FALSE;
                    }
                    else {
                        frame_size = audio_bitrate * 144 / audio_sampling_rate;
                        frame_size_check = audio_bitrate * 144 / audio_sampling_rate;
                    }
                }
            }
            if (audio_synced == FALSE && first_header == TRUE && second_header == TRUE) {
                if (pts_aligned != 0xffffffffffffffffLL || video_channel == 0) {
                    if (current_pts >= pts_aligned || video_channel == 0) {
                        audio_synced = TRUE;
                        frame_buffer_length[frame_buffer_count] = frame_buffer_index;
                        for (j = 0; j <= frame_buffer_count; j++) {
                            if ((frame_buffer_pts[j] + 2160) > pts_aligned || video_channel == 0) {
#if 0
                                printf("j = %d, pts = 0x%08x, length = %d\n", j, (unsigned int)frame_buffer_pts[j], frame_buffer_length[j]);
#endif
                                if (first_synced == FALSE) {
                                    first_synced = TRUE;
                                    if (video_channel == 0) {
                                        printf("First Audio PTS = 0x%08x\n", (unsigned int)frame_buffer_pts[j]);
                                    }
                                    else {
                                        printf("First Audio PTS = 0x%08x, %d\n", (unsigned int)frame_buffer_pts[j], (unsigned int)(frame_buffer_pts[j] - pts_aligned));
                                    }
                                    if (parse_only == FALSE) {
                                        fwrite(&frame_buffer_start, 1, 1, fpoutaudio);
                                    }
                                }
                                if (parse_only == FALSE) {
                                    fwrite(&frame_buffer[j][0], 1, frame_buffer_length[j], fpoutaudio);
                                }
                            }
                        }
                        if (parse_only == FALSE) {
                            fwrite(es_ptr - 1, 1, length - i, fpoutaudio);
                        }
                    }
                    else {
                        --frame_size;
                        frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                        if (frame_buffer_index == (3840 + 8)) {
                            --frame_buffer_index;
                        }
                    }
                }
                else {
                    --frame_size;
                    frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                    if (frame_buffer_index == (3840 + 8)) {
                        --frame_buffer_index;
                    }
                }
            }
            else if (first_header == TRUE) {
                --frame_size;
                frame_buffer[frame_buffer_count][frame_buffer_index++] = (unsigned char)parse & 0xff;
                if (frame_buffer_index == (3840 + 8)) {
                    --frame_buffer_index;
                }
            }
        }
    }
}

void parse_lpcm_audio(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int first_access_unit, unsigned short lpcm_header_flags)
{
    unsigned int i, j, channels, sample_bytes, adjusted_length, index = 0;
    static unsigned int sample;
    static unsigned int first_header_dump = FALSE;
    static unsigned int extra_bytes = 0;
    static unsigned char extra_bytes_buffer[4];
    static unsigned char temp_buffer[188];
    static unsigned char null_bytes[4] = {0, 0, 0, 0};

    if (first_access_unit == TRUE) {
        if (sample != 0) {
            extra_bytes = 0;
        }
    }
    for (i = 0; i < extra_bytes; i++) {
        temp_buffer[index++] = extra_bytes_buffer[i];
    }
    for (i = 0; i < length; i++) {
        temp_buffer[index++] = *es_ptr++;
    }
    length = length + extra_bytes;
    es_ptr = &temp_buffer[0];
    if (first_header_dump == FALSE) {
        first_header_dump = TRUE;
        switch ((lpcm_header_flags & 0xf000) >> 12) {
            case 1:
                printf("LPCM Audio Mode = 1/0\n");
                break;
            case 3:
                printf("LPCM Audio Mode = 2/0\n");
                break;
            case 4:
                printf("LPCM Audio Mode = 3/0\n");
                break;
            case 5:
                printf("LPCM Audio Mode = 2/1\n");
                break;
            case 6:
                printf("LPCM Audio Mode = 3/1\n");
                break;
            case 7:
                printf("LPCM Audio Mode = 2/2\n");
                break;
            case 8:
                printf("LPCM Audio Mode = 3/2\n");
                break;
            case 9:
                printf("LPCM Audio Mode = 3/2+lfe\n");
                break;
            case 10:
                printf("LPCM Audio Mode = 3/4\n");
                break;
            case 11:
                printf("LPCM Audio Mode = 3/4+lfe\n");
                break;
            default:
                printf("LPCM Audio Mode = reserved\n");
                break;
        }
        switch ((lpcm_header_flags & 0xc0) >> 6) {
            case 1:
                printf("LPCM Audio Bits/sample = 16\n");
                break;
            case 2:
                printf("LPCM Audio Bits/sample = 20\n");
                break;
            case 3:
                printf("LPCM Audio Bits/sample = 24\n");
                break;
            default:
                printf("LPCM Audio Bits/sample = reserved\n");
                break;
        }
        switch ((lpcm_header_flags & 0xf00) >> 8) {
            case 1:
                printf("LPCM Audio Sample Rate = 48000\n");
                break;
            case 4:
                printf("LPCM Audio Sample Rate = 96000\n");
                break;
            case 5:
                printf("LPCM Audio Sample Rate = 192000\n");
                break;
            default:
                printf("LPCM Audio Sample Rate = reserved\n");
                break;
        }
    }
    switch ((lpcm_header_flags & 0xf000) >> 12) {
        case 1:
            channels = 2;
            break;
        case 3:
            channels = 2;
            break;
        case 4:
            channels = 4;
            break;
        case 5:
            channels = 4;
            break;
        case 6:
            channels = 4;
            break;
        case 7:
            channels = 4;
            break;
        case 8:
            channels = 6;
            break;
        case 9:
            channels = 6;
            break;
        case 10:
            channels = 8;
            break;
        case 11:
            channels = 8;
            break;
        default:
            channels = 2;
            break;
    }
    switch ((lpcm_header_flags & 0xc0) >> 6) {
        case 1:
            sample_bytes = 2;
            break;
        case 2:
            sample_bytes = 3;
            break;
        case 3:
            sample_bytes = 3;
            break;
        default:
            sample_bytes = 2;
            break;
    }
    if (first_access_unit == TRUE) {
        if (sample != 0) {
            printf("LPCM sample resync, adding %d samples\n", channels - sample);
            for (i = 0; i < (channels - sample); i++) {
                fwrite(&null_bytes[0], 1, sample_bytes, fpoutaudio);
            }
            sample = 0;
        }
    }
    i = 0;
    j = length / sample_bytes;
    adjusted_length = j * sample_bytes;
    extra_bytes = length - adjusted_length;
    while (i < adjusted_length) {
        switch (sample) {
            case 0:
                if (parse_only == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 1:
                if (parse_only == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 2:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 3:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 4:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 5:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 6:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
            case 7:
                if (parse_only == FALSE && lpcm_mode == FALSE) {
                    fwrite(es_ptr, 1, sample_bytes, fpoutaudio);
                }
                es_ptr += sample_bytes;
                i += sample_bytes;
                sample++;
                if (sample == channels) {
                    sample = 0;
                }
                break;
        }
    }
    for (i = 0; i < extra_bytes; i++) {
        extra_bytes_buffer[i] = *es_ptr++;
    }
}

void parse_mpeg2_video(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int dts)
{
    unsigned int i, j;
    static int first = TRUE;
    static int first_sequence = FALSE;
    static int first_sequence_dump = FALSE;
    static int look_for_gop = FALSE;
    static int gop_found = FALSE;
    static unsigned int parse = 0;
    static unsigned int picture_parse = 0;
    static unsigned int extension_parse = 0;
    static unsigned int picture_coding_parse = 0;
    static unsigned int sequence_header_parse = 0;
    static unsigned int sequence_extension_parse = 0;
    static unsigned int picture_size = 0;
    static unsigned int picture_count = 0;
    static unsigned int time_code_field = 0;
    static unsigned int time_code_rate = 1;
    static long double frame_rate = 1.0;
    static unsigned char header[3] = {0x0, 0x0, 0x1};
    static unsigned char gop_header[9] = {0xb8, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
    static unsigned int progressive_sequence;
    static unsigned long long first_pts;
    static unsigned int first_pts_count = 0;
    static unsigned int extra_byte = FALSE;
    static unsigned int last_temporal_reference = 0;
    static unsigned int last_gop_temporal_reference = 0;
    static unsigned int running_average_start = 0;
    static unsigned int running_average_count = 0;
    static unsigned int running_average_frames = 0;
    static unsigned int running_average_samples[1024];
    static unsigned int running_average_fields[1024];
    unsigned int temporal_reference, temp_flags;
    unsigned int picture_coding_type;
    unsigned int whole_buffer = TRUE;
    unsigned char *start_es_ptr;
    unsigned char *middle_es_ptr;
    unsigned int middle_length = 0x55555555;
    unsigned char hours, minutes, seconds, pictures;
    unsigned char temp_temporal_reference;
    long double temp_running_average;
    long double temp_running_fields;
    int result, offset;

    start_es_ptr = es_ptr;
    for (i = 0; i < length; i++) {
        parse = (parse << 8) + *es_ptr++;
        if (parse == 0x00000100) {
            picture_parse = 2;
            if (first_sequence == TRUE) {
                coded_frames++;
            }
            if (first == TRUE) {
                picture_size = 0;
            }
            else {
#if 0
                printf("%8ld\n", picture_size * 8);
#endif
                running_average_samples[running_average_frames] = picture_size * 8;
                picture_size = 0;
            }
            if (look_for_gop == TRUE) {
                look_for_gop = FALSE;
                if (gop_found == FALSE) {
                    if (parse_only == FALSE) {
                        j = time_code_rate * 60 * 60;
                        hours = ((time_code_field / 2) / j) % 24;
                        j /= 60;
                        minutes = ((time_code_field / 2) / j) % 60;
                        j /= 60;
                        seconds = ((time_code_field / 2) / j) % 60;
                        pictures = ((time_code_field / 2) % j);
                        gop_header[1] = 0x00;
                        gop_header[2] = 0x08;
                        gop_header[3] = 0x00;
                        gop_header[4] = 0x00;
                        gop_header[1] |= (hours << 2) & 0x7c;
                        gop_header[1] |= (minutes >> 4) & 0x03;
                        gop_header[2] |= (minutes << 4) & 0xf0;
                        gop_header[2] |= (seconds >> 3) & 0x07;
                        gop_header[3] |= (seconds << 5) & 0xe0;
                        gop_header[3] |= (pictures >> 1) & 0x1f;
                        gop_header[4] |= (pictures << 7) & 0x80;
                        if (middle_length == 0x55555555) {
                            fwrite(start_es_ptr, 1, i, fpoutvideo);
                        }
                        else {
                            fwrite(middle_es_ptr, 1, middle_length - (length - i), fpoutvideo);
                        }
                        fwrite(&gop_header, 1, 9, fpoutvideo);
                        middle_es_ptr = es_ptr;
                        middle_length = length - i - 1;
                        whole_buffer = FALSE;
                    }
                }
            }
        }
        else if (parse == 0x000001b3) {
            sequence_header_parse = 7;
            look_for_gop = TRUE;
            gop_found = FALSE;
            last_gop_temporal_reference = last_temporal_reference;
            if (dump_index == TRUE) {
                printf("Sequence header at packet number %lld/%lld\r\n", packet_counter, (packet_counter - 1) * 188);
            }
            if (first_sequence == FALSE) {
                printf("Sequence Header found\n");
                printf("%d frames before first Sequence Header\n", picture_count);
                if (parse_only == FALSE) {
                    fwrite(&header, 1, 3, fpoutvideo);
                    middle_es_ptr = es_ptr - 1;
                    middle_length = length - i;
                    whole_buffer = FALSE;
                }
                first_sequence = TRUE;
                picture_count = 0;
                time_code_field = 0;
                first_pts_count = 2;
            }
            else {
                picture_count = 0;
            }
        }
        else if (sequence_header_parse != 0) {
            --sequence_header_parse;
            if (first_sequence_dump == FALSE) {
                switch (sequence_header_parse) {
                    case 6:
                        break;
                    case 5:
                        break;
                    case 4:
                        printf("Horizontal size = %d\n", (parse & 0xfff000) >> 12);
                        printf("Vertical size = %d\n", parse & 0xfff);
                        break;
                    case 3:
                        switch ((parse & 0xf0) >> 4) {
                            case 0:
                                printf("Aspect ratio = forbidden\n");
                                break;
                            case 1:
                                printf("Aspect ratio = square samples\n");
                                break;
                            case 2:
                                printf("Aspect ratio = 4:3\n");
                                break;
                            case 3:
                                printf("Aspect ratio = 16:9\n");
                                break;
                            case 4:
                                printf("Aspect ratio = 2.21:1\n");
                                break;
                            default:
                                printf("Aspect ratio = reserved\n");
                                break;
                        }
                        switch (parse & 0xf) {
                            case 0:
                                printf("Frame rate = forbidden\n");
                                time_code_rate = 1;
                                frame_rate = 1.0;
                                break;
                            case 1:
                                printf("Frame rate = 23.976\n");
                                time_code_rate = 24;
                                frame_rate = 24.0 * (1000.0 / 1001.0);
                                break;
                            case 2:
                                printf("Frame rate = 24\n");
                                time_code_rate = 24;
                                frame_rate = 24.0;
                                break;
                            case 3:
                                printf("Frame rate = 25\n");
                                time_code_rate = 25;
                                frame_rate = 25.0;
                                break;
                            case 4:
                                printf("Frame rate = 29.97\n");
                                time_code_rate = 30;
                                frame_rate = 30.0 * (1000.0 / 1001.0);
                                break;
                            case 5:
                                printf("Frame rate = 30\n");
                                time_code_rate = 30;
                                frame_rate = 30.0;
                                break;
                            case 6:
                                printf("Frame rate = 50\n");
                                time_code_rate = 50;
                                frame_rate = 50.0;
                                break;
                            case 7:
                                printf("Frame rate = 59.94\n");
                                time_code_rate = 60;
                                frame_rate = 60.0 * (1000.0 / 1001.0);
                                break;
                            case 8:
                                printf("Frame rate = 60\n");
                                time_code_rate = 60;
                                frame_rate = 60.0;
                                break;
                            default:
                                printf("Frame rate = reserved\n");
                                break;
                        }
                        break;
                    case 2:
                        break;
                    case 1:
                        break;
                    case 0:
                        printf("Sequence header bitrate = %d bps\n", ((parse & 0xffffc0) >> 6) * 400);
                        break;
                }
            }
        }
        else if (picture_parse != 0) {
            --picture_parse;
            switch (picture_parse) {
                case 1:
                    if (gop_found == FALSE) {
                        if (i == (length - 1)) {
                            length -= 1;
                            if (whole_buffer == FALSE) {
                                middle_length -= 1;
                            }
                            extra_byte = TRUE;
                        }
                    }
                    break;
                case 0:
                    temporal_reference = (parse & 0xffff) >> 6;
                    if (dts == 1) {
                        last_temporal_reference = temporal_reference;
                    }
                    if (temporal_reference >= (last_gop_temporal_reference + 1)) {
                        temporal_reference = temporal_reference - (last_gop_temporal_reference + 1);
                    }
                    else {
                        temporal_reference = (temporal_reference + 1024) - (last_gop_temporal_reference + 1);
                    }
                    if (extra_byte == TRUE) {
                        extra_byte = FALSE;
                        temp_temporal_reference = (temporal_reference >> 2) & 0xff;
                        if (gop_found == FALSE) {
                            if (parse_only == FALSE) {
                                fwrite(&temp_temporal_reference, 1, 1, fpoutvideo);
                            }
                            *(es_ptr - 1) = (unsigned char)(((temporal_reference & 0x3) << 6) | (parse & 0x3f));
                        }
                    }
                    else {
                        if (gop_found == FALSE) {
                            *(es_ptr - 2) = (temporal_reference >> 2) & 0xff;
                            *(es_ptr - 1) = (unsigned char)(((temporal_reference & 0x3) << 6) | (parse & 0x3f));
                        }
                    }
                    picture_coding_type = (parse & 0x38) >> 3;
                    if (picture_coding_type == 0 || picture_coding_type > 3) {
                        printf("illegal picture_coding_type = %d\n", picture_coding_type);
                    }
                    break;
            }
        }
        else if (parse == 0x000001b5) {
            extension_parse = 1;
        }
        else if (extension_parse != 0) {
            --extension_parse;
            switch (extension_parse) {
                case 0:
                    if ((parse & 0xf0) == 0x80) {
                        if (first_sequence == TRUE) {
                            picture_coding_parse = 5;
                        }
                        picture_count++;
                    }
                    else if ((parse & 0xf0) == 0x10) {
                        sequence_extension_parse = 1;
                    }
                    break;
            }
        }
        else if (picture_coding_parse != 0) {
            --picture_coding_parse;
            switch (picture_coding_parse) {
                case 4:
                    break;
                case 3:
                    break;
                case 2:
                    break;
                case 1:
                    if (timecode_mode == TRUE) {
                        if (progressive_sequence == 0) {
                            if (parse & 0x200) {
                                time_code_field += 3;
                            }
                            else {
                                time_code_field += 2;
                            }
                        }
                        else {
                            temp_flags = ((parse & 0x8000) >> 14) | ((parse & 0x200) >> 9);
                            switch(temp_flags & 0x3) {
                                case 3:
                                    time_code_field += 6;
                                    break;
                                case 2:
                                    time_code_field += 0;
                                    break;
                                case 1:
                                    time_code_field += 4;
                                    break;
                                case 0:
                                    time_code_field += 2;
                                    break;
                                default:
                                    time_code_field += 0;
                                    break;
                            }
                        }
                    }
                    else {
                        time_code_field += 2;
                    }
                    if (progressive_sequence == 0) {
                        if (parse & 0x200) {
                            video_fields += 3;
                            running_average_fields[running_average_frames] = 3;
                        }
                        else {
                            video_fields += 2;
                            running_average_fields[running_average_frames] = 2;
                        }
                    }
                    else {
                        temp_flags = ((parse & 0x8000) >> 14) | ((parse & 0x200) >> 9);
                        switch(temp_flags & 0x3) {
                            case 3:
                                video_fields += 3;
                                running_average_fields[running_average_frames] = 3;
                                break;
                            case 2:
                                video_fields += 0;
                                break;
                            case 1:
                                video_fields += 2;
                                running_average_fields[running_average_frames] = 2;
                                break;
                            case 0:
                                video_fields += 1;
                                running_average_fields[running_average_frames] = 1;
                                break;
                            default:
                                video_fields += 0;
                                break;
                        }
                    }
                    if (first == TRUE) {
                        first = FALSE;
                    }
                    else {
                        running_average_frames = (running_average_frames + 1) & 1023;
                        running_average_count++;
                        if (running_average_count == 300) {
                            running_average_count = 299;
                            temp_running_average = 0;
                            temp_running_fields = 0.0;
                            for (j = 0; j < 300; j++) {
                                temp_running_average += running_average_samples[(running_average_start + j) & 1023];
                                temp_running_fields += running_average_fields[(running_average_start + j) & 1023];
                            }
                            running_average_start = (running_average_start + 1) & 1023;
                            if (progressive_sequence == 0) {
                                running_average_bitrate = (unsigned int)((temp_running_average / 300.0) * ((600.0 / temp_running_fields) * frame_rate));
                            }
                            else {
                                running_average_bitrate = (unsigned int)((temp_running_average / 300.0) * ((300.0 / temp_running_fields) * frame_rate));
                            }
                            if (running_average_bitrate > running_average_bitrate_peak) {
                                running_average_bitrate_peak = running_average_bitrate;
                            }
                        }
                    }
                    if (first_pts_count != 0) {
                        if (first_pts_count == 2) {
                            first_pts = pts;
                        }
                        --first_pts_count;
                        if (first_pts_count == 0) {
                            if (first_pts > pts) {
                                first_pts = pts;
                                pts_aligned = first_pts;
                            }
                            else {
                                pts_aligned = first_pts;
                            }
                            printf("First Video PTS = 0x%08x\n", (unsigned int)pts_aligned);
                        }
                    }
                    break;
                case 0:
                    break;
            }
        }
        else if (sequence_extension_parse != 0) {
            --sequence_extension_parse;
            if (first_sequence_dump == FALSE) {
                switch (sequence_extension_parse) {
                    case 0:
                        printf("Progressive Sequence = %d\n", (parse & 0x8) >> 3);
                        progressive_sequence = (parse & 0x8) >> 3;
                        video_progressive = progressive_sequence;
                        first_sequence_dump = TRUE;
                        break;
                }
            }
        }
        else if (parse == 0x000001b8) {
            gop_found = TRUE;
        }
#if 0
        else if (parse == 0x000001b7) {
            if (parse_only == FALSE) {
                if (i < 3) {
                    offset = 0 - (3 - i);
                    result = fseek(fpoutvideo, offset, SEEK_CUR);
                    whole_buffer = FALSE;
                    middle_es_ptr = es_ptr;
                    middle_length = length - (i + 1);
                }
                else {
                    fwrite(start_es_ptr, 1, i - 3, fpoutvideo);
                    whole_buffer = FALSE;
                    middle_es_ptr = es_ptr;
                    middle_length = length - i - 1;
                }
            }
        }
#endif
        picture_size++;
    }
    if (parse_only == FALSE) {
        if (first_sequence == TRUE) {
            if (whole_buffer == TRUE) {
                fwrite(start_es_ptr, 1, length, fpoutvideo);
            }
            else {
                fwrite(middle_es_ptr, 1, middle_length, fpoutvideo);
            }
        }
    }
}

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

void parse_h264_video(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int dts)
{
    int i, j, bits;
    unsigned int temp;
    static int first = TRUE;
    static int first_sequence = FALSE;
    static int first_sequence_dump = FALSE;
    static int emulation_flag = FALSE;
    static unsigned int parse = 0;
    static unsigned int parsed = 0;
    static unsigned int access_unit_delimiter_parse = 0;
    static unsigned int sequence_parameter_set_parse = 0;
    static unsigned int sequence_parameter_set_index = 0;
    static unsigned char sequence_parameter_set[256 * 8];
    static unsigned char *sequence_parameter_ptr;
    static unsigned int coded_slice_parse = 0;
    static unsigned int coded_slice_index = 0;
    static unsigned char coded_slice[256 * 8];
    static unsigned char *coded_slice_ptr;
    static unsigned int sei_parse = 0;
    static unsigned int sei_index = 0;
    static unsigned char sei[256 * 8];
    static unsigned char *sei_ptr;
    static unsigned char *last_sei_ptr;
    static unsigned int picture_size = 0;
    static unsigned int picture_count = 0;
    static unsigned char header[5] = {0x0, 0x0, 0x0, 0x1, 0x9};
    static unsigned long long first_pts;
    static unsigned int first_pts_count = 0;
    static unsigned int running_average_start = 0;
    static unsigned int running_average_count = 0;
    static unsigned int running_average_frames = 0;
    static unsigned int running_average_samples[1024];
    static unsigned int running_average_fields[1024];
    static unsigned int profile_idc;
    static unsigned int constraint_set3_flag;
    static unsigned int level_idc;
    static unsigned int num_ref_frames_in_pic_order_cnt_cycle;
    static unsigned int pic_width_in_mbs_minus1;
    static unsigned int pic_height_in_map_units_minus1;
    static unsigned int frame_mbs_only_flag;
    static unsigned int mb_adaptive_frame_field_flag;
    static unsigned int aspect_ratio_idc;
    static unsigned int num_units_in_tick;
    static unsigned int time_scale;
    static unsigned int cpb_cnt_minus1;
    static unsigned int nal_hrd_parameters_present_flag;
    static unsigned int vcl_hrd_parameters_present_flag;
    static unsigned int nal_initial_cpb_removal_delay_length_minus1;
    static unsigned int nal_cpb_removal_delay_length_minus1;
    static unsigned int nal_dpb_output_delay_length_minus1;
    static unsigned int vcl_initial_cpb_removal_delay_length_minus1;
    static unsigned int vcl_cpb_removal_delay_length_minus1;
    static unsigned int vcl_dpb_output_delay_length_minus1;
    static unsigned int pic_struct_present_flag;
    static unsigned int payloadType;
    static unsigned int last_payload_type_byte;
    static unsigned int payloadSize;
    static unsigned int last_payload_size_byte;
    static unsigned int pic_struct;
    unsigned char primary_pic_type;
    unsigned int whole_buffer = TRUE;
    unsigned char *start_es_ptr;
    unsigned char *middle_es_ptr;
    unsigned int middle_length = 0x55555555;
    static long double frame_rate = 1.0;
    long double temp_running_average;
    long double temp_running_fields;

    start_es_ptr = es_ptr;
    for (i = 0; i < length; i++) {
        parsed = parse;
        parse = (parse << 8) + *es_ptr++;
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
                        temp = read_bits(&sequence_parameter_ptr, 1); /* seperate_colour_plane_flag */
                    }
                    temp = read_ue(&sequence_parameter_ptr);          /* bit_depth_luma_minus8 */
                    temp = read_ue(&sequence_parameter_ptr);          /* bit_depth_chroma_minus8 */
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* qpprime_y_zero_transform_bypass_flag */
                    temp = read_bits(&sequence_parameter_ptr, 1);     /* seq_scaling_matrix_present_flag */
                    if (temp) {
                        /* fix me */
                    }
                }
                temp = read_ue(&sequence_parameter_ptr);          /* log2_max_frame_num_minus4 */
                temp = read_ue(&sequence_parameter_ptr);          /* pic_order_cnt_type */
                if (temp == 0) {
                    temp = read_ue(&sequence_parameter_ptr);      /* log2_max_pic_order_cnt_lsb_minus4 */
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
                    temp = read_bits(&sequence_parameter_ptr, 4); /* bit_rate_scale */
                    temp = read_bits(&sequence_parameter_ptr, 4); /* cpb_size_scale */
                    for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                        temp = read_ue(&sequence_parameter_ptr);  /* bit_rate_value_minus1 */
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
                        temp = read_ue(&sequence_parameter_ptr);  /* bit_rate_value_minus1 */
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
                }
                first_sequence_dump = TRUE;
                sequence_parameter_set_parse = 0;
            }
            if (coded_slice_parse != 0) {
                coded_slice_ptr = &coded_slice[0];
                temp = read_ue(&coded_slice_ptr);    /* first_mb_in_slice */
                if (first_sequence_dump == TRUE && temp == 0) {
                    coded_frames++;
                    if (!pic_struct_present_flag) {
                        video_fields += 1;
                        running_average_fields[running_average_frames] = 1;
                    }
                }
                temp = read_ue(&coded_slice_ptr);    /* slice_type */
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
#if 0
                    printf("payloadType = %d, payloadSize = %d\n", payloadType, payloadSize);
#endif
                    switch (payloadType) {
                        case 0:
                            last_sei_ptr = sei_ptr;
                            temp = read_ue(&sei_ptr);    /* seq_parameter_set_id */
                            sei_index -= sei_ptr - last_sei_ptr;
                            payloadSize -= sei_ptr - last_sei_ptr;
                            if (nal_hrd_parameters_present_flag) {
                                for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                                    temp = read_bits(&sei_ptr, nal_initial_cpb_removal_delay_length_minus1 + 1);
                                    temp = read_bits(&sei_ptr, nal_initial_cpb_removal_delay_length_minus1 + 1);
                                }
                            }
                            if (vcl_hrd_parameters_present_flag) {
                                for (j = 0; j < (cpb_cnt_minus1 + 1); j++) {
                                    temp = read_bits(&sei_ptr, vcl_initial_cpb_removal_delay_length_minus1 + 1);
                                    temp = read_bits(&sei_ptr, vcl_initial_cpb_removal_delay_length_minus1 + 1);
                                }
                            }
                            temp = read_bits(&sei_ptr, payloadSize);
                            sei_index -= payloadSize;
                            break;
                        case 1:
                            if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                                temp = read_bits(&sei_ptr, nal_cpb_removal_delay_length_minus1 + 1);
                                temp = read_bits(&sei_ptr, nal_dpb_output_delay_length_minus1 + 1);
                            }
                            if (pic_struct_present_flag) {
                                pic_struct = read_bits(&sei_ptr, 4);
                                if (pic_struct == 0 || pic_struct == 1 || pic_struct == 2) {
                                    video_fields += 1;
                                    running_average_fields[running_average_frames] = 1;
                                }
                                else if (pic_struct == 3 || pic_struct == 4 || pic_struct == 7) {
                                    video_fields += 2;
                                    running_average_fields[running_average_frames] = 2;
                                }
                                else if (pic_struct == 5 || pic_struct == 6 || pic_struct == 8) {
                                    video_fields += 3;
                                    running_average_fields[running_average_frames] = 3;
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
                            sei_index -= payloadSize;
                            break;
                    }
                } while (sei_index);
                sei_parse = 0;
            }
        }
        if (parse == 0x00000109) {
            access_unit_delimiter_parse = 1;
            if (first == TRUE) {
                picture_size = 0;
                first = FALSE;
            }
            else {
#if 0
                if (first_sequence_dump == TRUE) {
                    printf("%d\n", picture_size * 8);
                }
#endif
                running_average_samples[running_average_frames] = picture_size * 8;
                picture_size = 0;
            }
        }
        else if (access_unit_delimiter_parse != 0) {
            --access_unit_delimiter_parse;
            primary_pic_type = (unsigned char)(parse & 0xff) >> 5;
#if 0
            printf("primary_pic_type = %d\n", primary_pic_type);
#endif
            if (first_sequence == FALSE && primary_pic_type == 0) {
                printf("%d frames before first I-frame\n", picture_count);
                if (parse_only == FALSE) {
                    fwrite(&header, 1, 5, fpoutvideo);
                    middle_es_ptr = es_ptr - 1;
                    middle_length = length - i;
                    whole_buffer = FALSE;
                }
                first_sequence = TRUE;
                first_pts_count = 2;
            }
            if (first_sequence_dump == TRUE) {
                running_average_frames = (running_average_frames + 1) & 1023;
                running_average_count++;
                if (running_average_count == 300) {
                    running_average_count = 299;
                    temp_running_average = 0;
                    temp_running_fields = 0.0;
                    for (j = 0; j < 300; j++) {
                        temp_running_average += running_average_samples[(running_average_start + j) & 1023];
                        temp_running_fields += running_average_fields[(running_average_start + j) & 1023];
                    }
                    running_average_start = (running_average_start + 1) & 1023;
                    running_average_bitrate = (unsigned int)((temp_running_average / 300.0) * ((300.0 / temp_running_fields) * frame_rate));
                    if (running_average_bitrate > running_average_bitrate_peak)  {
                        running_average_bitrate_peak = running_average_bitrate;
                    }
                }
            }
            if (first_pts_count != 0) {
                if (first_pts_count == 2) {
                    first_pts = pts;
                }
                --first_pts_count;
                if (first_pts_count == 0) {
                    if (first_pts > pts) {
                        first_pts = pts;
                        pts_aligned = first_pts;
                    }
                    else {
                        pts_aligned = first_pts;
                    }
                    printf("First Video PTS = 0x%08x\n", (unsigned int)pts_aligned);
                }
            }
            picture_count++;
        }
        else if (parse == 0x00000127 || parse == 0x00000147 || parse == 0x00000167) {
            sequence_parameter_set_parse = 256;
            sequence_parameter_set_index = 0;
            if (dump_index == TRUE) {
                printf("Sequence header at packet number %lld/%lld\r\n", packet_counter, (packet_counter - 1) * 188);
            }
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
            coded_slice_parse = 256;
            coded_slice_index = 0;
            if (dump_index == TRUE) {
                if ((parse & 0xf) == 0x5) {
                    printf("IDR picture\n");
                }
            }
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
        else if (parse == 0x0000010a ||  parse == 0x0000010b) {
            if (parse_only == FALSE) {
                middle_es_ptr = es_ptr - 1;
                *middle_es_ptr = 0xc;
            }
        }
        picture_size++;
    }
    if (parse_only == FALSE) {
        if (first_sequence == TRUE) {
            if (whole_buffer == TRUE) {
                fwrite(start_es_ptr, 1, length, fpoutvideo);
            }
            else {
                fwrite(middle_es_ptr, 1, middle_length, fpoutvideo);
            }
        }
    }
}

void parse_vc1_video(unsigned char *es_ptr, unsigned int length, unsigned long long pts, unsigned int dts)
{
    unsigned int i, j;
    static unsigned int parse = 0;
    static unsigned int frame_header_parse = 0;
    static unsigned int sequence_header_parse = 0;
    static unsigned int first = TRUE;
    static unsigned int first_sequence = FALSE;
    static unsigned int first_sequence_dump = FALSE;
    static unsigned int display_extension_parse = 0;
    static unsigned int display_framerate_parse = 0;
    static unsigned int display_frameratevalue_parse = 0;
    static unsigned int picture_size = 0;
    static unsigned int interlace;
    static unsigned int tfcntrflag;
    static unsigned int ptype;
    static unsigned int picture_type;
    static long double frame_rate = 1.0;
    static unsigned int framerateexp;
    static long double frameratenr;
    static long double frameratedr;
    static unsigned long long first_pts;
    static unsigned int first_pts_count = 0;
    static unsigned int picture_count = 0;
    static unsigned char header[3] = {0x0, 0x0, 0x1};
    static unsigned int running_average_start = 0;
    static unsigned int running_average_count = 0;
    static unsigned int running_average_frames = 0;
    static unsigned int running_average_samples[1024];
    static unsigned int running_average_fields[1024];
    unsigned int temp_flags;
    unsigned int whole_buffer = TRUE;
    unsigned char *start_es_ptr;
    unsigned char *middle_es_ptr;
    unsigned int middle_length = 0x55555555;
    long double temp_running_average;
    long double temp_running_fields;
    int result, offset;

    start_es_ptr = es_ptr;
    for (i = 0; i < length; i++) {
        parse = (parse << 8) + *es_ptr++;
        if (parse == 0x0000010d) {
            picture_count++;
            if (first_sequence == TRUE) {
                frame_header_parse = 4;
                coded_frames++;
            }
            if (first == TRUE) {
                picture_size = 0;
            }
            else {
#if 0
                printf("%8ld\n", picture_size * 8);
#endif
                running_average_samples[running_average_frames] = picture_size * 8;
                picture_size = 0;
            }
        }
        else if (frame_header_parse != 0) {
            --frame_header_parse;
            switch (frame_header_parse) {
                case 3:
                    break;
                case 2:
                    break;
                case 1:
                    break;
                case 0:
                    if (interlace == 1) {
                        if ((parse & 0x80000000) == 0) {
                            ptype = (parse & 0x78000000) >> 13;
                        }
                        else {
                            ptype = (parse & 0x3c000000) >> 12;
                        }
                    }
                    else {
                        ptype = (parse & 0xf0000000) >> 14;
                    }
                    if ((ptype & 0x20000) == 0) {
                        picture_type = P;
                        if (tfcntrflag) {
                            temp_flags = (ptype & 0x00180) >> 7;
                        }
                        else {
                            temp_flags = (ptype & 0x18000) >> 15;
                        }
                    }
                    else if ((ptype & 0x10000) == 0) {
                        picture_type = B;
                        if (tfcntrflag) {
                            temp_flags = (ptype & 0x00c0) >> 6;
                        }
                        else {
                            temp_flags = (ptype & 0xc000) >> 14;
                        }
                    }
                    else if ((ptype & 0x8000) == 0) {
                        picture_type = I;
                        if (tfcntrflag) {
                            temp_flags = (ptype & 0x0060) >> 5;
                        }
                        else {
                            temp_flags = (ptype & 0x6000) >> 13;
                        }
                    }
                    else if ((ptype & 0x4000) == 0) {
                        picture_type = BI;
                        if (tfcntrflag) {
                            temp_flags = (ptype & 0x0030) >> 4;
                        }
                        else {
                            temp_flags = (ptype & 0x3000) >> 12;
                        }
                    }
                    else {
                        picture_type = SKIPPED;
                        if (tfcntrflag) {
                            temp_flags = (ptype & 0x0018) >> 3;
                        }
                        else {
                            temp_flags = (ptype & 0x1800) >> 11;
                        }
                    }
#if 0
                    printf("picture_type = %d, %d\n", picture_type, temp_flags);
#endif
                    if (interlace == 1) {
                        if (temp_flags & 0x1) {
                            video_fields += 3;
                            running_average_fields[running_average_frames] = 3;
                        }
                        else {
                            video_fields += 2;
                            running_average_fields[running_average_frames] = 2;
                        }
                    }
                    else {
                        switch(temp_flags & 0x3) {
                            case 3:
                                video_fields += 4;
                                running_average_fields[running_average_frames] = 4;
                                break;
                            case 2:
                                video_fields += 3;
                                running_average_fields[running_average_frames] = 3;
                                break;
                            case 1:
                                video_fields += 2;
                                running_average_fields[running_average_frames] = 2;
                                break;
                            case 0:
                                video_fields += 1;
                                running_average_fields[running_average_frames] = 1;
                                break;
                            default:
                                video_fields += 0;
                                break;
                        }
                    }
                    if (first == TRUE) {
                        first = FALSE;
                    }
                    else {
                        running_average_frames = (running_average_frames + 1) & 1023;
                        running_average_count++;
                        if (running_average_count == 300) {
                            running_average_count = 299;
                            temp_running_average = 0;
                            temp_running_fields = 0.0;
                            for (j = 0; j < 300; j++) {
                                temp_running_average += running_average_samples[(running_average_start + j) & 1023];
                                temp_running_fields += running_average_fields[(running_average_start + j) & 1023];
                            }
                            running_average_start = (running_average_start + 1) & 1023;
                            if (interlace == 1) {
                                running_average_bitrate = (unsigned int)((temp_running_average / 300.0) * ((600.0 / temp_running_fields) * frame_rate));
                            }
                            else {
                                running_average_bitrate = (unsigned int)((temp_running_average / 300.0) * ((300.0 / temp_running_fields) * frame_rate));
                            }
                        }
                    }
                    if (first_pts_count != 0) {
                        if (first_pts_count == 2) {
                            first_pts = pts;
                        }
                        --first_pts_count;
                        if (first_pts_count == 0) {
                            if (first_pts > pts) {
                                first_pts = pts;
                                pts_aligned = first_pts;
                            }
                            else {
                                pts_aligned = first_pts;
                            }
                            printf("First Video PTS = 0x%08x\n", (unsigned int)pts_aligned);
                        }
                    }
                    break;
            }
        }
        else if (parse == 0x0000010f) {
            if (first_sequence_dump == FALSE) {
                printf("Sequence Header found\n");
                sequence_header_parse = 6;
            }
            if (first_sequence == FALSE) {
                printf("%d frames before first I-frame\n", picture_count);
                if (parse_only == FALSE) {
                    fwrite(&header, 1, 3, fpoutvideo);
                    middle_es_ptr = es_ptr - 1;
                    middle_length = length - i;
                    whole_buffer = FALSE;
                }
                first_sequence = TRUE;
                first_pts_count = 2;
            }
        }
        else if (sequence_header_parse != 0) {
            --sequence_header_parse;
            if (first_sequence_dump == FALSE) {
                switch (sequence_header_parse) {
                    case 5:
                        if (((parse & 0xc0) >> 6) == 3) {
                            printf("Advanced Profile\n");
                        }
                        else {
                            printf("Reserved Profile\n");
                        }
                        if (((parse & 0x38) >> 3) > 4) {
                            printf("Level = Reserved\n");
                        }
                        else {
                            printf("Level = %d\n", ((parse & 0x38) >> 3));
                        }
                        if (((parse & 0x6) >> 1) == 1) {
                            printf("Chroma Format = 4:2:0\n");
                        }
                        else {
                            printf("Chroma Format = Reserved\n");
                        }
                        break;
                    case 4:
                        break;
                    case 3:
                        break;
                    case 2:
                        break;
                    case 1:
                        printf("Horizontal size = %d\n", (((parse & 0xfff000) >> 12) * 2) + 2);
                        printf("Vertical size = %d\n", ((parse & 0xfff) * 2) + 2);
                        break;
                    case 0:
                        printf("Pulldown = %d\n", (parse & 0x80) >> 7);
                        printf("Interlace = %d\n", (parse & 0x40) >> 6);
                        interlace = (parse & 0x40) >> 6;
                        tfcntrflag = (parse & 0x20) >> 5;
                        video_progressive = !(interlace);
                        if (parse & 0x2) {
                            display_extension_parse = 4;
                        }
                        break;
                }
            }
        }
        else if (display_extension_parse != 0) {
            --display_extension_parse;
            if (first_sequence_dump == FALSE) {
                switch (display_extension_parse) {
                    case 3:
                        break;
                    case 2:
                        printf("Display Horizontal size = %d\n", ((parse & 0x1fff8) >> 3) + 1);
                        break;
                    case 1:
                        break;
                    case 0:
                        printf("Display Vertical size = %d\n", ((parse & 0x7ffe0) >> 5) + 1);
                        if (parse & 0x10) {
                            display_framerate_parse = 1;
                            switch (parse & 0xf) {
                                case 0:
                                    printf("Aspect ratio = unspecified\n");
                                    break;
                                case 1:
                                    printf("Aspect ratio = 1:1 (square samples)\n");
                                    break;
                                case 2:
                                    printf("Aspect ratio = 12:11 (704x576 4:3)\n");
                                    break;
                                case 3:
                                    printf("Aspect ratio = 10:11 (704x480 4:3)\n");
                                    break;
                                case 4:
                                    printf("Aspect ratio = 16:11 (704x576 16:9)\n");
                                    break;
                                case 5:
                                    printf("Aspect ratio = 40:33 (704x480 16:9)\n");
                                    break;
                                case 6:
                                    printf("Aspect ratio = 24:11 (352x576 4:3)\n");
                                    break;
                                case 7:
                                    printf("Aspect ratio = 20:11 (352x480 4:3)\n");
                                    break;
                                case 8:
                                    printf("Aspect ratio = 32:11 (352x576 16:9)\n");
                                    break;
                                case 9:
                                    printf("Aspect ratio = 80:33 (352x480 16:9)\n");
                                    break;
                                case 10:
                                    printf("Aspect ratio = 18:11 (480x576 4:3)\n");
                                    break;
                                case 11:
                                    printf("Aspect ratio = 15:11 (480x480 4:3)\n");
                                    break;
                                case 12:
                                    printf("Aspect ratio = 64:33 (528x576 16:9)\n");
                                    break;
                                case 13:
                                    printf("Aspect ratio = 160:99 (528x480 16:9)\n");
                                    break;
                                case 14:
                                    printf("Aspect ratio = Reserved\n");
                                    break;
                                case 15:
                                    break;
                            }
                        }
                        else {
                        }
                        break;
                }
            }
        }
        else if (display_framerate_parse != 0) {
            --display_framerate_parse;
            if (first_sequence_dump == FALSE) {
                switch (display_framerate_parse) {
                    case 0:
                        if (parse & 0x80) {
                            display_frameratevalue_parse = 2;
                        }
                        break;
                }
            }
        }
        else if (display_frameratevalue_parse != 0) {
            --display_frameratevalue_parse;
            if (first_sequence_dump == FALSE) {
                switch (display_frameratevalue_parse) {
                    case 0:
                        if (parse & 0x400000) {
                            framerateexp = (parse & 0x3fffc0) >> 10;
                            frame_rate = ((long double)(framerateexp + 1)) / 32.0;
                            printf("Frame Rate = %.3f\n", (double)frame_rate);
                            first_sequence_dump = TRUE;
                        }
                        else {
                            switch ((parse & 0x3fc000) >> 14) {
                                case 0:
                                    printf("Forbidden Frame Rate!\n");
                                    break;
                                case 1:
                                    frameratenr = 24000.0;
                                    break;
                                case 2:
                                    frameratenr = 25000.0;
                                    break;
                                case 3:
                                    frameratenr = 30000.0;
                                    break;
                                case 4:
                                    frameratenr = 50000.0;
                                    break;
                                case 5:
                                    frameratenr = 60000.0;
                                    break;
                                case 6:
                                    frameratenr = 48000.0;
                                    break;
                                case 7:
                                    frameratenr = 72000.0;
                                    break;
                                default:
                                    printf("Reserved Frame Rate!\n");
                                    break;
                            }
                            switch ((parse & 0x003c00) >> 10) {
                                case 0:
                                    printf("Forbidden Frame Rate!\n");
                                    break;
                                case 1:
                                    frameratedr = 1000.0;
                                    break;
                                case 2:
                                    frameratedr = 1001.0;
                                    break;
                                default:
                                    printf("Reserved Frame Rate!\n");
                                    break;
                            }
                            frame_rate = frameratenr / frameratedr;
                            printf("Frame Rate = %.3f\n", (double)frame_rate);
                            first_sequence_dump = TRUE;
                        }
                        break;
                }
            }
        }
        else if (parse == 0x0000010a) {
            if (parse_only == FALSE) {
                if (i < 3) {
                    offset = 0 - (3 - i);
                    result = fseek(fpoutvideo, offset, SEEK_CUR);
                    whole_buffer = FALSE;
                    middle_es_ptr = es_ptr;
                    middle_length = length - (i + 1);
                }
                else {
                    fwrite(start_es_ptr, 1, i - 3, fpoutvideo);
                    whole_buffer = FALSE;
                    middle_es_ptr = es_ptr;
                    middle_length = length - i - 1;
                }
            }
        }
        picture_size++;
    }
    if (parse_only == FALSE) {
        if (first_sequence == TRUE) {
            if (whole_buffer == TRUE) {
                fwrite(start_es_ptr, 1, length, fpoutvideo);
            }
            else {
                fwrite(middle_es_ptr, 1, middle_length, fpoutvideo);
            }
        }
    }
}

unsigned int demux_audio = TRUE;

unsigned int sync_state = FALSE;
unsigned int xport_packet_length;
unsigned int xport_header_parse = 0;
unsigned int adaptation_field_state = FALSE;
unsigned int adaptation_field_parse = 0;
unsigned int adaptation_field_length = 0;
unsigned int pcr_parse = 0;
unsigned int pat_section_start = FALSE;
unsigned int pmt_section_start = FALSE;

typedef struct    psip
{
unsigned int psip_section_start;
unsigned int psip_pointer_field;
unsigned int psip_section_length_parse;
unsigned int psip_section_parse;
unsigned int psip_xfer_state;
unsigned short psip_section_length;
unsigned int psip_offset;
unsigned int psip_index;
unsigned char psip_table_id;
unsigned short psip_table_id_ext;
unsigned char psip_section_number, psip_last_section_number;
unsigned char psip_table[4096];
}psip_t;

psip_t *psip_ptr[0x2000];

unsigned int video_packet_length_parse = 0;
unsigned int video_packet_parse = 0;
unsigned int video_pts_parse = 0;
unsigned int video_pts_dts_parse = 0;
unsigned int video_xfer_state = FALSE;
unsigned int video_packet_number = 0;
unsigned char video_pes_header_length = 0;

unsigned int audio_packet_length_parse = 0;
unsigned int audio_packet_parse = 0;
unsigned int audio_pts_parse = 0;
unsigned int audio_pts_dts_parse = 0;
unsigned int audio_lpcm_parse = 0;
unsigned int audio_xfer_state = FALSE;
unsigned int audio_packet_number = 0;
unsigned char audio_pes_header_length = 0;

unsigned int pat_pointer_field = 0;
unsigned int pat_section_length_parse = 0;
unsigned int pat_section_parse = 0;
unsigned int pat_xfer_state = FALSE;

unsigned int pmt_pointer_field = 0;
unsigned int pmt_section_length_parse = 0;
unsigned int pmt_section_parse = 0;
unsigned int pmt_xfer_state = FALSE;

unsigned int pmt_program_descriptor_length_parse = 0;
unsigned int pmt_program_descriptor_length = 0;

unsigned int pmt_ES_descriptor_length_parse = 0;
unsigned int pmt_ES_descriptor_length = 0;

unsigned long long previous_pcr = 0;
unsigned long long pcr_bytes = 0;

unsigned long long prev_video_dts = 0;
unsigned long long video_pts_count = 0;
unsigned long long prev_audio_pts = 0;

unsigned char continuity_counter[0x2000];

unsigned int skipped_bytes = 0;
unsigned int tp_extra_header_parse = 4;

unsigned int first_pat = TRUE;
unsigned int first_pmt = TRUE;

void    demux_mpeg2_transport_init(void)
{
    int    i;

    sync_state = FALSE;
    xport_header_parse = 0;
    adaptation_field_state = FALSE;
    adaptation_field_parse = 0;
    adaptation_field_length = 0;
    pcr_parse = 0;
    pat_section_start = FALSE;
    pmt_section_start = FALSE;

    video_packet_length_parse = 0;
    video_packet_parse = 0;
    video_pts_parse = 0;
    video_pts_dts_parse = 0;
    video_xfer_state = FALSE;
    video_packet_number = 0;
    video_pes_header_length = 0;

    audio_packet_length_parse = 0;
    audio_packet_parse = 0;
    audio_pts_parse = 0;
    audio_pts_dts_parse = 0;
    audio_lpcm_parse = 0;
    audio_xfer_state = FALSE;
    audio_packet_number = 0;
    audio_pes_header_length = 0;

    pat_pointer_field = 0;
    pat_section_length_parse = 0;
    pat_section_parse = 0;
    pat_xfer_state = FALSE;

    pmt_program_descriptor_length_parse = 0;
    pmt_program_descriptor_length = 0;

    pmt_ES_descriptor_length_parse = 0;
    pmt_ES_descriptor_length = 0;

    pmt_pointer_field = 0;
    pmt_section_length_parse = 0;
    pmt_section_parse = 0;
    pmt_xfer_state = FALSE;

    psip_ptr[0x1ffb] = malloc(sizeof(struct psip));
    psip_ptr[0x1ffb]->psip_section_start = FALSE;
    psip_ptr[0x1ffb]->psip_pointer_field = 0;
    psip_ptr[0x1ffb]->psip_section_length_parse = 0;
    psip_ptr[0x1ffb]->psip_section_parse = 0;
    psip_ptr[0x1ffb]->psip_xfer_state = FALSE;

    for (i = 0; i < 0x2000; i++) {
        continuity_counter[i] = 0xff;
        pid_counter[i] = 0;
        pid_first_packet[i] = 0;
    }

    tp_extra_header_parse = 4;
}

void    demux_mpeg2_transport(unsigned int length, unsigned char *buffer)
{
    unsigned int i, j, k, m, n, q, xfer_length;
    unsigned int video_channel_count, audio_channel_count;
    unsigned long long ts_rate, pcr_ext;
    unsigned long long pcrsave;
    unsigned char sync, temp;
    unsigned short temp_program_map_pid;

    unsigned int tp_extra_header;
    unsigned long long tp_extra_header_pcr_bytes;
    static unsigned int tp_extra_header_prev = 0;

    static unsigned int video_parse;
    static unsigned int video_packet_length;
    static unsigned long long video_temp_pts;
    static unsigned long long video_temp_dts;
    static unsigned long long video_pts;
    static unsigned char video_pes_header_flags;
    static unsigned int video_dts;

    static unsigned int audio_parse;
    static unsigned short audio_packet_length;
    static unsigned long long audio_temp_pts;
    static unsigned long long audio_pts;
    static unsigned char audio_pes_header_flags;
    static unsigned short audio_lpcm_header_flags;

    static unsigned short pat_section_length;
    static unsigned int pat_offset;

    static unsigned short pmt_section_length;
    static unsigned int pmt_offset;

    static unsigned char mgt_version_number;
    static unsigned char mgt_last_version_number = 0xff;
    static unsigned short mgt_tables_defined;
    static unsigned short mgt_table_type;
    static unsigned short mgt_table_type_pid;
    static unsigned short mgt_table_type_version;
    static unsigned int mgt_number_bytes;
    static unsigned short mgt_table_type_desc_length;
    static unsigned short mgt_desc_length;
    static unsigned int mgt_crc;

    static unsigned short ett_pid = 0xffff;
    static unsigned short eit0_pid = 0xffff;
    static unsigned short eit1_pid = 0xffff;
    static unsigned short eit2_pid = 0xffff;
    static unsigned short eit3_pid = 0xffff;
    static unsigned short ett0_pid = 0xffff;
    static unsigned short ett1_pid = 0xffff;
    static unsigned short ett2_pid = 0xffff;
    static unsigned short ett3_pid = 0xffff;

    static unsigned char vct_version_number;
    static unsigned char vct_last_version_number = 0xff;
    static unsigned char vct_num_channels;
    static unsigned short vct_major_channel_number;
    static unsigned short vct_minor_channel_number;
    static unsigned char vct_modulation_mode;
    static unsigned short vct_channel_tsid;
    static unsigned short vct_program_number;
    static unsigned char vct_service_type;
    static unsigned short vct_source_id;
    static unsigned short vct_desc_length;
    static unsigned short vct_add_desc_length;
    static unsigned int vct_crc;

    static unsigned char sld_desc_length;
    static unsigned short sld_pcr_pid;
    static unsigned char sld_num_elements;
    static unsigned char sld_stream_type;
    static unsigned short sld_elementary_pid;

    static unsigned char ecnd_desc_length;
    static unsigned char ac3_desc_length;
    static unsigned char csd_desc_length;
    static unsigned char cad_desc_length;
    static unsigned char rcd_desc_length;

    static unsigned char eit_version_number;
    static unsigned char eit_last_version_number[4] = {0xff, 0xff, 0xff, 0xff};
    static unsigned char eit_num_events;
    static unsigned short eit_event_id;
    static unsigned int eit_start_time;
    static unsigned int eit_length_secs;
    static unsigned char eit_title_length;
    static unsigned short eit_desc_length;

    static unsigned char transport_error_indicator, payload_unit_start_indicator;
    static unsigned char transport_priority, transport_scrambling_control;
    static unsigned char adaptation_field_control;
    static unsigned short pid;
    static unsigned short program_map_pid = 0xffff;
    static unsigned short transport_stream_id = 0xffff;
    static unsigned short program_number;
    static unsigned char pat_section_number, pat_last_section_number;
    static unsigned char pmt_section_number, pmt_last_section_number;
    static unsigned char pmt_stream_type;
    static unsigned short pmt_elementary_pid;
    static unsigned short pmt_program_info_length;
    static unsigned short pmt_ES_info_length;
    static unsigned char program_association_table[1024];
    static unsigned char program_map_table[1024];
    static unsigned short psip_pid_table[0x2000];
    static unsigned char video_pes_header[256 + 9];
    static unsigned char audio_pes_header[256 + 9];
    static unsigned char video_pes_header_index = 0;
    static unsigned char audio_pes_header_index = 0;
    static unsigned int first_audio_access_unit = FALSE;
    static unsigned long long pcr;

    if (1) {
        for (i = 0; i < length; i++) {
            if (sync_state == TRUE) {
                if (xport_header_parse != 0) {
                    --xport_packet_length;
                    pcr_bytes++;
                    --xport_header_parse;
                    switch (xport_header_parse) {
                        case 2:
                            temp = buffer[i];
                            transport_error_indicator = (temp >> 7) & 0x1;
                            payload_unit_start_indicator = (temp >> 6) & 0x1;
                            transport_priority = (temp >> 5) & 0x1;
                            pid = (temp & 0x1f) << 8;
                            break;
                        case 1:
                            temp = buffer[i];
                            pid |= temp;
                            packet_counter++;
                            if (transport_error_indicator == 0) {
                                pid_counter[pid]++;
                                if (pid_first_packet[pid] == 0) {
                                    pid_first_packet[pid] = packet_counter;
                                }
                                pid_last_packet[pid] = packet_counter;
                            }
                            if (dump_pids == TRUE) {
                                printf("  PID=%4x", pid);
                            }
                            break;
                        case 0:
                            temp = buffer[i];
                            transport_scrambling_control = (temp >> 6) & 0x3;
                            adaptation_field_control = (temp >> 4) & 0x3;
                            if (((continuity_counter[pid] + 1) & 0xf) != (temp & 0xf)) {
                                if (adaptation_field_control & 0x1 && pid != 0x1fff) {
                                    if (continuity_counter[pid] != 0xff) {
#if 1
                                        printf("Discontinuity!, pid = %d <0x%04x>, received = %2d, expected = %2d, at %lld\n", pid, pid, (temp & 0xf), (continuity_counter[pid] + 1) & 0xf, packet_counter);
#endif
                                    }
                                }
                            }
                            if (adaptation_field_control & 0x1 && pid) {
                                continuity_counter[pid] = temp & 0xf;
                            }
                            if ((adaptation_field_control & 0x2) == 0x2) {
                                adaptation_field_state = TRUE;
                            }
                            if (pid == 0 && payload_unit_start_indicator == 1) {
                                pat_section_start = TRUE;
                            }
                            if (pid == program_map_pid && payload_unit_start_indicator == 1) {
                                pmt_section_start = TRUE;
                            }
                            if (dump_psip == TRUE) {
                                if ((pid == 0x1ffb || pid == ett_pid || pid == eit0_pid || pid == eit1_pid || pid == eit2_pid || pid == eit3_pid || pid == ett0_pid || pid == ett1_pid || pid == eit2_pid || pid == eit3_pid) && payload_unit_start_indicator == 1) {
                                    psip_ptr[pid]->psip_section_start = TRUE;
                                }
                            }
                            if (pid == video_pid && payload_unit_start_indicator == 1) {
                                video_xfer_state = FALSE;
                            }
                            break;
                    }
                }
                else if (adaptation_field_state == TRUE) {
                    --xport_packet_length;
                    pcr_bytes++;
                    adaptation_field_parse = buffer[i];
                    adaptation_field_length = adaptation_field_parse;
#if 0
                    printf("Adaptation field length = %d\n", adaptation_field_length);
#endif
                    adaptation_field_state = FALSE;
                }
                else if (adaptation_field_parse != 0) {
                    --xport_packet_length;
                    pcr_bytes++;
                    --adaptation_field_parse;
                    if ((adaptation_field_length - adaptation_field_parse) == 1) {
                        if ((buffer[i] & 0x10) == 0x10) {
                            pcr_parse = 6;
                            pcr = 0;
#if 0
                            printf("Adaptation field length = %d\n", adaptation_field_length);
#endif
                        }
                    }
                    else if (pcr_parse != 0) {
                        --pcr_parse;
                        pcr = (pcr << 8) + buffer[i];
                        if (pcr_parse == 0 && pid == pcr_pid) {
                            pcr_ext = pcr & 0x1ff;
                            if (dump_pcr) {
                                printf("pcr = %d at packet number %lld\n", (unsigned int)(pcr >> 15), packet_counter);
                            }
                            pcr = (pcr >> 15) * 300;
                            pcr = pcr + pcr_ext;
                            pcrsave = pcr;
                            if (pcr < previous_pcr) {
                                pcr = pcr + (((long long)1) << 42);
                            }
                            if (pcr - previous_pcr != 0) {
                                if (suppress_tsrate == FALSE) {
                                    if (hdmv_mode == TRUE) {
                                        if (((pcr & 0x3fffffff) - tp_extra_header) == 0) {
                                            if (running_average_bitrate != 0) {
                                                printf("ts rate = unspecified, video rate = %9d, peak video rate = %9d\r", running_average_bitrate, running_average_bitrate_peak);
                                            }
                                            else {
                                                printf("ts rate = unspecified\r");
                                            }
                                        }
                                        else {
                                            ts_rate = ((((pcr_bytes - 2) - tp_extra_header_pcr_bytes) * 27000000) / ((pcr & 0x3fffffff) - tp_extra_header));
                                            if (running_average_bitrate != 0) {
                                                printf("ts rate = %9d, video rate = %9d, peak video rate = %9d\r", (unsigned int)ts_rate * 8, running_average_bitrate, running_average_bitrate_peak);
                                            }
                                            else {
                                                printf("ts rate = %9d\r", (unsigned int)ts_rate * 8);
                                            }
                                        }
                                    }
                                    else {
                                        ts_rate = ((pcr_bytes * 27000000) / (pcr - previous_pcr));
                                        if (running_average_bitrate != 0) {
                                            printf("ts rate = %9d, video rate = %9d, peak video rate = %9d\r", (unsigned int)ts_rate * 8, running_average_bitrate, running_average_bitrate_peak);
                                        }
                                        else {
                                            printf("ts rate = %9d\r", (unsigned int)ts_rate * 8);
                                        }
                                    }
                                }
                            }
                            previous_pcr = pcrsave;
                            pcr_bytes = 0;
                        } 
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else if (pid == 0) {
                    if (pat_xfer_state == TRUE) {
                        if ((length - i) >= pat_section_length) {
                            j = pat_section_length;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        else {
                            j = length - i;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
#if 0
                        printf("Burst length = %d\r\n", j);
#endif
                        for (k = 0; k < j; k++) {
                            program_association_table[pat_offset] = buffer[i];
#if 0
                            printf("PAT byte %d = 0x%02x\n", pat_offset, buffer[i]);
#endif
                            pat_offset++;
                            i++;
                            --pat_section_length;
                            --xport_packet_length;
                            pcr_bytes++;
                        }
                        --i;    /* adjust because of for loop */
                        if (pat_section_length == 0) {
#if 0
                            printf("End of PSI section = %d\r\n",i);
#endif
                            if (dump_index == TRUE) {
                                printf("PAT at packet number %lld/%lld\r\n", packet_counter, (packet_counter - 1) * 188);
                            }
                            pat_xfer_state = FALSE;
                            if (pat_section_number == pat_last_section_number) {
                                for (k = 0; k < (pat_offset - 4); k+=4) {
                                    program_number = program_association_table[k] << 8;
                                    program_number |= program_association_table[k + 1];
                                    if (first_pat == TRUE) {
                                        temp_program_map_pid = (program_association_table[k + 2] & 0x1f) << 8;
                                        temp_program_map_pid |= program_association_table[k + 3];
                                        printf("Program Number = %d (0x%04x), Program Map PID = %d (0x%04x)\n", program_number, program_number, temp_program_map_pid, temp_program_map_pid);
                                    }
                                    if (program_number == program) {
                                        program_map_pid = (program_association_table[k + 2] & 0x1f) << 8;
                                        program_map_pid |= program_association_table[k + 3];
#if 0
                                        printf("Program Map PID = %d\r\n",program_map_pid);
#endif
                                    }
                                }
                                first_pat = FALSE;
                            }
                        }
                    }
                    else {
                        --xport_packet_length;
                        pcr_bytes++;
                        if (pat_section_start == TRUE) {
                            pat_pointer_field = buffer[i];
                            if (pat_pointer_field == 0) {
                                pat_section_length_parse = 3;
                            }
                            pat_section_start = FALSE;
                        }
                        else if (pat_pointer_field != 0) {
                            --pat_pointer_field;
                            switch (pat_pointer_field) {
                                case 0:
                                    pat_section_length_parse = 3;
                                    break;
                            }
                        }
                        else if (pat_section_length_parse != 0) {
                            --pat_section_length_parse;
                            switch (pat_section_length_parse) {
                                case 2:
                                    break;
                                case 1:
                                    pat_section_length = (buffer[i] & 0xf) << 8;
                                    break;
                                case 0:
                                    pat_section_length |= buffer[i];
#if 0
                                    printf("Section length = %d\r\n", pat_section_length);
#endif
                                    if (pat_section_length > 1021) {
                                        printf("PAT Section length = %d\r\n", pat_section_length);
                                        pat_section_length = 0;
                                    }
                                    else {
                                        pat_section_parse = 5;
                                    }
                                    break;
                            }
                        }
                        else if (pat_section_parse != 0) {
                            --pat_section_length;
                            --pat_section_parse;
                            switch (pat_section_parse) {
                                case 4:
                                    transport_stream_id = buffer[i] << 8;
                                    break;
                                case 3:
                                    transport_stream_id |= buffer[i];
                                    break;
                                case 2:
                                    break;
                                case 1:
                                    pat_section_number = buffer[i];
                                    if (pat_section_number == 0) {
                                        pat_offset = 0;
                                    }
                                    break;
                                case 0:
                                    pat_last_section_number = buffer[i];
                                    pat_xfer_state = TRUE;
                                    break;
                            }
                        }
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else if (pid == program_map_pid) {
                    if (pmt_xfer_state == TRUE) {
                        if ((length - i) >= pmt_section_length) {
                            j = pmt_section_length;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        else {
                            j = length - i;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
#if 0
                        printf("Burst length = %d\r\n", j);
#endif
                        for (k = 0; k < j; k++) {
                            program_map_table[pmt_offset] = buffer[i];
#if 0
                            printf("PMT byte %d = 0x%02x\n", pmt_offset, buffer[i]);
#endif
                            pmt_offset++;
                            i++;
                            --pmt_section_length;
                            --xport_packet_length;
                            pcr_bytes++;
                        }
                        --i;    /* adjust because of for loop */
                        if (pmt_section_length == 0) {
#if 0
                            printf("End of PSI section = %d\r\n",i);
#endif
                            pmt_xfer_state = FALSE;
                            if (pmt_section_number == pmt_last_section_number) {
                                video_channel_count = 0;
                                audio_channel_count = 0;
                                for (k = 0; k < (pmt_offset - 4); k+=5) {
                                    pmt_stream_type = program_map_table[k];
                                    pmt_elementary_pid = (program_map_table[k+1] & 0x1f) << 8;
                                    pmt_elementary_pid |= program_map_table[k+2];
                                    if (pmt_stream_type == 0x1 || pmt_stream_type == 0x2 || (pmt_stream_type == 0x80 && hdmv_mode == FALSE) || pmt_stream_type == 0x1b || pmt_stream_type == 0xea) {
                                        video_channel_count++;
                                        if (video_channel_count == video_channel) {
                                            video_pid = pmt_elementary_pid;
                                            if (first_pmt == TRUE) {
                                                printf("Video PID = %4d <0x%04x>, type = 0x%02x\r\n", video_pid, video_pid, pmt_stream_type);
                                            }
                                            video_stream_type = pmt_stream_type;
                                        }
                                    }
                                    else if (pmt_stream_type == 0x3 || pmt_stream_type == 0x4 || pmt_stream_type == 0x80 || pmt_stream_type == 0x81 || pmt_stream_type == 0x6 || pmt_stream_type == 0x82 || pmt_stream_type == 0x83 || pmt_stream_type == 0x84 || pmt_stream_type == 0x85 || pmt_stream_type == 0x86 || pmt_stream_type == 0xa1 || pmt_stream_type == 0xa2 || pmt_stream_type == 0x11) {
                                        audio_channel_count++;
                                        if (audio_channel_count == audio_channel) {
                                            audio_pid = pmt_elementary_pid;
                                            if (first_pmt == TRUE) {
                                                printf("Audio PID = %4d <0x%04x>, type = 0x%02x\r\n", audio_pid, audio_pid, pmt_stream_type);
                                            }
                                            audio_stream_type = pmt_stream_type;
                                        }
                                    }
                                    pmt_ES_info_length = (program_map_table[k+3] & 0xf) << 8;
                                    pmt_ES_info_length |= program_map_table[k+4];
                                    if (pmt_ES_info_length != 0) {
                                        pmt_ES_descriptor_length_parse = 2;
                                        for (q = 0; q < pmt_ES_info_length; q++) {
                                            if (pmt_ES_descriptor_length_parse != 0) {
                                                --pmt_ES_descriptor_length_parse;
                                                switch (pmt_ES_descriptor_length_parse) {
                                                    case 1:
                                                        if (first_pmt == TRUE) {
                                                            printf("ES descriptor for stream type 0x%02x = 0x%02x", pmt_stream_type, program_map_table[k+5+q]);
                                                        }
                                                        break;
                                                    case 0:
                                                        pmt_ES_descriptor_length = program_map_table[k+5+q];
                                                        if (first_pmt == TRUE) {
                                                            printf(", 0x%02x", program_map_table[k+5+q]);
                                                            if (pmt_ES_descriptor_length == 0) {
                                                                printf("\n");
                                                            }
                                                        }
                                                        break;
                                                }
                                            }
                                            else if (pmt_ES_descriptor_length != 0) {
                                                --pmt_ES_descriptor_length;
                                                if (first_pmt == TRUE) {
                                                    printf(", 0x%02x", program_map_table[k+5+q]);
                                                }
                                                if (pmt_ES_descriptor_length == 0) {
                                                    if (first_pmt == TRUE) {
                                                        printf("\n");
                                                    }
                                                    if (q < pmt_ES_info_length) {
                                                        pmt_ES_descriptor_length_parse = 2;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    k += pmt_ES_info_length;
                                }
                                first_pmt = FALSE;
                            }
                        }
                    }
                    else {
                        --xport_packet_length;
                        pcr_bytes++;
                        if (pmt_section_start == TRUE) {
                            pmt_pointer_field = buffer[i];
                            if (pmt_pointer_field == 0) {
                                pmt_section_length_parse = 3;
                            }
                            pmt_section_start = FALSE;
                        }
                        else if (pmt_pointer_field != 0) {
                            --pmt_pointer_field;
                            switch (pmt_pointer_field) {
                                case 0:
                                    pmt_section_length_parse = 3;
                                    break;
                            }
                        }
                        else if (pmt_section_length_parse != 0) {
                            --pmt_section_length_parse;
                            switch (pmt_section_length_parse) {
                                case 2:
                                    if (buffer[i] != 0x2) {
                                        pmt_section_length_parse = 0;
                                    }
                                    break;
                                case 1:
                                    pmt_section_length = (buffer[i] & 0xf) << 8;
                                    break;
                                case 0:
                                    pmt_section_length |= buffer[i];
#if 0
                                    printf("Section length = %d\r\n", pmt_section_length);
#endif
                                    if (pmt_section_length > 1021) {
                                        printf("PMT Section length = %d\r\n", pmt_section_length);
                                        pmt_section_length = 0;
                                    }
                                    else {
                                        pmt_section_parse = 9;
                                    }
                                    break;
                            }
                        }
                        else if (pmt_section_parse != 0) {
                            --pmt_section_length;
                            --pmt_section_parse;
                            switch (pmt_section_parse) {
                                case 8:
                                    break;
                                case 7:
                                    break;
                                case 6:
                                    break;
                                case 5:
                                    pmt_section_number = buffer[i];
                                    if (pmt_section_number == 0) {
                                        pmt_offset = 0;
                                    }
                                    break;
                                case 4:
                                    pmt_last_section_number = buffer[i];
                                    break;
                                case 3:
                                    pcr_pid = (buffer[i] & 0x1f) << 8;
                                    break;
                                case 2:
                                    pcr_pid |= buffer[i];
#if 0
                                    printf("PCR PID = %d\r\n", pcr_pid);
#endif
                                    break;
                                case 1:
                                    pmt_program_info_length = (buffer[i] & 0xf) << 8;
                                    break;
                                case 0:
                                    pmt_program_info_length |= buffer[i];
                                    if (pmt_program_info_length == 0) {
                                        pmt_xfer_state = TRUE;
                                    }
                                    else {
                                        pmt_program_descriptor_length_parse = 2;
                                    }
                                    break;
                            }
                        }
                        else if (pmt_program_info_length != 0) {
                            --pmt_section_length;
                            --pmt_program_info_length;
                            if (pmt_program_descriptor_length_parse != 0) {
                                --pmt_program_descriptor_length_parse;
                                switch (pmt_program_descriptor_length_parse) {
                                    case 1:
                                        if (first_pmt == TRUE) {
                                            printf("program descriptor = 0x%02x", buffer[i]);
                                        }
                                        break;
                                    case 0:
                                        pmt_program_descriptor_length = buffer[i];
                                        if (first_pmt == TRUE) {
                                            printf(", 0x%02x", buffer[i]);
                                            if (pmt_program_descriptor_length == 0) {
                                                printf("\n");
                                            }
                                        }
                                        break;
                                }
                            }
                            else if (pmt_program_descriptor_length != 0) {
                                --pmt_program_descriptor_length;
                                if (first_pmt == TRUE) {
                                    printf(", 0x%02x", buffer[i]);
                                }
                                if (pmt_program_descriptor_length == 0) {
                                    if (first_pmt == TRUE) {
                                        printf("\n");
                                    }
                                    if (pmt_program_info_length != 0) {
                                        pmt_program_descriptor_length_parse = 2;
                                    }
                                }
                            }
                            switch (pmt_program_info_length) {
                                case 0:
                                    pmt_xfer_state = TRUE;
                                    break;
                            }
                        }
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else if (pid == video_pid && transport_scrambling_control == 0) {
                    video_parse = (video_parse << 8) + buffer[i];
                    if (video_xfer_state == TRUE) {
                        if ((length - i) >= video_packet_length) {
                            j = video_packet_length;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        else {
                            j = length - i;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        xfer_length = j;
#if 0
                        printf("Burst length = %d\r\n", j);
#endif
                        if (video_stream_type == 0x1 || video_stream_type == 0x2 || video_stream_type == 0x80) {
                            parse_mpeg2_video(&buffer[i], xfer_length, video_pts, video_dts);
                        }
                        else if (video_stream_type == 0x1b) {
                            parse_h264_video(&buffer[i], xfer_length, video_pts, video_dts);
                        }
                        else if (video_stream_type == 0xea) {
                            parse_vc1_video(&buffer[i], xfer_length, video_pts, video_dts);
                        }
                        else {
                            if (parse_only == FALSE) {
                                fwrite(&buffer[i], 1, xfer_length, fpoutvideo);
                            }
                        }
                        i = i + xfer_length;
                        video_packet_length = video_packet_length - xfer_length;
                        xport_packet_length = xport_packet_length - xfer_length;
                        pcr_bytes = pcr_bytes + xfer_length;
                        --i;    /* adjust because of for loop */
                        if (video_packet_length == 0) {
#if 0
                            printf("End of Packet = %d\r\n",i);
#endif
                            video_xfer_state = FALSE;
                        }
                    }
                    else {
                        --xport_packet_length;
                        pcr_bytes++;
                        if ((video_parse >= 0x000001e0 && video_parse <= 0x000001ef) || video_parse == 0x000001fd) {
#if 0
                            printf("PES start code, Video Stream number = %d.\r\n", (video_parse & 0x0f));
#endif
                            video_packet_length_parse = 2;
                            video_packet_number++;
                            video_pes_header_index = 0;
                            video_pes_header[video_pes_header_index++] = (video_parse >> 24) & 0xff;
                            video_pes_header[video_pes_header_index++] = (video_parse >> 16) & 0xff;
                            video_pes_header[video_pes_header_index++] = (video_parse >> 8) & 0xff;
                            video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                        }
                        else if (video_packet_length_parse == 2) {
                            --video_packet_length_parse;
                            video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                        }
                        else if (video_packet_length_parse == 1) {
                            --video_packet_length_parse;
                            video_packet_length = video_parse & 0xffff;
                            if (video_packet_length == 0) {
                                video_packet_length = 0xffffffff;
                            }
#if 0
                            printf("Packet length = %d\r\n", video_packet_length);
#endif
                            video_packet_parse = 3;
                            video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                        }
                        else if (video_packet_parse != 0) {
                            --video_packet_length;
                            --video_packet_parse;
                            switch (video_packet_parse) {
                                case 2:
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 1:
                                    video_pes_header_flags = video_parse & 0xff;
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 0:
                                    video_pes_header_length = video_parse & 0xff;
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
#if 0
                                    printf("Video PES header length = %d\r\n", video_pes_header_length);
#endif
                                    if ((video_pes_header_flags & 0xc0) == 0x80) {
                                        video_pts_parse = 5;
                                    }
                                    else if ((video_pes_header_flags & 0xc0) == 0xc0) {
                                        video_pts_dts_parse = 10;
                                    }
                                    if (video_pes_header_length == 0) {
                                        video_xfer_state = TRUE;
                                        if (parse_only == FALSE && pes_streams == TRUE) {
                                            fwrite(&video_pes_header[0], 1, video_pes_header_index, fpoutvideo);
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (video_pts_parse != 0) {
                            --video_packet_length;
                            --video_pes_header_length;
                            --video_pts_parse;
                            switch (video_pts_parse) {
                                case 4:
                                    video_temp_pts = 0;
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xe) << 29);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 3:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xff) << 22);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 2:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xfe) << 14);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 1:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xff) << 7);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 0:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xfe) >> 1);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    video_pts = video_temp_pts;
                                    if (video_pts > last_video_pts) {
                                        last_video_pts = video_pts;
                                    }
                                    video_dts = 0;
                                    if (dump_video_pts == TRUE) {
                                        if (video_pts_count == 0) {
                                            printf("Video PTS(B) = %llu\n", video_pts);
                                        }
                                        else {
                                            printf("Video PTS(B) = %llu, %llu\n", video_pts, (video_pts - prev_video_dts));
                                        }
                                    }
                                    last_video_pts_diff = video_pts - prev_video_dts;
                                    prev_video_dts = video_pts;
                                    video_pts_count++;
                                    if (video_pes_header_length == 0) {
                                        video_xfer_state = TRUE;
                                        if (parse_only == FALSE && pes_streams == TRUE) {
                                            fwrite(&video_pes_header[0], 1, video_pes_header_index, fpoutvideo);
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (video_pts_dts_parse != 0) {
                            --video_packet_length;
                            --video_pes_header_length;
                            --video_pts_dts_parse;
                            switch (video_pts_dts_parse) {
                                case 9:
                                    video_temp_pts = 0;
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xe) << 29);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 8:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xff) << 22);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 7:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xfe) << 14);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 6:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xff) << 7);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 5:
                                    video_temp_pts = video_temp_pts | ((video_parse & 0xfe) >> 1);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    video_pts = video_temp_pts;
                                    if (video_pts > last_video_pts) {
                                        last_video_pts = video_pts;
                                    }
                                    video_dts = 1;
                                    break;
                                case 4:
                                    video_temp_dts = 0;
                                    video_temp_dts = video_temp_dts | ((video_parse & 0xe) << 29);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 3:
                                    video_temp_dts = video_temp_dts | ((video_parse & 0xff) << 22);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 2:
                                    video_temp_dts = video_temp_dts | ((video_parse & 0xfe) << 14);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 1:
                                    video_temp_dts = video_temp_dts | ((video_parse & 0xff) << 7);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    break;
                                case 0:
                                    video_temp_dts = video_temp_dts | ((video_parse & 0xfe) >> 1);
                                    video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                                    if (dump_video_pts == TRUE) {
                                        if (video_pts_count == 0) {
                                            printf("Video PTS(P) = %llu, DTS(P) = %llu, %llu\n", video_pts, video_temp_dts, (video_pts - video_temp_dts));
                                        }
                                        else {
                                            printf("Video PTS(P) = %llu, DTS(P) = %llu, %llu, %llu\n", video_pts, video_temp_dts, (video_temp_dts - prev_video_dts), (video_pts - video_temp_dts));
                                        }
                                    }
                                    last_video_pts_diff = video_temp_dts - prev_video_dts;
                                    prev_video_dts = video_temp_dts;
                                    video_pts_count++;
                                    if (video_pes_header_length == 0) {
                                        video_xfer_state = TRUE;
                                        if (parse_only == FALSE && pes_streams == TRUE) {
                                            fwrite(&video_pes_header[0], 1, video_pes_header_index, fpoutvideo);
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (video_pes_header_length != 0) {
                            --video_packet_length;
                            --video_pes_header_length;
                            video_pes_header[video_pes_header_index++] = video_parse & 0xff;
                            switch (video_pes_header_length) {
                                case 0:
                                    video_xfer_state = TRUE;
                                    if (parse_only == FALSE && pes_streams == TRUE) {
                                        fwrite(&video_pes_header[0], 1, video_pes_header_index, fpoutvideo);
                                    }
                                    break;
                            }
                        }
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else if (pid == audio_pid && transport_scrambling_control == 0) {
                    audio_parse = (audio_parse << 8) + buffer[i];
                    if (audio_xfer_state == TRUE) {
                        if ((length - i) >= audio_packet_length) {
                            j = audio_packet_length;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        else {
                            j = length - i;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        xfer_length = j;
#if 0
                        printf("Burst length = %d\r\n", j);
#endif
                        if (demux_audio == TRUE) {
                            if (audio_stream_type == 0x81 || audio_stream_type == 0x6) {
                                parse_ac3_audio(&buffer[i], xfer_length, audio_pts, first_audio_access_unit);
                            }
                            else if (audio_stream_type == 0x3 || audio_stream_type == 0x4) {
                                parse_mp2_audio(&buffer[i], xfer_length, audio_pts, first_audio_access_unit);
                            }
                            else if (audio_stream_type == 0x80) {
                                parse_lpcm_audio(&buffer[i], xfer_length, audio_pts, first_audio_access_unit, audio_lpcm_header_flags);
                            }
                            else {
                                if (parse_only == FALSE) {
                                    fwrite(&buffer[i], 1, xfer_length, fpoutaudio);
                                }
                            }
                            first_audio_access_unit = FALSE;
                            i = i + xfer_length;
                            audio_packet_length = audio_packet_length - xfer_length;
                            xport_packet_length = xport_packet_length - xfer_length;
                            pcr_bytes = pcr_bytes + xfer_length;
                            --i;    /* adjust because of for loop */
                            if (audio_packet_length == 0) {
#if 0
                                printf("End of Packet = %d\r\n",i);
#endif
                                audio_xfer_state = FALSE;
                            }
                        }
                        else {
                            --xport_packet_length;
                            pcr_bytes++;
                            if ((length - i) >= xport_packet_length) {
                                i = i + xport_packet_length;
                                pcr_bytes = pcr_bytes + xport_packet_length;
                                xport_packet_length = 0;
                            }
                            else {
                                xport_packet_length = xport_packet_length - (length - i) + 1;
                                pcr_bytes = pcr_bytes + (length - i) - 1;
                                i = length;
                            }
                            audio_packet_length = audio_packet_length - j;
                            if (audio_packet_length == 0) {
                                audio_xfer_state = FALSE;
                            }
                        }
                    }
                    else {
                        --xport_packet_length;
                        pcr_bytes++;
                        if (((audio_parse >= 0x000001c0 && audio_parse <= 0x000001df) && (audio_stream_type == 0x3 || audio_stream_type == 0x4 || audio_stream_type == 0x6)) || audio_parse == 0x000001bd || audio_parse == 0x000001fa) {
#if 0
                            printf("PES start code = 0x%08x, stream_type = 0x%02x\r\n", audio_parse, audio_stream_type);
#endif
                            audio_packet_length_parse = 2;
                            audio_packet_number++;
                            audio_pes_header_index = 0;
                            audio_pes_header[audio_pes_header_index++] = (audio_parse >> 24) & 0xff;
                            audio_pes_header[audio_pes_header_index++] = (audio_parse >> 16) & 0xff;
                            audio_pes_header[audio_pes_header_index++] = (audio_parse >> 8) & 0xff;
                            audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                        }
                        else if (audio_packet_length_parse == 2) {
                            --audio_packet_length_parse;
                            audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                        }
                        else if (audio_packet_length_parse == 1) {
                            --audio_packet_length_parse;
                            audio_packet_length = audio_parse & 0xffff;
#if 0
                            printf("Packet length = %d\r\n", audio_packet_length);
#endif
                            audio_packet_parse = 3;
                            audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                        }
                        else if (audio_packet_parse != 0) {
                            --audio_packet_length;
                            --audio_packet_parse;
                            switch (audio_packet_parse) {
                                case 2:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 1:
                                    audio_pes_header_flags = audio_parse & 0xff;
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 0:
                                    audio_pes_header_length = audio_parse & 0xff;
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
#if 0
                                    printf("Audio PES header length = %d\r\n", audio_pes_header_length);
#endif
                                    if ((audio_pes_header_flags & 0xc0) == 0x80) {
                                        audio_pts_parse = 5;
                                    }
                                    else if ((audio_pes_header_flags & 0xc0) == 0xc0) {
                                        audio_pts_dts_parse = 10;
                                    }
                                    if (audio_pes_header_length == 0) {
                                        audio_xfer_state = TRUE;
                                        if (parse_only == FALSE && pes_streams == TRUE) {
                                            fwrite(&audio_pes_header[0], 1, audio_pes_header_index, fpoutaudio);
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (audio_pts_parse != 0) {
                            --audio_packet_length;
                            --audio_pes_header_length;
                            --audio_pts_parse;
                            switch (audio_pts_parse) {
                                case 4:
                                    audio_temp_pts = 0;
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xe) << 29);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 3:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xff) << 22);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 2:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xfe) << 14);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 1:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xff) << 7);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 0:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xfe) >> 1);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    audio_pts = audio_temp_pts;
                                    if (audio_pts > last_audio_pts) {
                                        last_audio_pts = audio_pts;
                                    }
                                    first_audio_access_unit = TRUE;
                                    if (dump_audio_pts == TRUE) {
                                        printf("Audio PTS = %llu, %llu\r\n", audio_pts, (audio_pts - prev_audio_pts));
                                    }
                                    last_audio_pts_diff = audio_pts - prev_audio_pts;
                                    prev_audio_pts = audio_pts;
                                    if (audio_pes_header_length == 0) {
                                        if (audio_stream_type == 0x80) {
                                            audio_lpcm_parse = 4;
                                        }
                                        else {
                                            audio_xfer_state = TRUE;
                                            if (parse_only == FALSE && pes_streams == TRUE) {
                                                fwrite(&audio_pes_header[0], 1, audio_pes_header_index, fpoutaudio);
                                            }
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (audio_pts_dts_parse != 0) {
                            --audio_packet_length;
                            --audio_pes_header_length;
                            --audio_pts_dts_parse;
                            switch (audio_pts_dts_parse) {
                                case 9:
                                    audio_temp_pts = 0;
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xe) << 29);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 8:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xff) << 22);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 7:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xfe) << 14);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 6:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xff) << 7);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 5:
                                    audio_temp_pts = audio_temp_pts | ((audio_parse & 0xfe) >> 1);
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    audio_pts = audio_temp_pts;
                                    if (audio_pts > last_audio_pts) {
                                        last_audio_pts = audio_pts;
                                    }
                                    first_audio_access_unit = TRUE;
                                    last_audio_pts_diff = audio_pts - prev_audio_pts;
                                    prev_audio_pts = audio_pts;
#if 0
                                    printf("Audio PTS(DTS) = %d\r\n", audio_pts);
#endif
                                    break;
                                case 4:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 3:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 2:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 1:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    break;
                                case 0:
                                    audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
                                    if (audio_pes_header_length == 0) {
                                        audio_xfer_state = TRUE;
                                        if (parse_only == FALSE && pes_streams == TRUE) {
                                            fwrite(&audio_pes_header[0], 1, audio_pes_header_index, fpoutaudio);
                                        }
                                    }
                                    break;
                            }
                        }
                        else if (audio_lpcm_parse != 0) {
                            --audio_packet_length;
                            --audio_lpcm_parse;
                            switch (audio_lpcm_parse) {
                                case 3:
                                    break;
                                case 2:
                                    break;
                                case 1:
                                    break;
                                case 0:
                                    audio_lpcm_header_flags = audio_parse & 0xffff;
                                    audio_xfer_state = TRUE;
                                    if (parse_only == FALSE && pes_streams == TRUE) {
                                        fwrite(&audio_pes_header[0], 1, audio_pes_header_index, fpoutaudio);
                                    }
                                    break;
                            }
                        }
                        else if (audio_pes_header_length != 0) {
                            --audio_packet_length;
                            --audio_pes_header_length;
                            audio_pes_header[audio_pes_header_index++] = audio_parse & 0xff;
#if 0
                            audio_demux_ptr[audio_demux_buffer_index] = 0xff;
                            audio_demux_buffer_index = (audio_demux_buffer_index + 1) & audio_buf_size;
#endif
                            switch (audio_pes_header_length) {
                                case 0:
                                    audio_xfer_state = TRUE;
                                    if (parse_only == FALSE && pes_streams == TRUE) {
                                        fwrite(&audio_pes_header[0], 1, audio_pes_header_index, fpoutaudio);
                                    }
                                    break;
                            }
                        }
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else if (pid == 0x1ffb || pid == ett_pid || pid == eit0_pid || pid == eit1_pid || pid == eit2_pid || pid == eit3_pid || pid == ett0_pid || pid == ett1_pid || pid == eit2_pid || pid == eit3_pid) {
                    if (psip_ptr[pid]->psip_xfer_state == TRUE) {
                        if ((length - i) >= psip_ptr[pid]->psip_section_length) {
                            j = psip_ptr[pid]->psip_section_length;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
                        else {
                            j = length - i;
                            if (xport_packet_length <= j) {
                                j = xport_packet_length;
                            }
                        }
#if 0
                        printf("Burst length = %d\r\n", j);
#endif
                        for (k = 0; k < j; k++) {
                            psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_offset] = buffer[i];
#if 0
                            printf("PSIP byte %d = 0x%02x\n", psip_ptr[pid]->psip_offset, buffer[i]);
#endif
                            psip_ptr[pid]->psip_offset++;
                            i++;
                            --psip_ptr[pid]->psip_section_length;
                            --xport_packet_length;
                            pcr_bytes++;
                        }
                        --i;    /* adjust because of for loop */
                        if (psip_ptr[pid]->psip_section_length == 0) {
#if 0
                            printf("End of PSIP section = %d\r\n",i);
#endif
                            psip_ptr[pid]->psip_xfer_state = FALSE;
                            if (psip_ptr[pid]->psip_section_number == psip_ptr[pid]->psip_last_section_number) {
                                if (psip_ptr[pid]->psip_table_id == 0xc7 && mgt_version_number != mgt_last_version_number) {
                                    mgt_last_version_number = mgt_version_number;
                                    psip_ptr[pid]->psip_index = 0;
                                    mgt_tables_defined = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                    mgt_tables_defined |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("MGT tables defined = %d\n\n", mgt_tables_defined);
                                    for (k = 0; k < mgt_tables_defined; k++) {
                                        mgt_table_type = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        mgt_table_type |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("MGT table type = 0x%04x\n", mgt_table_type);
                                        mgt_table_type_pid = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x1f) << 8;
                                        mgt_table_type_pid |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("MGT table type pid = 0x%04x\n", mgt_table_type_pid);
                                        if (mgt_table_type == 0x4) {
                                            ett_pid = mgt_table_type_pid;
                                            psip_pid_table[ett_pid] = mgt_table_type;
                                            psip_ptr[ett_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[ett_pid]->psip_section_start = FALSE;
                                            psip_ptr[ett_pid]->psip_pointer_field = 0;
                                            psip_ptr[ett_pid]->psip_section_length_parse = 0;
                                            psip_ptr[ett_pid]->psip_section_parse = 0;
                                            psip_ptr[ett_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x100) {
                                            eit0_pid = mgt_table_type_pid;
                                            psip_pid_table[eit0_pid] = mgt_table_type;
                                            psip_ptr[eit0_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[eit0_pid]->psip_section_start = FALSE;
                                            psip_ptr[eit0_pid]->psip_pointer_field = 0;
                                            psip_ptr[eit0_pid]->psip_section_length_parse = 0;
                                            psip_ptr[eit0_pid]->psip_section_parse = 0;
                                            psip_ptr[eit0_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x101) {
                                            eit1_pid = mgt_table_type_pid;
                                            psip_pid_table[eit1_pid] = mgt_table_type;
                                            psip_ptr[eit1_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[eit1_pid]->psip_section_start = FALSE;
                                            psip_ptr[eit1_pid]->psip_pointer_field = 0;
                                            psip_ptr[eit1_pid]->psip_section_length_parse = 0;
                                            psip_ptr[eit1_pid]->psip_section_parse = 0;
                                            psip_ptr[eit1_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x102) {
                                            eit2_pid = mgt_table_type_pid;
                                            psip_pid_table[eit2_pid] = mgt_table_type;
                                            psip_ptr[eit2_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[eit2_pid]->psip_section_start = FALSE;
                                            psip_ptr[eit2_pid]->psip_pointer_field = 0;
                                            psip_ptr[eit2_pid]->psip_section_length_parse = 0;
                                            psip_ptr[eit2_pid]->psip_section_parse = 0;
                                            psip_ptr[eit2_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x103) {
                                            eit3_pid = mgt_table_type_pid;
                                            psip_pid_table[eit3_pid] = mgt_table_type;
                                            psip_ptr[eit3_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[eit3_pid]->psip_section_start = FALSE;
                                            psip_ptr[eit3_pid]->psip_pointer_field = 0;
                                            psip_ptr[eit3_pid]->psip_section_length_parse = 0;
                                            psip_ptr[eit3_pid]->psip_section_parse = 0;
                                            psip_ptr[eit3_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x200) {
                                            ett0_pid = mgt_table_type_pid;
                                            psip_pid_table[ett0_pid] = mgt_table_type;
                                            psip_ptr[ett0_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[ett0_pid]->psip_section_start = FALSE;
                                            psip_ptr[ett0_pid]->psip_pointer_field = 0;
                                            psip_ptr[ett0_pid]->psip_section_length_parse = 0;
                                            psip_ptr[ett0_pid]->psip_section_parse = 0;
                                            psip_ptr[ett0_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x201) {
                                            ett1_pid = mgt_table_type_pid;
                                            psip_pid_table[ett1_pid] = mgt_table_type;
                                            psip_ptr[ett1_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[ett1_pid]->psip_section_start = FALSE;
                                            psip_ptr[ett1_pid]->psip_pointer_field = 0;
                                            psip_ptr[ett1_pid]->psip_section_length_parse = 0;
                                            psip_ptr[ett1_pid]->psip_section_parse = 0;
                                            psip_ptr[ett1_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x202) {
                                            ett2_pid = mgt_table_type_pid;
                                            psip_pid_table[ett2_pid] = mgt_table_type;
                                            psip_ptr[ett2_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[ett2_pid]->psip_section_start = FALSE;
                                            psip_ptr[ett2_pid]->psip_pointer_field = 0;
                                            psip_ptr[ett2_pid]->psip_section_length_parse = 0;
                                            psip_ptr[ett2_pid]->psip_section_parse = 0;
                                            psip_ptr[ett2_pid]->psip_xfer_state = FALSE;
                                        }
                                        else if (mgt_table_type == 0x203) {
                                            ett3_pid = mgt_table_type_pid;
                                            psip_pid_table[ett3_pid] = mgt_table_type;
                                            psip_ptr[ett3_pid] = malloc(sizeof(struct psip));
                                            psip_ptr[ett3_pid]->psip_section_start = FALSE;
                                            psip_ptr[ett3_pid]->psip_pointer_field = 0;
                                            psip_ptr[ett3_pid]->psip_section_length_parse = 0;
                                            psip_ptr[ett3_pid]->psip_section_parse = 0;
                                            psip_ptr[ett3_pid]->psip_xfer_state = FALSE;
                                        }
                                        mgt_table_type_version = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x1f;
                                        printf("MGT table type version = 0x%02x\n", mgt_table_type_version);
                                        mgt_number_bytes = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 24;
                                        mgt_number_bytes |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 16;
                                        mgt_number_bytes |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        mgt_number_bytes |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("MGT table bytes = 0x%08x\n", mgt_number_bytes);
                                        mgt_table_type_desc_length = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0xf) << 8;
                                        mgt_table_type_desc_length |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("MGT table desc bytes = 0x%04x\n\n", mgt_table_type_desc_length);
                                        for (m = 0; m < mgt_table_type_desc_length; m++) {
                                            psip_ptr[pid]->psip_index++;
                                        }
                                    }
                                    mgt_desc_length = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0xf) << 8;
                                    mgt_desc_length |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("MGT desc bytes = 0x%04x\n", mgt_desc_length);
                                    for (m = 0; m < mgt_desc_length; m++) {
                                        psip_ptr[pid]->psip_index++;
                                    }
                                    mgt_crc = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 24;
                                    mgt_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 16;
                                    mgt_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                    mgt_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("MGT CRC = 0x%08x, %d, %d\n", mgt_crc, psip_ptr[pid]->psip_offset, psip_ptr[pid]->psip_index);
                                    printf("\n");
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xc8 && vct_version_number != vct_last_version_number) {
                                    vct_last_version_number = vct_version_number;
                                    psip_ptr[pid]->psip_index = 0;
                                    vct_num_channels = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("TVCT number of channels = %d\n\n", vct_num_channels);
                                    for (k = 0; k < vct_num_channels; k++) {
                                        printf("TVCT short name = ");
                                        for (m = 0; m < 14; m++) {
                                            if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] != 0) {
                                                printf("%c", psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++]);
                                            }
                                            else {
                                                psip_ptr[pid]->psip_index++;
                                            }
                                        }
                                        printf("\n");
                                        vct_major_channel_number = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0xf) << 8;
                                        vct_major_channel_number |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] & 0xfc;
                                        vct_major_channel_number >>= 2;
                                        vct_minor_channel_number = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x3) << 8;
                                        vct_minor_channel_number |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT channel number = %d.%d\n", vct_major_channel_number, vct_minor_channel_number);
                                        vct_modulation_mode = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT modulation mode = 0x%02x\n", vct_modulation_mode);
                                        psip_ptr[pid]->psip_index += 4;
                                        vct_channel_tsid = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        vct_channel_tsid |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT channel TSID = 0x%04x\n", vct_channel_tsid);
                                        vct_program_number = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        vct_program_number |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT program number = 0x%04x\n", vct_program_number);
                                        psip_ptr[pid]->psip_index++;
                                        vct_service_type = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x3f;
                                        printf("TVCT service type = 0x%04x\n", vct_service_type);
                                        vct_source_id = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        vct_source_id |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT source id = 0x%04x\n", vct_source_id);
                                        vct_desc_length = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x3) << 8;
                                        vct_desc_length |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("TVCT desc bytes = 0x%04x\n\n", vct_desc_length);
                                        while (vct_desc_length != 0) {
                                            if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0xa0) {
                                                psip_ptr[pid]->psip_index++;
                                                ecnd_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                vct_desc_length -= (ecnd_desc_length + 2);
                                                printf("Extended Channel Name = ");
                                                for (m = 0; m < ecnd_desc_length; m++) {
                                                    printf("%c", psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++]);
                                                }
                                                printf("\n\n");
                                            }
                                            else if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0xa1) {
                                                psip_ptr[pid]->psip_index++;
                                                sld_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                vct_desc_length -= (sld_desc_length + 2);
                                                sld_pcr_pid = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x1f) << 8;
                                                sld_pcr_pid |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                printf("SLD PCR pid = 0x%04x\n", sld_pcr_pid);
                                                sld_num_elements = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                for (m = 0; m < sld_num_elements; m++) {
                                                    sld_stream_type = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                    printf("SLD stream type = 0x%02x\n", sld_stream_type);
                                                    sld_elementary_pid = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x1f) << 8;
                                                    sld_elementary_pid |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                    printf("SLD elementary pid = 0x%04x\n", sld_elementary_pid);
                                                    printf("SLD language code = ");
                                                    for (n = 0; n < 3; n++) {
                                                        if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] != 0) {
                                                            printf("%c", psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++]);
                                                        }
                                                        else {
                                                            psip_ptr[pid]->psip_index++;
                                                        }
                                                    }
                                                    printf("\n\n");
                                                }
                                            }
                                            else if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0xa2) {
                                                psip_ptr[pid]->psip_index++;
                                            }
                                        }
                                    }
                                    vct_add_desc_length = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x3) << 8;
                                    vct_add_desc_length |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("TVCT additional desc bytes = 0x%04x\n", vct_add_desc_length);
                                    for (m = 0; m < vct_add_desc_length; m++) {
                                        psip_ptr[pid]->psip_index++;
                                    }
                                    vct_crc = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 24;
                                    vct_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 16;
                                    vct_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                    vct_crc |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("TVCT CRC = 0x%08x, %d, %d\n", vct_crc, psip_ptr[pid]->psip_offset, psip_ptr[pid]->psip_index);
                                    printf("\n");
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xca) {
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xcb && eit_version_number != eit_last_version_number[psip_pid_table[pid] & 0x3]) {
                                    eit_last_version_number[psip_pid_table[pid] & 0x3] = eit_version_number;
                                    psip_ptr[pid]->psip_index = 0;
                                    eit_num_events = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                    printf("EIT%d events defined = %d\n\n", psip_pid_table[pid] & 0x3, eit_num_events);
                                    for (k = 0; k < eit_num_events; k++) {
                                        eit_event_id = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0x3f) << 8;
                                        eit_event_id |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("EIT event id = 0x%04x\n", eit_event_id);
                                        eit_start_time = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 24;
                                        eit_start_time |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 16;
                                        eit_start_time |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        eit_start_time |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("EIT start time = 0x%08x\n", eit_start_time);
                                        eit_length_secs = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0xf) << 16;
                                        eit_length_secs |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] << 8;
                                        eit_length_secs |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("EIT length in seconds = %d\n", eit_length_secs);
                                        eit_title_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("EIT title length = 0x%02x\n", eit_title_length);
                                        for (m = 0; m < eit_title_length; m++) {
                                            if ((psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] >= 0x20) && (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] < 0x7f)) {
                                                printf("%c", psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index]);
                                            }
                                            psip_ptr[pid]->psip_index++;
                                        }
                                        printf("\n");
                                        eit_desc_length = (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++] & 0xf) << 8;
                                        eit_desc_length |= psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                        printf("EIT desc bytes = 0x%04x\n", eit_desc_length);
                                        while (eit_desc_length != 0) {
                                            if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0x81) {
                                                printf("AC-3 Audio Descriptor\n");
                                                psip_ptr[pid]->psip_index++;
                                                ac3_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                eit_desc_length -= (ac3_desc_length + 2);
                                                psip_ptr[pid]->psip_index += ac3_desc_length;
                                            }
                                            else if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0x86) {
                                                printf("Caption Service Descriptor\n");
                                                psip_ptr[pid]->psip_index++;
                                                csd_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                eit_desc_length -= (csd_desc_length + 2);
                                                psip_ptr[pid]->psip_index += csd_desc_length;
                                            }
                                            else if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0x87) {
                                                printf("Content Advisory Descriptor\n");
                                                psip_ptr[pid]->psip_index++;
                                                cad_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                eit_desc_length -= (cad_desc_length + 2);
                                                psip_ptr[pid]->psip_index += cad_desc_length;
                                            }
                                            else if (psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index] == 0xaa) {
                                                psip_ptr[pid]->psip_index++;
                                                rcd_desc_length = psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++];
                                                eit_desc_length -= (rcd_desc_length + 2);
                                                printf("RCD information = ");
                                                for (m = 0; m < rcd_desc_length; m++) {
                                                    printf("0x%02x, ", psip_ptr[pid]->psip_table[psip_ptr[pid]->psip_index++]);
                                                }
                                                printf("\n");
                                            }
                                        }
                                        printf("\n");
                                    }
                                    printf("\n");
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xcd) {
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xd3) {
                                }
                                else if (psip_ptr[pid]->psip_table_id == 0xd4) {
                                }
                            }
                        }
                    }
                    else {
                        --xport_packet_length;
                        pcr_bytes++;
                        if (psip_ptr[pid]->psip_section_start == TRUE) {
                            psip_ptr[pid]->psip_pointer_field = buffer[i];
                            if (psip_ptr[pid]->psip_pointer_field == 0) {
                                psip_ptr[pid]->psip_section_length_parse = 3;
                            }
                            psip_ptr[pid]->psip_section_start = FALSE;
                        }
                        else if (psip_ptr[pid]->psip_pointer_field != 0) {
                            --psip_ptr[pid]->psip_pointer_field;
                            switch (psip_ptr[pid]->psip_pointer_field) {
                                case 0:
                                    psip_ptr[pid]->psip_section_length_parse = 3;
                                    break;
                            }
                        }
                        else if (psip_ptr[pid]->psip_section_length_parse != 0) {
                            --psip_ptr[pid]->psip_section_length_parse;
                            switch (psip_ptr[pid]->psip_section_length_parse) {
                                case 2:
                                    psip_ptr[pid]->psip_table_id = buffer[i];
                                    break;
                                case 1:
                                    psip_ptr[pid]->psip_section_length = (buffer[i] & 0xf) << 8;
                                    break;
                                case 0:
                                    psip_ptr[pid]->psip_section_length |= buffer[i];
#if 0
                                    printf("Section length = %d\r\n", psip_ptr[pid]->psip_section_length);
#endif
                                    psip_ptr[pid]->psip_section_parse = 6;
                                    break;
                            }
                        }
                        else if (psip_ptr[pid]->psip_section_parse != 0) {
                            --psip_ptr[pid]->psip_section_length;
                            --psip_ptr[pid]->psip_section_parse;
                            switch (psip_ptr[pid]->psip_section_parse) {
                                case 5:
                                    psip_ptr[pid]->psip_table_id_ext = buffer[i] << 8;
                                    break;
                                case 4:
                                    psip_ptr[pid]->psip_table_id_ext |= buffer[i];
                                    break;
                                case 3:
                                    switch (psip_ptr[pid]->psip_table_id) {
                                        case 0xc7:
                                            mgt_version_number = buffer[i] & 0x1f;
                                            break;
                                        case 0xc8:
                                            vct_version_number = buffer[i] & 0x1f;
                                            break;
                                        case 0xca:
                                            break;
                                        case 0xcb:
                                            eit_version_number = buffer[i] & 0x1f;
                                            break;
                                        case 0xcd:
                                            break;
                                        case 0xd3:
                                            break;
                                        case 0xd4:
                                            break;
                                    }
                                    break;
                                case 2:
                                    psip_ptr[pid]->psip_section_number = buffer[i];
                                    if (psip_ptr[pid]->psip_section_number == 0) {
                                        psip_ptr[pid]->psip_offset = 0;
                                    }
                                    break;
                                case 1:
                                    psip_ptr[pid]->psip_last_section_number = buffer[i];
                                    break;
                                case 0:
                                    psip_ptr[pid]->psip_xfer_state = TRUE;
                                    break;
                            }
                        }
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
                else {
                    --xport_packet_length;
                    pcr_bytes++;
                    if ((length - i) >= xport_packet_length) {
                        i = i + xport_packet_length;
                        pcr_bytes = pcr_bytes + xport_packet_length;
                        xport_packet_length = 0;
                    }
                    else {
                        xport_packet_length = xport_packet_length - (length - i) + 1;
                        pcr_bytes = pcr_bytes + (length - i) - 1;
                        i = length;
                    }
                    if (xport_packet_length == 0) {
                        sync_state = FALSE;
                    }
                }
            }
            else {
                sync = buffer[i];
                if (hdmv_mode == FALSE) {
                    if (sync == 0x47) {
                        sync_state = TRUE;
                        xport_packet_length = 187;
                        pcr_bytes++;
                        xport_header_parse = 3;
                        if (skipped_bytes != 0) {
                            printf("Transport Sync Error, skipped %d bytes, at %lld\n", skipped_bytes, packet_counter);
                            skipped_bytes = 0;
                        }
                    }
                    else {
                        skipped_bytes++;
                    }
                }
                else {
                    if (tp_extra_header_parse != 0) {
                        --tp_extra_header_parse;
                        switch (tp_extra_header_parse) {
                            case 3:
                                tp_extra_header = 0;
                                tp_extra_header |= (buffer[i] & 0x3f) << 24;
                                break;
                            case 2:
                                tp_extra_header |= (buffer[i] & 0xff) << 16;
                                break;
                            case 1:
                                tp_extra_header |= (buffer[i] & 0xff) << 8;
                                break;
                            case 0:
                                tp_extra_header |= (buffer[i] & 0xff);
                                if (dump_extra == TRUE) {
                                    printf("arrival_time_stamp delta = %d\n", tp_extra_header - tp_extra_header_prev);
                                }
                                tp_extra_header_prev = tp_extra_header;
                                break;
                        }
                    }
                    else if (sync == 0x47) {
                        sync_state = TRUE;
                        xport_packet_length = 187;
                        tp_extra_header_pcr_bytes = pcr_bytes;
                        pcr_bytes++;
                        xport_header_parse = 3;
                        if (skipped_bytes != 0) {
                            printf("Transport Sync Error, skipped %d bytes, at %lld\n", skipped_bytes, packet_counter);
                            skipped_bytes = 0;
                        }
                        tp_extra_header_parse = 4;
                    }
                    else {
                        skipped_bytes++;
                    }
                }
            }
        }
    }
    else {
    }
}
