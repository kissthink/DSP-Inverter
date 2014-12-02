#include "DSP28x_Project.h"
#include "PWM_Header.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//声明本函数需要的外部变量
extern float32 vg,i,vg_rms; //vg,ig为采集量，采样频率10khz，Vg为vg有效值，
extern Uint16 mode; //三个开关
extern float32 Dp,J,I_PI,K;//控制参数，
extern float32 P,Te,W;
extern float32 e;
extern float32 P_set,Q_set,Q;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//声明定义本函数变量
Uint16 flag_SysEnable=0,flag_PWMEnable=0; //各种控制标志位
float32 Ig=0; //ig的有效值
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//测试变量

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//主函数
void main(void)
{
	InitSysCtrl();
	DINT;
    InitPieCtrl();
    IER=0x0000;
    IFR=0x0000;
    InitPieVectTable();
    DELAY_US(50000L); //延时50ms，等待其他模块完成初始化
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC=0; //配置ePWM前关闭TBCLK
    EDIS;
    EALLOW;
	PieVectTable.EPWM1_INT=&epwm1_timer_isr; //ePWM中断
	PieVectTable.TINT0=&cpu_timer0_isr; //tmer0中断
	EDIS;
	IER|=M_INT1; //使能CPU的INT1中断，Timer0使用INT1
	IER|=M_INT3; //使能CPU的INT3中断，ePWM使用INT3
	PieCtrlRegs.PIEIER1.bit.INTx7=1; //TINT0与PIE组1中第七位
	PieCtrlRegs.PIEIER3.bit.INTx1=1;
    //各种模块初始化
	TimerInit(); //定时器初始化
    InitEPwm1();//EPwm1初始化
    InitEPwm2();//EPwm2初始化
    InitEPwm1Gpio();//EPwm1 GPIO初始化
    InitEPwm2Gpio();//EPwm2 GPIO初始化
    //初始化完成
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC=1; //重启TBCLK
    EDIS;
    EPwm1Regs.AQCSFRC.all=PWMS_ALBL;//强制输出低电平
    EPwm2Regs.AQCSFRC.all=PWMS_ALBL;//强制输出低电平
    ADCInit();//ADC初始化
    CalculateInit();//参数初始化
    EINT;
    ERTM;
    while(1)
    {
    	if(flag_PWMEnable==0)
    	{
    		 EPwm1Regs.AQCSFRC.all=PWMS_ALBL;
    		 EPwm2Regs.AQCSFRC.all=PWMS_ALBL;
    	}
    	else
    	{
    		 EPwm1Regs.AQCSFRC.all=PWMS_FRC_DISABLE;
    		 EPwm1Regs.AQCSFRC.all=PWMS_FRC_DISABLE;
    	}
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ePWM1中断，负责运算
interrupt void epwm1_timer_isr(void)
{
	if(flag_SysEnable==1) //可以优化
	{
		switch(mode)
		{
		case 0: //自同步模式
			I_PI=10;
			K=1000000;
			P_set=0;
			Q_set=0;
			Pset_cal();
			Qset_cal();
			break;
		case 1: //Pset Qset模式
			I_PI=10;
			K=1000000;
			P_set=4.5;
			Pset_cal();
			Qset_cal();
			break;
		}
	}
	EPwm1Regs.CMPA.half.CMPA=(e/100.0+0.5)*EPwm1_TIMER_TBPRD;
	EPwm2Regs.CMPA.half.CMPA=(-e/100.0+0.5)*EPwm2_TIMER_TBPRD;
	EPwm1Regs.ETCLR.bit.INT=1;
	PieCtrlRegs.PIEACK.all=PIEACK_GROUP3;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Timer0中断，负责采样
interrupt void cpu_timer0_isr(void) //1khz采样
{
	vg=vg_sample();
	vg_rms=Vg_RMS(vg);
	i=i_sample();
	Ig=I_RMS(i);
	if(vg_rms!=0)
		flag_SysEnable=1;
	PieCtrlRegs.PIEACK.all=PIEACK_GROUP1;
}





