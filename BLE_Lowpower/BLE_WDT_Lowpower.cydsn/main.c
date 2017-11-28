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

#define OFF 1
#define ON  0

#if 0
#define LED_RED
#define LED_GREEN
#define LED_BLUE

#define LED_TOGGLE_DELAY_MS 200	
	
void LED_Write(uint8 LED_Color, uint8 LED_NewDataReg);
void LED_Toggle(uint8 LED_Color, uint8 numToggles)
#endif 

#define WDT_COUNT0_MATCH    (0x4000u)

CYBLE_API_RESULT_T apiResult;

CY_ISR(WDT_Handly);
void AppCallBack(uint32 event, void* eventParam);
void ManageSystemPower();


int main(void)
{
	
	LED_Status_Write(0);
	CyDelay(200);
	LED_Status_Write(1);
	CyDelay(200);
	
	isr_WDT_StartEx(WDT_Handly);
	
	CyGlobalIntEnable;
	
	//CySysClkEcoStop();
	//CySysClkImoStop();
	//CySysClkWcoStop();
    
	
	CySysWdtWriteMode(CY_SYS_WDT_COUNTER0,CY_SYS_WDT_MODE_INT);
	CySysWdtWriteMatch(CY_SYS_WDT_COUNTER0,WDT_COUNT0_MATCH);
	CySysWdtWriteClearOnMatch(CY_SYS_WDT_COUNTER0, 1u);
	
	CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);
	#if 0
    //关闭看门狗之前要上锁；
	CySysWdtLock();
	CySysWdtDisable(CY_SYS_WDT_COUNTER1_MASK);
	CySysWdtUnlock();
    #endif
	//配置协议栈，开启蓝牙；
	apiResult = CyBle_Start(AppCallBack);
      while (CyBle_GetState() == CYBLE_STATE_INITIALIZING)
    {
        CyBle_ProcessEvents(); 
    }
    //配置Deppsleep；
	CySysClkIloStop();
	CySysClkWriteEcoDiv(CY_SYS_CLK_ECO_DIV8);
	
	
	for(;;)
    {	
     CyBle_ProcessEvents();   
     CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
	 ManageSystemPower();
	
	}
}
CY_ISR(WDT_Handly)
{
	CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
    isr_WDT_ClearPending();
	//
	LED_Status_Write(~(LED_Status_Read()));

   }


void AppCallBack(uint32 event, void* eventParam)
{
    uint32 register_value;
    CYBLE_BLESS_CLK_CFG_PARAMS_T clockConfig;
    
    (void)eventParam;
    
    switch(event)
    {
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
        
		case CYBLE_EVT_STACK_ON:
            apiResult = CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;     
        default:
            break;
    }
}


void ManageSystemPower()
{
  
    CYBLE_BLESS_STATE_T blePower;
    uint8 interruptStatus ;
    interruptStatus  = CyEnterCriticalSection();//关闭全局中断//
    blePower = CyBle_GetBleSsState();//获取BLE的休眠状态
    if(blePower == CYBLE_BLESS_STATE_DEEPSLEEP|| blePower == CYBLE_BLESS_STATE_ECO_ON)
    {
        CySysPmDeepSleep();//如果BLE处于深度睡眠模式，CPU进入深度睡眠模式
    }
    else if((blePower != CYBLE_BLESS_STATE_EVENT_CLOSE))
    {
            CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_ECO); 
            CySysClkImoStop();            
            CySysPmSleep();           
            CySysClkImoStart();
            CySysClkWriteHfclkDirect(CY_SYS_CLK_HFCLK_IMO);
    }
    
    CyExitCriticalSection(interruptStatus );

}


#if 0
void LED_Toggle(uint8 LED_Color, uint8 numToggles)
{
    while(numToggles > 0)
    {
      
        LED_Write(LED_Color, LED_ON);
        CyDelay(LED_TOGGLE_DELAY_MS);
        LED_Write(LED_Color, LED_OFF);
        CyDelay(LED_TOGGLE_DELAY_MS);
        numToggles--;
    }
}

void LED_Write(uint8 LED_Color, uint8 LED_NewDataReg)
{
    switch(LED_Color)
    {
    case LED_RED:
        Pin_RedLED_Write(LED_NewDataReg);
        break;
    case LED_GREEN:
        Pin_GreenLED_Write(LED_NewDataReg);
        break;
    case LED_BLUE:
        Pin_BlueLED_Write(LED_NewDataReg);
        break;
    default:
        break;
    }
}
#endif 