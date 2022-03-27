#ifndef _GPIO_DEF_H
#define _GPIO_DEF_H

#define PIN(GPIOx, IOx) ((GPIOx - 1) * 32 + IOx)

/****************************** input *******************************/
// CN9B
#define PIN_CN9_7 PIN(1, 10)
#define PIN_CN9_8 PIN(1, 11)
#define PIN_CN9_9 PIN(1, 12)
#define PIN_CN9_10 PIN(1, 23)

#define PIN_MAIN_FAILURE PIN_CN9_7
#define PIN_BATTERY_LOW PIN_CN9_8
#define PIN_BATTERY_OPEN PIN_CN9_9

// CN7
#define PIN_CN7_1 PIN(1, 13)
#define PIN_CN7_2 PIN(1, 14)
#define PIN_CN7_3 PIN(1, 15)
#define PIN_CN7_4 PIN(4, 23)
#define PIN_CN7_7 PIN(1, 22)
#define PIN_CN7_8 PIN(4, 20)
#define PIN_CN7_9 PIN(4, 24)
#define PIN_CN7_10 PIN(4, 19)

#define PIN_G1_AUTO PIN_CN7_3
#define PIN_G1_MSG1 PIN_CN7_2
#define PIN_G1_MSG2 PIN_CN7_1

#define PIN_MSG3 PIN_CN7_7
#define PIN_MSG4 PIN_CN7_8
#define PIN_MSG5 PIN_CN7_9
#define PIN_MSG6 PIN_CN7_10

// spi gpio 504-511 on CN6
#define PIN_IN1 504
#define PIN_IN2 505
#define PIN_IN3 506
#define PIN_IN4 507
#define PIN_IN5 508
#define PIN_IN6 509
#define PIN_IN7 510
#define PIN_IN8 511

/****************************** output *******************************/
// command control power
#define PIN_CN9_2 PIN(4, 16)
#define PIN_CN9_4 PIN(1, 27)

#define PIN_MOSFET1_CTRL PIN_CN9_4
#define PIN_MOSFET2_CTRL PIN_CN9_2

// heartbeat & status LED
#define PIN_HB_LED PIN(1, 5)
#define PIN_ST_LED PIN(1, 9)

#define PIN_RELAY_CTRL PIN(4, 14)

#define PIN_WDT PIN(1, 18)

#endif //_GPIO_DEF_H
