#include "ch.h"
#include "hal.h"

// #include "wing.h"

#include <cstring>

#define LEFT_LED_LINE PAL_LINE(GPIOA, 15)
#define RIGHT_LED_LINE PAL_LINE(GPIOB, 2)

static const CANConfig canConfig500 =
{
    CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
    /*
     For 48MHz http://www.bittiming.can-wiki.info/ gives us Pre-scaler=6, Seq 1=13 and Seq 2=2. Subtract '1' for register values
    */
    CAN_BTR_SJW(0) | CAN_BTR_BRP(5)  | CAN_BTR_TS1(12) | CAN_BTR_TS2(1),
};

static void setLeftStatusLed(bool state)
{
    if (state)
    {
        palSetLine(LEFT_LED_LINE);
    }
    else
    {
        palClearLine(LEFT_LED_LINE);
    }
}

static void setRightStatusLed(bool state)
{
    if (state)
    {
        palSetLine(RIGHT_LED_LINE);
    }
    else
    {
        palClearLine(RIGHT_LED_LINE);
    }
}

static void initStatusLeds()
{
    setLeftStatusLed(false);
    setRightStatusLed(false);

    palSetLineMode(LEFT_LED_LINE, PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(RIGHT_LED_LINE, PAL_MODE_OUTPUT_PUSHPULL);
}

// static constexpr I2CConfig i2cConfig =
// {

// };

// Wing left (I2CD1, i2cConfig, PAL_LINE(GPIOB, 6), PAL_LINE(GPIOB, 7));
// Wing right(I2CD1, i2cConfig, PAL_LINE(GPIOB, 10), PAL_LINE(GPIOB, 11));

int main(void)
{
    halInit();
    chSysInit();

    initStatusLeds();

    canStart(&CAND1, &canConfig500);

    // left.Init();
    // right.Init();

    while (true) {
        setLeftStatusLed(true);
        chThdSleepMilliseconds(100);
        setLeftStatusLed(false);
        chThdSleepMilliseconds(100);
        setRightStatusLed(true);
        chThdSleepMilliseconds(100);
        setRightStatusLed(false);
        chThdSleepMilliseconds(100);

        // auto l = left.ReadButtons();

        // setLeftStatusLed(l & 0x1);
        // setRightStatusLed(l & 0x2);
        //chThdSleepMilliseconds(100);
    }
}

typedef enum  {
	Reset = 1,
	NMI = 2,
	HardFault = 3,
	MemManage = 4,
	BusFault = 5,
	UsageFault = 6,
} FaultType;

#define bkpt() __asm volatile("BKPT #0\n")

extern "C" void HardFault_Handler_C(void* sp) {
	//Copy to local variables (not pointers) to allow GDB "i loc" to directly show the info
	//Get thread context. Contains main registers including PC and LR
	struct port_extctx ctx;
	memcpy(&ctx, sp, sizeof(struct port_extctx));

	//Interrupt status register: Which interrupt have we encountered, e.g. HardFault?
	FaultType faultType = (FaultType)__get_IPSR();
	(void)faultType;
#if (__CORTEX_M > 0)
	//For HardFault/BusFault this is the address that was accessed causing the error
	uint32_t faultAddress = SCB->BFAR;

	//Flags about hardfault / busfault
	//See http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Cihdjcfc.html for reference
	bool isFaultPrecise = ((SCB->CFSR >> SCB_CFSR_BUSFAULTSR_Pos) & (1 << 1) ? true : false);
	bool isFaultImprecise = ((SCB->CFSR >> SCB_CFSR_BUSFAULTSR_Pos) & (1 << 2) ? true : false);
	bool isFaultOnUnstacking = ((SCB->CFSR >> SCB_CFSR_BUSFAULTSR_Pos) & (1 << 3) ? true : false);
	bool isFaultOnStacking = ((SCB->CFSR >> SCB_CFSR_BUSFAULTSR_Pos) & (1 << 4) ? true : false);
	bool isFaultAddressValid = ((SCB->CFSR >> SCB_CFSR_BUSFAULTSR_Pos) & (1 << 7) ? true : false);
	(void)faultAddress;
	(void)isFaultPrecise;
	(void)isFaultImprecise;
	(void)isFaultOnUnstacking;
	(void)isFaultOnStacking;
	(void)isFaultAddressValid;
#endif

	//Cause debugger to stop. Ignored if no debugger is attached
	bkpt();
	NVIC_SystemReset();
}

extern "C" void UsageFault_Handler_C(void* sp) {
	//Copy to local variables (not pointers) to allow GDB "i loc" to directly show the info
	//Get thread context. Contains main registers including PC and LR
	struct port_extctx ctx;
	memcpy(&ctx, sp, sizeof(struct port_extctx));

	//Interrupt status register: Which interrupt have we encountered, e.g. HardFault?
	FaultType faultType = (FaultType)__get_IPSR();
	(void)faultType;
#if (__CORTEX_M > 0)
	//Flags about hardfault / busfault
	//See http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Cihdjcfc.html for reference
	bool isUndefinedInstructionFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 0) ? true : false);
	bool isEPSRUsageFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 1) ? true : false);
	bool isInvalidPCFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 2) ? true : false);
	bool isNoCoprocessorFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 3) ? true : false);
	bool isUnalignedAccessFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 8) ? true : false);
	bool isDivideByZeroFault = ((SCB->CFSR >> SCB_CFSR_USGFAULTSR_Pos) & (1 << 9) ? true : false);
	(void)isUndefinedInstructionFault;
	(void)isEPSRUsageFault;
	(void)isInvalidPCFault;
	(void)isNoCoprocessorFault;
	(void)isUnalignedAccessFault;
	(void)isDivideByZeroFault;
#endif

	bkpt();
	NVIC_SystemReset();
}

extern "C" void MemManage_Handler_C(void* sp) {
	//Copy to local variables (not pointers) to allow GDB "i loc" to directly show the info
	//Get thread context. Contains main registers including PC and LR
	struct port_extctx ctx;
	memcpy(&ctx, sp, sizeof(struct port_extctx));

	//Interrupt status register: Which interrupt have we encountered, e.g. HardFault?
	FaultType faultType = (FaultType)__get_IPSR();
	(void)faultType;
#if (__CORTEX_M > 0)
	//For HardFault/BusFault this is the address that was accessed causing the error
	uint32_t faultAddress = SCB->MMFAR;
	(void)faultAddress;

	//Flags about hardfault / busfault
	//See http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Cihdjcfc.html for reference
	bool isInstructionAccessViolation = ((SCB->CFSR >> SCB_CFSR_MEMFAULTSR_Pos) & (1 << 0) ? true : false);
	bool isDataAccessViolation = ((SCB->CFSR >> SCB_CFSR_MEMFAULTSR_Pos) & (1 << 1) ? true : false);
	bool isExceptionUnstackingFault = ((SCB->CFSR >> SCB_CFSR_MEMFAULTSR_Pos) & (1 << 3) ? true : false);
	bool isExceptionStackingFault = ((SCB->CFSR >> SCB_CFSR_MEMFAULTSR_Pos) & (1 << 4) ? true : false);
	bool isFaultAddressValid = ((SCB->CFSR >> SCB_CFSR_MEMFAULTSR_Pos) & (1 << 7) ? true : false);
	(void)isInstructionAccessViolation;
	(void)isDataAccessViolation;
	(void)isExceptionUnstackingFault;
	(void)isExceptionStackingFault;
	(void)isFaultAddressValid;
#endif

	bkpt();
	NVIC_SystemReset();
}
