/*
 * Copyright 2016,2018 Ron Economos (w6rz@comcast.net)
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

/*
PAPR with Complementary Cumulative Distribution Function calculator
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TRUE            1
#define FALSE           0
#define OPTION_CHAR    '-'
#define CHUNK_SIZE 16384

int main(int argc, char **argv)
{
    FILE *fp;
    static float buffer[CHUNK_SIZE];
    int i, j, length, graph = FALSE;
    long long offset = 0;
    long long peak_offset = 0;
    double sum = 0.0;
    float peak = 0.0;
    float papr, value, index;
    float peak_real_pos = 0.0;
    float peak_imag_pos = 0.0;
    float peak_real_neg = 0.0;
    float peak_imag_neg = 0.0;
    long long peak_real_pos_offset = 0;
    long long peak_imag_pos_offset = 0;
    long long peak_real_neg_offset = 0;
    long long peak_imag_neg_offset = 0;
    float *level;
    long long *level_count;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: papr -g <infile>\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\tg = graph suitable output\n");
        exit(-1);
    }
    if (argc == 2)
    {
        /*--- open binary file (for parsing) ---*/
        fp = fopen(argv[1], "r");
        if (fp == 0) {
            fprintf(stderr, "Cannot open bitstream file <%s>\n", argv[1]);
            exit(-1);
        }
    }
    else
    {
        if(*argv[1] == OPTION_CHAR)
        {
            for(i = 1; i < strlen(argv[1]); i++)
            {
                switch (argv[1][i])
                {
                    case 'g':
                    case 'G':
                        graph = TRUE;
                        break;
                    default:
                        fprintf(stderr, "Unsupported Option: %c\n", argv[1][i]);
                }
            }
        }
        else
        {
            fprintf(stderr, "usage: papr -g <infile>\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "\tg = graph suitable output\n");
            exit(-1);
        }
        /*--- open binary file (for parsing) ---*/
        fp = fopen(argv[2], "r");
        if (fp == 0) {
            fprintf(stderr, "Cannot open bitstream file <%s>\n", argv[2]);
            exit(-1);
        }
    }

    while (!feof(fp)) {
        length = fread(&buffer[0], sizeof(float), CHUNK_SIZE, fp);
        for (i = 0; i < length; i += 2) {
            value = (buffer[i] * buffer[i]) + (buffer[i + 1] * buffer[i + 1]);
            sum += value;
            if (value > peak) {
                peak = value;
                peak_offset = offset;
            }
            value = buffer[i];
            if (value > peak_real_pos) {
                peak_real_pos = value;
                peak_real_pos_offset = offset;
            }
            if (value < peak_real_neg) {
                peak_real_neg = value;
                peak_real_neg_offset = offset;
            }
            value = buffer[i + 1];
            if (value > peak_imag_pos) {
                peak_imag_pos = value;
                peak_imag_pos_offset = offset;
            }
            if (value < peak_imag_neg) {
                peak_imag_neg = value;
                peak_imag_neg_offset = offset;
            }
            offset++;
        }
    }
    if (graph == FALSE) {
        sum = sum / offset;
        printf("Peak magnitude = %f\n", sqrt(peak));
        printf("average power = %lf, peak power = %f @ %lld\n\n", sum, peak, peak_offset * 8);
        papr = 10 * log10((peak) / (sum));
        printf("Maximum PAPR = %f\n", papr);
        level = (float *)malloc(((int)papr + 1) * sizeof(float));
        level_count = (long long *)malloc(((int)papr + 1) * sizeof(long long));
        for (i = 0; i <= (int)papr; i++) {
            level[i] = pow(10, ((float)i / 10)) * (sum);
            level_count[i] = 0;
        }
        fseeko(fp, 0, SEEK_SET);
        while (!feof(fp)) {
            length = fread(&buffer[0], sizeof(float), CHUNK_SIZE, fp);
            for (i = 0; i < length; i += 2) {
                value = (buffer[i] * buffer[i]) + (buffer[i + 1] * buffer[i + 1]);
                for (j = 0; j <= (int)papr; j++) {
                    if (value > level[j]) {
                        level_count[j]++;
                    }
                }
            }
        }
        for (i = 0; i <= (int)papr; i++) {
            printf("percentage above %d dB = %0.8f\n", i, ((float)level_count[i] / (float)offset) * 100.0);
        }
        printf("\n");
        printf("peak real positive = %f, peak imaginary positive = %f\n", peak_real_pos, peak_imag_pos);
        printf("peak real negative = %f, peak imaginary negative = %f\n\n", peak_real_neg, peak_imag_neg);
        printf("peak real positive @ %lld, peak imaginary positive @ %lld\n", peak_real_pos_offset * 8, (peak_imag_pos_offset * 8) + 1);
        printf("peak real negative @ %lld, peak imaginary negative @ %lld\n", peak_real_neg_offset * 8, (peak_imag_neg_offset * 8) + 1);
    }
    else {
        sum = sum / offset;
        papr = 10 * log10((peak) / (sum));
        level = (float *)malloc(((int)(papr * 10) + 1) * sizeof(float));
        level_count = (long long *)malloc(((int)(papr * 10) + 1) * sizeof(long long));
        index = 0.0;
        for (i = 0; i <= (int)(papr * 10); i++) {
            level[i] = pow(10, (index / 10)) * (sum);
            level_count[i] = 0;
            index = index + 0.1;
        }
        fseeko(fp, 0, SEEK_SET);
        while (!feof(fp)) {
            length = fread(&buffer[0], sizeof(float), CHUNK_SIZE, fp);
            for (i = 0; i < length; i += 2) {
                value = (buffer[i] * buffer[i]) + (buffer[i + 1] * buffer[i + 1]);
                for (j = 0; j <= (int)(papr * 10); j++) {
                    if (value > level[j]) {
                        level_count[j]++;
                    }
                }
            }
        }
        index = 0.0;
        for (i = 0; i <= (int)(papr * 10); i++) {
           printf("%0.8f\n", ((float)level_count[i] / (float)offset) * 100.0);
           index = index + 0.1;
        }
    }
    free(level_count);
    free(level);
    fclose(fp);
    return 0;
}
