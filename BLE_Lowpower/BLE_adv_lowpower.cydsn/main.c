/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

#define ON              0u
#define OFF             1u

#define DEEPSLEEP_ONLY             0u
#define ADV_RAND_DISABLE_MASK      0x100
#define CAPACITOR_TRIM_VALUE       0x00004355

#define ManageBlePower()            CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP)
/******************************************************************************
Global variables declaration
*******************************************************************************/
CYBLE_API_RESULT_T apiResult;
typedef enum PowerMode_t {
    WAKEUP_SLEEP,
    WAKEUP_DEEPSLEEP,
    ACTIVE,
    SLEEP,
    DEEPSLEEP
} PowerMode_t;

PowerMode_t applicationPower;

void AppCallBack(uint32 event, void* eventParam)
{
    uint32 register_value;
    CYBLE_BLESS_CLK_CFG_PARAMS_T clockConfig;
    
    (void)eventParam;
    
    switch(event)
    {
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
        
		case CYBLE_EVT_STACK_ON:

            /* load capacitors on the ECO should be tuned and the tuned value
            ** must be set in the CY_SYS_XTAL_BLERD_BB_XO_CAPTRIM_REG  */
            CY_SYS_XTAL_BLERD_BB_XO_CAPTRIM_REG = CAPACITOR_TRIM_VALUE;    
        
            /* Get the configured clock parameters for BLE sub-system */
            CyBle_GetBleClockCfgParam(&clockConfig);    
            
            /* Update the sleep clock inaccuracy PPM based on WCO crystal used */
            /* If you see frequent link disconnection, tune your WCO or update the sleep clock accuracy here */
            clockConfig.bleLlSca =   CYBLE_LL_SCA_000_TO_020_PPM;
            
            /* set the clock configuration parameter of BLE sub-system with updated values*/
            CyBle_SetBleClockCfgParam(&clockConfig);
            
            /* Disable randomization of advertisment interval in CYREG_BLE_BLELL_ADV_CONFIG register 
            ** This is for current measurement ONLY to ensure the BLE component uses fixed advertisement **
            ** Interval. This code should not be used in production builds */
    
            register_value = CY_GET_XTND_REG32(CYREG_BLE_BLELL_ADV_CONFIG);
            register_value |= ADV_RAND_DISABLE_MASK;
            CY_SET_XTND_REG32(CYREG_BLE_BLELL_ADV_CONFIG, register_value);
            
            /* Put the device into discoverable mode so that a central device can connect to it */
            apiResult = CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;
            
        default:
            break;
    }
}

__inline void ManageSystemPower()
{
    /* Variable declarations */
    CYBLE_BLESS_STATE_T blePower;
    uint8 interruptStatus ;
    
   /* Disable global interrupts to avoid any other tasks from interrupting this section of code*/
    interruptStatus  = CyEnterCriticalSection();
    
    /* Get current state of BLE sub system to check if it has successfully entered deep sleep state */
    blePower = CyBle_GetBleSsState();
    
    /* System can enter DeepSleep only when BLESS and rest of the application are in DeepSleep or equivalent
     * power modes */
    if((blePower == CYBLE_BLESS_STATE_DEEPSLEEP || blePower == CYBLE_BLESS_STATE_ECO_ON) && 
        applicationPower == DEEPSLEEP)
    {
	    LED_Status_Write(OFF);
		CyDelay(200);
        applicationPower = WAKEUP_DEEPSLEEP;
        CySysPmDeepSleep();
    }
    else if((blePower != CYBLE_BLESS_STATE_EVENT_CLOSE))
    {
        if(applicationPower == DEEPSLEEP)
        {
            applicationPower = WAKEUP_DEEPSLEEP;
            
            /* change HF clock source from IMO to ECO, as IMO is not required and can be stopped to save power */
            CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_ECO); 
            /* stop IMO for reducing power consumption */
            CySysClkImoStop();            
            /* put the CPU to sleep */
            CySysPmSleep();           
            /* starts execution after waking up, start IMO */
            CySysClkImoStart();
            /* change HF clock source back to IMO */
            CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_IMO);
        }
        else if(applicationPower == SLEEP )
        {
            /* If the application requires IMO for its operation, we should not switch the HFCLK source */
            applicationPower = WAKEUP_SLEEP;        
            CySysPmSleep();
 
        }
    }
    
    /* Enable interrupts */
    CyExitCriticalSection(interruptStatus );

}

__inline void ManageApplicationPower()
{
    switch(applicationPower)
    {
        case ACTIVE: // dont need to do anything
        break;
        
        case WAKEUP_SLEEP: // do whatever wakeup needs to be done - probably never do anything     
            applicationPower = ACTIVE;
        break;
        
        case WAKEUP_DEEPSLEEP: // do whatever wakeup needs to be done.       
            applicationPower = ACTIVE;
        break;
        
        case SLEEP: 
        /** Add code to place the application components to sleep here  */

        break;
        
        case DEEPSLEEP:
        /** Add code to place the application components to deep sleep here  */
        
        break;
    }
}

__inline void RunApplication()
{
    /**  Place your application code here **/
    
    /* Once application is done with all processing, enter low-power mode */
    if(0) /* if you are ready to sleep then set it up to go to sleep */
    {
        applicationPower = SLEEP;
    }
    
    if(1) /* if you are done with everything, then deepsleep */
    {
        applicationPower = DEEPSLEEP;
    }
}
void RunBle()
{
    CyBle_ProcessEvents();
}


int main(void)
{
	LED_Status_Write(ON);
	CyDelay(200);
    LED_Status_Write(OFF);
	CyDelay(200);
	
    CyGlobalIntEnable; /* Enable global interrupts. */
    CySysClkIloStop();    /* Internal low power oscillator is stopped as it is not used in this project */   
    
	CySysClkWriteEcoDiv(CY_SYS_CLK_ECO_DIV8);/* Set the divider for ECO, ECO will be used as source when IMO is switched off to save power */

    apiResult = CyBle_Start(AppCallBack);  /* Start CYBLE component and register the generic event handler */
       
    /* Wait for BLE Component to Initialize */
    while (CyBle_GetState() == CYBLE_STATE_INITIALIZING)
    {
        CyBle_ProcessEvents(); 
    }
   
    applicationPower = ACTIVE;
    
    /*If DEEPSLEEP_ONLY is enabled, then the device stays in deep sleep all the time  
    * This is a good way to check if there are any leakage currents in the device. */
    #if DEEPSLEEP_ONLY
    CySysClkEcoStop();    
    CySysPmDeepSleep();
    while(1);
    #endif    

    

    for(;;)
    {
	    RunBle();
        ManageBlePower(); 
        if(applicationPower == ACTIVE)
        {
            RunApplication();
        }
        ManageApplicationPower();   
        ManageSystemPower();  
    }
}

/* [] END OF FILE */
