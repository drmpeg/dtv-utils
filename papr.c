/*
PAPR calculator
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <math.h>

#define CHUNK_SIZE 16384

int main(int argc, char **argv)
{
    FILE *fp;
    static float buffer[CHUNK_SIZE];
    int i, j, length;
    long long offset = 0;
    long long peak_offset;
    double sum = 0.0;
    float peak = 0.0;
    float papr, value;
    float peak_real_pos = 0.0;
    float peak_imag_pos = 0.0;
    float peak_real_neg = 0.0;
    float peak_imag_neg = 0.0;
    float *level;
    long long *level_count;

    if (argc != 2) {
        fprintf(stderr, "usage: papr <infile>\n");
        exit(-1);
    }

    /*--- open binary file (for parsing) ---*/
    fp = fopen(argv[1], "r");
    if (fp == 0) {
        fprintf(stderr, "Cannot open bitstream file <%s>\n", argv[1]);
        exit(-1);
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
            }
            if (value < peak_real_neg) {
                peak_real_neg = value;
            }
            value = buffer[i + 1];
            if (value > peak_imag_pos) {
                peak_imag_pos = value;
            }
            if (value < peak_imag_neg) {
                peak_imag_neg = value;
            }
            offset++;
        }
    }
    sum = sum / offset;
    printf("Peak magnitude = %f\n", sqrt(peak));
    printf("average power = %lf, peak power = %f @ %lld/%lld\n", sum, peak, peak_offset, peak_offset * 4);
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
    printf("peak real positive = %f, peak imaginary positive = %f\n", peak_real_pos, peak_imag_pos);
    printf("peak real negative = %f, peak imaginary negative = %f\n", peak_real_neg, peak_imag_neg);
    free(level_count);
    free(level);
    fclose(fp);
    return 0;
}
