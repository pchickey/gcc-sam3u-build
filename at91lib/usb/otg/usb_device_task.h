/**
 * @file usb_device_task.h,v
 *
 * Copyright (c) 2004 Atmel.
 *
 * Please read file license.txt for copyright notice.
 *
 * @brief This file contains the function declarations for USB device task
 *
 * @version 1.2 at90usb128-otg-dual_role-toggle-1_0_0 $Id: usb_device_task.h,v 1.2 2007/02/13 10:17:53 arobert Exp $
 *
 * @todo
 * @bug
 */

#ifndef _USB_DEVICE_TASK_H_
#define _USB_DEVICE_TASK_H_

//! @defgroup usb_device_task USB device task module
//! @{

//_____ I N C L U D E S ____________________________________________________


//_____ M A C R O S ________________________________________________________


//_____ D E C L A R A T I O N S ____________________________________________

//!
//! @brief Definitions of B-Device states
//!
#define B_IDLE              0
#define B_SRP_INIT          1
#define B_PERIPHERAL        2
#define B_WAIT_ACON         3
#define B_HOST              4
#define B_END_HNP_SUSPEND   5

//!
//! @brief OTG B-Device SRP protocole specific states or events
//!
extern  U8  otg_device_sessions;
extern  U8  otg_b_device_state;
extern U8  sof_seen_in_session;

  //!  Is the current session a result of a SRP ?
#define Start_session_with_srp()          (otg_device_sessions |= 0x01)
#define End_session_with_srp()            (otg_device_sessions &= ~0x01)
#define Is_session_started_with_srp()     (((otg_device_sessions&0x01) != 0) ? TRUE : FALSE)

  //! Has a SRP been sent, and waiting for an answer
#define Srp_sent_and_waiting_answer()     (otg_device_sessions |= 0x02)
#define Ack_srp_sent_and_answer()         (otg_device_sessions &= ~0x02)
#define Is_srp_sent_and_waiting_answer()  (((otg_device_sessions&0x02) != 0) ? TRUE : FALSE)

  //! Is the Tb_Srp counter enabled ? Cleared by timer if Tb_Srp_Fail elapsed
//! Tb_Srp_Fail must be between 5 and 6 sec. With an interrupt routine executed each 2ms, its value becomes 2500 (used:5.2sec)
#define TB_SRP_FAIL_MIN       0x0A28
extern  U16  otg_tb_srp_cpt;
#define Init_tb_srp_counter()             (otg_tb_srp_cpt = 0)
#define Is_tb_srp_counter_overflow()      ((otg_tb_srp_cpt > TB_SRP_FAIL_MIN) ? TRUE : FALSE)


//!
//! @brief This function initializes the USB device controller.
//!
//! This function enables the USB controller and init the USB interrupts.
//! The aim is to allow the USB connection detection in order to send
//! the appropriate USB event to the operating mode manager.
//!
//!
//! @param none
//!
//! @return none
//!
void usb_device_task_init     (void);

//!
//! @brief This function initializes the USB device controller
//!
//! This function enables the USB controller and init the USB interrupts.
//! The aim is to allow the USB connection detection in order to send
//! the appropriate USB event to the operating mode manager.
//! Start device function is executed once VBUS connection has been detected
//! either by the VBUS change interrupt either by the VBUS high level
//!
//! @param none
//!
//! @return none
//!
void usb_start_device  (void);

//! @brief Entry point of the USB device mamagement
//!
//! This function is the entry point of the USB management. Each USB
//! event is checked here in order to launch the appropriate action.
//! If a Setup request occurs on the Default Control Endpoint,
//! the usb_process_request() function is call in the usb_standard_request.c file
//!
//! @param none
//!
//! @return none
void usb_device_task   (void);

//! @}

#endif /* _USB_DEVICE_TASK_H_ */

