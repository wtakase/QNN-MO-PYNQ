/*
    Copyright (c) 2018, Xilinx, Inc.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1.  Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    2.  Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    3.  Neither the name of the copyright holder nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
    OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "qnn-library.h"
#include "config.h"


static ap_uint<MAX_SIMD> fcWeightMem[MAX_PE_FC][MAX_FC_WMEM];
static ap_uint<THRESHOLDS_BITS> fcThresMem[MAX_PE_FC][MAX_FC_TMEM];

unsigned int paddedSizeHW(unsigned int in, unsigned int padTo) {
    if(in % padTo == 0)
        return in;
    else
        return in + padTo - (in % padTo);
}

void StreamingDoMemInit(ap_uint<DATAWIDTH> *in1, ap_uint<DATAWIDTH> *in2, const unsigned int KernelDim) {
#pragma HLS DATAFLOW

    hls::stream<ap_uint<DATAWIDTH> > streamIn1("streamInMem1");
    hls::stream<ap_uint<DATAWIDTH> > streamIn2("streamInMem2");

#pragma HLS STREAM variable=streamIn1 depth=1
#pragma HLS STREAM variable=streamIn2 depth=1

    Mem2Stream<DATAWIDTH, (FC_MEM_BITS/MEM_CHANNELS) / 8> (in1, streamIn1, (FC_MEM_BITS / MEM_CHANNELS) / 8);

    Mem2Stream<DATAWIDTH, (FC_MEM_BITS/MEM_CHANNELS) / 8> (in2, streamIn2, (FC_MEM_BITS / MEM_CHANNELS) / 8);

    StreamingInitMemory_Precision<DATAWIDTH, THRESHOLDS_BITS, MAX_SIMD, MAX_PE_FC, 0, MAX_PE_FC / 2, MAX_FC_WMEM, MAX_FC_TMEM>
            (streamIn1, fcWeightMem, fcThresMem, MAX_FC_WMEM, MAX_FC_TMEM);

    StreamingInitMemory_Precision<DATAWIDTH, THRESHOLDS_BITS, MAX_SIMD, MAX_PE_FC, MAX_PE_FC / 2, MAX_PE_FC, MAX_FC_WMEM, MAX_FC_TMEM>
            (streamIn2, fcWeightMem, fcThresMem, MAX_FC_WMEM, MAX_FC_TMEM);
}

void DoCompute(ap_uint<DATAWIDTH> * in,	ap_uint<DATAWIDTH> * out,
        const unsigned int KernelDim, const unsigned int Stride,
        const unsigned int IFMCh, const unsigned int OFMCh,
        const unsigned int IFMDim, const unsigned int PaddedDim,
        const unsigned int OFMDim, const unsigned int PoolInDim,
        const unsigned int PoolOutDim, const unsigned int PoolStride,
        const ap_uint<1> enablePool) {
#pragma HLS DATAFLOW

    hls::stream<ap_uint<DATAWIDTH>> memInStream("memInStream");
    hls::stream<ap_uint<DATAWIDTH>> fcStream("fcStream");
    hls::stream<ap_uint<DATAWIDTH>> memOutStream("memOutStream");

#pragma HLS STREAM variable=memInStream depth=1
#pragma HLS STREAM variable=fcStream depth=1
#pragma HLS STREAM variable=memOutStream depth=1

#pragma HLS RESOURCE variable=memInStream core=FIFO_LUTRAM
#pragma HLS RESOURCE variable=fcStream core=FIFO_LUTRAM
#pragma HLS RESOURCE variable=memOutStream core=FIFO_LUTRAM

    const unsigned int inBits = ACTIVATION_BITS * MAX_MH;
    const unsigned int paddedInBytes = inBits / 8;
    const unsigned int outBits = ACTIVATION_BITS * MAX_MW;
    const unsigned int paddedOutBytes = outBits / 8;

    Mem2Stream<DATAWIDTH, paddedInBytes> (in, memInStream, paddedInBytes);

    StreamingFCLayer<MAX_SIMD, MAX_PE_FC, POPCOUNT_WIDTH, MAX_FC_WMEM, MAX_FC_TMEM>
            (fcStream, memOutStream, fcWeightMem, fcThresMem, IFMCh, OFMCh, 1);

    Stream2Mem<DATAWIDTH, paddedOutBytes> (memOutStream, out, paddedOutBytes);
}

void BlackBoxJam(ap_uint<64> * in1, ap_uint<64> * in2, ap_uint<64> * out,
        bool doInit, unsigned int layerType,
        const unsigned int KernelDim, const unsigned int Stride,
        const unsigned int IFMCh, const unsigned int OFMCh,
        const unsigned int IFMDim, const unsigned int PaddedDim,
        const unsigned int OFMDim, const unsigned int PoolInDim,
        const unsigned int PoolOutDim, const unsigned int PoolStride)
{
    // signals to be mapped to the AXI Lite slave port
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=doInit bundle=control
#pragma HLS INTERFACE s_axilite port=layerType bundle=control
#pragma HLS INTERFACE s_axilite port=KernelDim bundle=control
#pragma HLS INTERFACE s_axilite port=Stride bundle=control
#pragma HLS INTERFACE s_axilite port=IFMCh bundle=control
#pragma HLS INTERFACE s_axilite port=OFMCh bundle=control
#pragma HLS INTERFACE s_axilite port=IFMDim bundle=control
#pragma HLS INTERFACE s_axilite port=PaddedDim bundle=control
#pragma HLS INTERFACE s_axilite port=OFMDim bundle=control
#pragma HLS INTERFACE s_axilite port=PoolInDim bundle=control
#pragma HLS INTERFACE s_axilite port=PoolOutDim bundle=control
#pragma HLS INTERFACE s_axilite port=PoolStride bundle=control
    // signals to be mapped to the AXI master port (hostmem1, hostmem2)
#pragma HLS INTERFACE m_axi offset=slave port=in1 bundle=hostmem1 depth=1
#pragma HLS INTERFACE s_axilite port=in1 bundle=control
#pragma HLS INTERFACE m_axi offset=slave port=out bundle=hostmem1 depth=1
#pragma HLS INTERFACE s_axilite port=out bundle=control
#pragma HLS INTERFACE m_axi offset=slave port=in2 bundle=hostmem2 depth=1
#pragma HLS INTERFACE s_axilite port=in2 bundle=control
    // partition PE arrays
#pragma HLS ARRAY_PARTITION variable=fcWeightMem complete dim=1
#pragma HLS ARRAY_PARTITION variable=fcThresMem complete dim=1
#pragma HLS RESOURCE variable=fcThresMem core=RAM_2P_LUTRAM

    if (doInit) {
        StreamingDoMemInit(in1, in2, KernelDim);
    } else {
        DoCompute(in1, out, KernelDim, Stride, IFMCh, OFMCh, IFMDim, PaddedDim, OFMDim, OFMDim, OFMDim, 0, 0);
    }
}
