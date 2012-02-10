#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/apollo.h>

#include "wm8994_def.h"
#include "wm8994_gain.h"

#define GAIN_TBL_NUM 	 	21 // current
#define GAIN_POINT_NUM 	19

struct gain_info_t codec_vol_table[GAIN_TBL_NUM][GAIN_POINT_NUM] = {
	{ 	
	/*  Default Gain table 	All path enable				(0) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},

/*==================== Playback Path ====================*/
	{ 	
	/* MM_AUDIO_PLAYBACK_RCV 					(1) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_PLAYBACK_SPK 					(2) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x2}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_PLAYBACK_HP 					(3) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_PLAYBACK_BT 					(4) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 0,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_PLAYBACK_SPK_HP 					(5) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x2}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x21}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x21}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_PLAYBACK_RING_SPK_HP 					(6) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x2}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x21}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x21}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},

/*==================== VoiceCall Path ====================*/
	{ 	
	/* MM_AUDIO_VOICECALL_RCV 					(7) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x0}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICECALL_SPK 					(8) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0C}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute
			//SUB MIC
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x6}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x39}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x39}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICECALL_HP 					(9) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x33}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x33}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICECALL_BT 					(10) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 0,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 0,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 0,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 0,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICECALL_SPK_LOOP 					(11) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 0,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 0,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 0,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 0,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 0,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 0,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},

/*==================== VoiceMemo Path ====================*/
	{ 	
	/* MM_AUDIO_VOICEMEMO_MAIN 					(12) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICEMEMO_SUB 					(13) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICEMEMO_EAR 					(14) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_VOICEMEMO_BT 					(15) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
		
/*==================== FM Radio Path ====================*/
	{ 	
	/* MM_AUDIO_FMRADIO_RCV 					(16) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_FMRADIO_SPK 					(17) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_FMRADIO_HP 					(18) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_FMRADIO_BT 					(19) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x00}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
	},
	{ 	
	/* MM_AUDIO_FMRADIO_SPK_HP 					(20) */
		// INPUT GAIN
			// MAIN MIC
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_3,					.gain = 0x120}, // IN2L_TO_MIXINL[8] : 0 mute, IN2L_MIXINL_VOL[7] : 1 30dB
																			// IN1L_TO_MIXINL[5] : 0 mute, IN1L_MIXINL_VOL[4] : 1 30dB
																			// MIXOUTL_MIXINL_VOL[2:0] : 0 mute

			// EAR MIC
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,	.gain = 0x0B}, // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]
		{.flag = 1,	.reg = WM8994_INPUT_MIXER_4,					.gain = 0x120}, // IN2R_TO_MIXINR[8] : 0 mute, IN2R_MIXINR_VOL[7] : 1 30dB
																			// IN1R_TO_MIXINR[5] : 0 mute, IN1R_MIXINR_VOL[4] : 1 30dB
																			// MIXOUTR_MIXINR_VOL[2:0] : 0 mute

			//SUB MIC
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_5,					.gain = 0x7}, // IN2LP_MIXINL_VOL[2:0]
		{.flag = 0,	.reg = WM8994_INPUT_MIXER_6,					.gain = 0x7}, // IN2LP_MIXINR_VOL[2:0]

			// FM Radio? (Single ended)
		{.flag = 1,	.reg = WM8994_LEFT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
		{.flag = 1,	.reg = WM8994_RIGHT_LINE_INPUT_3_4_VOLUME,	.gain = 0x0B}, // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
		//OUTPUT GAIN
			// SPK
		{.flag = 1,	.reg = WM8994_CLASSD,						.gain = 0x0}, // SPKOUTL_BOOST [5:3]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_LEFT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPEAKER_VOLUME_RIGHT,			.gain = 0x3F}, // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
		{.flag = 1,	.reg = WM8994_SPKMIXL_ATTENUATION,			.gain = 0x0}, // DAC2L_SPKMIXL_VOL[6], MIXINL_SPKMIXL_VOL[5]
																			// IN1LP_SPKMIXL_VOL[4], MIXOUTL_SPKMIXL_VOL[3]
																			// DAC1L_SPKMIXL_VOL[2], SPKMIXL_VOL[1:0] : 3 mute
		{.flag = 1,	.reg = WM8994_SPKMIXR_ATTENUATION,			.gain = 0x0}, // DAC2R_SPKMIXR_VOL[6], MIXINR_SPKMIXR_VOL[5]
																			// IN1RP_SPKMIXR_VOL[4], MIXOUTR_SPKMIXR_VOL[3]
																			// DAC1R_SPKMIXR_VOL[2], SPKMIXR_VOL[1:0] : 3 mute

			// RCV
		{.flag = 1,	.reg = WM8994_LEFT_OPGA_VOLUME,				.gain = 0x39}, // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OPGA_VOLUME,			.gain = 0x39}, // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
		{.flag = 1,	.reg = WM8994_HPOUT2_VOLUME,				.gain = 0x0}, // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			// EAR
		{.flag = 1,	.reg = WM8994_LEFT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
		{.flag = 1,	.reg = WM8994_RIGHT_OUTPUT_VOLUME,			.gain = 0x35}, // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]

		// DIGITALb GAIN
/*
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_LEFT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
		{.flag = 1,	.reg = WM8994_AIF1_DAC1_RIGHT_VOLUME,		.gain = 0xEF}, // AIF1ADC1_VU[8], AIF1ADC1R_VOL[7:0]
*/
		{.flag = 1,	.reg = WM8994_AIF2_CONTROL_2,				.gain = 0x0}, // AIF2DAC_BOOST[11:10]
	},
};

void wm8994_set_volume(struct snd_soc_codec *codec, unsigned short flag, unsigned short gain_reg, unsigned short gain_val)
{
	u16 temp = 0;
	
	switch(gain_reg){
		case WM8994_LEFT_LINE_INPUT_1_2_VOLUME : // IN1_VU[8], IN1L_MUTE [7], IN1L_VOL [4:0] 


			if(flag)
			{
				temp = codec->read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME);
				temp &= ~WM8994_IN1L_VOL_MASK;
				temp |= (WM8994_IN1L_VU | (gain_val<<WM8994_IN1L_VOL_SHIFT)  & WM8994_IN1L_VOL_MASK);
				codec->write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, temp);
//				printk("WM8994_LEFT_LINE_INPUT_1_2_VOLUME : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN1L_MUTE_MASK;
				temp |= WM8994_IN1L_MUTE;
				codec->write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, temp);
//				printk("WM8994_LEFT_LINE_INPUT_1_2_VOLUME [MUTE] : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_RIGHT_LINE_INPUT_1_2_VOLUME : // IN1_VU[8], IN1R_MUTE [7], IN1R_VOL [4:0]


			if(flag)
			{
				temp = codec->read(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);
				temp &= ~WM8994_IN1R_VOL_MASK;
				temp |= (WM8994_IN1R_VU | (gain_val<<WM8994_IN1R_VOL_SHIFT) & WM8994_IN1R_VOL_MASK);
				codec->write(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, temp);
//				printk("WM8994_RIGHT_LINE_INPUT_1_2_VOLUME : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN1R_MUTE_MASK;
				temp |= WM8994_IN1R_MUTE;
				codec->write(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, temp);
//				printk("WM8994_RIGHT_LINE_INPUT_1_2_VOLUME [MUTE] : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_LEFT_LINE_INPUT_3_4_VOLUME : // IN2_VU[8], IN2L_MUTE [7], IN2L_VOL [4:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME);
				temp &= ~WM8994_IN2L_VOL_MASK;
				temp |= (WM8994_IN2L_VU | (gain_val<<WM8994_IN2L_VOL_SHIFT)  & WM8994_IN2L_VOL_MASK);
				codec->write(codec,  WM8994_LEFT_LINE_INPUT_3_4_VOLUME, temp);
//				printk("WM8994_LEFT_LINE_INPUT_3_4_VOLUME : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN2L_MUTE_MASK;
				temp |= WM8994_IN2L_MUTE;
				codec->write(codec,  WM8994_LEFT_LINE_INPUT_3_4_VOLUME, temp);
//				printk("WM8994_LEFT_LINE_INPUT_3_4_VOLUME [MUTE] : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_RIGHT_LINE_INPUT_3_4_VOLUME : // IN2_VU[8], IN1R_MUTE [7], IN2R_VOL [4:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME);
				temp &= ~WM8994_IN2R_VOL_MASK;
				temp |= (WM8994_IN2R_VU | (gain_val<<WM8994_IN2R_VOL_SHIFT)  & WM8994_IN2R_VOL_MASK);
				codec->write(codec,  WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, temp);
//				printk("WM8994_RIGHT_LINE_INPUT_3_4_VOLUME : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN2R_MUTE_MASK;
				temp |= WM8994_IN2R_MUTE;
				codec->write(codec,  WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, temp);
//				printk("WM8994_RIGHT_LINE_INPUT_3_4_VOLUME [MUTE] : \t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_INPUT_MIXER_3 : 
//			temp = codec->read(codec, WM8994_INPUT_MIXER_3);
//			printk("WM8994_INPUT_MIXER_3 : Don't setting reg(0x%x = 0x%x) \n", gain_reg, temp);

			break;

		case WM8994_INPUT_MIXER_4 : 
//			temp = codec->read(codec, WM8994_INPUT_MIXER_4);
//			printk("WM8994_INPUT_MIXER_4 : Don't setting reg(0x%x = 0x%x) \n", gain_reg, temp);

			break;

		case WM8994_INPUT_MIXER_5 : 
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_INPUT_MIXER_5);	
				temp &= ~WM8994_IN2LP_MIXINL_VOL_MASK;
				temp |= ((gain_val<<WM8994_IN2LP_MIXINL_VOL_SHIFT)  & WM8994_IN2LP_MIXINL_VOL_MASK);
				codec->write(codec,  WM8994_INPUT_MIXER_5, temp);
//				printk("WM8994_INPUT_MIXER_5 : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN2LP_MIXINL_VOL_MASK;
				temp |= 0x0;
				codec->write(codec,  WM8994_INPUT_MIXER_5, temp);
//				printk("WM8994_INPUT_MIXER_5 [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_INPUT_MIXER_6 : 
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_INPUT_MIXER_6);
				temp &= ~WM8994_IN2LP_MIXINR_VOL_MASK;
				temp |= ((gain_val<<WM8994_IN2LP_MIXINR_VOL_SHIFT)  & WM8994_IN2LP_MIXINR_VOL_MASK);
				codec->write(codec,  WM8994_INPUT_MIXER_6, temp);
//				printk("WM8994_INPUT_MIXER_6 : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_IN2LP_MIXINR_VOL_MASK;
				temp |= 0x0;
				codec->write(codec,  WM8994_INPUT_MIXER_6, temp);
//				printk("WM8994_INPUT_MIXER_6 [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_CLASSD : // SPKOUTL_BOOST [5:3]
			if(flag)
			{
				temp = codec->read(codec, WM8994_CLASSD);
				temp &= ~WM8994_SPKOUTL_BOOST_MASK;
				temp |= ((gain_val<<WM8994_SPKOUTL_BOOST_SHIFT)  & WM8994_SPKOUTL_BOOST_MASK);
				codec->write(codec,  WM8994_CLASSD, temp);
	//			printk("WM8994_CLASSD : \t\t\t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
			break;
		
		case WM8994_SPEAKER_VOLUME_LEFT : // SPKOUT_VU[8], SPKOUTL_MUTE_N[6], SPKOUTL_VOL [5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_SPEAKER_VOLUME_LEFT);
				temp &= ~WM8994_SPKOUTL_VOL_MASK;
				temp |= (WM8994_SPKOUT_VU | (gain_val<<WM8994_SPKOUTL_VOL_SHIFT)  & WM8994_SPKOUTL_VOL_MASK);
				codec->write(codec,  WM8994_SPEAKER_VOLUME_LEFT, temp);
//				printk("WM8994_SPEAKER_VOLUME_LEFT : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_SPKOUTL_MUTE_N_MASK;
				codec->write(codec,  WM8994_SPEAKER_VOLUME_LEFT, temp);
//				printk("WM8994_SPEAKER_VOLUME_LEFT [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_SPEAKER_VOLUME_RIGHT : // SPKOUT_VU[8], SPKOUTR_MUTE_N[6], SPKOUTR_VOL [5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
				temp &= ~WM8994_SPKOUTR_VOL_MASK;
				temp |= (WM8994_SPKOUT_VU | (gain_val<<WM8994_SPKOUTR_VOL_SHIFT)  & WM8994_SPKOUTR_VOL_MASK);
				codec->write(codec,  WM8994_SPEAKER_VOLUME_RIGHT, temp);
//				printk("WM8994_SPEAKER_VOLUME_RIGHT : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_SPKOUTR_MUTE_N_MASK;
				codec->write(codec,  WM8994_SPEAKER_VOLUME_RIGHT, temp);
//				printk("WM8994_SPEAKER_VOLUME_RIGHT [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_SPKMIXL_ATTENUATION : // SPKMIXL_VOL[1:0] : 3 mute
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_SPKMIXL_ATTENUATION);
				temp &= ~WM8994_SPKMIXL_VOL_MASK;
				temp |= ((gain_val<<WM8994_SPKMIXL_VOL_SHIFT)  & WM8994_SPKMIXL_VOL_MASK);
				codec->write(codec,  WM8994_SPKMIXL_ATTENUATION, temp);
//				printk("WM8994_SPKMIXL_ATTENUATION : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_SPKMIXL_VOL_MASK;
				temp |= ((0x3<<WM8994_SPKMIXL_VOL_SHIFT)  & WM8994_SPKMIXL_VOL_MASK);
				codec->write(codec,  WM8994_SPKMIXL_ATTENUATION, temp);
//				printk("WM8994_SPKMIXL_ATTENUATION [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_SPKMIXR_ATTENUATION : // SPKMIXL_VOL[1:0] : 3 mute
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_SPKMIXR_ATTENUATION);
				temp &= ~WM8994_SPKMIXR_VOL_MASK;
				temp |= ((gain_val<<WM8994_SPKMIXR_VOL_SHIFT)  & WM8994_SPKMIXR_VOL_MASK);
				codec->write(codec,  WM8994_SPKMIXR_ATTENUATION, temp);
//				printk("WM8994_SPKMIXR_ATTENUATION : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_SPKMIXR_VOL_MASK;
				temp |= ((0x3<<WM8994_SPKMIXR_VOL_SHIFT)  & WM8994_SPKMIXR_VOL_MASK);
				codec->write(codec,  WM8994_SPKMIXR_ATTENUATION, temp);
//				printk("WM8994_SPKMIXR_ATTENUATION [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_LEFT_OPGA_VOLUME : // MIXOUT_VU[8], MIXOUTL_MUTE_N[6], MIXOUTL_VOL[5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_LEFT_OPGA_VOLUME);
				temp &= ~WM8994_MIXOUTL_VOL_MASK;
				temp |= (WM8994_MIXOUT_VU | (gain_val<<WM8994_MIXOUTL_VOL_SHIFT)  & WM8994_MIXOUTL_VOL_MASK);
				codec->write(codec,  WM8994_LEFT_OPGA_VOLUME, temp);
//				printk("WM8994_LEFT_OPGA_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_MIXOUTL_MUTE_N_MASK;
				codec->write(codec,  WM8994_LEFT_OPGA_VOLUME, temp);
//				printk("WM8994_LEFT_OPGA_VOLUME [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_RIGHT_OPGA_VOLUME : // MIXOUT_VU[8], MIXOUTR_MUTE_N[6], MIXOUTR_VOL[5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_RIGHT_OPGA_VOLUME);
				temp &= ~WM8994_MIXOUTR_VOL_MASK;
				temp |= (WM8994_MIXOUT_VU | (gain_val<<WM8994_MIXOUTR_VOL_SHIFT)  & WM8994_MIXOUTR_VOL_MASK);
				codec->write(codec,  WM8994_RIGHT_OPGA_VOLUME, temp);
//				printk("WM8994_RIGHT_OPGA_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_MIXOUTR_MUTE_N_MASK;
				codec->write(codec,  WM8994_RIGHT_OPGA_VOLUME, temp);
//				printk("WM8994_RIGHT_OPGA_VOLUME [MUTE] : \t\t%d(0x%x), treg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_HPOUT2_VOLUME : // HPOUT2_MUTE[5], HPOUT2_VOL[4]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_HPOUT2_VOLUME);
				temp &= ~WM8994_HPOUT2_VOL_MASK;
				temp |= ((gain_val<<WM8994_HPOUT2_VOL_SHIFT)  & WM8994_HPOUT2_VOL_MASK);
				codec->write(codec,  WM8994_HPOUT2_VOLUME, temp);
//				printk("WM8994_HPOUT2_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_HPOUT2_MUTE_MASK;
				temp |= WM8994_HPOUT2_MUTE;
				codec->write(codec,  WM8994_HPOUT2_VOLUME, temp);
//				printk("WM8994_HPOUT2_VOLUME [MUTE] : \t\t%d(0x%x), treg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_LEFT_OUTPUT_VOLUME : // HPOUT1_VU[8], HPOUT1L_MUTE_N[6], HPOUT1L_VOL[5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_LEFT_OUTPUT_VOLUME);
				temp &= ~WM8994_HPOUT1L_VOL_MASK;
				if(apollo_get_remapped_hw_rev_pin() >= 6)
				{
					temp |= (WM8994_HPOUT1_VU | ((gain_val+4)<<WM8994_HPOUT1L_VOL_SHIFT)  & WM8994_HPOUT1L_VOL_MASK);
				}
				else
				{
					temp |= (WM8994_HPOUT1_VU | (gain_val<<WM8994_HPOUT1L_VOL_SHIFT)  & WM8994_HPOUT1L_VOL_MASK);
				}
				codec->write(codec,  WM8994_LEFT_OUTPUT_VOLUME, temp);
//				printk("WM8994_LEFT_OUTPUT_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_HPOUT1L_MUTE_N_MASK;
				codec->write(codec,  WM8994_LEFT_OUTPUT_VOLUME, temp);
//				printk("WM8994_LEFT_OUTPUT_VOLUME [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_RIGHT_OUTPUT_VOLUME : // HPOUT1_VU[8], HPOUT1R_MUTE_N[6], HPOUT1R_VOL[5:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_RIGHT_OUTPUT_VOLUME);
				temp &= ~WM8994_HPOUT1R_VOL_MASK;
				if(apollo_get_remapped_hw_rev_pin() >= 6)
				{
					temp |= (WM8994_HPOUT1_VU | ((gain_val+4)<<WM8994_HPOUT1R_VOL_SHIFT)  & WM8994_HPOUT1R_VOL_MASK);
				}
				else
				{
					temp |= (WM8994_HPOUT1_VU | (gain_val<<WM8994_HPOUT1R_VOL_SHIFT)  & WM8994_HPOUT1R_VOL_MASK);
				}
				codec->write(codec,  WM8994_RIGHT_OUTPUT_VOLUME, temp);
//				printk("WM8994_RIGHT_OUTPUT_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_HPOUT1R_MUTE_N_MASK;
				codec->write(codec,  WM8994_RIGHT_OUTPUT_VOLUME, temp);
//				printk("WM8994_RIGHT_OUTPUT_VOLUME [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_AIF1_DAC1_LEFT_VOLUME : // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME);
				temp &= ~WM8994_AIF1DAC1L_VOL_MASK;
				temp |= (WM8994_AIF1DAC1_VU | (gain_val<<WM8994_AIF1DAC1L_VOL_SHIFT)  & WM8994_AIF1DAC1L_VOL_MASK);
				codec->write(codec,  WM8994_AIF1_DAC1_LEFT_VOLUME, temp);
//				printk("WM8994_AIF1_DAC1_LEFT_VOLUME : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_AIF1DAC1L_VOL_MASK;
				temp |= WM8994_AIF1DAC1_VU;
				codec->write(codec,  WM8994_AIF1_DAC1_LEFT_VOLUME, temp);
//				printk("WM8994_AIF1_DAC1_LEFT_VOLUME : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		case WM8994_AIF1_DAC1_RIGHT_VOLUME : // AIF1ADC1_VU[8], AIF1ADC1L_VOL[7:0]
			

			if(flag)
			{
				temp = codec->read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME);
				temp &= ~WM8994_AIF1DAC1R_VOL_MASK;
				temp |= (WM8994_AIF1DAC1_VU | (gain_val<<WM8994_AIF1DAC1R_VOL_SHIFT)  & WM8994_AIF1DAC1R_VOL_MASK);
				codec->write(codec,  WM8994_AIF1_DAC1_RIGHT_VOLUME, temp);
//				printk("WM8994_AIF1_DAC1_RIGHT_VOLUME : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
				temp &= ~WM8994_AIF1DAC1R_VOL_MASK;
				temp |= WM8994_AIF1DAC1_VU;
				codec->write(codec,  WM8994_AIF1_DAC1_RIGHT_VOLUME, temp);
//				printk("WM8994_AIF1_DAC1_RIGHT_VOLUME : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;
		
		case WM8994_DAC1_LEFT_VOLUME : // DAC1L_MUTE[9], DAC1_VU[8], DAC1L_VOL[7:0]
//			temp = codec->read(codec, WM8994_DAC1_LEFT_VOLUME);
#if 0
			if(flag)
			{
				temp &= ~WM8994_DAC1L_VOL_MASK;
				temp |= (WM8994_DAC1_VU | (gain_val<<WM8994_DAC1L_VOL_SHIFT)  & WM8994_DAC1L_VOL_MASK);
				codec->write(codec,  WM8994_DAC1_LEFT_VOLUME, temp);
				printk("WM8994_DAC1_LEFT_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
			else
			{
				temp &= ~WM8994_DAC1L_MUTE_MASK;
				temp |= WM8994_DAC1L_MUTE;
				codec->write(codec,  WM8994_DAC1_LEFT_VOLUME, temp);
				printk("WM8994_DAC1_LEFT_VOLUME [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#else
//			printk("WM8994_DAC1_LEFT_VOLUME : Don't setting reg(0x%x = 0x%x) \n", gain_reg, temp);
#endif
			break;
		
		case WM8994_DAC1_RIGHT_VOLUME : // DAC1R_MUTE[9], DAC1_VU[8], DAC1R_VOL[7:0]
//			temp = codec->read(codec, WM8994_DAC1_RIGHT_VOLUME);
#if 0
			if(flag)
			{
				temp &= ~WM8994_DAC1R_VOL_MASK;
				temp |= (WM8994_DAC1_VU | (gain_val<<WM8994_DAC1R_VOL_SHIFT)  & WM8994_DAC1R_VOL_MASK);
				codec->write(codec,  WM8994_DAC1_RIGHT_VOLUME, temp);
				printk("WM8994_DAC1_RIGHT_VOLUME : \t\t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
			else
			{
				temp &= ~WM8994_DAC1R_MUTE_MASK;
				temp |= WM8994_DAC1R_MUTE;
				codec->write(codec,  WM8994_DAC1_RIGHT_VOLUME, temp);
				printk("WM8994_DAC1_RIGHT_VOLUME [MUTE] : \t\t%d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#else
//			printk("WM8994_DAC1_RIGHT_VOLUME : Don't setting reg(0x%x = 0x%x) \n", gain_reg, temp);
#endif
			break;

		case WM8994_AIF2_CONTROL_2 : // AIF2DAC_BOOST[11:10]


			if(flag)
			{
				temp = codec->read(codec, WM8994_AIF2_CONTROL_2);
				temp &= ~WM8994_AIF2DAC_BOOST_MASK;
				temp |= ((gain_val<<WM8994_AIF2DAC_BOOST_SHIFT)  & WM8994_AIF2DAC_BOOST_MASK);
				codec->write(codec,  WM8994_AIF2_CONTROL_2, temp);
//				printk("WM8994_AIF2_CONTROL_2 : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#if 0
			else
			{
//				printk("WM8994_AIF2_CONTROL_2 : %d(0x%x), reg(0x%x)\n", gain_val, temp, gain_reg);
			}
#endif
			break;

		default :
//			printk(KERN_ERR "Unknown register (0x%x)\n", gain_reg);
			break;
	}

}

int wm8994_get_gain_table_index(int codec_path)
{
	int mode, out_path, tbl_index;
	mode = (codec_path & 0x00F0);
	out_path = (codec_path & 0x00FF);
	if(mode == MM_AUDIO_PLAYBACK) // PLAY_BACK
	{
		if(out_path == MM_AUDIO_PLAYBACK_RCV)
			tbl_index = 1;
		else if(out_path == MM_AUDIO_PLAYBACK_SPK)
			tbl_index = 2;
		else if(out_path == MM_AUDIO_PLAYBACK_HP)
			tbl_index = 3;
		else if(out_path == MM_AUDIO_PLAYBACK_BT)
			tbl_index = 4;
		else if(out_path == MM_AUDIO_PLAYBACK_SPK_HP)
			tbl_index = 5;
		else if(out_path == MM_AUDIO_PLAYBACK_RING_SPK_HP)
			tbl_index = 6;
		else
		{
			printk(KERN_ERR "%s : Unknown playback path\n", __func__);
			tbl_index = 0;
		}
	}
	else if(mode == MM_AUDIO_VOICECALL) // VOICE_CALL
	{
		if(out_path == MM_AUDIO_VOICECALL_RCV)
			tbl_index = 7;
		else if(out_path == MM_AUDIO_VOICECALL_SPK)
			tbl_index = 8;
		else if(out_path == MM_AUDIO_VOICECALL_HP)
			tbl_index = 9;
		else if(out_path == MM_AUDIO_VOICECALL_BT)
			tbl_index = 10;
		else if(out_path == MM_AUDIO_VOICECALL_SPK_LOOP)
			tbl_index = 11;
		else
		{
			printk(KERN_ERR "%s : Unknown voice call path\n", __func__);
			tbl_index = 0;
		}
	}
	else if(mode == MM_AUDIO_VOICEMEMO) // VOICE_MEMO
	{
		if(out_path == MM_AUDIO_VOICEMEMO_MAIN)
			tbl_index = 12;
		else if(out_path == MM_AUDIO_VOICEMEMO_SUB)
			tbl_index = 13;
		else if(out_path == MM_AUDIO_VOICEMEMO_EAR)
			tbl_index = 14;
		else if(out_path == MM_AUDIO_VOICEMEMO_BT)
			tbl_index = 15;
		else
		{
			printk(KERN_ERR "%s : Unknown voice memo path\n", __func__);
			tbl_index = 0;
		}
	}
	else if(mode == MM_AUDIO_FMRADIO) // FM_RADIO
	{
		if(out_path == MM_AUDIO_FMRADIO_RCV)
			tbl_index = 16;
		else if(out_path == MM_AUDIO_FMRADIO_SPK)
			tbl_index = 17;
		else if(out_path == MM_AUDIO_FMRADIO_HP)
			tbl_index = 18;
		else if(out_path == MM_AUDIO_FMRADIO_BT)
			tbl_index = 19;
		else if(out_path == MM_AUDIO_FMRADIO_SPK_HP)
			tbl_index = 20;
		else
		{
			printk(KERN_ERR "%s : Unknown fm radio path\n", __func__);
			tbl_index = 0;
		}
	}
	else // UNKNOWN
	{
		printk(KERN_ERR "%s : unknown codec mode\n", __func__);
		tbl_index = 0;
	}

	printk("%s : tbl_index : %d\n", __func__, tbl_index);
	return tbl_index;
}


void wm8994_set_default_gain(struct snd_soc_codec *codec, int codec_path)
{
//	struct gain_info_t gain_table_val;
	int i, path_i;
	unsigned short flag, gain_reg, gain_val;

//	gain_table_val = codec_vol_table[wm8994_get_gain_table_index(codec_path)][0];

	path_i = wm8994_get_gain_table_index(codec_path);

	for(i = 0; i < GAIN_POINT_NUM; i++){
//		if(gain_table_val[i].flag) {
/*
			flag = gain_table_val[i].flag;
			gain_reg = gain_table_val[i].reg;
			gain_val = gain_table_val[i].gain;
*/
			flag = codec_vol_table[path_i][i].flag;
			gain_reg = codec_vol_table[path_i][i].reg;
			gain_val = codec_vol_table[path_i][i].gain;
			
			wm8994_set_volume(codec, flag, gain_reg, gain_val);
//		}
	}
	
}


