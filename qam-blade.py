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

from gnuradio import gr, blocks, digital, filter
from gnuradio.filter import firdes
import sys, os
import osmosdr
import qam

def main(args):
    nargs = len(args)
    if nargs == 1:
        infile  = args[0]
        outfile = None
    elif nargs == 2:
        infile  = args[0]
        outfile  = args[1]
    else:
        sys.stderr.write("Usage: qam-blade.py input_file [output_file]\n");
        sys.exit(1)

    samp_rate = 5056941 * 2
    center_freq = 429000000
    rrc_taps = 100
    txvga1_gain = -6
    txvga2_gain = 9
    bandwidth = 6000000
    I_taps = 128
    J_increment = 4
    Control_Word = 6

    tb = gr.top_block()

    src = blocks.file_source(gr.sizeof_char, infile, True)

    qam_transport_framing = qam.transport_framing_enc_bb()
    blocks_packed_to_unpacked = blocks.packed_to_unpacked_bb(7, gr.GR_MSB_FIRST)
    qam_reed_solomon = qam.reed_solomon_enc_bb()
    qam_interleaver = qam.interleaver_bb(I_taps,J_increment)
    qam_randomizer = qam.randomizer_bb()
    qam_frame_sync = qam.frame_sync_enc_bb(Control_Word)
    qam_trellis = qam.trellis_enc_bb()
    digital_chunks_to_symbols = digital.chunks_to_symbols_bc(([complex(1,1), complex(1,-1), complex(1,-3), complex(-3,-1), complex(-3,1), complex(1,3), complex(-3,-3), complex(-3,3), complex(-1,1), complex(-1,-1), complex(3,1), complex(-1,3), complex(-1,-3), complex(3,-1), complex(3,-3), complex(3,3), complex(5,1), complex(1,-5), complex(1,-7), complex(-7,-1), complex(-3,5), complex(5,3), complex(-7,-3), complex(-3,7), complex(-1,5), complex(-5,-1), complex(7,1), complex(-1,7), complex(-5,-3), complex(3,-5), complex(3,-7), complex(7,3), complex(1,5), complex(5,-1), complex(5,-3), complex(-3,-5), complex(-7,1), complex(1,7), complex(-3,-7), complex(-7,3), complex(-5,1), complex(-1,-5), complex(3,5), complex(-5,3), complex(-1,-7), complex(7,-1), complex(7,-3), complex(3,7), complex(5,5), complex(5,-5), complex(5,-7), complex(-7,-5), complex(-7,5), complex(5,7), complex(-7,-7), complex(-7,7), complex(-5,5), complex(-5,-5), complex(7,5), complex(-5,7), complex(-5,-7), complex(7,-5), complex(7,-7), complex(7,7)]), 1)

    interp_fir_filter = filter.interp_fir_filter_ccc(2, (firdes.root_raised_cosine(0.14, samp_rate, samp_rate/2, 0.18, rrc_taps)))
    interp_fir_filter.declare_sample_delay(0)

    out = osmosdr.sink(args="bladerf=0,buffers=128,buflen=32768")
    out.set_sample_rate(samp_rate)
    out.set_center_freq(center_freq, 0)
    out.set_freq_corr(0, 0)
    out.set_gain(txvga2_gain, 0)
    out.set_bb_gain(txvga1_gain, 0)
    out.set_bandwidth(bandwidth, 0)

    tb.connect(src, qam_transport_framing)
    tb.connect(qam_transport_framing, blocks_packed_to_unpacked)
    tb.connect(blocks_packed_to_unpacked, qam_reed_solomon)
    tb.connect(qam_reed_solomon, qam_interleaver)
    tb.connect(qam_interleaver, qam_randomizer)
    tb.connect(qam_randomizer, qam_frame_sync)
    tb.connect(qam_frame_sync, qam_trellis)
    tb.connect(qam_trellis, digital_chunks_to_symbols)
    tb.connect(digital_chunks_to_symbols, interp_fir_filter)
    tb.connect(interp_fir_filter, out)


    if outfile:
        dst = blocks.file_sink(gr.sizeof_gr_complex, outfile)
        tb.connect(interp_fir_filter, dst)

    tb.run()


if __name__ == '__main__':
    main(sys.argv[1:])

