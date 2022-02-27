#pragma once

/*
* Address areas of local IDs
*/
#define MM_ACC 			0x3000	/**< Address range for Märklin Motorola accessorys */
#define DCC_ACC 		0x3800	/**< Address range for DCC accessorys */
#define MM_TRACK 		0x0000	/**< Address range for Märklin Motorola locos */
#define DCC_TRACK 		0xC000	/**< Address range for DCC locos */

/*
* Märklin CAN commands
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
#define CMD_BOOTLOADER	0x1B	/**< Märklin bootloader command */
#define CMD_MCAN_BOOT	0x40	/**< MCAN bootloader command */