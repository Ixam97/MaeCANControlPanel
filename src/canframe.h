/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <ixam97@ixam97> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 *
 * ----------------------------------------------------------------------------
 * https://github.com/Ixam97
 * ----------------------------------------------------------------------------
 * M‰CAN Control Panel
 * canframe.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-03.1]
 */

#pragma once
#include <stdint.h>

/*
* Address areas of local IDs
*/
#define MM_ACC 			0x3000	/**< Address range for M‰rklin Motorola accessorys */
#define DCC_ACC 		0x3800	/**< Address range for DCC accessorys */
#define MM_TRACK 		0x0000	/**< Address range for M‰rklin Motorola locos */
#define DCC_TRACK 		0xC000	/**< Address range for DCC locos */

/*
* M‰rklin CAN commands
*/
#define SYS_CMD			0x00 	/**< System commands */
#define SYS_STOP 	0x00 	/**< System subcommand "STOP" */
#define SYS_GO		0x01	/**< System subcommand "GO" */
#define SYS_HALT	0x02	/**< System subcommand "Emergency stop" */
#define SYS_STAT	0x0b	/**< System subcommand to send changed config values or meassurement values */
#define CMD_SWITCH_ACC 	0x0b	/**< Command to switch accessorys */
#define CMD_S88_EVENT	0x11	/**< Command to send S88 events */
#define CMD_PING 		0x18	/**< Command to send or request pings */
#define CMD_CONFIG		0x1d	/**< Command to send or request config or meassurement deffinitions */
#define CMD_BOOTLOADER	0x1B	/**< M‰rklin bootloader command */
#define CMD_MCAN_BOOT	0x40	/**< MCAN bootloader command */

struct canFrame {
	canFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash = 0x300);
	canFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8]);
	canFrame();
	uint32_t id = 0;
	uint8_t cmd = 0;
	uint8_t resp = 0;
	uint16_t can_hash = 0;
	uint8_t dlc = 0;
	uint8_t data[8] = { 0,0,0,0,0,0,0,0 };
};

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash = 0x300);
canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint16_t _hash = 0x300);
