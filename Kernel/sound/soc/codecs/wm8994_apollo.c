#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <plat/regs-clock.h>
#include <mach/apollo.h>

//#ifdef CONFIG_SND_VOODOO
#include "wm8994_voodoo.h"
//#endif

#include "wm8994.h"
#include "wm8994_gain.h"

#define DAC_TO_DIRECT_HPOUT1 // when playback headset, DAC1 directly connected to HPOUT1 mixer (not passed through MIXOUTL R)
#define VOICE_PCM_IF
#define FM_PATH_DRC_BLOCK

#define AUDIO_COMMON_DEBUG
#define SUBJECT "WM8994_APOLLO"

#ifdef AUDIO_COMMON_DEBUG
#define DEBUG_LOG_L1(format,...)\
      printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

#define DEBUG_LOG(format,...)\
   printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#endif


int enable_path_mode;

extern int wm8994_rev;
extern int data_call;
// extern int wm8994_voice_recognition;

/*Audio Routing routines for the universal board..wm8994 codec*/
int audio_init(void)
{
   printk("[WM8994] %s\n", __func__);
   /* AUDIO_EN */
   if (gpio_is_valid(GPIO_CODEC_LDO_EN)) {
      if (gpio_request( GPIO_CODEC_LDO_EN, "CODEC_LDO_EN")) 
         printk(KERN_ERR "Failed to request CODEC_LDO_EN! \n");
      gpio_direction_output(GPIO_CODEC_LDO_EN, 0);
   }
   s3c_gpio_setpull(GPIO_CODEC_LDO_EN, S3C_GPIO_PULL_NONE);

   /* MICBIAS_EN */
   if (gpio_is_valid(GPIO_MICBIAS2_EN)) {
      if (gpio_request(GPIO_MICBIAS2_EN, "MICBIAS2_EN")) 
         printk(KERN_ERR "Failed to request MICBIAS2_EN! \n");
      gpio_direction_output(GPIO_MICBIAS2_EN, get_headset_status());
   }
   s3c_gpio_setpull(GPIO_MICBIAS2_EN, S3C_GPIO_PULL_NONE);

   /* Ear Select */
   if(gpio_is_valid(GPIO_EAR_SEL)) {
   if (gpio_request( GPIO_EAR_SEL, "EAR3.5_SW")) 
       printk(KERN_ERR "failed to request GPIO_EAR_SEL for codec control\n");
      gpio_direction_output( GPIO_EAR_SEL , 1);
   }
        gpio_set_value(GPIO_EAR_SEL, 0);
   return 0;
}

int audio_power(int en)
{
   int err;
   u32 temp;

// printk("[WM8994] %s (%d)\n", __func__, en);
   
   temp = __raw_readl(S5P_CLK_OUT);

   // In univ6442 board, CLK_OUT is connected with WM8994 MCLK(main clock)
   // CLK_OUT : bypass external crystall clock
   temp &= 0xFFFE0FFF; // clear bits CLKSEL[16:12]

   if(en)
   {
      gpio_set_value(GPIO_CODEC_LDO_EN, 1);
      msleep(200); // Codec needs about 160 ms delay after LDO enabled for i2c communication - Wolfoson report
      
#ifdef CONFIG_SND_WM8994_MASTER_MODE
      temp |= (0x11 << 12);   // crystall : CLKSEL 10001 = OSSCLK
//    printk("\n[WM8994 Master Mode] : using crystall clock(CLK_OUT) to drive WM8994 \n");
#else
      temp |= (0x02 << 12);   // epll : CLKSEL 00010 = FOUTEPLL
      temp |= (0x5 << 20); // DIVVAL(Divide rateio = DIVVAL + 1 = 6)
#endif
//    printk("CLKOUT reg is %x\n",temp);
      __raw_writel(temp, S5P_CLK_OUT); 
      temp = __raw_readl(S5P_CLK_OUT);
//    printk("CLKOUT reg is %x\n",temp);

   }
   else
   {
      gpio_set_value(GPIO_CODEC_LDO_EN, 0);
#ifdef CONFIG_SND_WM8994_MASTER_MODE
      temp &= ~(0x11 << 12); //crystall
#else
      temp &= ~(0x02 << 12); // epll
      temp &= ~(0x5 << 20);
#endif
//    printk("CLKOUT reg is %x\n",temp);
      __raw_writel(temp, S5P_CLK_OUT); 
      temp = __raw_readl(S5P_CLK_OUT);
//    printk("CLKOUT reg is %x\n",temp);
   }

// DEBUG_LOG("AUDIO POWER COMPLETED : %d", en);
   return 0;
}



void wm8994_disable_playback_path(struct snd_soc_codec *codec, int mode)
{
   u16 val;

   return; // do not disable playback to remove pop noise
   val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );

   switch(mode)
   {
      case MM_AUDIO_PLAYBACK_RCV:
      printk("..Disabling RCV\n");
      val&= ~(WM8994_HPOUT2_ENA_MASK);
      //disbale the HPOUT2 
      break;

      case MM_AUDIO_PLAYBACK_SPK:
      printk("..Disabling SPK\n");
      //disbale the SPKOUTL
      val&= ~(WM8994_SPKOUTL_ENA_MASK ); 
      break;

      case MM_AUDIO_PLAYBACK_HP:
      printk("..Disabling HP\n");
      //disble the HPOUT1
      val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK );
      break;

      case MM_AUDIO_OUT_SPK_HP:
      case MM_AUDIO_OUT_RING_SPK_HP:
      printk("..Disabling SPK & HP\n");
      //disble the SPKOUTL & HPOUT1
      val&=~(WM8994_SPKOUTL_ENA_MASK | WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK );
      break;

      default:
      break;

   }
      wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
}


void wm8994_playback_route_headset(struct snd_soc_codec *codec)
{
   u16 val;
   struct wm8994_priv *wm8994 = codec->private_data;
   wm8994->stream_state |= PLAYBACK_ACTIVE;
   wm8994->stream_state |= PCM_STREAM_PLAYBACK;

   u16 TestReturn1=0;
   u16 TestReturn2=0;
   u16 TestLow1=0;
   u16 TestHigh1=0;
   u8 TestLow=0;
   u8 TestHigh=0;

   printk("%s() \n",__func__);

   wm8994_write(codec,0x00,0x0000);
   wm8994_write(codec,0x01,0x0005);
   msleep(50);
   // FLL setting from Wolfson
   wm8994_write(codec,0x39,0x0060);
   wm8994_write(codec,0x01,0x0003);
   msleep(50);


   wm8994_write(codec,0x224,0x0C80);
   wm8994_write(codec,0x221,0x0700);
   wm8994_write(codec,0x222,0x86C2);
   wm8994_write(codec,0x223,0x00E0);
   wm8994_write(codec,0x220,0x0005);
   msleep(5);
   wm8994_write(codec,0x208,0x000A);
   wm8994_write(codec,0x300,0x4010);
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec,0x700,0xA101);
   wm8994_write(codec,0x210,0x0073);
   wm8994_write(codec,0x200,0x0011);
   
   wm8994_write(codec,0x601,0x0001);
   wm8994_write(codec,0x602,0x0001);

   wm8994_write(codec,0x610,0x00C0);
   wm8994_write(codec,0x611,0x00C0);

/* ==================== Output Paths Configuration ==================== */
#ifdef DAC_TO_DIRECT_HPOUT1
   // Enbale DAC1L to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
   val |= WM8994_DAC1L_TO_HPOUT1L ;    
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale DAC1R to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
   val |= WM8994_DAC1R_TO_HPOUT1R ;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= WM8994_DAC1L_TO_MIXOUTL;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= WM8994_DAC1R_TO_MIXOUTR;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif


/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );

/* ==================== Output Paths Configuration ==================== */
   // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
   val |= WM8994_HPOUT1L_MUTE_N;
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
   val |= WM8994_HPOUT1R_MUTE_N;
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);

/* ==================== EAR_SWITCH_SEL ==================== */
// gpio_set_value(GPIO_EAR_SEL, 0);
   if(data_call)
      gpio_set_value(GPIO_EAR_SEL, 1);
   else
      gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Disable all EQ ==================== */
   val = wm8994_read(codec, WM8994_AIF1_DAC1_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC1_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF1_DAC2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC2_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC2_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF2DAC_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF2_EQ_GAINS_1, val);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //-------------------------------------- Analog Output
#ifndef DAC_TO_DIRECT_HPOUT1
   //MIXOUT_VOL
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
   val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
   val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
#endif
   if(apollo_get_remapped_hw_rev_pin() >= 6)
   {
      //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
      if(data_call)
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL_DATACALL);
      else
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL);
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
      if(data_call)
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL_DATACALL);
      else
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL);
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }
   else
   {
   //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_HP_VOL-4));
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_HP_VOL-4));
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }

   //Disable end point for preventing pop up noise.
   wm8994_write(codec,0x102,0x0003);
   wm8994_write(codec,0x56,0x0003);
   wm8994_write(codec,0x102,0x0000);
// wm8994_write(codec,WM8994_CLASS_W_1,0x0005);
   wm8994_write(codec,0x5D,0x0002);
   wm8994_write(codec,WM8994_DC_SERVO_2,0x03E0);

/* ==================== Bias Configuration ==================== */

   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0303);
   wm8994_write(codec,WM8994_ANALOGUE_HP_1,0x0022);
   
   //Enable Charge Pump 
   wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x9F25);

   msleep(5);  // 5ms delay

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,0x0303);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,0x0030);  

   wm8994_write(codec,WM8994_DC_SERVO_1,0x0303);
   
   msleep(160);   // 160ms delay

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-DC_SERVO_OFFSET)&0x00ff;
   TestHigh1=(((u16)TestHigh-DC_SERVO_OFFSET)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

#if 1
   wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);

   msleep(20);
    
   // Intermediate HP settings 
   val = wm8994_read(codec, WM8994_ANALOGUE_HP_1);    
   val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
      WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK );
   val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |
      WM8994_HPOUT1R_RMV_SHORT | WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
   wm8994_write(codec, WM8994_ANALOGUE_HP_1 ,val);
#else
   wm8994_write(codec,  0x3000, 0x0054);
   wm8994_write(codec,  0x3001, 0x000F);
   wm8994_write(codec, 0x3002, 0x0300);
   wm8994_write(codec,  0x3003, 0x0008); // delay 16ms

   wm8994_write(codec,  0x3004, 0x0060);
   wm8994_write(codec, 0x3005, 0x00EE);
   wm8994_write(codec, 0x3006, 0x0700);
   wm8994_write(codec,  0x3007, 0x0008);
    
   wm8994_write(codec, 0x3008, 0x0420);
   wm8994_write(codec, 0x3009, 0x0000);
   wm8994_write(codec, 0x300A, 0x0008);
   wm8994_write(codec, 0x300B, 0x010D); // wm seq end

   wm8994_write(codec, 0x110, 0x8100); //8100 -> 8113
#endif

}

void wm8994_playback_route_speaker(struct snd_soc_codec *codec)
{
   u16 val;
   struct wm8994_priv *wm8994 = codec->private_data;
   wm8994->stream_state |= PLAYBACK_ACTIVE;
   wm8994->stream_state |= PCM_STREAM_PLAYBACK;

   printk("%s() \n",__func__);

   wm8994_write(codec,0x00,0x0000);
   wm8994_write(codec,0x01,0x0005);
   msleep(50);
   
   // FLL setting from Wolfson
   wm8994_write(codec,0x39,0x0060);
   wm8994_write(codec,0x01,0x0003);
   msleep(50);
   wm8994_write(codec,0x224,0x0C80);
   wm8994_write(codec,0x221,0x0700);
   wm8994_write(codec,0x222,0x86C2);
   wm8994_write(codec,0x223,0x00E0);
   wm8994_write(codec,0x220,0x0005);
   msleep(5);
   wm8994_write(codec,0x208,0x000A);
   wm8994_write(codec,0x300,0x4010);
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec,0x700,0xA101);
   wm8994_write(codec,0x210,0x0073);
   wm8994_write(codec,0x200,0x0011);
   
   wm8994_write(codec,0x601,0x0001);
   wm8994_write(codec,0x602,0x0001);

   wm8994_write(codec,0x610,0x00C0);
   wm8994_write(codec,0x611,0x00C0);

/* ==================== Bias Configuration ==================== */
   //Enbale bias,vmid and Left speaker
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
   val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_SPKOUTL_ENA_MASK |WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
   val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |  WM8994_SPKOUTL_ENA  );  
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
   
/* ==================== Digital Paths Configuration ==================== */
   //Enable Dac1 and DAC2 and the Timeslot0 for AIF1
   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 );    
   val &= ~(WM8994_AIF1DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_DAC1R_ENA_MASK);
   val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);

   // Unmute the AF1DAC1
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   // AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );

/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== Disable all EQ ==================== */

   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_1, 0x6319);
   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_2, 0x62C0); 

   val = wm8994_read(codec, WM8994_AIF1_DAC1_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC1_EQ_ENA_MASK);
   val |= 0x0001;
   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF1_DAC2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC2_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC2_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF2DAC_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF2_EQ_GAINS_1, val);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= (TUNE_PLAYBACK_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
   wm8994_write(codec, WM8994_CLASSD, val);
}

void wm8994_playback_route_receiver(struct snd_soc_codec *codec)
{
   u16 val;
   printk("%s() \n",__func__);

   wm8994_write(codec,0x00,0x0000);
   wm8994_write(codec,0x01,0x0005);
   msleep(50);
   
/* ==================== Bias Configuration ==================== */
   //Enbale bias,vmid and Left speaker
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
   val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_HPOUT2_ENA_MASK |WM8994_SPKOUTL_ENA_MASK | WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK);
   val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |  WM8994_HPOUT2_ENA  ); 
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
   
   msleep(50);
   
   wm8994_write(codec,0x224,0x0C80);
   wm8994_write(codec,0x221,0x0700);
   wm8994_write(codec,0x222,0x86C2);
   wm8994_write(codec,0x223,0x00E0);
   wm8994_write(codec,0x220,0x0005);
   msleep(5);
   wm8994_write(codec,0x208,0x000A);
   wm8994_write(codec,0x300,0x4010);
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec,0x700,0xA101);
   wm8994_write(codec,0x210,0x0073);
   wm8994_write(codec,0x200,0x0011);
   
   wm8994_write(codec,0x601,0x0001);
   wm8994_write(codec,0x602,0x0001);

   wm8994_write(codec,0x610,0x00C0);
   wm8994_write(codec,0x611,0x00C0);

/* ==================== Digital Paths Configuration ==================== */
   //Enable Dac1 and DAC2 and the Timeslot0 for AIF1
   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 );    
   val &= ~(WM8994_AIF1DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_DAC1R_ENA_MASK);
   val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);
   // Unmute the AF1DAC1
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   // AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );

#ifdef CONFIG_SND_VOODOO
  voodoo_hook_playback_speaker();
#endif

/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
       val |= (WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTLVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);

   // DAC1L -> MIXOUTL
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, 0x0001 ); // DAC1L_TO_HPOUT1L = 0, DAC1L_TO_MIXOUTL = 1
   // DAC1R -> MIXOUTR
   wm8994_write(codec, WM8994_OUTPUT_MIXER_2, 0x0001 ); // DAC1R_TO_HPOUT1R = 0, DAC1R_TO_MIXOUTR = 1, 

   // MIXOUTL_MUTE_N, MIXOUTR_MUTE_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUTL_MUTE_N_MASK);
   val |= WM8994_MIXOUTL_MUTE_N;
   wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUTR_MUTE_N_MASK);
   val |= WM8994_MIXOUTR_MUTE_N;
   wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val);
   // MIXOUTL, R -> HPOUT2
   wm8994_write(codec, WM8994_HPOUT2_MIXER, 0x0018 ); // MIXOUTLVOL_TO_HPOUT2 unmute, MIXOUTRVOL_TO_HPOUT2 unmute
   // Unmute HPOUT2 ( RCV )
   val = wm8994_read(codec, WM8994_HPOUT2_VOLUME );
   val &= ~(WM8994_HPOUT2_MUTE_MASK);
   wm8994_write(codec, WM8994_HPOUT2_VOLUME, val );

/* ==================== Disable all EQ ==================== */
   val = wm8994_read(codec, WM8994_AIF1_DAC1_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC1_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF1_DAC2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC2_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC2_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF2DAC_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF2_EQ_GAINS_1, val);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //-------------------------------------- Analog Output
   //MIXOUT_VOL
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
   val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
   val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
   //MIXOUTLVOL, MIXOUTRVOL
   val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUT_VU_MASK | WM8994_MIXOUTL_VOL_MASK);
   val |= (WM8994_MIXOUT_VU | 0x39); // 0 dB
   wm8994_write(codec, WM8994_LEFT_OPGA_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUT_VU_MASK | WM8994_MIXOUTR_VOL_MASK);
   val |= (WM8994_MIXOUT_VU | 0x39); // 0 dB
   wm8994_write(codec, WM8994_RIGHT_OPGA_VOLUME, val);
   // HPOUT2_VOL
   val = wm8994_read(codec, WM8994_HPOUT2_VOLUME );
   val &= ~(WM8994_HPOUT2_VOL_MASK);
   val |= 0x0000; // 
   wm8994_write(codec, WM8994_HPOUT2_VOLUME, val );
}

void wm8994_playback_route_speaker_headset(struct snd_soc_codec *codec)
{
   u16 val;
   
   u16 TestReturn1=0;
   u16 TestReturn2=0;
   u16 TestLow1=0;
   u16 TestHigh1=0;
   u8 TestLow=0;
   u8 TestHigh=0;
   
   printk("%s() \n",__func__);

   wm8994_write(codec,0x00,0x0000);
   wm8994_write(codec,0x01,0x0005);
   msleep(50);
   // FLL setting from Wolfson
   wm8994_write(codec,0x39,0x0060);
   wm8994_write(codec,0x01,0x0003);
   msleep(50);

   
   wm8994_write(codec,0x224,0x0C80);
   wm8994_write(codec,0x221,0x0700);
   wm8994_write(codec,0x222,0x86C2);
   wm8994_write(codec,0x223,0x00E0);
   wm8994_write(codec,0x220,0x0005);
   msleep(5);
   wm8994_write(codec,0x208,0x000A);
   wm8994_write(codec,0x300,0x4010);
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec,0x700,0xA101);
   wm8994_write(codec,0x210,0x0073);
   wm8994_write(codec,0x200,0x0011);
   
   wm8994_write(codec,0x601,0x0001);
   wm8994_write(codec,0x602,0x0001);

   wm8994_write(codec,0x610,0x00C0);
   wm8994_write(codec,0x611,0x00C0);

   //Disable end point for preventing pop up noise.
   wm8994_write(codec,0x102,0x0003);
   wm8994_write(codec,0x56,0x0003);
   wm8994_write(codec,0x102,0x0000);
// wm8994_write(codec,WM8994_CLASS_W_1,0x0005);
   wm8994_write(codec,0x5D,0x0002);
   wm8994_write(codec,WM8994_DC_SERVO_2,0x03E0);

/* ==================== Bias Configuration ==================== */

   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x1303);
   wm8994_write(codec,WM8994_ANALOGUE_HP_1,0x0022);
   
   //Enable Charge Pump 
   wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x9F25);

   msleep(5);  // 5ms delay

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,0x0303);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,0x0330);

/* ==================== Output Paths Configuration ==================== */
#ifdef DAC_TO_DIRECT_HPOUT1
   // Enbale DAC1L to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
   val |= WM8994_DAC1L_TO_HPOUT1L ;    
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale DAC1R to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
   val |= WM8994_DAC1R_TO_HPOUT1R ;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= WM8994_DAC1L_TO_MIXOUTL;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= WM8994_DAC1R_TO_MIXOUTR;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif


/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );

/* ==================== Output Paths Configuration ==================== */
   // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);
   val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
   val |= WM8994_HPOUT1L_MUTE_N;
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
   val |= WM8994_HPOUT1R_MUTE_N;
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);

   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);

   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Disable all EQ ==================== */
   val = wm8994_read(codec, WM8994_AIF1_DAC1_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC1_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC1_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF1_DAC2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF1DAC2_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF1_DAC2_EQ_GAINS_1, val);

   val = wm8994_read(codec, WM8994_AIF2_EQ_GAINS_1 );
   val &= ~(WM8994_AIF2DAC_EQ_ENA_MASK);
   wm8994_write(codec,WM8994_AIF2_EQ_GAINS_1, val);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);

#ifndef DAC_TO_DIRECT_HPOUT1
   //MIXOUT_VOL
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
   val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
   val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
#endif

   if(enable_path_mode == MM_AUDIO_PLAYBACK_SPK_HP)
   {
      // SPKLVOL, SPKRVOL
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
      if(apollo_get_hw_type()) // GT-I5800
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL_OPEN);
      else // GT-I5801
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
      if(apollo_get_hw_type()) // GT-I5800
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL_OPEN);
      else // GT-I5801
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
      // SPKOUTLBOOST
      val = wm8994_read(codec, WM8994_CLASSD);
      val &= ~(WM8994_SPKOUTL_BOOST_MASK);
      val |= (TUNE_PLAYBACK_NOTI_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
      wm8994_write(codec, WM8994_CLASSD, val);

      if(apollo_get_remapped_hw_rev_pin() >= 6)
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_NOTI_HP_VOL);
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_NOTI_HP_VOL);
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
      else
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_NOTI_HP_VOL-4));
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_NOTI_HP_VOL-4));
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
   }
   else if(enable_path_mode == MM_AUDIO_PLAYBACK_RING_SPK_HP)
   {
      // SPKLVOL, SPKRVOL
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
      if(apollo_get_hw_type()) // GT-I5800
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL_OPEN);
      else // GT-I5801
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
      if(apollo_get_hw_type()) // GT-I5800
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL_OPEN);
      else // GT-I5801
         val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
      // SPKOUTLBOOST
      val = wm8994_read(codec, WM8994_CLASSD);
      val &= ~(WM8994_SPKOUTL_BOOST_MASK);
      val |= (TUNE_PLAYBACK_RING_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
      wm8994_write(codec, WM8994_CLASSD, val);

      if(apollo_get_remapped_hw_rev_pin() >= 6)
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_RING_HP_VOL);
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_RING_HP_VOL);
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
      else
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_RING_HP_VOL-4));
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_RING_HP_VOL-4));
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
   }
   else
   {
      // SPKLVOL, SPKRVOL
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
      val |= (WM8994_SPKOUT_VU | 0x3F);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
      val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
      val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
      val |= (WM8994_SPKOUT_VU | 0x3F);
      wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
      // SPKOUTLBOOST
      val = wm8994_read(codec, WM8994_CLASSD);
      val &= ~(WM8994_SPKOUTL_BOOST_MASK);
      val |= (0x2<<WM8994_SPKOUTL_BOOST_SHIFT); 
      wm8994_write(codec, WM8994_CLASSD, val);

      if(apollo_get_remapped_hw_rev_pin() >= 6)
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | 0x23);
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | 0x23);
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
      else
      {
         //HPOUT_VOL
         val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | 0x19);
         wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
         val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
         val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
         val |= (WM8994_HPOUT1_VU_MASK | 0x19);
         wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
      }
   }

//==============================================

   wm8994_write(codec,WM8994_DC_SERVO_1,0x0303);
   
   msleep(160);   // 160ms delay

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-DC_SERVO_OFFSET)&0x00ff;
   TestHigh1=(((u16)TestHigh-DC_SERVO_OFFSET)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

#if 1
   wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);

   msleep(20);

   // Intermediate HP settings 
   val = wm8994_read(codec, WM8994_ANALOGUE_HP_1);    
   val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
      WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK );
   val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |
      WM8994_HPOUT1R_RMV_SHORT | WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
   wm8994_write(codec, WM8994_ANALOGUE_HP_1 ,val);
#else
   wm8994_write(codec,  0x3000, 0x0054);
   wm8994_write(codec,  0x3001, 0x000F);
   wm8994_write(codec, 0x3002, 0x0300);
   wm8994_write(codec,  0x3003, 0x0008); // delay 16ms

   wm8994_write(codec,  0x3004, 0x0060);
   wm8994_write(codec, 0x3005, 0x00EE);
   wm8994_write(codec, 0x3006, 0x0700);
   wm8994_write(codec,  0x3007, 0x0008);

   wm8994_write(codec, 0x3008, 0x0420);
   wm8994_write(codec, 0x3009, 0x0000);
   wm8994_write(codec, 0x300A, 0x0008);
   wm8994_write(codec, 0x300B, 0x010D); // wm seq end

    wm8994_write(codec, 0x110, 0x8100); //8100 -> 8113
#endif
}

void wm8994_disable_rec_path(struct snd_soc_codec *codec, int mode) 
{
        u16 val;
        val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);

        switch(mode)
        {
                case MM_AUDIO_VOICEMEMO_MAIN:
                //printk("Disabling MAIN Mic Path..\n");
                val&= ~(WM8994_MICB1_ENA_MASK);
                break;
      
                case MM_AUDIO_VOICEMEMO_SUB:
                //printk("Disbaling SUB Mic path..\n");
                val&= ~(WM8994_MICB1_ENA_MASK);
                break;
   
      default:
                break;
        }

        wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);
}

void wm8994_record_headset_mic(struct snd_soc_codec *codec) 
{
   u16 val;

   printk("%s() \n",__func__);

/* ==================== Bias Configuration ==================== */

#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
      gpio_set_value(GPIO_MICBIAS2_EN, 1);
#endif

   //Enable Right Input Mixer,Enable IN1R PGA
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2 );
   val &= ~(WM8994_IN1R_ENA_MASK |WM8994_MIXINR_ENA_MASK  );
   val |= (WM8994_MIXINR_ENA | WM8994_IN1R_ENA );
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);
   
/* ==================== Input Path Configuration ==================== */
   // Enable volume,unmute Right Line  
   val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);   
   val &= ~(WM8994_IN1R_MUTE_MASK);
   wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, val);
   //IN1RP_TO_IN1R, IN1RN_TO_IN1R
   val = wm8994_read(codec,WM8994_INPUT_MIXER_2);
   val &= ~( WM8994_IN1RN_TO_IN1R_MASK | WM8994_IN1RP_TO_IN1R_MASK);
   val |= (WM8994_IN1RN_TO_IN1R | WM8994_IN1RP_TO_IN1R)  ;  
   wm8994_write(codec,WM8994_INPUT_MIXER_2,val);
   // IN1R_TO_MIXNR
   val = wm8994_read(codec,WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN1R_TO_MIXINR_MASK |WM8994_IN2R_TO_MIXINR_MASK );   
   val |= WM8994_IN1R_TO_MIXINR; 
   wm8994_write(codec,WM8994_INPUT_MIXER_4 ,val);
   
/* ==================== Digital Path Configuration ==================== */
   //Digital Paths
   //Enable right ADC and time slot
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4);
   val &= ~(WM8994_ADCL_ENA_MASK|WM8994_ADCR_ENA_MASK |WM8994_AIF1ADC1L_ENA_MASK|WM8994_AIF1ADC1R_ENA_MASK);
   val |= (WM8994_AIF1ADC1L_ENA|WM8994_AIF1ADC1R_ENA |WM8994_ADCL_ENA |WM8994_ADCR_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);
   
   //ADC Right mixer routing
   val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING);
   val &= ~( WM8994_ADC1R_TO_AIF1ADC1R_MASK);
   val |= WM8994_ADC1R_TO_AIF1ADC1R;
   wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING,val);
   
   //ADC Left mixer routing
   val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING);
   val &= ~( WM8994_ADC1L_TO_AIF1ADC1L_MASK);
   val |= WM8994_ADC1L_TO_AIF1ADC1L;
   wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING,val);

   // AIF1 CONTROL1 : Right ADC to right / left
   val = wm8994_read(codec,WM8994_AIF1_CONTROL_1);
   val &= ~(WM8994_AIF1ADCL_SRC_MASK|WM8994_AIF1ADCR_SRC_MASK);
   val |= WM8994_AIF1ADCL_SRC|WM8994_AIF1ADCR_SRC;
   wm8994_write(codec,WM8994_AIF1_CONTROL_1,val);

   wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable

/* ==================== Set default Gain ==================== */

/*
   if(wm8994_voice_recognition)
   {
      wm8994_write(codec, 0x410, 0x1800); // Filter
      
      //-------------------------------------- Analog Input
      // Enable volume, unmute Right Line 
      val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);   
      val &= ~(WM8994_IN1R_VU_MASK | WM8994_IN1R_VOL_MASK);
      val |= (WM8994_IN1R_VU | TUNE_MEMO_EAR_MIC_VOL);
      wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, val);
      //IN1R_TO_MIXINR
      val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
      val &= ~(WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
      wm8994_write(codec, WM8994_INPUT_MIXER_4, val ); 
      //-------------------------------------- Digital
      //AIF1_ADC1_R VOL
      val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_VOLUME);  
      val &= ~(WM8994_AIF1ADC1_VU_MASK|WM8994_AIF1ADC1R_VOL_MASK);
      val |= (WM8994_AIF1ADC1_VU | 0xE5);
      wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_VOLUME, val);
   }
   else
*/      
   {
      //-------------------------------------- Analog Input
      // Enable volume, unmute Right Line 
      val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);   
      val &= ~(WM8994_IN1R_VU_MASK | WM8994_IN1R_VOL_MASK);
      val |= (WM8994_IN1R_VU | TUNE_MEMO_EAR_MIC_VOL);
      wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, val);
      //IN1R_TO_MIXINR
      val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
      val &= ~(WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
      val |= WM8994_IN1R_MIXINR_VOL | 0x0007; // +30 dB, MIXOUTR_TO_MIXINR_VOL 6 dB
      wm8994_write(codec, WM8994_INPUT_MIXER_4, val ); 
      //-------------------------------------- Digital
      //AIF1_ADC1_R VOL
      val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_VOLUME);  
      val &= ~(WM8994_AIF1ADC1_VU_MASK|WM8994_AIF1ADC1R_VOL_MASK);
      val |= (WM8994_AIF1ADC1_VU | 0xC0);
      wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_VOLUME, val);
   }
}

void wm8994_record_main_mic(struct snd_soc_codec *codec)
{
   u16 val;

   printk("%s() \n",__func__);
   
/* ==================== Output Path Configuration ==================== */
   val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK|WM8994_MIXINR_TO_SPKMIXR_MASK | WM8994_IN1LP_TO_SPKMIXL | WM8994_IN1RP_TO_SPKMIXR);
   wm8994_write(codec,WM8994_SPEAKER_MIXER,val);
   
/* ==================== Bias Configuration ==================== */
   wm8994_write(codec, WM8994_ANTIPOP_2, 0x006C );

   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
   val &= ~( WM8994_VMID_SEL_MASK | WM8994_BIAS_ENA_MASK | WM8994_SPKOUTL_ENA_MASK | WM8994_MICB1_ENA_MASK);
   val |= (WM8994_VMID_SEL_NORMAL |WM8994_BIAS_ENA | WM8994_SPKOUTL_ENA | WM8994_MICB1_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
   msleep( 50 );
   
/* ==================== Input Path Configuration ==================== */
   //Enable left input mixer and IN1L PGA
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2  );
   val &= ~( WM8994_IN1L_ENA_MASK | WM8994_MIXINL_ENA_MASK );
   val |= (WM8994_MIXINL_ENA |WM8994_IN1L_ENA );
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

   // Unmute IN1L PGA, update volume
   val = wm8994_read(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME );   
   val &= ~(WM8994_IN1L_MUTE_MASK);
   wm8994_write(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);

   //Connect IN1LN and IN1LP to the inputs
   val = wm8994_read(codec,WM8994_INPUT_MIXER_2);  
   val &= (WM8994_IN1LN_TO_IN1L_MASK | WM8994_IN1LP_TO_IN1L_MASK);
   val |= (WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L);
   wm8994_write(codec,WM8994_INPUT_MIXER_2, val);

   //IN1L_TO_MIXINL
   val = wm8994_read(codec,WM8994_INPUT_MIXER_3 );
   val &= (WM8994_IN1L_TO_MIXINL_MASK | WM8994_IN2L_TO_MIXINL_MASK);
   val |= (WM8994_IN1L_TO_MIXINL);
   wm8994_write(codec,WM8994_INPUT_MIXER_3 ,val);
   
/* ==================== Digital Path Configuration ==================== */
   //Digital Paths
   //Enable Left ADC and time slot
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4 );
   val &= ~(WM8994_AIF2ADCL_ENA_MASK | WM8994_AIF2ADCR_ENA_MASK | WM8994_ADCL_ENA_MASK |WM8994_AIF1ADC1L_ENA_MASK  );
   val |= ( WM8994_AIF1ADC1L_ENA | WM8994_ADCL_ENA );
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 , val);
/*
   //ADC Right mixer routing (Disable)
   val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING);
   val &= ~( WM8994_ADC1R_TO_AIF1ADC1R_MASK);
   wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING,val);
*/
   //ADC Left mixer routing
   val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING);
   val &= ~( WM8994_ADC1L_TO_AIF1ADC1L_MASK);
   val |= WM8994_ADC1L_TO_AIF1ADC1L;
   wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING,val);

   wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable

   wm8994_write(codec, 0x4C, 0x9F25 ); // Charge Pump
   msleep( 10 );
   
   wm8994_write(codec, 0x60, 0x0022 );
   wm8994_write(codec, 0x54, 0x0033 );
   msleep( 200 );
   wm8994_write(codec, 0x60, 0x00EE );

   // AIF1 CONTROL1 : Right ADC to right / left
   val = wm8994_read(codec,WM8994_AIF1_CONTROL_1);
   val &= ~(WM8994_AIF1ADCL_SRC_MASK|WM8994_AIF1ADCR_SRC_MASK);
   wm8994_write(codec,WM8994_AIF1_CONTROL_1,val);

/* ==================== Set default Gain ==================== */
/*
   if(wm8994_voice_recognition)
   {
      wm8994_write(codec, 0x410, 0x1800); // Filter
   
      //-------------------------------------- Analog Input
      // Unmute IN1L PGA, update volume
      val = wm8994_read(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME );   
      val &= ~(WM8994_IN1L_VU_MASK | WM8994_IN1L_VOL_MASK);
      val |= (WM8994_IN1L_VU | TUNE_MEMO_MAIN_MIC_VOL); //volume
      wm8994_write(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME , val);
      //IN1L_TO_MIXINL
      val = wm8994_read(codec,WM8994_INPUT_MIXER_3 );
      val &= ~(WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
      wm8994_write(codec,WM8994_INPUT_MIXER_3 ,val); 
      
      //-------------------------------------- Digital
      //AIF1_ADC1_L VOL
      val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_VOLUME);   
      val &= ~(WM8994_AIF1ADC1_VU_MASK|WM8994_AIF1ADC1L_VOL_MASK);
      val |= (WM8994_AIF1ADC1_VU | 0xE5); // 0dB
      wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_VOLUME, val);
   }
   else
*/   
   {
      //-------------------------------------- Analog Input
      // Unmute IN1L PGA, update volume
      val = wm8994_read(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME );   
      val &= ~(WM8994_IN1L_VU_MASK | WM8994_IN1L_VOL_MASK);
      val |= (WM8994_IN1L_VU | TUNE_MEMO_MAIN_MIC_VOL); //volume
      wm8994_write(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME , val);
      //IN1L_TO_MIXINL
      val = wm8994_read(codec,WM8994_INPUT_MIXER_3 );
      val &= ~(WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
      val |= (WM8994_IN1L_MIXINL_VOL | 0x5); //+30 dB, MIXOUTL_TO_MIXINL 0dB
      wm8994_write(codec,WM8994_INPUT_MIXER_3 ,val); 
      
      //-------------------------------------- Digital
      //AIF1_ADC1_L VOL
      val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_VOLUME);   
      val &= ~(WM8994_AIF1ADC1_VU_MASK|WM8994_AIF1ADC1L_VOL_MASK);
      val |= (WM8994_AIF1ADC1_VU | 0xC0); // 0dB
      wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_VOLUME, val);
   }

#ifdef CONFIG_SND_VOODOO_RECORD_PRESETS
      voodoo_hook_record_main_mic();
#endif
}

void wm8994_record_sub_mic(struct snd_soc_codec *codec) 
{
   u16 val;

   printk("%s() \n",__func__);

   // Enable micbias,vmid,mic1
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
   val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_MICB1_ENA_MASK  );
   val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_MICB1_ENA  );  
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

   msleep(50);
#if 1
   wm8994_write(codec, 0x33, 0x0020 ); // IN2LRP_TO_HPOUT2 ( direct voice )
   wm8994_write(codec, 0x1F, 0x0020 ); // HPOUT2_MUTE = 1 , HPOUT2_VOL = 0 ( 0dB )
   
   wm8994_write(codec, 0x01, 0x0813 ); // HPOUT2_ENA
   
   wm8994_write(codec, 0x38, 0x0040 ); // HPOUT2_IN_ENA
#else
   //3 Enable left input mixer and IN2LP/VRXN PGA  
   //3 Enable right input mixer and IN2RP/VRXP PGA 
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2  );
   val &= ~( WM8994_MIXINL_ENA_MASK /*| WM8994_MIXINR_ENA_MASK*/);
   val |= (WM8994_MIXINL_ENA /*| WM8994_MIXINR_ENA */);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

   //3 Unmute IN2LP_MIXINL_VOL 
   val = wm8994_read(codec,WM8994_INPUT_MIXER_5 ); 
   val &= ~(WM8994_IN2LP_MIXINL_VOL_MASK );
   val |=0x7; //volume ( 111 = 6dB )
   wm8994_write(codec,WM8994_INPUT_MIXER_5 ,val);
   
   //3 Digital Paths   ADCL
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4 );
   val &= ~(WM8994_ADCL_ENA_MASK|WM8994_AIF1ADC1L_ENA_MASK );
   val |= ( WM8994_AIF1ADC1L_ENA | WM8994_ADCL_ENA );
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);

   //3 Enable timeslots
   val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING );
   val |=WM8994_ADC1L_TO_AIF1ADC1L ;  
   wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING ,val);

   wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable
#endif
}


void wm8994_disable_voicecall_path(struct snd_soc_codec *codec, int mode)
{
   u16 val;
   val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );

   switch(mode)
   {
      case MM_AUDIO_VOICECALL_RCV:
      printk("..Disabling VOICE_RCV\n");
      val&= ~(WM8994_HPOUT2_ENA_MASK);
      //disbale the HPOUT2 
      break;

      case MM_AUDIO_VOICECALL_SPK:
      printk("..Disabling VOICE_SPK\n");
      //disbale the SPKOUTL
      val&= ~(WM8994_SPKOUTL_ENA_MASK ); 
      break;

      case MM_AUDIO_VOICECALL_HP:
      printk("..Disabling VOICE_HP\n");
      //disble the HPOUT1
      val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK );
      break;

      default:
      break;

   }
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
}


//Voice path setting - CP to WM8994
void wm8994_voice_rcv_path(struct snd_soc_codec *codec)
{

   u16 val;
   
   printk("%s() \n",__func__);

   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(48);

/* ==================== Bias Configuration ==================== */
   wm8994_write(codec, WM8994_ANTIPOP_2, 0x0068 );
   //msleep(5);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0003); // BIAS_ENA, VMID = normal
   msleep(50);

   wm8994_write(codec, 0x500, 0x0100); // AIF2 ADC L mute
   wm8994_write(codec, 0x501, 0x0100); // AIF2 ADC R mute

   wm8994_write(codec, 0x102, 0x0002);
   wm8994_write(codec, 0x817, 0x0000);
   wm8994_write(codec, 0x102, 0x0000);
   
   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   //8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   //8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1   
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   // FLL2 = 12.288MHz , fs = 8kHz
   wm8994_write(codec, 0x244, 0x0C83 ); // 0x0C88 -> 0x0C80 FLL2_CLK_REV_DIV = 1 = MCLK / FLL2, FLL2 = MCLK1
   wm8994_write(codec, 0x241, 0x0700 ); // FLL2_OUTDIV = 23 = Fvco / Fout
   wm8994_write(codec, 0x243, 0x0600 ); // FLL2_N[14:5] = 8, FLL2_GAIN[3:0] = X1
   
   wm8994_write(codec, 0x240, 0x0001 ); // FLL2 Enable, FLL2 Fractional Mode
   msleep(2);
   
// wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0813 ); // + HPOUT2_ENA[11] = 1(Enable).  MIC_BIAS1_ENA[4]=1(Enable)
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 0x6240 ); // Thermal Sensor Enable, Thermal Shutdown Enable, MIXINL_ENA[9] =1, IN1L_ENA[6] = 1, 
   
/* ==================== Input Path Configuration ==================== */
   // IN1LN, P -> IN1L
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0030); // IN1LN_TO_IN1L = 1, IN1LP_TO_IN1L
   // IN1L_MUTE unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );
   val &= ~(WM8994_IN1L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
   // IN1L_TO_MIXINL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK | WM8994_IN1L_TO_MIXINL_MASK);
   val |= WM8994_IN1L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   // IN2LP_MIXINL_VOL mute
   val = wm8994_read(codec, WM8994_INPUT_MIXER_5 );
   val &= ~(WM8994_IN2LP_MIXINL_VOL_MASK);
   val |= 0x0000; // SUB MIC mute
   wm8994_write(codec, WM8994_INPUT_MIXER_5, val);
   
/* ==================== output Path Configuration ==================== */
wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x00F0 ); // MIXOUTLVOL_ENA, MIXOUTRVOL_ENA, MIXOUTL_ENA, MIXOUTR_ENA

wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0813 ); // + HPOUT2_ENA[11] = 1(Enable).  MIC_BIAS1_ENA[4]=1(Enable)   

/* ==================== Digital Path Configuration ==================== */
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, 0x2002 ); // AIF2ADCL_ENA, ADCL_ENA
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303); // 303 ); // AIF2DACL_ENA, AIF2DACR_ENA, AIF1DAC1L_ENA, AIF1DAC1R_ENA, DAC1L_EN, DAC1R_EN
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2L_TO DAC1L, AIF1DAC1L_TO_DAC1L
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2L_TO DAC1L, AIF1DAC1L_TO_DAC1L
   wm8994_write(codec, 0x603, 0x000C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
   wm8994_write(codec, 0x604, 0x0010 ); // ADC1_TO_DAC2L[4] = Enable(1)
   wm8994_write(codec, 0x621, 0x01C0 ); // Digital Sidetone HPF, fcut-off = 370Hz

/* ==================== Clocking and AIF Configuration ==================== */   
   wm8994_write(codec, 0x620, 0x0000 ); // ADC / DAC oversample rate disable
   wm8994_write(codec, 0x210, 0x0073 );
   wm8994_write(codec, 0x300, 0x4010 );
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec, 0x211, 0x0009 ); // AIF2 Sample Rate = 8kHz AIF2CLK_RATE=256 => AIF2CLK = 8k*256 = 2.048MHz
   wm8994_write(codec, 0x310, 0x4118 ); // DSP A mode, 16bit, BCLK2 invert
   wm8994_write(codec, 0x311, 0x0000); // AIF2_LOOPBACK
   wm8994_write(codec, 0x208, 0x000E ); // 0x000E -> 0x0007 // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz
   wm8994_write(codec, 0x200, 0x0011 ); // 0x000E -> 0x0007 // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz
   wm8994_write(codec, 0x204, 0x0019 ); // AIF2CLK_SRC = FLL2, AIF2CLK_INV[2] = 0,  AIF2CLK_DIV[1] = 0, AIF2CLK_ENA[0] = 1



// wm8994_write(codec, 0x303, 0x0000 );
// wm8994_write(codec, 0x304, 0x0100 );
// wm8994_write(codec, 0x305, 0x0100 );

   
   msleep(50);

   //-------------------------------------- Tx Path
   //Unmute LeftDAC2
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME );
   val &= ~(WM8994_DAC2L_MUTE_MASK);
// val |= WM8994_DAC2L_MUTE;
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   //Unmute RightDAC2
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME );
   val &= ~(WM8994_DAC2R_MUTE_MASK);
// val |= WM8994_DAC2R_MUTE;
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME, val);

   //-------------------------------------- Rx Path
   wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, 0x0000 ); // unmute DAC1L AND DAC1R
   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 ); // unmute AIF2DAC, 
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2R_TO_DAC1R, AIF1DAC1R_TO_DAC1R
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME );
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   val |= WM8994_DAC1R_MUTE;
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME, val);

/* ==================== Output Path Configuration ==================== */
   // DAC1L -> MIXOUTL
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, 0x0001 ); // DAC1L_TO_HPOUT1L = 0, DAC1L_TO_MIXOUTL = 1
   // DAC1R -> MIXOUTR
   wm8994_write(codec, WM8994_OUTPUT_MIXER_2, 0x0001 ); // DAC1R_TO_HPOUT1R = 0, DAC1R_TO_MIXOUTR = 1, 
   // MIXOUTL_MUTE_N, MIXOUTR_MUTE_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUTL_MUTE_N_MASK);
   val |= WM8994_MIXOUTL_MUTE_N;
   wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUTR_MUTE_N_MASK);
   val |= WM8994_MIXOUTR_MUTE_N;
   wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val);
   // MIXOUTL, R -> HPOUT2
   wm8994_write(codec, WM8994_HPOUT2_MIXER, 0x0018 ); // MIXOUTLVOL_TO_HPOUT2 unmute, MIXOUTRVOL_TO_HPOUT2 unmute
   // Unmute HPOUT2 ( RCV )
   val = wm8994_read(codec, WM8994_HPOUT2_VOLUME );
   val &= ~(WM8994_HPOUT2_MUTE_MASK);
   wm8994_write(codec, WM8994_HPOUT2_VOLUME, val );
   
/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   // IN1L
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );
   val &= ~(WM8994_IN1L_VU_MASK | WM8994_IN1L_VOL_MASK);
   val |= (WM8994_IN1L_VU | TUNE_CALL_RCV_MAIN_MIC_VOL); //0 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
   // IN1L_MIXINL_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
   val |= (0x0010); //+30 dB, MIXOUTL_TO_MIXINL_VOL mute
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   
   //-------------------------------------- Digital
   // AIF2_DAC_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF2_DAC_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF2DAC_VU_MASK | WM8994_AIF2DACL_VOL_MASK);
   val |= (WM8994_AIF2DAC_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_AIF2_DAC_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF2_DAC_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF2DAC_VU_MASK | WM8994_AIF2DACL_VOL_MASK);
   val |= (WM8994_AIF2DAC_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_AIF2_DAC_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

   // DAC2_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2L_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2R_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME,val);
   
   // AIF2DAC_BOOST
   val = wm8994_read(codec, WM8994_AIF2_CONTROL_2 ); 
   val &= ~(WM8994_AIF2DAC_BOOST_MASK);
   val |= (0x000); //0 dB volume    
   wm8994_write(codec,WM8994_AIF2_CONTROL_2, val);
   
   //-------------------------------------- Analog Output
   //MIXOUT_VOL
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
   val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
   val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
   //MIXOUTLVOL, MIXOUTRVOL
   val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUT_VU_MASK | WM8994_MIXOUTL_VOL_MASK);
   val |= (WM8994_MIXOUT_VU | TUNE_CALL_RCV_RCV_VOL); // 0 dB
   wm8994_write(codec, WM8994_LEFT_OPGA_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME );
   val &= ~(WM8994_MIXOUT_VU_MASK | WM8994_MIXOUTR_VOL_MASK);
   val |= (WM8994_MIXOUT_VU | TUNE_CALL_RCV_RCV_VOL); // 0 dB
   wm8994_write(codec, WM8994_RIGHT_OPGA_VOLUME, val);
   // HPOUT2_VOL
   val = wm8994_read(codec, WM8994_HPOUT2_VOLUME );
   val &= ~(WM8994_HPOUT2_VOL_MASK);
   val |= 0x0000; // 
   wm8994_write(codec, WM8994_HPOUT2_VOLUME, val );

   wm8994_write(codec, 0x500, 0x01C0); // AIF2 ADC L unmute (0dB)
   wm8994_write(codec, 0x501, 0x01C0); // AIF2 ADC R unmute (0dB)

}

void wm8994_voice_ear_path(struct snd_soc_codec *codec)
{
   u16 val;
   
   u16 TestReturn1=0;
   u16 TestReturn2=0;
   u16 TestLow1=0;
   u16 TestHigh1=0;
   u8 TestLow=0;
   u8 TestHigh=0;
   
   printk("%s() \n",__func__);
   
   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(48);

       wm8994_write(codec, WM8994_ANTIPOP_2, 0x0068 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0003); // BIAS_ENA, VMID = normal
   msleep(50);

   wm8994_write(codec, 0x500, 0x0100); // AIF2 ADC L mute
   wm8994_write(codec, 0x501, 0x0100); // AIF2 ADC R mute

   wm8994_write(codec, 0x102, 0x0002);
   wm8994_write(codec, 0x817, 0x0000);
   wm8994_write(codec, 0x102, 0x0000);

   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   //8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   //8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1   
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   // FLL2 = 12.288MHz , fs = 8kHz
   wm8994_write(codec, 0x244, 0x0C83 ); // 0x0C88 -> 0x0C80 FLL2_CLK_REV_DIV = 1 = MCLK / FLL2, FLL2 = MCLK1
   wm8994_write(codec, 0x241, 0x0700 ); // FLL2_OUTDIV = 23 = Fvco / Fout
   wm8994_write(codec, 0x243, 0x0600 ); // FLL2_N[14:5] = 8, FLL2_GAIN[3:0] = X1
   wm8994_write(codec, 0x240, 0x0001 ); // FLL2 Enable, FLL2 Fractional Mode
   msleep(2);
   
/* ==================== Bias Configuration ==================== */
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
   gpio_set_value(GPIO_MICBIAS2_EN, 1);
#endif

/* ==================== Power Management ==================== */
#ifndef DAC_TO_DIRECT_HPOUT1
   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3 ); 
   val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
   val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);  
#endif
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 0x6110 );
   
/* ==================== Input Path Configuration ==================== */
   // IN1RN_TO_IN1R, IN1RP_TO_IN1R
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0003 ); // 0x0001 -> 0x0003
   // IN1R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME );
   val &= ~(WM8994_IN1R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, val);
   //IN1R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN1R_TO_MIXINR_MASK | WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN1R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val ); 
   
/* ==================== Digital Path Configuration ==================== */
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, 0x3003 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303); // 303 );

   if(wm8994_rev >= 3)
   {
      wm8994_write(codec, 0x603, 0x018C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
      wm8994_write(codec, 0x604, 0x0030 ); // ADC1_TO_DAC2L[4] = Enable(1)
      wm8994_write(codec, 0x620, 0x0000 );
      wm8994_write(codec, 0x621, 0x0181 ); // Digital Sidetone HPF, fcut-off = 370Hz
   }
   else
   {
      wm8994_write(codec, 0x603, 0x000C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
      wm8994_write(codec, 0x604, 0x0010 ); // ADC1_TO_DAC2L[4] = Enable(1)
      wm8994_write(codec, 0x620, 0x0000 );
      wm8994_write(codec, 0x621, 0x01C1 ); // Digital Sidetone HPF, fcut-off = 370Hz
   }

       wm8994_write(codec, 0x620, 0x0000); // ADC oversampling disabled, DAC oversampling disabled
       wm8994_write(codec, 0x210, 0x0073 );
       wm8994_write(codec, 0x300, 0x4010 );
   wm8994_write(codec, 0x302, 0x7000);
       wm8994_write(codec, 0x211, 0x0009 ); // AIF2 Sample Rate = 8kHz AIF2CLK_RATE=256 => AIF2CLK = 8k*256 = 2.048MHz
       wm8994_write(codec, 0x310, 0x4118 ); // DSP A mode, 16bit, BCLK2 invert
       wm8994_write(codec, 0x311, 0x0000); // AIF2_LOOPBACK
       wm8994_write(codec, 0x208, 0x000E ); // 0x000E -> 0x0007 // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz
       wm8994_write(codec, 0x200, 0x0011 ); // AIF1 Enable, AIF1CLK = FLL1    
   wm8994_write(codec, 0x204, 0x0019 ); // AIF2CLK_SRC = FLL2, AIF2CLK_INV[2] = 0,  AIF2CLK_DIV[1] = 0, AIF2CLK_ENA[0] = 1 

   wm8994_write(codec, 0x621, 0x01C0 ); // Digital Sidetone HPF, fcut-off = 370Hz
   
   msleep(50);
   
   //-------------------------------------- Tx Path
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME );
   val &= ~(WM8994_DAC2L_MUTE_MASK);
// val |= WM8994_DAC2L_MUTE;
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME );
   val &= ~(WM8994_DAC2R_MUTE_MASK);
// val |= WM8994_DAC2R_MUTE;
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME, val);
   
   //-------------------------------------- Rx Path
   // AIF1DAC1_MUTE, AIF2DAC_MUTE unmute
   wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, 0x0000 ); 
   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 );
   // AIF2DACL_TO_DACL1, AIF1DAC1L_TO_DAC1L enable
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 );

   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME );
   val &= ~(WM8994_DAC1R_MUTE_MASK);
// val |= WM8994_DAC1R_MUTE;
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME, val);

/* ==================== Output Path Configuration ==================== */
   //DAC1L_TO_HPOUT1L
#ifdef DAC_TO_DIRECT_HPOUT1
   // Enbale DAC1L to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
   val |= WM8994_DAC1L_TO_HPOUT1L ;    
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale DAC1R to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
   val |= WM8994_DAC1R_TO_HPOUT1R ;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= WM8994_DAC1L_TO_MIXOUTL;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= WM8994_DAC1R_TO_MIXOUTR;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif
   // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
   val |= WM8994_HPOUT1L_MUTE_N;
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
   val |= WM8994_HPOUT1R_MUTE_N;
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val); 
   
/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 1);
   
/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME );
   val &= ~(WM8994_IN1R_VU_MASK | WM8994_IN1R_VOL_MASK);
   val |= (WM8994_IN1R_VU | TUNE_CALL_HP_EAR_MIC_VOL); // +3 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME, val);
   //IN1R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
   val |= 0x0010; // + 30 dB, MIXOUTR_TO_MIXINR_VOL mute
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val ); 

   //-------------------------------------- Digital
   // AIF2_DAC_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF2_DAC_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF2DAC_VU_MASK | WM8994_AIF2DACL_VOL_MASK);
   val |= (WM8994_AIF2DAC_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_AIF2_DAC_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF2_DAC_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF2DAC_VU_MASK | WM8994_AIF2DACL_VOL_MASK);
   val |= (WM8994_AIF2DAC_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_AIF2_DAC_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

   // DAC2_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2L_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2R_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME,val);

   // AIF2DAC_BOOST
   val = wm8994_read(codec, WM8994_AIF2_CONTROL_2 ); 
   val &= ~(WM8994_AIF2DAC_BOOST_MASK);
   val |= (0x000); //0 dB volume    
   wm8994_write(codec,WM8994_AIF2_CONTROL_2, val);

   //-------------------------------------- Analog Output
#ifndef DAC_TO_DIRECT_HPOUT1
   //MIXOUT_VOL
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
   val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
   val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
   val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
   val |= 0x000;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
#endif
   //HPOUT_VOL
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | TUNE_CALL_HP_HP_VOL); // 0x39(0dB) -> 0x33(-6dB)
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | TUNE_CALL_HP_HP_VOL); // 0x39(0dB) -> 0x33(-6dB)
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);


/* ==================== Pop Noise Suppression ==================== */
   //Disable end point for preventing pop up noise.
   wm8994_write(codec,0x102,0x0003);
   wm8994_write(codec,0x56,0x0003);
   wm8994_write(codec,0x102,0x0000);
// wm8994_write(codec,WM8994_CLASS_W_1,0x0005);
   wm8994_write(codec,0x5D,0x0002);
   wm8994_write(codec,WM8994_DC_SERVO_2,0x03E0);

/* ==================== Bias Configuration ==================== */

   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0303);
   wm8994_write(codec,WM8994_ANALOGUE_HP_1,0x0022);
   
   //Enable Charge Pump 
   wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x9F25);

   msleep(5);  // 5ms delay

   wm8994_write(codec,WM8994_DC_SERVO_1,0x0303);
   
   msleep(160);   // 160ms delay

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-DC_SERVO_OFFSET)&0x00ff;
   TestHigh1=(((u16)TestHigh-DC_SERVO_OFFSET)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

#if 1
   wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);
   msleep(20);
   wm8994_write(codec, WM8994_ANALOGUE_HP_1, 0x00EE );
#else
   wm8994_write(codec, 0x110, 0x8100); //8100 -> 8113
#endif

   wm8994_write(codec, 0x500, 0x01C0); // AIF2 ADC L unmute (0dB)
   wm8994_write(codec, 0x501, 0x01C0); // AIF2 ADC R unmute (0dB) 

}

void wm8994_voice_spkr_path(struct snd_soc_codec *codec)
{
   u16 val;
   
   printk("%s() \n",__func__);
   
   // wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   
/* ==================== Pop Noise Suppression ==================== */
   wm8994_write(codec, WM8994_ANTIPOP_2, 0x0068 );
   msleep( 5 );

/* ==================== Bias Configuration ==================== */
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
   gpio_set_value(GPIO_MICBIAS2_EN, 1);
#endif
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0003 ); // BIAS_ENA, VMID = normal
   msleep(50);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x1003 ); // + SPK_OUTL[12] = 1(Enable)

   wm8994_write(codec, 0x500, 0x0100); // AIF2 ADC L mute
   wm8994_write(codec, 0x501, 0x0100); // AIF2 ADC R mute

       wm8994_write(codec, 0x102, 0x0002);
   wm8994_write(codec, 0x817, 0x0000);
   wm8994_write(codec, 0x102, 0x0000);

   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   //8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   //8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1   
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   // FLL2 = 12.288MHz , fs = 8kHz
   wm8994_write(codec, 0x244, 0x0C83 ); // 0x0C88 -> 0x0C80 FLL2_CLK_REV_DIV = 1 = MCLK / FLL2, FLL2 = MCLK1
   wm8994_write(codec, 0x241, 0x0700 ); // FLL2_OUTDIV = 23 = Fvco / Fout
   wm8994_write(codec, 0x243, 0x0600 ); // FLL2_N[14:5] = 8, FLL2_GAIN[3:0] = X1
   wm8994_write(codec, 0x240, 0x0001 ); // FLL2 Enable, FLL2 Fractional Mode
   msleep(2);
   

   if(apollo_get_remapped_hw_rev_pin() >= 3)
      wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 0x6240 ); // Thermal Sensor Enable, Thermal Shutdown Enable, MIXINL_ENA[9] =1, IN1L_ENA[6] = 1,  
   else
      wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 0x6200 ); // Thermal Sensor Enable, Thermal Shutdown Enable, MIXINL_ENA[9] =1 

/* ==================== Input Path Configuration ==================== */
   if(apollo_get_remapped_hw_rev_pin() >= 3)
   {
      // IN1LN, P -> IN1L
      wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0030); // IN1LN_TO_IN1L = 1, IN1LP_TO_IN1L
      // IN1L_MUTE unmute
      val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );
      val &= ~(WM8994_IN1L_MUTE_MASK);
      wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
      // IN1L_TO_MIXINL
      val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
      val &= ~(WM8994_IN2L_TO_MIXINL_MASK | WM8994_IN1L_TO_MIXINL_MASK);
      val |= WM8994_IN1L_TO_MIXINL;
      wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
      // IN2LP_MIXINL_VOL mute
      val = wm8994_read(codec, WM8994_INPUT_MIXER_5 );
      val &= ~(WM8994_IN2LP_MIXINL_VOL_MASK);
      val |= 0x0000; // SUB MIC mute
      wm8994_write(codec, WM8994_INPUT_MIXER_5, val);
   }
   else
   {
      // IN1L_MUTE mute
      val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );
      val &= ~(WM8994_IN1L_MUTE_MASK);
      val |= WM8994_IN1L_MUTE;
      wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
      //IN2LP_MIXINL_VOL, IN2LP_MIXINR_VOL unmute (0 dB)
      val = wm8994_read(codec, WM8994_INPUT_MIXER_5 );
      val &= ~(WM8994_IN2LP_MIXINL_VOL_MASK);
      val |= 0x0005; // SUB MIC 0 dB
      wm8994_write(codec, WM8994_INPUT_MIXER_5, val);
      val = wm8994_read(codec, WM8994_INPUT_MIXER_6 );
      val &= ~(WM8994_IN2LP_MIXINR_VOL_MASK);
      val |= 0x0005; // SUB MIC 0 dB
      wm8994_write(codec, WM8994_INPUT_MIXER_6, val);
   }

/* ==================== Digital Path Configuration ==================== */
       wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, 0x2002 ); // AIF2ADCL_ENA, ADCL_ENA
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303); // 303 ); // AIF2DACL_ENA, AIF2DACR_ENA, AIF1DAC1L_ENA, AIF1DAC1R_ENA, DAC1L_EN, DAC1R_EN
       wm8994_write(codec, 0x603, 0x000C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
   wm8994_write(codec, 0x604, 0x0010 ); // ADC1_TO_DAC2L[4] = Enable(1)
   wm8994_write(codec, 0x621, 0x01C0 ); // Digital Sidetone HPF, fcut-off = 370Hz

/* ==================== Clocking and AIF Configuration ==================== */
   wm8994_write(codec, 0x620, 0x0000); // ADC oversampling disabled, DAC oversampling disabled
       wm8994_write(codec, 0x210, 0x0073); // AIF1 Sample Rate = 44.1kHz AIF1CLK_RATE=256 => AIF1CLK = 11.2896 MHz
       wm8994_write(codec, 0x300, 0x4010);
       wm8994_write(codec, 0x302, 0x7000);
   wm8994_write(codec, 0x211, 0x0009); // AIF2 Sample Rate = 8kHz AIF2CLK_RATE=256 => AIF2CLK = 8k*256 = 2.048MHz
   wm8994_write(codec, 0x310, 0x4118); // DSP A mode, 16bit, BCLK2 invert
       wm8994_write(codec, 0x311, 0x0000); // AIF2_LOOPBACK
       wm8994_write(codec, 0x208, 0x000E); // 0x000E -> 0x0007 // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz   
       wm8994_write(codec, 0x200, 0x0011); // AIF1 Enable, AIF1CLK = FLL1 
       wm8994_write(codec, 0x204, 0x0019 ); // AIF2CLK_SRC = FLL2, AIF2CLK_INV[2] = 0,  AIF2CLK_DIV[1] = 0, AIF2CLK_ENA[0] = 1


     
   //-------------------------------------- Tx Path
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME );
   val &= ~(WM8994_DAC2L_MUTE_MASK);
// val |= WM8994_DAC2L_MUTE;
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME );
   val &= ~(WM8994_DAC2R_MUTE_MASK);
// val |= WM8994_DAC2R_MUTE;
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME, val);
   
   //-------------------------------------- Rx Path

   // AIF1DAC1_MUTE, AIF2DAC_MUTE unmute
   wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, 0x0000 ); 
   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 );
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME );
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME, val);
   
   // Enable the Timeslot0 to DAC1L
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2L_TO DAC1L, AIF1DAC1L_TO_DAC1L
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2L_TO DAC1L, AIF1DAC1L_TO_DAC1L
   
/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== Set default Gain ==================== */
   if(apollo_get_remapped_hw_rev_pin() >= 3)
   {
      //-------------------------------------- Analog Input
      // IN1L
      val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );
      val &= ~(WM8994_IN1L_VU_MASK | WM8994_IN1L_VOL_MASK);
      val |= (WM8994_IN1L_VU | TUNE_CALL_SPK_SUB_MIC_VOL); // 4.5 dB
      wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
      // IN1L_MIXINL_VOL
      val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
      val &= ~(WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
      val |= (0x0010); //+30 dB, MIXOUTL_TO_MIXINL_VOL mute
      wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   }
   else
   {
      //-------------------------------------- Analog Input
      //IN2LP_MIXINL_VOL, IN2LP_MIXINR_VOL unmute (6 dB)
      val = wm8994_read(codec, WM8994_INPUT_MIXER_5 );
      val &= ~(WM8994_IN2LP_MIXINL_VOL_MASK);
      val |= 0x0007; // SUB MIC 6 dB
      wm8994_write(codec, WM8994_INPUT_MIXER_5, val);
      val = wm8994_read(codec, WM8994_INPUT_MIXER_6 );
      val &= ~(WM8994_IN2LP_MIXINR_VOL_MASK);
      val |= 0x0007; // SUB MIC 6 dB
      wm8994_write(codec, WM8994_INPUT_MIXER_6, val);
   }

   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

   // DAC2_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2L_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC2_VU_MASK | WM8994_DAC2R_VOL_MASK);
   val |= (WM8994_DAC2_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME,val);

   // AIF2DAC_BOOST
   val = wm8994_read(codec, WM8994_AIF2_CONTROL_2 ); 
   val &= ~(WM8994_AIF2DAC_BOOST_MASK);
   val |= (0x000); //0 dB volume    
   wm8994_write(codec,WM8994_AIF2_CONTROL_2, val);

   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0;
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0;
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_CALL_SPK_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_CALL_SPK_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_CALL_SPK_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_CALL_SPK_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= (TUNE_CALL_SPK_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
   wm8994_write(codec, WM8994_CLASSD, val);

   wm8994_write(codec, 0x500, 0x01C0); // AIF2 ADC L unmute (0dB)
   wm8994_write(codec, 0x501, 0x01C0); // AIF2 ADC R unmute (0dB) 

}

void wm8994_voice_BT_path(struct snd_soc_codec *codec)
{
   u16 val;

   printk("%s() \n",__func__);

#if 1 // CP Master, Codec Slave
   // FLL setting from Wolfson
   wm8994_write(codec,0x39,0x0060);
   wm8994_write(codec,0x01,0x0003);
   msleep(50);

   //Enable Dac1 and DAC2 and the Timeslot0 for AIF1
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, 0x3000); // AIF2ADCL_ENA, ADCL_ENA
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,0x330C);
   

/* ==================== Digital Path Configuration ==================== */
   wm8994_write(codec, 0x620, 0x0000 );
   wm8994_write(codec, 0x621, 0x01C0 ); // Digital Sidetone HPF, fcut-off = 370Hz
   
   wm8994_write(codec, 0x208, 0x000E ); // 0x000E -> 0x0007 for noise suppression // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz
   wm8994_write(codec, 0x210, 0x0073 ); // AIF1 Sample Rate = 44.1kHz AIF1CLK_RATE=256 => AIF1CLK = 11.2896 MHz
   wm8994_write(codec, 0x211, 0x0009 ); // AIF2 Sample Rate = 8kHz AIF2CLK_RATE=256 => AIF2CLK = 8k*256 = 2.048MHz
   wm8994_write(codec, 0x310, 0x4118 ); // DSP A mode, 16bit, BCLK2 invert

   wm8994_write(codec, 0x311, 0x0400); // AIF2_LOOPBACK

   // DIGITAL Master/Slave Mode
   wm8994_write(codec, 0x312, 0x0000 ); // SLAVE mode

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1
   wm8994_write(codec, 0x200, 0x0011 ); // AIF1 Enable, AIF1CLK = FLL1 

   // FLL2 = 2.048MHz , fs = 8kHz
   wm8994_write(codec, 0x240, 0x0005 ); // FLL2 Enable, FLL2 Fractional Mode
   wm8994_write(codec, 0x241, 0x0700 ); // FLL2_OUTDIV = 23 = Fvco / Fout
// wm8994_write(codec, 0x242, 0x3126 ); // FLL2_K = 12582
   wm8994_write(codec, 0x244, 0x0C83 ); // 0x0C88 -> 0x0C80 FLL2_CLK_REV_DIV = 1 = MCLK / FLL2, FLL2 = MCLK1
   wm8994_write(codec, 0x204, 0x0019 ); // AIF2CLK_SRC = FLL2, AIF2CLK_INV[2] = 0,  AIF2CLK_DIV[1] = 0, AIF2CLK_ENA[0] = 1
   wm8994_write(codec, 0x243, 0x0600 ); // FLL2_N[14:5] = 8, FLL2_GAIN[3:0] = X1

   wm8994_write(codec, 0x244, 0x0C83 ); // 0x0C88 -> 0x0C80 FLL2_CLK_REV_DIV = 1 = MCLK / FLL2, FLL2 = BCLK2
   wm8994_write(codec, 0x241, 0x0700 ); // FLL2_OUTDIV = 23 = Fvco / Fout
   wm8994_write(codec, 0x243, 0x0600 ); // FLL2_N[14:5] = 8, FLL2_GAIN[3:0] = X1
   wm8994_write(codec, 0x240, 0x0001 ); // FLL2 Enable, FLL2 Fractional Mode
   wm8994_write(codec, 0x208, 0x000E ); // 0x000E -> 0x0007 // DSP1, DSP2 processing Enable, SYSCLK = AIF1CLK = 11.2896 MHz
   wm8994_write(codec, 0x204, 0x0019 ); // AIF2CLK_SRC = FLL2, AIF2CLK_INV[2] = 0,  AIF2CLK_DIV[1] = 0, AIF2CLK_ENA[0] = 1
   wm8994_write(codec, 0x211, 0x0009 ); // AIF2 Sample Rate = 8kHz AIF2CLK_RATE=256 => AIF2CLK = 8k*256 = 2.048MHz
   wm8994_write(codec, 0x310, 0x4118 ); // DSP A mode, 16bit, BCLK2 invert

   wm8994_write(codec,0x620, 0x0000); // ADC oversampling disabled, DAC oversampling disabled


   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   wm8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   wm8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

   wm8994_write(codec, 0x603, 0x000C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
   wm8994_write(codec, 0x604, 0x0010 ); // ADC1_TO_DAC2L[4] = Enable(1)


   //-------------------------------------- Rx Path
   wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, 0x0000 ); // unmute DAC1L AND DAC1R
   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 ); // unmute AIF2DAC, 
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2L_TO DAC1L, AIF1DAC1L_TO_DAC1L
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 ); // AIF2DAC2R_TO_DAC1R, AIF1DAC1R_TO_DAC1R
// wm8994_write(codec, 0x603, 0x018C ); // ADC2_DAC2_VOL2[8:5] = -36dB(0000) ADC1_DAC2_VOL[3:0] = 0dB(1100), 
   wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, 0x0005 ); // ADC1_TO_DAC2L[4] = Enable(1)
   wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, 0x0005 ); // ADC1_TO_DAC2L[4] = Enable(1)

   wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x1C0 );
   wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x1C0 ); 

/* ==================== Bias Configuration ==================== */
   //Enbale bias,vmid and Left speaker
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
   val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_SPKOUTL_ENA_MASK |WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
   val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL/* |  WM8994_SPKOUTL_ENA */ );  
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
   
/* ==================== Digital Paths Configuration ==================== */


   // Unmute the AF1DAC1
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);

   // Enable the Timeslot0 to DAC2L
   val = wm8994_read(codec, WM8994_DAC2_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC2L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC2L;
   wm8994_write(codec,WM8994_DAC2_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC2R
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC2R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC2R;
   wm8994_write(codec,WM8994_DAC2_RIGHT_MIXER_ROUTING ,val);


   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC2_LEFT_VOLUME );
   val &= ~(WM8994_DAC2L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC2R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME,val);
   

/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK | WM8994_DAC2L_TO_SPKMIXL_MASK | WM8994_DAC2R_TO_SPKMIXR_MASK);
   val |= (WM8994_DAC2L_TO_SPKMIXL | WM8994_DAC2R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Digital
   // AIF1_DAC1_VOL 1st step
   val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
   val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
   val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
   wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
   // DAC1_VOL 2nd step
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
   val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | 0x3F);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | 0x3F);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= 0x0010; // 3 dB
   wm8994_write(codec, WM8994_CLASSD, val);

   wm8994_write(codec, 0x06, 0x000C );  
   wm8994_write(codec, 0x700, 0xA101 ); 
   wm8994_write(codec, 0x702, 0x8100 ); 
   wm8994_write(codec, 0x703, 0x8100 ); 
   wm8994_write(codec, 0x704, 0x8100 ); 
   wm8994_write(codec, 0x705, 0xA101 ); 
   wm8994_write(codec, 0x706, 0x0100 ); 
   wm8994_write(codec, 0x707, 0x8100 ); 
   wm8994_write(codec, 0x708, 0x0100 ); 
   wm8994_write(codec, 0x709, 0x0100 ); 
   wm8994_write(codec, 0x70A, 0x0100 ); 
#else
    wm8994_write(codec,0x01, 0x0003 );
    wm8994_write(codec,0x04, 0x2000 );
    wm8994_write(codec,0x06, 0x0014 );
    wm8994_write(codec,0x310, 0x4188 );

    wm8994_write(codec,0x312, 0x0000 );

    wm8994_write(codec,0x700, 0xA101 );
    wm8994_write(codec,0x702, 0x8100 );
    wm8994_write(codec,0x703, 0x8100 );
    wm8994_write(codec,0x704, 0x8100 );
    wm8994_write(codec,0x705, 0xA101 );
    wm8994_write(codec,0x706, 0x0100 );
    wm8994_write(codec,0x707, 0x8100 );
    wm8994_write(codec,0x708, 0x0100 );
    wm8994_write(codec,0x709, 0x0100 );
    wm8994_write(codec,0x70A, 0x0100 );
#endif


}


void wm8994_disable_fmradio_path(struct snd_soc_codec *codec, int mode)
{
   return; // Do not excute this function because pop noise
   
   u16 val;
   val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );

   switch(mode)
   {
      case MM_AUDIO_FMRADIO_RCV:
      printk("..Disabling FM Radio RCV\n");
      val&= ~(WM8994_HPOUT2_ENA_MASK);
      //disbale the HPOUT2 
      break;

      case MM_AUDIO_FMRADIO_SPK:
      printk("..Disabling FM Radio SPK\n");
      //disbale the SPKOUTL
      val&= ~(WM8994_SPKOUTL_ENA_MASK ); 
      break;

      case MM_AUDIO_FMRADIO_HP:
      printk("..Disabling FM Radio Ear\n");
      //disble the HPOUT1
      val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK );
      break;

      case MM_AUDIO_FMRADIO_SPK_HP:
      printk("..Disabling FM Radio SPK Ear\n");
      //disble the HPOUT1
      val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK |WM8994_SPKOUTL_ENA_MASK);
      break;

      default:
      break;

   }
      wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
}



void wm8994_fmradio_route_receiver(struct snd_soc_codec *codec)
{
   printk("%s IS NOT IMPLEMENTED \n",__func__);
}

void wm8994_fmradio_route_speaker(struct snd_soc_codec *codec)
{
   u16 val = 0;
   
   printk("%s() \n",__func__);

#if 0 //def FM_PATH_DRC_BLOCK // When DRC blcok used, gain is smaller so don't use it SPK mode
   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(50);
   
   wm8994_write(codec, 0x01,0x0003 ); 
   msleep(50);

   wm8994_write(codec, WM8994_GPIO_3, 0x0100 );
   wm8994_write(codec, WM8994_GPIO_4, 0x0100 );
   wm8994_write(codec, WM8994_GPIO_5, 0x8100 );
   wm8994_write(codec, WM8994_GPIO_6, 0xA101 );
   wm8994_write(codec, WM8994_GPIO_7, 0x0100 );
    

   // FLL1 setting 
   wm8994_write(codec, WM8994_FLL1_CONTROL_1,0x0005 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_2,0x0700 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_3,0x86C2 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_5,0x0C80 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_4,0x00E0 );
   wm8994_write(codec, WM8994_AIF2_CLOCKING_1,0x0011 );

   wm8994_write(codec,WM8994_AIF1_CONTROL_1,0x4010);
   wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE,0x7000);
   wm8994_write(codec,WM8994_AIF1_RATE,0x0073);
   wm8994_write(codec,WM8994_AIF1_CLOCKING_1,0x0011);
   wm8994_write(codec, WM8994_CLOCKING_1,0x000E );
   msleep(5);

   wm8994_write(codec, 0x312, 0x7000 );
   wm8994_write(codec, 0x311, 0x4001 );

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_2,0x63A0 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4,0x3003 );
   wm8994_write(codec, WM8994_DAC2_MIXER_VOLUMES, 0x018C );
   wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, 0x0010 );
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING);	//605H : 0x0010
   val &= ~(WM8994_ADC2_TO_DAC2R_MASK);
   val |= (WM8994_ADC2_TO_DAC2R);			// Changed value to support stereo
   wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, val);

/* ==================== Input Path Configuration ==================== */
   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0044 );
   
   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

#if 0
   /*
   wm8994_write(codec,  0x541, 0x0845 );
   wm8994_write(codec,  0x542, 0x0000 );
   wm8994_write(codec,  0x543, 0x0000 );
   wm8994_write(codec,  0x544, 0x001F );
   wm8994_write(codec,  0x540, 0x01BC );
   */

   //DRC for Noise-gate (AIF2) from S1
   wm8994_write(codec, 0x541, 0x0850);
   wm8994_write(codec, 0x542, 0x0800);
   wm8994_write(codec, 0x543, 0x0001);
   wm8994_write(codec, 0x544, 0x0008);
   wm8994_write(codec, 0x540, 0x01BC);
#endif
   
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_AIF1_DAC1_EQ_GAINS_1, 0x0000 );


/* ==================== Bias Configuration ==================== */
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x1003);
   msleep(5);  // 5ms delay

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x03F0 );

/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK | WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_MIXINL_TO_SPKMIXL | WM8994_MIXINR_TO_SPKMIXR | WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

   msleep(20);

   wm8994_write(codec, WM8994_OVERSAMPLING, 0x0001 );

   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 ); // SMbus_16inx_16dat     Write  0x34


/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | TUNE_FM_SPK_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | TUNE_FM_SPK_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_MIXINL_SPKMIXL_VOL_MASK | WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_MIXINR_SPKMIXR_VOL_MASK | WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= (TUNE_FM_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); // 7.5 dB
   wm8994_write(codec, WM8994_CLASSD, val);

#else /* FM_PATH_DRC_BLOCK */
   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(50);

   wm8994_write(codec,WM8994_ANTIPOP_2, 0x006C );  

/* ==================== Power Management ==================== */
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0003 ); 
   msleep(50);                 
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x1003 ); // HPOUT1L_ENA=1, HPOUT1R_ENA=1

/*Analogue Input Configuration*/
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, 0x6BA0 ); /*MIXIN L-R, IN2 L-R*/

   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3, 0x03F0 ); // MIXOUTLVOL_ENA=1, MIXOUTRVOL_ENA=1, MIXOUTL_ENA=1, MIXOUTR_ENA=1

/* ==================== Input Path Configuration ==================== */
   // IN2LN_TO_IN2L, IN2RN_TO_IN2R
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0044);

   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);

   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);
#if 1
/* ==================== Digital Paths Configuration ==================== */
   //Enable Dac1 and DAC2 and the Timeslot0 for AIF1
   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 );    
   val &= ~(WM8994_AIF1DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_DAC1R_ENA_MASK);
   val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );
#endif
/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK | WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_MIXINL_TO_SPKMIXL | WM8994_MIXINR_TO_SPKMIXR | WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

/* ==================== Clock Setting ==================== */
   /*Clocking*/
   wm8994_write(codec, 0x208, 0x000A );

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1
   wm8994_write(codec, 0x200, 0x0011 ); // AIF1 Enable, AIF1CLK = FLL1 

   wm8994_write(codec,WM8994_AIF1_CONTROL_1,0x4010);
   wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE,0x7000);
   wm8994_write(codec,WM8994_AIF1_RATE,0x0073);
   wm8994_write(codec,WM8994_AIF1_CLOCKING_1,0x0011);
   wm8994_write(codec, WM8994_CLOCKING_1,0x000E );

   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   wm8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   wm8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | TUNE_FM_SPK_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | TUNE_FM_SPK_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_MIXINL_SPKMIXL_VOL_MASK | WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_MIXINR_SPKMIXR_VOL_MASK | WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   if(apollo_get_hw_type()) // GT-I5800
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL_OPEN);
   else // GT-I5801
      val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= (TUNE_FM_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); // 7.5 dB
   wm8994_write(codec, WM8994_CLASSD, val);
#endif /* FM_PATH_DRC_BLOCK */
}

void wm8994_fmradio_route_headset(struct snd_soc_codec *codec)
{
   u16 val = 0;
   
   u16 TestReturn1=0;
   u16 TestReturn2=0;
   u16 TestLow1=0;
   u16 TestHigh1=0;
   u8 TestLow=0;
   u8 TestHigh=0;
   
   printk("%s() \n",__func__);
#ifdef FM_PATH_DRC_BLOCK
   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(50);
   
   wm8994_write(codec, 0x01,0x0003 ); // SMbus_16inx_16dat     Write  0x34      * Power Management (1)(01H): 0003  SPKOUTR_ENA=0, SPKOUTL_ENA=0, HPOUT2_ENA=0, HPOUT1L_ENA=0, HPOUT1R_ENA=0, MICB2_ENA=0, MICB1_ENA=0, VMID_SEL=01, BIAS_ENA=1
   msleep(50);

   wm8994_write(codec,  WM8994_GPIO_3, 0x0100 );
   wm8994_write(codec,  WM8994_GPIO_4, 0x0100 );
   wm8994_write(codec,  WM8994_GPIO_5, 0x8100 );
   wm8994_write(codec,  WM8994_GPIO_6, 0xA101 );
   wm8994_write(codec,  WM8994_GPIO_7, 0x0100 );
    

   // FLL1 setting 
   wm8994_write(codec,  WM8994_FLL1_CONTROL_1,0x0005 );
   wm8994_write(codec,  WM8994_FLL1_CONTROL_2,0x0700 );
   wm8994_write(codec,  WM8994_FLL1_CONTROL_3,0x86C2 );
   wm8994_write(codec,  WM8994_FLL1_CONTROL_5,0x0C80 );
   wm8994_write(codec,  WM8994_FLL1_CONTROL_4,0x00E0 );
   wm8994_write(codec,  WM8994_AIF2_CLOCKING_1,0x0011 );

   wm8994_write(codec,WM8994_AIF1_CONTROL_1,0x4010);
   wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE,0x7000);
   wm8994_write(codec,WM8994_AIF1_RATE,0x0073);
   wm8994_write(codec,WM8994_AIF1_CLOCKING_1,0x0011);
   wm8994_write(codec, WM8994_CLOCKING_1,0x000E );
   msleep(5);


   wm8994_write(codec, 0x312, 0x7000 );
   wm8994_write(codec, 0x311, 0x4001 );

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_2,0x63A0 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4,0x3003 );
   wm8994_write(codec, WM8994_DAC2_MIXER_VOLUMES, 0x018C );
   wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, 0x0010 );
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING);	//605H : 0x0010
   val &= ~(WM8994_ADC2_TO_DAC2R_MASK);
   val |= (WM8994_ADC2_TO_DAC2R);			// Changed value to support stereo
   wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, val);

/* ==================== Input Path Configuration ==================== */
   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0044 );
   
   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);


#if 0
   /*
   wm8994_write(codec,  0x541, 0x0845 );
   wm8994_write(codec,  0x542, 0x0000 );
   wm8994_write(codec,  0x543, 0x0000 );
   wm8994_write(codec,  0x544, 0x001F );
   wm8994_write(codec,  0x540, 0x01BC );
   */

   //DRC for Noise-gate (AIF2) from S1
   wm8994_write(codec, 0x541, 0x0850);
   wm8994_write(codec, 0x542, 0x0800);
   wm8994_write(codec, 0x543, 0x0001);
   wm8994_write(codec, 0x544, 0x0008);
   wm8994_write(codec, 0x540, 0x01BC);
#endif
   
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_AIF1_DAC1_EQ_GAINS_1, 0x0000 );

/* Pop noise suppression */
   wm8994_write(codec,0x102,0x0003);
   wm8994_write(codec,0x56,0x0003);
   wm8994_write(codec,0x102,0x0000);
// wm8994_write(codec,WM8994_CLASS_W_1,0x0005);
   wm8994_write(codec,0x5D,0x0002);
   wm8994_write(codec,WM8994_DC_SERVO_2,0x03E0);

/* ==================== Bias Configuration ==================== */
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0303);
   wm8994_write(codec,WM8994_ANALOGUE_HP_1,0x0022);
   
   //Enable Charge Pump 
   wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x9F25);

   msleep(5);  // 5ms delay

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x00F0 );

/* ==================== Output Paths Configuration ==================== */
#ifdef DAC_TO_DIRECT_HPOUT1
   // Enbale DAC1L to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
   val |= WM8994_DAC1L_TO_HPOUT1L ;    
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale DAC1R to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
   val |= WM8994_DAC1R_TO_HPOUT1R ;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= WM8994_DAC1L_TO_MIXOUTL;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= WM8994_DAC1R_TO_MIXOUTR;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif


/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | TUNE_FM_HP_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | TUNE_FM_HP_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   //HPOUT_VOL
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
//   val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol); // 4 dB
   val |= (WM8994_HPOUT1_VU_MASK | 0x003F); // 4 dB   
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
//   val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol); // 4 dB
   val |= (WM8994_HPOUT1_VU_MASK | 0x003F); // 4 dB   
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);

   if(apollo_get_remapped_hw_rev_pin() >= 6)
   {
      //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
//      val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol); // 4 dB
      val |= (WM8994_HPOUT1_VU_MASK | 0x003F); // 4 dB      
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
//      val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol); // 4 dB
      val |= (WM8994_HPOUT1_VU_MASK | 0x003F); // 4 dB      
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }
   else
   {
      //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
//      val |= (WM8994_HPOUT1_VU_MASK | (new_fm_hp_vol-4)); // 4 dB
      val |= (WM8994_HPOUT1_VU_MASK | (0x003F-4)); // 4 dB      
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
//      val |= (WM8994_HPOUT1_VU_MASK | (new_fm_hp_vol-4)); // 4 dB
      val |= (WM8994_HPOUT1_VU_MASK | (0x003F-4)); // 4 dB      
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }

/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

   wm8994_write(codec,WM8994_DC_SERVO_1,0x0303);
   msleep(160);

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-DC_SERVO_OFFSET)&0x00ff;
   TestHigh1=(((u16)TestHigh-DC_SERVO_OFFSET)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

   wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);

   msleep(20);

   wm8994_write(codec, WM8994_ANALOGUE_HP_1, 0x00EE );

   wm8994_write(codec, WM8994_OVERSAMPLING, 0x0001 );
    
   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0030 ); // SMbus_16inx_16dat     Write  0x34

// wm8994_set_fm_input_mute(5);
// wm8994_write(codec, 0x110, 0x8100); //8100 -> 8113
#else /* FM_PATH_DRC_BLOCK */
   wm8994_write(codec,0x00,0x0000);
   wm8994_write(codec,0x01,0x0005);
   msleep(50);
   wm8994_write(codec, 0x01, 0x0003);
   msleep(50);
// FLL1 setting for noti sound
   wm8994_write(codec,0x224,0x0C80);
   wm8994_write(codec,0x221,0x0700);
   wm8994_write(codec,0x222,0x86C2);
   wm8994_write(codec,0x223,0x00E0);
   wm8994_write(codec,0x220,0x0005);
   msleep(5);
   wm8994_write(codec,0x208,0x000A);
   wm8994_write(codec,0x300,0x4010);
   wm8994_write(codec,0x302,0x7000);
   wm8994_write(codec,0x210,0x0073);
   wm8994_write(codec,0x200,0x0011);
// DAC1 LR mixer
   wm8994_write(codec,0x601,0x0001);
   wm8994_write(codec,0x602,0x0001);
   wm8994_write(codec,0x610,0x00C0);
   wm8994_write(codec,0x611,0x00C0);

// Wolfson setting seq
   wm8994_write(codec, 0x200, 0x0011);
   wm8994_write(codec, 0x101, 0x0004);
// wm8994_write(codec, 0x02, 0x03A0);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, 0x6BA0 ); /*MIXIN L-R, IN2 L-R*/
   wm8994_write(codec, 0x01, 0x0003);
/*
   wm8994_write(codec, 0x19, 0x0116);
   wm8994_write(codec, 0x1B, 0x0116);
*/
   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   
   wm8994_write(codec, 0x28, 0x0044);
/*
   wm8994_write(codec, 0x29, 0x0100);
   wm8994_write(codec, 0x2A, 0x0100);
*/
   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);
/*
   wm8994_write(codec, 0x2D, 0x0040);
   wm8994_write(codec, 0x2E, 0x0040);
*/
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= (WM8994_MIXINL_TO_MIXOUTL | WM8994_DAC1L_TO_MIXOUTL);
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= (WM8994_MIXINL_TO_MIXOUTL | WM8994_DAC1R_TO_MIXOUTR);
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);

   
   wm8994_write(codec, 0x102, 0x0003);
   wm8994_write(codec, 0x56, 0x0003);
   wm8994_write(codec, 0x102, 0x0000);
   wm8994_write(codec, 0x5D, 0x0002);
   wm8994_write(codec, 0x55, 0x03E0);
   wm8994_write(codec, 0x01, 0x0303);
   wm8994_write(codec, 0x60, 0x0022);
   wm8994_write(codec, 0x4C, 0x9F25);

   msleep(5);

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,0x0303);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,0x0030);
   
   wm8994_write(codec, 0x54, 0x0303);

   msleep(160);

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-4)&0x00ff;
   TestHigh1=(((u16)TestHigh-4)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);
   
   wm8994_write(codec, 0x54, 0x000F);

   msleep(20);
   
   wm8994_write(codec, 0x60, 0x00EE);
   wm8994_write(codec, 0x03, 0x00F0);
/*
   wm8994_write(codec, 0x1C, 0x01FD);
   wm8994_write(codec, 0x1D, 0x01FD);
*/
/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );

/* ==================== Output Path Configuration ==================== */
   // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
   val |= WM8994_HPOUT1L_MUTE_N;
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
   val |= WM8994_HPOUT1R_MUTE_N;
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);

/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | 0x000F); // 6 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | 0x000F); // 6 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   //HPOUT_VOL
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | 0x003D); // 4 dB
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | 0x003D); // 4 dB
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
#endif /* FM_PATH_DRC_BLOCK */
}

void wm8994_fmradio_route_speaker_headset(struct snd_soc_codec *codec)
{
   u16 val = 0;
   
   u16 TestReturn1=0;
   u16 TestReturn2=0;
   u16 TestLow1=0;
   u16 TestHigh1=0;
   u8 TestLow=0;
   u8 TestHigh=0;
   
   printk("%s() \n",__func__);

#ifdef FM_PATH_DRC_BLOCK
   wm8994_write(codec,WM8994_SOFTWARE_RESET, 0x0000 );
   msleep(50);
   
   wm8994_write(codec, 0x01,0x0003 ); // SMbus_16inx_16dat     Write  0x34      * Power Management (1)(01H): 0003  SPKOUTR_ENA=0, SPKOUTL_ENA=0, HPOUT2_ENA=0, HPOUT1L_ENA=0, HPOUT1R_ENA=0, MICB2_ENA=0, MICB1_ENA=0, VMID_SEL=01, BIAS_ENA=1
   msleep(50);

   wm8994_write(codec, WM8994_GPIO_3, 0x0100 );
   wm8994_write(codec, WM8994_GPIO_4, 0x0100 );
   wm8994_write(codec, WM8994_GPIO_5, 0x8100 );
   wm8994_write(codec, WM8994_GPIO_6, 0xA101 );
   wm8994_write(codec, WM8994_GPIO_7, 0x0100 );
    

   // FLL1 setting 
   wm8994_write(codec, WM8994_FLL1_CONTROL_1,0x0005 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_2,0x0700 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_3,0x86C2 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_5,0x0C80 );
   wm8994_write(codec, WM8994_FLL1_CONTROL_4,0x00E0 );
   wm8994_write(codec, WM8994_AIF2_CLOCKING_1,0x0011 );

   wm8994_write(codec,WM8994_AIF1_CONTROL_1,0x4010);
   wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE,0x7000);
   wm8994_write(codec,WM8994_AIF1_RATE,0x0073);
   wm8994_write(codec,WM8994_AIF1_CLOCKING_1,0x0011);
   wm8994_write(codec, WM8994_CLOCKING_1,0x000E );
   msleep(5);


   wm8994_write(codec, 0x312, 0x7000 );
   wm8994_write(codec, 0x311, 0x4001 );

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_2,0x63A0 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_4,0x3003 );
   wm8994_write(codec, WM8994_DAC2_MIXER_VOLUMES, 0x018C );
   wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, 0x0010 );
   val = wm8994_read(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING);	//605H : 0x0010
   val &= ~(WM8994_ADC2_TO_DAC2R_MASK);
   val |= (WM8994_ADC2_TO_DAC2R);			// Changed value to support stereo
   wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, val);

/* ==================== Input Path Configuration ==================== */
   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0044 );
   
   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);


#if 0
   /*
   wm8994_write(codec,  0x541, 0x0845 );
   wm8994_write(codec,  0x542, 0x0000 );
   wm8994_write(codec,  0x543, 0x0000 );
   wm8994_write(codec,  0x544, 0x001F );
   wm8994_write(codec,  0x540, 0x01BC );
   */

   //DRC for Noise-gate (AIF2) from S1
   wm8994_write(codec, 0x541, 0x0850);
   wm8994_write(codec, 0x542, 0x0800);
   wm8994_write(codec, 0x543, 0x0001);
   wm8994_write(codec, 0x544, 0x0008);
   wm8994_write(codec, 0x540, 0x01BC);
#endif
   
   wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 );
   wm8994_write(codec, WM8994_DAC1_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC1_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x01C0 );
   wm8994_write(codec, WM8994_AIF1_DAC1_EQ_GAINS_1, 0x0000 );

/* Pop noise suppression */
   wm8994_write(codec, 0x102, 0x0003);
   wm8994_write(codec, 0x56, 0x0003);
   wm8994_write(codec, 0x102, 0x0000);
// wm8994_write(codec,WM8994_CLASS_W_1,0x0005);
   wm8994_write(codec, 0x5D, 0x0002);
   wm8994_write(codec, WM8994_DC_SERVO_2, 0x03E0);

/* ==================== Bias Configuration ==================== */
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x1303);
   wm8994_write(codec, WM8994_ANALOGUE_HP_1,0x0022);
   
   //Enable Charge Pump 
   wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x9F25);

   msleep(5);  // 5ms delay

   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x3303 );
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x00F0 );

/* ==================== Output Paths Configuration ==================== */
#ifdef DAC_TO_DIRECT_HPOUT1
   // Enbale DAC1L to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
   val |= WM8994_DAC1L_TO_HPOUT1L ;    
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale DAC1R to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
   val |= WM8994_DAC1R_TO_HPOUT1R ;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= WM8994_DAC1L_TO_MIXOUTL;
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= WM8994_DAC1R_TO_MIXOUTR;
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif

   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK | WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_MIXINL_TO_SPKMIXL | WM8994_MIXINR_TO_SPKMIXR | WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);


/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | TUNE_FM_SPK_HP_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | TUNE_FM_SPK_HP_IN_VOL); // 6 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   if(apollo_get_remapped_hw_rev_pin() >= 6)
   {
      //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | TUNE_FM_SPK_HP_HP_VOL); // 4 dB
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | TUNE_FM_SPK_HP_HP_VOL); // 4 dB
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }
   else
   {
      //HPOUT_VOL
      val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | (TUNE_FM_SPK_HP_HP_VOL-4)); // 4 dB
      wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
      val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
      val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
      val |= (WM8994_HPOUT1_VU_MASK | (TUNE_FM_SPK_HP_HP_VOL-4)); // 4 dB
      wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
   }

   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_MIXINL_SPKMIXL_VOL_MASK | WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_MIXINR_SPKMIXR_VOL_MASK | WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_HP_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | TUNE_FM_SPK_HP_SPK_VOL);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= (TUNE_FM_SPK_HP_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); // 7.5 dB
   wm8994_write(codec, WM8994_CLASSD, val);


/* ==================== Digital Paths Configuration ==================== */
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

   wm8994_write(codec,WM8994_DC_SERVO_1,0x0303);
   msleep(160);

   TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
   TestLow=(u8)(TestReturn1 & 0xff);
   TestHigh=(u8)((TestReturn1>>8) & 0xff);
   TestLow1=((u16)TestLow-DC_SERVO_OFFSET)&0x00ff;
   TestHigh1=(((u16)TestHigh-DC_SERVO_OFFSET)<<8)&0xff00;
   TestReturn2=TestLow1|TestHigh1;
   wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

   wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);

   msleep(20);

   wm8994_write(codec, WM8994_ANALOGUE_HP_1, 0x00EE );

   wm8994_write(codec, WM8994_OVERSAMPLING, 0x0001 );

   wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0030 ); // SMbus_16inx_16dat     Write  0x34


// wm8994_write(codec, 0x110, 0x8100); //8100 -> 8113

#else /* FM_PATH_DRC_BLOCK */
   wm8994_write(codec,WM8994_ANTIPOP_2, 0x006C );  

/* ==================== Power Management ==================== */
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0003 ); 
   msleep(50);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x1303 ); // HPOUT1L_ENA=1, HPOUT1R_ENA=1

/*Analogue Input Configuration*/
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, 0x6BA0 ); /*MIXIN L-R, IN2 L-R*/

   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3, 0x03F0 ); // MIXOUTLVOL_ENA=1, MIXOUTRVOL_ENA=1, MIXOUTL_ENA=1, MIXOUTR_ENA=1

/* ==================== Input Path Configuration ==================== */
   // IN2LN_TO_IN2L, IN2RN_TO_IN2R
   wm8994_write(codec, WM8994_INPUT_MIXER_2, 0x0044);

   // IN2L unmute
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_MUTE_MASK);
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   // IN2R unmute
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_MUTE_MASK);
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);

   // IN2L_TO_MIXINL, IN2R_TO_MIXINR
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
   val |= WM8994_IN2L_TO_MIXINL;
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
   val |= WM8994_IN2R_TO_MIXINR;
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);
#if 1
/* ==================== Digital Paths Configuration ==================== */
   //Enable Dac1 and DAC2 and the Timeslot0 for AIF1
   val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 );    
   val &= ~(WM8994_AIF1DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_DAC1R_ENA_MASK);
   val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA);
   wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);
   // Unmute the AF1DAC1   
   val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 );   
   val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
   val |= WM8994_AIF1DAC1_UNMUTE;
   wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
   // Enable the Timeslot0 to DAC1L
   val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  );    
   val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
   val |= WM8994_AIF1DAC1L_TO_DAC1L;
   wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
   //Enable the Timeslot0 to DAC1R
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  );   
   val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
   val |= WM8994_AIF1DAC1R_TO_DAC1R;
   wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
   //Unmute LeftDAC1
   val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME );
   val &= ~(WM8994_DAC1L_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
   //Unmute RightDAC1
   val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
   val &= ~(WM8994_DAC1R_MUTE_MASK);
   wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
   //AIF Clock enable & SYSCLK select : AIF1+SYSCLK=AIF1
   wm8994_write(codec,WM8994_CLOCKING_1, 0x000A );
#endif
/* ==================== Output Path Configuration ==================== */
   //SPKMIXL, SPKLVOL PGA enable
   val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
   val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
   val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
   wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
   //SPKMIXL, R
   val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
   val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK | WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
   val |= (WM8994_MIXINL_TO_SPKMIXL | WM8994_MIXINR_TO_SPKMIXR | WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
   wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
   val |= WM8994_SPKOUTL_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
   val |= WM8994_SPKOUTR_MUTE_N;
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
   val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
   val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
   wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

   // Enbale MIXOUTL to HPOUT1L path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
   val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
   val |= (WM8994_MIXINL_TO_MIXOUTL | WM8994_DAC1L_TO_MIXOUTL);
   wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
   // Enbale MIXOUTR to HPOUT1R path
   val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
   val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
   val |= (WM8994_MIXINL_TO_MIXOUTL | WM8994_DAC1R_TO_MIXOUTR);
   wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);

   // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
   val |= WM8994_HPOUT1L_MUTE_N;
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
   val |= WM8994_HPOUT1R_MUTE_N;
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val); 

/* ==================== EAR_SWITCH_SEL ==================== */
   gpio_set_value(GPIO_EAR_SEL, 0);

/* ==================== Enable Charge Pump ==================== */
   wm8994_write(codec,WM8994_CHARGE_PUMP_1,0x9F25);
      
   wm8994_write(codec,0x60, 0x0022 ); // HPOUT1L_DLY=1, HPOUT1R_DLY=1
   wm8994_write(codec,0x54, 0x0033 ); // Kick-off DC Servo cal on both channels
   mdelay(200);   
   wm8994_write(codec,0x60, 0x00EE ); // HPOUT1L_RMV_SHORT=1, HPOUT1L_OUTP=1, HPOUT1L_DLY=1, HPOUT1R_RMV_SHORT=1, HPOUT1R_OUTP=1, HPOUT1R_DLY=1

/* ==================== Clock Setting ==================== */
   /*Clocking*/
   wm8994_write(codec, 0x208, 0x000A );

   // FLL1 = 11.2896 MHz
   wm8994_write(codec, 0x220, 0x0005 ); // FLL1 Enable, FLL1 Fractional Mode
   wm8994_write(codec, 0x221, 0x0700 ); // FLL1_OUT_DIV = 8 = Fvco / Fout
   wm8994_write(codec, 0x222, 0x86C2 ); // FLL1_N = 34498
   wm8994_write(codec, 0x224, 0x0C80 ); // 0x0C88 -> 0x0C80 FLL1_CLK_REV_DIV = 1 = MCLK / FLL1, FLL1 = MCLK1
   wm8994_write(codec, 0x223, 0x00E0 ); // FLL1_N = 7, FLL1_GAIN = X1
   wm8994_write(codec, 0x200, 0x0011 ); // AIF1 Enable, AIF1CLK = FLL1 

   // GPIO
   wm8994_write(codec, 0x700, 0xA101 );  // GPIO1 is Input Enable
   wm8994_write(codec, 0x701, 0x8100 ); // GPIO 2 - Set to MCLK2 input
   wm8994_write(codec, 0x702, 0x8100 ); // GPIO 3 - Set to BCLK2 input
   wm8994_write(codec, 0x703, 0x8100 ); // GPIO 4 - Set to DACLRCLK2 input
   wm8994_write(codec, 0x704, 0x8100 ); // GPIO 5 - Set to DACDAT2 input
   wm8994_write(codec, 0x705, 0xA101 ); // GPIO 6 is Input Enable
   wm8994_write(codec, 0x706, 0x0100 ); // GPIO 7 - Set to ADCDAT2 output

/* ==================== Set default Gain ==================== */
   //-------------------------------------- Analog Input
   val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_VOL_MASK);
   val |= (WM8994_IN2L_VU | 0x0011); // 9 dB
   wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val);
   val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME );
   val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_VOL_MASK);
   val |= (WM8994_IN2R_VU | 0x0011); // 9 dB
   wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val);
   // IN2L_MIXINL_VOL, IN2R_MIXINR_VOL
   val = wm8994_read(codec, WM8994_INPUT_MIXER_3 );
   val &= ~(WM8994_IN2L_MIXINL_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
   val = wm8994_read(codec, WM8994_INPUT_MIXER_4 );
   val &= ~(WM8994_IN2R_MIXINR_VOL_MASK);
   val |= 0x0000; // 0dB
   wm8994_write(codec, WM8994_INPUT_MIXER_4, val);

   //-------------------------------------- Analog Output
   // SPKMIXL, R VOL unmute
   val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
   val &= ~(WM8994_MIXINL_SPKMIXL_VOL_MASK | WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
   val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
   val &= ~(WM8994_MIXINR_SPKMIXR_VOL_MASK | WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
   val |= 0x0; 
   wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
   // SPKLVOL, SPKRVOL
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | 0x3F);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
   val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
   val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
   val |= (WM8994_SPKOUT_VU | 0x3F);
   wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
   // SPKOUTLBOOST
   val = wm8994_read(codec, WM8994_CLASSD);
   val &= ~(WM8994_SPKOUTL_BOOST_MASK);
   val |= 0x0000; // 0 dB
   wm8994_write(codec, WM8994_CLASSD, val);

   //HPOUT_VOL
   val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | 0x0030); // -9 dB
   wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
   val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
   val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
   val |= (WM8994_HPOUT1_VU_MASK | 0x0030); // -9 dB
   wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
#endif /* FM_PATH_DRC_BLOCK */
}

void set_mic_bias(struct snd_soc_codec *codec, int mode)
{
   unsigned char method = mode & 0xF0; // CAll, Playback, Voicememo, Fmradio, etc... 
   unsigned char device = mode & 0x0F; // RCV, SPK, HP, SPK_HP

   if( method == MM_AUDIO_VOICECALL )
   {
      wm8994_write(codec, 0x500, 0x0100); // AIF2 ADC L mute
      wm8994_write(codec, 0x501, 0x0100); // AIF2 ADC R mute
      if( device == MM_AUDIO_OUT_RCV)
      {
         wm8994_micbias_control(codec,0,1); // MICBIAS1 : mainmic
      }
      else if ( device == MM_AUDIO_OUT_HP || device == MM_AUDIO_OUT_SPK )
      {
         wm8994_micbias_control(codec,1,1); // MICBIAS2 : submic, earmic
      }
      msleep(5);
      wm8994_write(codec, 0x500, 0x01C0); // AIF2 ADC L unmute (0dB)
      wm8994_write(codec, 0x501, 0x01C0); // AIF2 ADC R unmute (0dB)
   }
   else if ( method == MM_AUDIO_VOICEMEMO )
   {
      if ( device == MM_AUDIO_IN_SUB )
      {
         wm8994_micbias_control(codec,0,1); // MICBIAS1 : mainmic
      }
      else if ( device == MM_AUDIO_IN_MAIN || device == MM_AUDIO_IN_EAR )
      {
         wm8994_micbias_control(codec,1,1); // MICBIAS2 : submic, earmic
      }
   }
   else if( method == MM_AUDIO_PLAYBACK )
   {
      if( device == MM_AUDIO_OUT_HP || device == MM_AUDIO_OUT_SPK_HP || device == MM_AUDIO_OUT_RING_SPK_HP )
      {
         wm8994_micbias_control(codec,1,1); // DISABLE micbias1,2
      }
   }
   else
   {
      if(!get_headset_status() ) // for earphone button fuction
         wm8994_micbias_control(codec,0,0); // DISABLE micbias1,2
   }
   
}

/* 
** Control MICBIAS by GPIO & WM8994
** 
** sel / 0:MainMic, 1:Submic or Earmic
** en  / 0:Mute,    1:Enable
**
*/
void wm8994_micbias_control(struct snd_soc_codec *codec ,int sel, int en)
{
   u16 tmp; 
   
   printk("[%s] MIC_SWITCH = %d (0:main mic, 1:submic,earmic) MIC_ENABLE = %d(0:mute, 1:Enable) \n",__func__,sel,en);
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
   if (en )
   {
      printk("mic enabled \n");
      if( sel ) // micbias2 : submic, earmic
      {
         printk("SubMic, EarMic Enable\n");
         // MIC_SEL low
         gpio_set_value(GPIO_MIC_SEL, 1);
         // Enable Micbias 2
         gpio_set_value(GPIO_MICBIAS2_EN, (en&sel) );

         // Disable Micbias 1
         tmp = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
         tmp &= ~( WM8994_MICB1_ENA_MASK) ; // MICB1_ENA Clear
         wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, tmp);
      }
      else // micbia1 : mainmic
      {
         printk("MainMic Enable\n");

         // MIC_SEL low
         gpio_set_value(GPIO_MIC_SEL, 0);

         // Disable Micbias 2
         if(!get_headset_status())
         {
            gpio_set_value(GPIO_MICBIAS2_EN, (en&sel) );
         }
         else
         {
            printk("Headset device detected so don't disable MICBIAS2\n");
         }

         // Enable Micbias 1
         tmp = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
         tmp &= ~( WM8994_MICB1_ENA_MASK ) ; //  MICB1_ENA Clear
         tmp |= WM8994_MICB1_ENA; //  MICB1_ENA[4]=1
         wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, tmp);
      }
   }
   else
   {
      printk("mic disabled \n");
      // Disable Micbias 2
      if(!get_headset_status())
      {
         gpio_set_value(GPIO_MICBIAS2_EN, en);
      }
      else
      {
         printk("Headset device detected so don't disable MICBIAS2\n");
      }

      // Disable Micbias 1
      tmp = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
      tmp &= ~( WM8994_MICB1_ENA_MASK) ; // MICB1_ENA Clear
      wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, tmp);
   }
#endif
}

int wm8994_disable_path(struct snd_soc_codec *codec, int mode)
{
   int SndMethod = mode & 0xF0;
   int SndDevice = mode & 0x0F;
   
   printk("%s() method = 0x%02x, device = 0x%02x\n",__func__,SndMethod, SndDevice);

   switch(SndMethod)
   {
      
      case MM_AUDIO_PLAYBACK:
      {
         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            wm8994_disable_playback_path(codec, MM_AUDIO_PLAYBACK_RCV);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            wm8994_disable_playback_path(codec, MM_AUDIO_PLAYBACK_SPK);

         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            wm8994_disable_playback_path(codec, MM_AUDIO_PLAYBACK_HP);

         }
         else if ( SndDevice == MM_AUDIO_OUT_BT )
         {
            printk("###### MM_AUDIO_PLAYBACK_BT PATH IS NOT IMPLEMENTED ######\n");
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            wm8994_disable_playback_path(codec, MM_AUDIO_OUT_SPK_HP);
         }
         else if ( SndDevice == MM_AUDIO_OUT_RING_SPK_HP )
         {
            wm8994_disable_playback_path(codec, MM_AUDIO_OUT_RING_SPK_HP);
         }
      }
      break;
      
      case MM_AUDIO_VOICECALL:
      {
         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            wm8994_disable_voicecall_path (codec, MM_AUDIO_VOICECALL_RCV );
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            wm8994_disable_voicecall_path(codec ,MM_AUDIO_VOICECALL_SPK );
         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            wm8994_disable_voicecall_path(codec , MM_AUDIO_VOICECALL_HP );
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            printk("###### MM_AUDIO_VOICECALL_SPK_HP PATH IS NOT IMPLEMENTED ######\n");
         }
      }
      break;
      
      case MM_AUDIO_FMRADIO:
      {
         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            printk("###### MM_AUDIO_FMRADIO_RCV PATH IS NOT IMPLEMENTED ######\n");
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            wm8994_disable_fmradio_path(codec,MM_AUDIO_FMRADIO_SPK);
         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            wm8994_disable_fmradio_path(codec,MM_AUDIO_FMRADIO_HP);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            wm8994_disable_fmradio_path(codec,MM_AUDIO_FMRADIO_SPK_HP);
         }
      }
      break;
      
      case MM_AUDIO_VOICEMEMO:
      {
         if( SndDevice == MM_AUDIO_IN_MAIN )
         {
            wm8994_disable_rec_path(codec, MM_AUDIO_VOICEMEMO_MAIN);
         }
         else if ( SndDevice == MM_AUDIO_IN_SUB )
         {
            wm8994_disable_rec_path(codec, MM_AUDIO_VOICEMEMO_SUB);
         }
         else if ( SndDevice == MM_AUDIO_IN_EAR )
         {
            wm8994_disable_rec_path(codec, MM_AUDIO_VOICEMEMO_EAR);
         }
         else if ( SndDevice == MM_AUDIO_IN_BT )
         {
            printk("###### MM_AUDIO_IN_BT PATH IS NOT IMPLEMENTED ######");
         }
      }
      break;
      
      default:
      {
         printk("###### Unknown Codec Path : NOT IMPLEMENTED ######\n");
      }
      break;
   }
   return 0;
}


int wm8994_enable_path(struct snd_soc_codec *codec, int mode)
{
   struct wm8994_priv *wm8994 = codec->private_data;

   int SndMethod = mode & 0xF0;
   int SndDevice = mode & 0x0F;
   
   enable_path_mode = mode;
   printk("%s() method = 0x%02x, device = 0x%02x\n",__func__,SndMethod, SndDevice);

   switch(SndMethod)
   {
      case MM_AUDIO_PLAYBACK:
      {
	wm8994->codec_state |= PLAYBACK_ACTIVE;
	wm8994->stream_state |= PCM_STREAM_PLAYBACK;

         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            wm8994_playback_route_receiver(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            wm8994_playback_route_speaker(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            wm8994_playback_route_headset(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_BT )
         {
            printk("###### MM_AUDIO_OUT_BT PATH IS NOT IMPLEMENTED ######\n");
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            wm8994_playback_route_speaker_headset(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_RING_SPK_HP )
         {
            wm8994_playback_route_speaker_headset(codec);
         }
      }
      break;
      
      case MM_AUDIO_VOICECALL:
      {
	wm8994->codec_state |= CALL_ACTIVE;

         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            printk("routing voice path to RCV \n");
            wm8994_voice_rcv_path (codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            printk("routing  voice path to   SPK \n");
            wm8994_voice_spkr_path(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            printk("routing voice path to HP \n");
            wm8994_voice_ear_path(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_BT )
         {
            printk("routing voice path to BT \n");
            wm8994_voice_BT_path(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            printk("###### MM_AUDIO_VOICECALL_SPK_HP PATH IS NOT IMPLEMENTED ######\n");
         }
      }
      break;
      
      case  MM_AUDIO_FMRADIO:
      {
	wm8994->codec_state |= FMRADIO_ACTIVE;

         if( SndDevice == MM_AUDIO_OUT_RCV )
         {
            printk("###### MM_AUDIO_FMRADIO_RCV PATH IS NOT IMPLEMENTED ######\n");
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK )
         {
            wm8994_fmradio_route_speaker(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_HP )
         {
            wm8994_fmradio_route_headset(codec);
         }
         else if ( SndDevice == MM_AUDIO_OUT_SPK_HP )
         {
            wm8994_fmradio_route_speaker_headset(codec);
#if 0
            /* FM mode do not support EAR_SPK mode 
            so if this audio path setted, regards as notification or alarm sound mixed on FM playing. 
            when alarm sounds with FM radio, it have the FM noise, 
            so mute the FM sound by setting playback path instead of FM path*/
            wm8994_playback_route_speaker_headset(codec);
#endif
         }
      }
      break;
      
      case MM_AUDIO_VOICEMEMO:
      {
         if( SndDevice == MM_AUDIO_IN_MAIN )
         {
            wm8994_record_sub_mic(codec);
         }
         else if ( SndDevice == MM_AUDIO_IN_SUB )
         {
            wm8994_record_main_mic(codec);
         }
         else if ( SndDevice == MM_AUDIO_IN_EAR )
         {
            wm8994_record_headset_mic(codec);
         }
         else if ( SndDevice == MM_AUDIO_IN_BT )
         {
            printk("###### MM_AUDIO_IN_BT PATH IS NOT IMPLEMENTED ######\n");
         }
      }
      break;
      
      default:
      {
         printk("###### Unknown Codec Path : NOT IMPLEMENTED ######\n");
      }
      break;
   }
   return 0;
}



int wm8994_change_path(struct snd_soc_codec *codec, int to_mode, int from_mode)
{
   int ret = 1;
   u16 val = 0;

   printk("%s : from_mode 0x%x, to_mode 0x%x ==> SPK\n", __func__, from_mode, to_mode);
   switch (to_mode) 
   {
         
      case MM_AUDIO_PLAYBACK_SPK :
      case MM_AUDIO_PLAYBACK_HP :
         switch (from_mode) 
         {
#if 0       
            case MM_AUDIO_PLAYBACK_HP :
            case MM_AUDIO_PLAYBACK_SPK :
#endif               
            case MM_AUDIO_PLAYBACK_SPK_HP : 
            case MM_AUDIO_PLAYBACK_RING_SPK_HP : 
               if (to_mode == MM_AUDIO_PLAYBACK_SPK) 
               {
                  printk("PLAYBACK_SPK_HP ==> SPK\n");
                  /* ==================== Bias Configuration ==================== */
                  //Disable hp left and right
                  val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
                  val &= ~(WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK );
                  wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

                  /* ==================== Power Management ==================== */
                  // Disable MIXOUTL / MIXOUTR
                  val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3 ); 
                  val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
                  val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
                  wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);  

                  //MIXOUT_VOL
                  val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
                  val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
                  val |= 0x111; // -21dB
                  wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
                  val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
                  val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
                  val |= 0x111; // -21dB
                  wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);

                  // Enbale MIXOUTL to HPOUT1L path
                  val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
                  val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
                  val |= WM8994_DAC1L_TO_MIXOUTL;
                  wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
                  // Enbale MIXOUTR to HPOUT1R path
                  val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
                  val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
                  val |= WM8994_DAC1R_TO_MIXOUTR;
                  wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);

                  // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N mute
                  val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
                  val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
                  wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
                  val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME); 
                  val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
                  wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);

               /* ==================== Set default Gain ==================== */
                  //-------------------------------------- Digital
                  // AIF1_DAC1_VOL 1st step
                  val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
                  val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
                  val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
                  wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
                  val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
                  val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
                  val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
                  wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
                  // DAC1_VOL 2nd step
                  val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
                  val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
                  val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
                  wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
                  val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
                  val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
                  val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
                  wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
                  //-------------------------------------- Analog Output
                  // SPKMIXL, R VOL unmute
                  val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
                  val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
                  val |= 0x0; 
                  wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
                  val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
                  val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
                  val |= 0x0; 
                  wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);
                  // SPKLVOL, SPKRVOL
                  val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
                  val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
                  if(apollo_get_hw_type()) // GT-I5800
                     val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL_OPEN);
                  else // GT-I5801
                     val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL);
                  wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
                  val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
                  val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
                  if(apollo_get_hw_type()) // GT-I5800
                     val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL_OPEN);
                  else // GT-I5801
                     val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_SPK_VOL);
                  wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
                  // SPKOUTLBOOST
                  val = wm8994_read(codec, WM8994_CLASSD);
                  val &= ~(WM8994_SPKOUTL_BOOST_MASK);
                  val |= (TUNE_PLAYBACK_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
                  wm8994_write(codec, WM8994_CLASSD, val);
               }
               else
               {
                  printk("PLAYBACK_SPK_HP ==> HP\n");
#if 0
                  wm8994_playback_route_headset(codec);
                  return 1;
#else
                  // Disable Left speaker
                  val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
                  val &= ~(WM8994_SPKOUTL_ENA_MASK);
                  wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

                  //SPKMIXL, SPKLVOL PGA disable
                  val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
                  val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
                  wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);

                  // SPKMIXL_TO_SPKOUTL / SPKMIXR_TO_SPKOUTR mute
                  val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
                  val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
                  wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

                  // SPKLVOL, SPKRVOL mute
                  val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
                  val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
                  wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
                  val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
                  val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
                  wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

               /* ==================== Set default Gain ==================== */
                  //-------------------------------------- Digital
                  // AIF1_DAC1_VOL 1st step
                  val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
                  val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
                  val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
                  wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
                  val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
                  val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
                  val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
                  wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
                  // DAC1_VOL 2nd step
                  val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
                  val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
                  val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
                  wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
                  val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
                  val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
                  val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
                  wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
                  //-------------------------------------- Analog Output
#ifndef DAC_TO_DIRECT_HPOUT1
                  //MIXOUT_VOL
                  val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
                  val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
                  val |= 0x000;
                  wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
                  val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
                  val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
                  val |= 0x000;
                  wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
#endif
                  if(apollo_get_remapped_hw_rev_pin() >= 6)
                  {
                     //HPOUT_VOL
                     val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
                     val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
                     val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL);
                     wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
                     val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
                     val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
                     val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL);
                     wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
                  }
                  else
                  {
                     //HPOUT_VOL
                     val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
                     val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
                     val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_HP_VOL-4));
                     wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
                     val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
                     val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
                     val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_HP_VOL-4));
                     wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
                  }

                  return 1;
#endif
               }
//             wm8994_set_default_gain(codec, to_mode);
               break;      
#if 0
            case MM_AUDIO_VOICEMEMO_MAIN :
            case MM_AUDIO_VOICEMEMO_SUB :
            case MM_AUDIO_VOICEMEMO_EAR :
               printk("VOICEMEMO->PLAYBACK\n");
               if (to_mode == MM_AUDIO_PLAYBACK_SPK) 
                  ;//amp_set_path(AK4671_AMP_PATH_SPK);
               else
                  ;//amp_set_path(AK4671_AMP_PATH_HP);

               //codec->write(codec, 0x00, 0xC1);        // DAC Enable, ADC Disable, MicAMP Disable
               //set_path_gain(codec, to_mode);
               //set_mic_bias(codec, to_mode);
               //set_amp_gain(to_mode);               
               break;
#endif                           
            default :
               ret = -1;
               break;
         }
         break;
                     
      case MM_AUDIO_PLAYBACK_SPK_HP :
      case MM_AUDIO_PLAYBACK_RING_SPK_HP :         
#if 0
         switch(from_mode)
         {
            case MM_AUDIO_PLAYBACK_HP :      
               printk("PLAYBACK_HP ==> SPK_HP / RING_SPK_HP\n");
               /* ==================== Bias Configuration ==================== */
               //Enbale bias,vmid and Left speaker
               val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
               val &= ~(WM8994_SPKOUTL_ENA_MASK);
               val |= (WM8994_SPKOUTL_ENA);  
               wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
   
               //SPKMIXL, SPKLVOL PGA enable
               val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
               val &= ~( WM8994_SPKLVOL_ENA_MASK | WM8994_SPKRVOL_ENA_MASK);
               val |= (WM8994_SPKLVOL_ENA | WM8994_SPKRVOL_ENA);
               wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 , val);
               //SPKMIXL, R
               val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
               val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
               val |= (WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
               wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
               // SPKLVOL, SPKRVOL
               val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
               val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
               val |= WM8994_SPKOUTL_MUTE_N;
               wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
               val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
               val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
               val |= WM8994_SPKOUTR_MUTE_N;
               wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
               // SPKOUTLBOOST
               val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
               val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
               val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
               wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

               // SPKMIXL, R VOL unmute
               val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
               val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
               wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
               val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
               val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
               wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);

               wm8994_set_default_gain(codec, to_mode);
               break;
               
            case MM_AUDIO_PLAYBACK_SPK :
               printk("PLAYBACK_SPK ==> SPK_HP / RING_SPK_HP\n"); 

            /* ==================== Bias Configuration ==================== */
               //Enable vmid,bias,hp left and right
               val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
               val &= ~(WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK );
               val |= (WM8994_HPOUT1R_ENA  | WM8994_HPOUT1L_ENA  );  
               wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);             
               
#ifdef DAC_TO_DIRECT_HPOUT1
               // Enbale DAC1L to HPOUT1L path
               val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
               val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
               val |= WM8994_DAC1L_TO_HPOUT1L ;    
               wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
               // Enbale DAC1R to HPOUT1R path
               val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
               val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK ;
               val |= WM8994_DAC1R_TO_HPOUT1R ;
               wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#else
               /* ==================== Power Management ==================== */
               val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3 ); 
               val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
               val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
               wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);  

               //MIXOUT_VOL
               val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
               val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
               val |= 0x000;
               wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
               val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
               val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
               val |= 0x000;
               wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);

               // Enbale MIXOUTL to HPOUT1L path
               val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1 );
               val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
               val |= WM8994_DAC1L_TO_MIXOUTL;
               wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val);
               // Enbale MIXOUTR to HPOUT1R path
               val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2 );
               val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
               val |= WM8994_DAC1R_TO_MIXOUTR;
               wm8994_write(codec,WM8994_OUTPUT_MIXER_2 ,val);
#endif
               /* ==================== EAR_SWITCH_SEL ==================== */
               gpio_set_value(GPIO_EAR_SEL, 0);
               
               /* ==================== Enable Charge Pump ==================== */
               val = wm8994_read(codec,WM8994_CHARGE_PUMP_1);
               val &= ~WM8994_CP_ENA_MASK ;
               val = WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson   
               wm8994_write(codec,WM8994_CHARGE_PUMP_1 ,val);

               // Intermediate HP settings 
               val = wm8994_read(codec, WM8994_ANALOGUE_HP_1);    
               val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
                  WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK );
               val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |
                  WM8994_HPOUT1R_RMV_SHORT | WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
               wm8994_write(codec, WM8994_ANALOGUE_HP_1 ,val);
               // HPOUT1L_MUTEM_N, HPOUT1R_MUTEM_N unmute
               val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
               val &= ~(WM8994_HPOUT1L_MUTE_N_MASK);
               val |= WM8994_HPOUT1L_MUTE_N;
               wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
               val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
               val &= ~(WM8994_HPOUT1R_MUTE_N_MASK);
               val |= WM8994_HPOUT1R_MUTE_N;
               wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
               
               wm8994_set_default_gain(codec, to_mode);
               break;
            default : 
               ret = -1;
               break;
         }
#else
      if(from_mode == MM_AUDIO_PLAYBACK_HP)
      {
         printk("PLAYBACK_HP ==> PLAYBACK_SPK_HP\n");
         // Enable Left speaker
         val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
         val &= ~(WM8994_SPKOUTL_ENA_MASK);
         val |= WM8994_SPKOUTL_ENA;
         wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

         // SPKMIXL, R VOL unmute
         val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
         val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
         val |= 0x0; 
         wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
         val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
         val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
         val |= 0x0; 
         wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);

         //SPKMIXL, R
         val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
         val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK | WM8994_DAC1R_TO_SPKMIXR_MASK);
         val |= (WM8994_DAC1L_TO_SPKMIXL | WM8994_DAC1R_TO_SPKMIXR);
         wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
         // SPKLVOL, SPKRVOL
         val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
         val &= ~(WM8994_SPKOUTL_MUTE_N_MASK);
         val |= WM8994_SPKOUTL_MUTE_N;
         wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
         val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
         val &= ~(WM8994_SPKOUTR_MUTE_N_MASK);
         val |= WM8994_SPKOUTR_MUTE_N;
         wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
         // SPKOUTLBOOST
         val = wm8994_read(codec, WM8994_SPKOUT_MIXERS);
         val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTL_MASK);
         val |= (WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL);
         wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

         /* ==================== Set default Gain ==================== */
         //-------------------------------------- Digital
         // AIF1_DAC1_VOL 1st step
         val = wm8994_read(codec, WM8994_AIF1_DAC1_LEFT_VOLUME ); 
         val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1L_VOL_MASK);
         val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
         wm8994_write(codec,WM8994_AIF1_DAC1_LEFT_VOLUME, val);
         val = wm8994_read(codec, WM8994_AIF1_DAC1_RIGHT_VOLUME ); 
         val &= ~(WM8994_AIF1DAC1_VU_MASK | WM8994_AIF1DAC1R_VOL_MASK);
         val |= (WM8994_AIF1DAC1_VU | 0xC0); //0 dB volume  
         wm8994_write(codec,WM8994_AIF1_DAC1_RIGHT_VOLUME, val);
         // DAC1_VOL 2nd step
         val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME ); 
         val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1L_VOL_MASK);
         val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
         wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME,val);
         val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
         val &= ~(WM8994_DAC1_VU_MASK | WM8994_DAC1R_VOL_MASK);
         val |= (WM8994_DAC1_VU | 0xC0); //0 dB volume   
         wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);

         //-------------------------------------- Analog Output
         // SPKMIXL, R VOL unmute
         val = wm8994_read(codec,WM8994_SPKMIXL_ATTENUATION);
         val &= ~(WM8994_DAC1L_SPKMIXL_VOL_MASK | WM8994_SPKMIXL_VOL_MASK);
         val |= 0x0; 
         wm8994_write(codec,WM8994_SPKMIXL_ATTENUATION  ,val);
         val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
         val &= ~(WM8994_DAC1R_SPKMIXR_VOL_MASK | WM8994_SPKMIXR_VOL_MASK);
         val |= 0x0; 
         wm8994_write(codec,WM8994_SPKMIXR_ATTENUATION  ,val);

#ifndef DAC_TO_DIRECT_HPOUT1
         //MIXOUT_VOL
         val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5 );
         val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
         val |= 0x000;
         wm8994_write(codec, WM8994_OUTPUT_MIXER_5, val);
         val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6 );
         val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
         val |= 0x000;
         wm8994_write(codec, WM8994_OUTPUT_MIXER_6, val);
#endif


         if(to_mode == MM_AUDIO_PLAYBACK_SPK_HP)
         {
            // SPKLVOL, SPKRVOL
            val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
            val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
            if(apollo_get_hw_type()) // GT-I5800
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL_OPEN);
            else // GT-I5801
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL);
            wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
            val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
            val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
            if(apollo_get_hw_type()) // GT-I5800
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL_OPEN);
            else // GT-I5801
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_NOTI_SPK_VOL);
            wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
            // SPKOUTLBOOST
            val = wm8994_read(codec, WM8994_CLASSD);
            val &= ~(WM8994_SPKOUTL_BOOST_MASK);
            val |= (TUNE_PLAYBACK_NOTI_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
            wm8994_write(codec, WM8994_CLASSD, val);

            if(apollo_get_remapped_hw_rev_pin() >= 6)
            {
               //HPOUT_VOL
               val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_NOTI_HP_VOL);
               wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
               val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_NOTI_HP_VOL);
               wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
            }
            else
            {
               //HPOUT_VOL
               val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_NOTI_HP_VOL-4));
               wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
               val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_NOTI_HP_VOL-4));
               wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
            }
         }
         else if(to_mode == MM_AUDIO_PLAYBACK_RING_SPK_HP)
         {
            // SPKLVOL, SPKRVOL
            val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
            val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTL_VOL_MASK);
            if(apollo_get_hw_type()) // GT-I5800
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL_OPEN);
            else // GT-I5801
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL);
            wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
            val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
            val &= ~(WM8994_SPKOUT_VU_MASK | WM8994_SPKOUTR_VOL_MASK);
            if(apollo_get_hw_type()) // GT-I5800
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL_OPEN);
            else // GT-I5801
               val |= (WM8994_SPKOUT_VU | TUNE_PLAYBACK_RING_SPK_VOL);
            wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
            // SPKOUTLBOOST
            val = wm8994_read(codec, WM8994_CLASSD);
            val &= ~(WM8994_SPKOUTL_BOOST_MASK);
            val |= (TUNE_PLAYBACK_RING_SPK_CLASSD<<WM8994_SPKOUTL_BOOST_SHIFT); 
            wm8994_write(codec, WM8994_CLASSD, val);

            if(apollo_get_remapped_hw_rev_pin() >= 6)
            {
               //HPOUT_VOL
               val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_RING_HP_VOL);
               wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
               val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_RING_HP_VOL);
               wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
            }
            else
            {
               //HPOUT_VOL
               val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);  
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_RING_HP_VOL-4));
               wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
               val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);    
               val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
               val |= (WM8994_HPOUT1_VU_MASK | (TUNE_PLAYBACK_RING_HP_VOL-4));
               wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
            }
         }
      }
      else
      {
         wm8994_playback_route_speaker_headset(codec);
//       wm8994_set_default_gain(codec, to_mode);
      }
#endif
         break;
      case MM_AUDIO_PLAYBACK_RCV :
      case MM_AUDIO_PLAYBACK_BT :
      case MM_AUDIO_VOICECALL_RCV :
      case MM_AUDIO_VOICECALL_SPK :
      case MM_AUDIO_VOICECALL_HP :
      case MM_AUDIO_VOICECALL_BT :
         ret = -1;
         break;

#if 0 // For FM and notification sound mixed playing
      case MM_AUDIO_FMRADIO_SPK_HP :
         switch(from_mode)
         {
            case MM_AUDIO_FMRADIO_HP:

               break;

            default:
               ret = -1;
         }
         break;
         
      case MM_AUDIO_FMRADIO_HP :
         switch(from_mode)
         {
            case MM_AUDIO_FMRADIO_SPK_HP:

               break;

            default:
               ret = -1;
         }
         break;
#endif         
      default :
         ret = -1;
         break;
   }

   return ret;
}

int idle_mode_enable(struct snd_soc_codec *codec, int mode)
{
   printk("Enable Idle Mode : 0x%x\n", mode);

   switch(mode) {
      case 0:
         break;

      case MM_AUDIO_PLAYBACK_RCV :
         printk("%s() ### NOT IMPLEMENTED for MM_AUDIO_PLAYBACK_RCV\n", __func__);
         break;

      case MM_AUDIO_PLAYBACK_SPK :
         printk("%s() MM_AUDIO_PLAYBACK_SPK\n",__func__);
         wm8994_playback_route_speaker(codec);
         break;
      case MM_AUDIO_PLAYBACK_HP :
         printk("%s() MM_AUDIO_PLAYBACK_HP\n",__func__);
         wm8994_playback_route_headset(codec);
         break;
      case MM_AUDIO_PLAYBACK_SPK_HP :
        case MM_AUDIO_PLAYBACK_RING_SPK_HP :
         printk("%s() MM_AUDIO_PLAYBACK_SPK_HP or MM_AUDIO_PLAYBACK_RING_SPK_HP\n", __func__);
         wm8994_playback_route_speaker_headset(codec);
         break;

      case MM_AUDIO_PLAYBACK_BT :
         printk("%s() ### NOT IMPLEMENTED for MM_AUDIO_PLAYBACK_BT\n", __func__);
         break;

      default :
         printk("%s() invalid IDLE mode(0x%02x)!!! \n",__func__, mode);
         break;
   }

   //set_mic_bias(codec, mode);

   return 0;
}

int idle_mode_disable(struct snd_soc_codec *codec, int mode)
{
   printk("%s() Disable Idle mode : PATH : 0x%x\n", __func__, mode);
/*
   if(!get_headset_status())
      wm8994_micbias_control(codec,0,0); // DISABLE micbias1,2
*/
   // amp_enable(0);

   u16 val;
   switch(mode) {
      case 0:
         printk("Path : Off\n");
         break;

      case MM_AUDIO_PLAYBACK_RCV :
         printk("MM_AUDIO_PLAYBACK_RCV Off\n");
         val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
         val&= ~(WM8994_HPOUT2_ENA_MASK);
         wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
         break;

      case MM_AUDIO_PLAYBACK_SPK :
         printk("MM_AUDIO_PLAYBACK_SPK Off\n");
/*
         val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
         val&= ~(WM8994_SPKOUTL_ENA_MASK);
         wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
*/
         wm8994_write(codec,0x00 ,0x0000);
         wm8994_write(codec,0x01 ,0x0005);
         break;
         
      case MM_AUDIO_PLAYBACK_SPK_HP :
         case MM_AUDIO_PLAYBACK_RING_SPK_HP  :
         printk("MM_AUDIO_PLAYBACK_SPK_HP or MM_AUDIO_PLAYBACK_RING_SPK_HP Off\n");
/*
         wm8994_write(codec,0x610 ,0x02C0);
         wm8994_write(codec,0x611 ,0x02C0);
         wm8994_write(codec,0x60 ,0x0022);
         wm8994_write(codec,0x2D ,0x0000);
         wm8994_write(codec,0x2E ,0x0000);
         wm8994_write(codec,0x05 ,0x0300);
         wm8994_write(codec,0x4C ,0x1F25);

         val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
         val&= ~(WM8994_SPKOUTL_ENA_MASK);
         wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
*/
         wm8994_write(codec,0x00 ,0x0000);
         wm8994_write(codec,0x01 ,0x0005);
         break;

      case MM_AUDIO_PLAYBACK_HP :
         printk("MM_AUDIO_PLAYBACK_HP Off\n");
/*
         wm8994_write(codec,0x610 ,0x02C0);
         wm8994_write(codec,0x611 ,0x02C0);
         wm8994_write(codec,0x60 ,0x0022);
         wm8994_write(codec,0x2D ,0x0000);
         wm8994_write(codec,0x2E ,0x0000);
         wm8994_write(codec,0x05 ,0x0300);
         wm8994_write(codec,0x4C ,0x1F25);
*/
         wm8994_write(codec,0x00 ,0x0000);
         wm8994_write(codec,0x01 ,0x0005);
         break;

      case MM_AUDIO_PLAYBACK_BT :
         break;
         
      case MM_AUDIO_VOICEMEMO_MAIN :
      case MM_AUDIO_VOICEMEMO_SUB :
      case MM_AUDIO_VOICEMEMO_EAR :
         printk("MM_AUDIO_VOICEMEMO Off\n");
         wm8994_write(codec,0x00 ,0x0000);
         wm8994_write(codec,0x01 ,0x0005);
         break;
         
      default:
         printk("%s() invalid IDLE mode(0x%02x)!!! \n", __func__, mode);
         break;
   }

   return 0;

}
