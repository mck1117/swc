#include "ch.h"
#include "hal.h"

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

int main(void)
{
    halInit();
    chSysInit();

    initStatusLeds();

    canStart(&CAND1, &canConfig500);

    while (true) {
        setLeftStatusLed(true);
        chThdSleepMilliseconds(100);
        setLeftStatusLed(false);
        chThdSleepMilliseconds(100);
        setRightStatusLed(true);
        chThdSleepMilliseconds(100);
        setRightStatusLed(false);
        chThdSleepMilliseconds(100);
    }
}
