#include "ch.h"
#include "hal.h"

#include "wing.h"

#include <cstring>

#define LEFT_LED_LINE PAL_LINE(GPIOA, 15)
#define RIGHT_LED_LINE PAL_LINE(GPIOB, 2)

static constexpr uint32_t txCanId = 0x741;
static constexpr uint32_t rxCanId = 0x742;

// static const CANConfig canConfig100 =
// {
//     CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
//     /*
//      For 48MHz http://www.bittiming.can-wiki.info/ gives us Pre-scaler=30, Seq 1=13 and Seq 2=2. Subtract '1' for register values
//     */
//     CAN_BTR_SJW(1) | CAN_BTR_BRP(29) | CAN_BTR_TS1(12) | CAN_BTR_TS2(1),
// };

static constexpr CANConfig canConfig1000 =
{
    CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
    /*
     For 48MHz http://www.bittiming.can-wiki.info/ gives us Pre-scaler=30, Seq 1=13 and Seq 2=2. Subtract '1' for register values
    */
    CAN_BTR_SJW(1) | CAN_BTR_BRP(2) | CAN_BTR_TS1(12) | CAN_BTR_TS2(1),
};

static void initCan()
{
    palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(4));
    palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(4));
    canStart(&CAND1, &canConfig1000);
}

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

static Wing left (PAL_LINE(GPIOB, 6), PAL_LINE(GPIOB, 7));
static Wing right(PAL_LINE(GPIOB, 10), PAL_LINE(GPIOB, 11));

static_assert(STM32_SYSCLK == 48e6);

static const uint16_t startupAnimation[] =
{
    0x0001,
    0x0002,
    0x0004,
    0x0008,
    0x0010,
    0x1000,
    0x0800,
    0x0400,
    0x0200,
    0x0100,
    0x0200,
    0x0400,
    0x0800,
    0x1000,
    0x0010,
    0x0008,
    0x0004,
    0x0002,
    0x0001,

    // Turn all on for a moment
    0x1F1F,
    0x1F1F,
    0x1F1F,
    0x1F1F,
    0x1F1F,
};

static uint8_t brightness = 0;
static uint8_t brightness2 = 3;
static const uint8_t maxBrightness = 10;
static uint8_t brightCounter = 0;

static void driveLeds(uint8_t ledsLeft, uint8_t ledsRight)
{
    brightCounter++;
    if (brightCounter == maxBrightness) brightCounter = 0;

    uint8_t pressedMask = brightness2 > brightCounter ? 0x1F : 0;

    uint8_t l = brightness > brightCounter ? 0x1F : 0;
    uint8_t r = l;

    l |= (ledsLeft & pressedMask);
    r |= (ledsRight & pressedMask);

    left.WriteLeds(l);
    right.WriteLeds(r);
}

static const CANFilter canFilter =
{
    .filter = 0,
    .mode = 1,
    .scale = 1,
    .assignment = 0,
    .register1 = 0xE8400000,
    .register2 = 0
};

int main(void)
{
    halInit();
    chSysInit();

    initStatusLeds();

    canSTM32SetFilters(&CAND1, 1, 1, &canFilter);

    initCan();

    left.Init();
    right.Init();

    for (size_t i = 0; i < (sizeof(startupAnimation) / sizeof(startupAnimation[0])); i++)
    {
        uint16_t data = startupAnimation[i];
        uint8_t l = data & 0xFF;
        uint8_t r = data >> 8;

        left.WriteLeds(l);
        right.WriteLeds(r);

        chThdSleepMilliseconds(80);
    }

    uint8_t canCounter = 0;

    uint8_t ledsLeft = 0;
    uint8_t ledsRight = 0;

    while (true)
    {
        setLeftStatusLed(left.CheckAliveAndReinit());
        setRightStatusLed(right.CheckAliveAndReinit());

        auto buttonsLeft = left.ReadButtons();
        auto knobLeft = left.ReadKnob();
        auto buttonsRight = right.ReadButtons();
        auto knobRight = right.ReadKnob();

        {
            CANRxFrame rxFrame;
            msg_t res = canReceiveTimeout(&CAND1, 0, &rxFrame, TIME_IMMEDIATE);
            if (res == MSG_OK && rxFrame.SID == rxCanId)
            {
                ledsLeft = rxFrame.data8[0];
                ledsRight = rxFrame.data8[1];
            }
        }

        driveLeds(ledsLeft, ledsRight);

        if (canCounter == 0)
        {
            canCounter = 20;

            CANTxFrame frame;
            frame.SID = txCanId;
            frame.IDE = 0;
            frame.RTR = 0;

            frame.data8[0] = buttonsLeft;
            frame.data8[1] = buttonsRight;
            frame.data8[2] = knobLeft;
            frame.data8[3] = knobRight;
            frame.DLC = 4;

            canTransmitTimeout(&CAND1, 0, &frame, TIME_IMMEDIATE);
        }

        canCounter--;
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
