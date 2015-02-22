#!/usr/bin/env /usr/bin/python

# Copyright 2014 Ron Economos (w6rz@comcast.net)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from gnuradio import blocks
from gnuradio import digital
from gnuradio import fft
from gnuradio import gr
from gnuradio.fft import window
import dvbt
import osmosdr
import sys
import argparse

def main(args):
    d = 'Transmit a DVB-T signal using the bladeRF and gr-dvbt'
    parser = argparse.ArgumentParser()

    mode_help = '# of carriers. Options: t2k, t8k (default).'
    parser.add_argument('-m', '--mode', dest='mode', default='t8k',
                        type=str, help=mode_help)

    ch_help = 'channel width in MHz. Options: 5, 6, 7, 8 (default).'
    parser.add_argument('-c', '--channel', dest='channel', default=8,
                        metavar='CH', type=int, help=ch_help)

    const_help = 'constellation. qpsk, qam16, qam64 (default).'
    parser.add_argument('-C', '--cons', dest='cons', default='qam64',
                        metavar='TYPE', type=str, help=const_help)

    rate_help = 'Viterbi rate. 1/2, 2/3, 3/4, 5/6, 7/8 (default).'
    parser.add_argument('-r', '--rate', dest='rate', default='7/8',
                        type=str, help=rate_help)

    guard_help = 'guard interval. 1/32 (default), 1/16, 1/8, 1/4.'
    parser.add_argument('-g', '--guard', dest='interval', default='1/32',
                        metavar='D', type=str, help=guard_help)

    freq_help = 'center frequency (Hz). Default is 429e6.'
    parser.add_argument('-f', '--freq', dest='freq', default=429e6,
                        type=float, help=freq_help)

    vga1_help = 'bladeRF TXVGA1 gain. Default is -6.'
    parser.add_argument('--txvga1', dest='txvga1', default=-6,
                        metavar='gain', type=int, help=vga1_help)

    vga2_help = 'bladeRF TXVGA2 gain. Default is 9.'
    parser.add_argument('--txvga2', dest='txvga2', default=9,
                        metavar='gain', type=int, help=vga2_help)

    outfile_help = 'write to specified file.'
    parser.add_argument('-o', '--output', dest='outfile', default=None,
                        metavar='OUT', type=str, help=outfile_help)

    parser.add_argument('infile', metavar='input-file', type=str,
                        help='Input file')

    args = parser.parse_args()
    print args

    if args.mode.lower() == 't2k':
        mode = dvbt.T2k
    elif args.mode.lower() == 't8k':
        mode  = dvbt.T8k
    else:
        sys.stderr.write('Invalid mode provided: ' + args.mode + '\n')
        sys.exit(1)

    if args.channel < 5 or args.channel > 8:
        sys.stderr.write('Invalid channel: ' + str(args.channel) + '\n')
        sys.exit(1)
    else:
        channel_mhz = args.channel

    if args.cons.lower() == 'qpsk':
        constellation = dvbt.QPSK
    elif args.cons.lower() == 'qam16':
        constellation = dvbt.QAM16
    elif args.cons.lower() == 'qam64':
        constellation = dvbt.QAM64
    else:
        sys.stderr.write('Invalid constellation type: ' + args.cons + '\n')
        sys.exit(1)

    if args.rate == '1/2':
        code_rate = dvbt.C1_2
    elif args.rate == '2/3':
        code_rate = dvbt.C2_3
    elif args.rate == '3/4':
        code_rate = dvbt.C3_4
    elif args.rate == '5/6':
        code_rate = dvbt.C5_6
    elif args.rate == '7/8':
        code_rate = dvbt.C7_8
    else:
        sys.stderr.write('Invalid Viterbi rate: ' + args.rate + '\n')
        sys.exit(1)

    if args.interval == '1/32':
        guard_interval = dvbt.G1_32
    elif args.interval == '1/16':
        guard_interval = dvbt.G1_16
    elif args.interval == '1/8':
        guard_interval = dvbt.G1_8
    elif args.interval == '1/4':
        guard_interval = dvbt.G1_4
    else:
        sys.stderr.write('Invalid guard interval: ' + args.interval + '\n')
        sys.exit(1)

    if args.freq < 300e6 or args.freq > 3.8e9:
        sys.stderr.write('Invalid center frequency: ' + str(args.freq) + '\n')
        sys.exit(1)
    else:
        center_freq = int(args.freq)

    if args.txvga1 < -35 or args.txvga1 > -4:
        sys.stderr.write('Invalid bladeRF TXVGA1 gain: ' +
                         str(args.txvga1) + '\n')
        sys.exit(1)
    else:
        txvga1_gain = args.txvga1

    if args.txvga2 < 0 or args.txvga2 > 25:
        sys.stderr.write('Invalid bladeRF TXVGA2 gain: ' +
                         str(args.txvga2) + '\n')
        sys.exit(1)
    else:
        txvga2_gain = args.txvga2

    infile = args.infile
    outfile = args.outfile
    symbol_rate = channel_mhz * 8000000.0 / 7

    if mode == dvbt.T2k:
        factor = 1
        carriers = 2048
    elif mode == dvbt.T8k:
        factor = 4
        carriers = 8192

    if guard_interval == dvbt.G1_32:
        gi = carriers / 32
    elif guard_interval == dvbt.G1_16:
        gi = carriers / 16
    elif guard_interval == dvbt.G1_8:
        gi = carriers / 8
    elif guard_interval == dvbt.G1_4:
        gi = carriers / 4

    if channel_mhz == 8:
        bandwidth = 8750000
    elif channel_mhz == 7:
        bandwidth = 7000000
    elif channel_mhz == 6:
        bandwidth = 6000000
    elif channel_mhz == 5:
        bandwidth = 5000000
    else:
        bandwidth = 8750000

    tb = gr.top_block()

    src = blocks.file_source(gr.sizeof_char, infile, True)

    dvbt_energy_dispersal = dvbt.energy_dispersal(1 * factor)
    dvbt_reed_solomon_enc = dvbt.reed_solomon_enc(2, 8, 0x11d, 255, 239, 8, 51, (8 * factor))
    dvbt_convolutional_interleaver = dvbt.convolutional_interleaver((136 * factor), 12, 17)
    dvbt_inner_coder = dvbt.inner_coder(1, (1512 * factor), constellation, dvbt.NH, code_rate)
    dvbt_bit_inner_interleaver = dvbt.bit_inner_interleaver((1512 * factor), constellation, dvbt.NH, mode)
    dvbt_symbol_inner_interleaver = dvbt.symbol_inner_interleaver((1512 * factor), mode, 1)
    dvbt_dvbt_map = dvbt.dvbt_map((1512 * factor), constellation, dvbt.NH, mode, 1)
    dvbt_reference_signals = dvbt.reference_signals(gr.sizeof_gr_complex, (1512 * factor), carriers, constellation, dvbt.NH, code_rate, code_rate, guard_interval, mode, 0, 0)
    fft_vxx = fft.fft_vcc(carriers, False, (window.rectangular(carriers)), True, 10)
    digital_ofdm_cyclic_prefixer = digital.ofdm_cyclic_prefixer(carriers, carriers+(gi), 0, "")
    blocks_multiply_const_vxx = blocks.multiply_const_vcc((0.0022097087, ))

    out = osmosdr.sink(args="bladerf=0,buffers=128,buflen=32768")
    out.set_sample_rate(symbol_rate)
    out.set_center_freq(center_freq, 0)
    out.set_freq_corr(0, 0)
    out.set_gain(txvga2_gain, 0)
    out.set_bb_gain(txvga1_gain, 0)
    out.set_bandwidth(bandwidth, 0)

    tb.connect(src, dvbt_energy_dispersal)
    tb.connect(dvbt_energy_dispersal, dvbt_reed_solomon_enc)
    tb.connect(dvbt_reed_solomon_enc, dvbt_convolutional_interleaver)
    tb.connect(dvbt_convolutional_interleaver, dvbt_inner_coder)
    tb.connect(dvbt_inner_coder, dvbt_bit_inner_interleaver)
    tb.connect(dvbt_bit_inner_interleaver, dvbt_symbol_inner_interleaver)
    tb.connect(dvbt_symbol_inner_interleaver, dvbt_dvbt_map)
    tb.connect(dvbt_dvbt_map, dvbt_reference_signals)
    tb.connect(dvbt_reference_signals, fft_vxx)
    tb.connect(fft_vxx, digital_ofdm_cyclic_prefixer)
    tb.connect(digital_ofdm_cyclic_prefixer, blocks_multiply_const_vxx)
    tb.connect(blocks_multiply_const_vxx, out)


    if outfile:
        dst = blocks.file_sink(gr.sizeof_gr_complex, outfile)
        tb.connect(blocks_multiply_const_vxx, dst)

    tb.run()


if __name__ == '__main__':
    main(sys.argv[1:])

