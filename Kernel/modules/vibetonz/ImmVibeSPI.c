/*
** =========================================================================
** File:
**     ImmVibeSPI.c
**
** Description: 
**     Device-dependent functions called by Immersion TSP API
**     to control PWM duty cycle, amp enable/disable, save IVT file, etc...
**
** Portions Copyright (c) 2008-2009 Immersion Corporation. All Rights Reserved. 
**
** This file contains Original Code and/or Modifications of Original Code 
** as defined in and that are subject to the GNU Public License v2 - 
** (the 'License'). You may not use this file except in compliance with the 
** License. You should have received a copy of the GNU General Public License 
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or contact 
** TouchSenseSales@immersion.com.
**
** The Original Code and all software distributed under the License are 
** distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESS OR IMPLIED, AND IMMERSION HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
** INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
** FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see 
** the License for the specific language governing rights and limitations 
** under the License.
** =========================================================================
*/
#include <linux/pwm.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include "tspdrv.h"

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS 1


/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
#if 0
#error Please review the code between the #if and #endif

    if (g_bAmpEnabled)
    {
        DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpDisable.\n"));

        g_bAmpEnabled = false;

#if 0
        mhn_gpio_set_level(GPIO_EN, GPIO_LEVEL_LOW);
	    mz_ops.bstat &= ~HN_BATTERY_MOTOR;
#endif
    }
#endif
    if (g_bAmpEnabled)
    {
		g_bAmpEnabled = false;				
		vib_state = 0;
		set_vibetonz(vib_state);  /*queue_work() not required as Touch Sense Player serialises(schedules) all the calls*/
    }

	return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
#if 0
#error Please review the code between the #if and #endif

    if (!g_bAmpEnabled)
    {
        DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_AmpEnable.\n"));

        g_bAmpEnabled = true;

#if 0
        /* 
        ** Ensure the PWM frequency is at the expected value. These 2 lines of code
        ** can be removed if no other application alters the PWM frequency.
        */
        PWM_CTRL  = 0;                  /* 13Mhz / (0 + 1) = 13MHz */
        PWM_PERIOD = PWM_DUTY_MAX;      /* 13Mhz / (PWM_DUTY_MAX + 1) = 22.4kHz */

        /* Set duty cycle to 50% */
        PWM_DUTY = (PWM_DUTY_MAX+1)>>1; /* Duty cycle range = [0, PWM_DUTY_MAX] */

        /* Enable amp */
        mhn_gpio_set_level(GPIO_EN, GPIO_LEVEL_HIGH);
        mz_ops.bstat |= HN_BATTERY_MOTOR;
#endif
    }
#endif

    if (!g_bAmpEnabled)
    {
	    g_bAmpEnabled = true;
		vib_state = 1;
		set_vibetonz(vib_state); 
    }
    return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
	int ret =0;
#if 0
#error Please review the code between the #if and #endif

    DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Initialize.\n"));

    g_bAmpEnabled = true;   /* to force ImmVibeSPI_ForceOut_AmpDisable disabling the amp */

    /* 
    ** Disable amp.
    ** If multiple actuators are supported, please make sure to call ImmVibeSPI_ForceOut_AmpDisable
    ** for each actuator (provide the actuator index as input argument).
    */
    ImmVibeSPI_ForceOut_AmpDisable(0);

#if 0
    /* 
    ** PWM frequency:
    ** The PWM frequency must be set to a fixed value and shouldn't change
    ** during the lifetime of the app. The ideal solution would be to use a
    ** frequency value between 20kHz and 50kHz. A frequency value slightly
    ** outside of the above limits should still work and be compliant with
    ** TSP requirements (please refer to the TSP integration guide for
    ** further information).
    */

    /* 22.4kHz PWM, duty cycle 50% */
    PWM_CTRL = 0;                   /* 13Mhz / (0 + 1) = 13MHz */
    PWM_PERIOD = PWM_DUTY_MAX;      /* 13Mhz / (PWM_DUTY_MAX + 1) = 22.4kHz */
    PWM_DUTY = (PWM_DUTY_MAX+1)>>1; /* Duty cycle range = [0, PWM_DUTY_MAX] */
#endif
#endif
	
	g_bAmpEnabled = true; 

	/* Vibetonz Enable GPIO initialization - default LOW to OFF*/
	if (gpio_is_valid(GPIO_HAPTIC_EN)) 
	{
		if (gpio_request(GPIO_HAPTIC_EN, "GPJ1"))	
		{
			printk("Failed to request GPIO_HAPTIC_EN \n");
			ret =-1;
			goto ERR_EXIT;
		}
		if(gpio_direction_output(GPIO_HAPTIC_EN, GPIO_LEVEL_LOW))
		{
			printk("Failed to set output direction GPIO_HAPTIC_EN\n");
			ret=-1;
			goto ERR_EXIT;
		}		
	}
	else
	{
		printk("GPIO_HAPTIC_EN is not valid \n");
		ret =-1;
		goto ERR_EXIT;
	}
	/*Setting GPIO_HAPTIC_EN to LOW*/
	s3c_gpio_setpull(GPIO_HAPTIC_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_HAPTIC_EN, GPIO_LEVEL_LOW);	
    	
	/* Vibetonz PWM GPIO initialisation */
   	s3c_gpio_cfgpin(GPIO_VIBTONE_PWM, S3C_GPIO_SFN(GPIO_VIBTONE_PWM_STATE));
    s3c_gpio_setpull(GPIO_VIBTONE_PWM, S3C_GPIO_PULL_NONE);	
	
	/*Setting Vibration PWM Duty = 50%*/
	/*Setting Vibrator_Period and Vibrator_On_Period ---> set_vib_period() is called by set_vib_pwm_duty() */	
	vib_pwm_duty = 50;
	set_vib_pwm_duty(vib_pwm_duty); 
	
	ImmVibeSPI_ForceOut_AmpDisable(0);

    return VIBE_S_SUCCESS;
ERR_EXIT:
	return ret;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
//#error Please review the code between the #if and #endif

 //   DbgOut((KERN_DEBUG "ImmVibeSPI_ForceOut_Terminate.\n"));

    /* 
    ** Disable amp.
    ** If multiple actuators are supported, please make sure to call ImmVibeSPI_ForceOut_AmpDisable
    ** for each actuator (provide the actuator index as input argument).
    */
    ImmVibeSPI_ForceOut_AmpDisable(0);

#if 0
    /* Set PWM frequency */
    PWM_CTRL  = 0;                  /* 13Mhz / (0 + 1) = 13MHz */
    PWM_PERIOD = PWM_DUTY_MAX;      /* 13Mhz / (PWM_DUTY_MAX + 1) = 22.4kHz */

    /* Set duty cycle to 50% */
    PWM_DUTY = (PWM_DUTY_MAX+1)>>1; /* Duty cycle range = [0, PWM_DUTY_MAX] */
#endif

    return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM duty cycle, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Set(VibeUInt8 nActuatorIndex, VibeInt8 nForce)
{
	unsigned long pwm_duty = 50;
#if 0
#error Please review the code between the #if and #endif

#if 0
    VibeUInt32 nTmp;
#endif

    if (nForce == 0)
    {
#if 0
		/* Set 50% duty cycle */
        PWM_DUTY = (PWM_DUTY_MAX+1)>>1;
#endif
    }
    else
    {
#if 0
        /* Map force from [-127, 127] to [0, PWM_DUTY_MAX] */
        nTmp = ((nForce + 127) * 9) >> 2;
        
        /* 
        ** The above mapping will output PWM duty cycle in [0, PWM_DUTY_MAX] range.
        ** For PWM_DUTY_MAX == 579, max duty cycle is 98.6%.
        ** As long as we don't hit the 100% mark, we're OK
        */
        PWM_DUTY = nTmp;
#endif
    }
#endif
	
	if (nForce == 0)
	{
		ImmVibeSPI_ForceOut_AmpDisable(0);
	}
	else
	{
		/*
		 * pwm_duty control only in Apollo_OPEN model, if required for all the models remove the if condition part,
		 * pwm_duty is set always MAXIMUM(99%) irrespective of the nForce value in Apollo_ORANGE model
		 */
		/* if(!(apollo_get_remapped_hw_rev_pin() == 1 || apollo_get_remapped_hw_rev_pin() == 5) ) 
		{
			if(nForce < 0)
				nForce = -125;
			else
				nForce = 125;
		}	 */   
			
		/* 
		 * if(nForce > -126 && nForce < 126) condition is for increasing pwm_duty value to MAXIMUM(99%) 
		 * as maximum nForce value issued by immevibed daemon service is abs|125| 
		 */
		if(nForce > -126 && nForce < 126)
			pwm_duty = (99*nForce)/252 + 50;
		else
			pwm_duty = (99*nForce)/255 + 50;
		
		set_vib_pwm_duty(pwm_duty); 			
		ImmVibeSPI_ForceOut_AmpEnable(0);
	}
	//printk("nForce = %d, pwm_duty = %lu , enable = %d \n",nForce,pwm_duty,g_bAmpEnabled);
	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set force output, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex, VibeUInt16 nOutputSignalBitDepth, VibeUInt16 nBufferSizeInBytes, VibeInt8* pForceOutputBuffer)
{
    /* This function is not called for LRA device */
    return VIBE_S_SUCCESS;
}

/*
** Called to set force output frequency parameters
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetFrequency(VibeUInt8 nActuatorIndex, VibeUInt16 nFrequencyParameterID, VibeUInt32 nFrequencyParameterValue)
{
//#error Please review the code between the #if and #endif

#if 0 
    #error  "The OEM must handle different frequency parameters here"
#endif

    return VIBE_S_SUCCESS;
}

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
//#error Please review the code between the #if and #endif

#if 0   /* The following code is provided as a sample. Please modify as required. */
    if ((!szDevName) || (nSize < 1)) return VIBE_E_FAIL;

    DbgOut((KERN_DEBUG "ImmVibeSPI_Device_GetName.\n"));

    strncpy(szDevName, "Generic Linux Device", nSize-1);
    szDevName[nSize - 1] = '\0';    /* make sure the string is NULL terminated */
#endif

    return VIBE_S_SUCCESS;
}
