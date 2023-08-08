#include "ch.h"
#include "hal.h"

static const CANConfig canConfig500 =
{
    CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
    /*
     For 48MHz http://www.bittiming.can-wiki.info/ gives us Pre-scaler=6, Seq 1=13 and Seq 2=2. Subtract '1' for register values
    */
    CAN_BTR_SJW(0) | CAN_BTR_BRP(5)  | CAN_BTR_TS1(12) | CAN_BTR_TS2(1),
};

int main(void) {
    halInit();
    chSysInit();

    canStart(&CAND1, &canConfig500);

    while (true) {
        chThdSleepMilliseconds(1);
    }
}
