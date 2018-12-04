/**
 * Finnthesizer Config-File Generation
 *
 **/

#ifndef __LAYER_CONFIG_H_
#define __LAYER_CONFIG_H_

/**
 * Fully-Connected Layer L1:
 *     MatW =   512 MatH =   512
 *     SIMD =    32  PE  =    64
 *     WMEM =   128 TMEM =     8
 *     #Ops  = 524288   Ext Latency  =   128
**/

#define L1_SIMD 32
#define L1_PE 64
#define L1_WMEM 128
#define L1_TMEM 8
#define L1_MW 512
#define L1_MH 512
#define L1_WPI 1
#define L1_API 2
#define L1_WPF 0
#define L1_APF 0

/**
 * Fully-Connected Layer L2:
 *     MatW =   512 MatH =   512
 *     SIMD =    64  PE  =    32
 *     WMEM =   128 TMEM =    16
 *     #Ops  = 524288   Ext Latency  =   128
**/

#define L2_SIMD 64
#define L2_PE 32
#define L2_WMEM 128
#define L2_TMEM 16
#define L2_MW 512
#define L2_MH 512
#define L2_WPI 1
#define L2_API 2
#define L2_WPF 0
#define L2_APF 0

#endif //__LAYER_CONFIG_H_
