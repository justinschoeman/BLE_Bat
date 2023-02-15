#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
 * general config
 */

// main battery poll interval
#define BAT_CFG_POLL_TIME 1000UL
// time to wait before sequential BLE poll writes (helps prevent stampeding herd deadlocks)
#define BAT_CFG_POLL_DELAY 50
// timeout on initial connection to all batteries after reboot
#define BAT_CFG_CONNECT_TIMEOUT (100UL * 1000UL)
// max age on any given battery data before we decide it is irretrievably fubar and try a reboot...
#define BAT_CFG_STATE_TIMEOUT (100UL * 1000UL)

#endif