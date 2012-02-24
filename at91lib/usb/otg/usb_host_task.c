/**
 * @file usb_host_task.c,v
 *
 * Copyright (c) 2004 Atmel.
 *
 * Please read file license.txt for copyright notice.
 *
 * @brief This file manages the USB Host controller, the host enumeration process
 *        and suspend resume host requests.
 *
 * This task dos not belongs to the scheduler tasks but is called directly from the general usb_task
 *
 * @version 1.21 at90usb128-otg-dual_role-toggle-1_0_0 $Id: usb_host_task.c,v 1.21 2007/02/13 12:10:34 arobert Exp $
 *
 *
 * @todo
 * @bug
 */

//_____  I N C L U D E S ___________________________________________________

#include "config.h"
#include "conf_usb.h"
#include "usb/otg/usb_task.h"
#include "usb_host_task.h"
#include "usb/otg/usb_drv.h"
//#include "lib_mcu/pll/pll_drv.h"
#include "usb/otg/usb_host_enum.h"
#include <utility/trace.h>

#define __interrupt
#define __enable_interrupt

//jcb #define WAIT_100MS  100
// Wait 100 x 125 us = 12,5 ms before USB reset
//#define WAIT_100MS  800
#define WAIT_100MS  100

extern void DelayMS(unsigned int ms);
extern unsigned int gSystemTick;
#if (USB_OTG_FEATURE == ENABLED)
  #include "usb/otg/usb_device_task.h"
  #if (TARGET_BOARD==SPIDER)
//    #include "lib_board/lcd/lcd_drv.h"
  #endif

  #ifndef OTG_ENABLE_HNP_AFTER_SRP
    #warning  OTG_ENABLE_HNP_AFTER_SRP should be defined somewhere in config files (conf_usb.h)
  #endif

  #ifndef OTG_B_DEVICE_AUTORUN_HNP_IF_REQUIRED
    #warning  OTG_B_DEVICE_AUTORUN_HNP_IF_REQUIRED should be defined somewhere in config files (conf_usb.h)
  #endif

  #ifndef OTG_VBUS_AUTO_WHEN_A_PLUG
    #warning  OTG_VBUS_AUTO_WHEN_A_PLUG should be defined somewhere in config files (conf_usb.h)
  #endif

  #ifndef OTG_ADEV_SRP_REACTION
    #warning  OTG_ADEV_SRP_REACTION should be defined somewhere in config files (conf_usb.h)
  #endif

  #if (HOST_STRICT_VID_PID_TABLE != ENABLE)
    #warning  HOST_STRICT_VID_PID_TABLE must be defined to ENABLED to comply with Targeted Peripheral List requirements
  #endif
#endif


#if (USB_HOST_FEATURE == DISABLED)
   #warning trying to compile a file used with the USB HOST without USB_HOST_FEATURE enabled
#endif

#if (USB_HOST_FEATURE == ENABLED)

#ifndef DEVICE_ADDRESS
   #error DEVICE_ADDRESS should be defined somewhere in config files (conf_usb.h)
#endif

#ifndef SIZEOF_DATA_STAGE
   #error SIZEOF_DATA_STAGE should be defined in conf_usb.h
#endif

#ifndef HOST_CONTINUOUS_SOF_INTERRUPT
   #error HOST_CONTINUOUS_SOF_INTERRUPT should be defined as ENABLE or DISABLE in conf_usb.h
#endif

#ifndef USB_HOST_PIPE_INTERRUPT_TRANSFER
   #error USB_HOST_PIPE_INTERRUPT_TRANSFER should be defined as ENABLE or DISABLE in conf_usb.h
#endif

#ifndef Usb_id_transition_action
   #define Usb_id_transition_action()
#endif
#ifndef  Host_device_disconnection_action
   #define Host_device_disconnection_action()
#endif
#ifndef  Host_device_connection_action
   #define Host_device_connection_action()
#endif
#ifndef  Host_sof_action
   #define Host_sof_action()
#endif
#ifndef  Host_suspend_action
   #define Host_suspend_action()
#endif
#ifndef  Host_hwup_action
   #define Host_hwup_action()
#endif
#ifndef  Host_device_not_supported_action
   #define Host_device_not_supported_action()
#endif
#ifndef  Host_device_class_not_supported_action
   #define Host_device_class_not_supported_action()
#endif
#ifndef  Host_device_supported_action
   #define Host_device_supported_action()
#endif
#ifndef  Host_device_error_action
   #define Host_device_error_action()
#endif

#if (OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED)
  extern U8 id_changed_to_host_event;
#endif

//_____ M A C R O S ________________________________________________________

#ifndef LOG_STR_CODE
#define LOG_STR_CODE(str)
#else
U8 code log_device_connected[]="Device Connection";
U8 code log_device_enumerated[]="Device Enumerated";
U8 code log_device_unsupported[]="Unsupported Device";
U8 code log_going_to_suspend[]="Usb suspend";
U8 code log_usb_resumed[]="Usb resumed";
#endif

//_____ D E F I N I T I O N S ______________________________________________

//_____ D E C L A R A T I O N S ____________________________________________

#if (USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE)
   volatile S_pipe_int   it_pipe_str[MAX_EP_NB];
   volatile U8 pipe_nb_save;
   U8 g_sav_int_sof_enable;
#endif

   
#if (USB_OTG_FEATURE == ENABLED)
  //! Public : (U8) otg_device_connected;
  //! Min. delay after an SRP received, with VBUS delivered ON (waiting for a B-DEV connect)
  U16 otg_ta_srp_wait_connect;
  
  //! Public : (U8) otg_device_connected;
  //! Max. delay for a B-DEVICE to disconnect once the A-HOST has set suspend mode
  U16 otg_ta_aidl_bdis_tmr;
  
  //! Public : (U8) otg_device_connected;
  //! Max. delay for vbus to reach Va_vbus_valid threshold
  U8  otg_ta_vbus_rise;
  
  //! Public : (U8) otg_device_connected;
  //! Max. delay once B-Device attached to respond to the first A-Device requests
  U16 otg_timeout_bdev_respond;
  
  //! Public : (U8) otg_device_connected;
  //! Indicates if the connected peripheral is an OTG Device or not
  U8  otg_device_connected;
  
  //! Public : (U8) otg_a_device_srp;
  //! Stores special events about SRP in A-Device mode
  U8  otg_a_device_srp;

  //! Public : (U8) otg_end_hnp_vbus_delay;
  //! Variable used for timing Vbus discharge to avoid bounces around vbus_valid threshold
  U16 otg_end_hnp_vbus_delay;
#endif


//!
//! Public : U8 device_state
//! Its value represent the current state of the
//! device connected to the usb host controller
//! Value can be:
//! - DEVICE_ATTACHED
//! - DEVICE_POWERED
//! - DEVICE_SUSPENDED
//! - DEVICE_DEFAULT
//! - DEVICE_ADDRESSED
//! - DEVICE_CONFIGURED
//! - DEVICE_ERROR
//! - DEVICE_UNATTACHED
//! - DEVICE_READY
//! - DEVICE_WAIT_RESUME
//! - DEVICE_DISCONNECTED
//! - DEVICE_DISCONNECTED_ACK
//!
//! - and these have been added for OTG :
//! - A_PERIPHERAL
//! - A_INIT_HNP
//! - A_SUSPEND
//! - A_END_HNP_WAIT_VFALL
//!/
U8 device_state;

//! For control requests management over pipe 0
S_usb_setup_data    usb_request;

//!
//! Public : U8 data_stage[SIZEOF_DATA_STAGE];
//! Internal RAM buffer for USB data stage content
//! This buffer is required to setup host enumeration process
//! Its contains the device descriptors received.
//! Depending on the device descriptors lenght, its size can be optimized
//! with the SIZEOF_DATA_STAGE define of conf_usb.h file
//!
//!/
U8 data_stage[SIZEOF_DATA_STAGE];

U8 device_status;
U8 request_resume;

static U16  c;                // As internal host start of frame counter

U8 new_device_connected=0;

/**
 * @brief This function initializes the USB controller in host mode and
 *        the associated variables.
 *
 * This function enables the USB controller for host mode operation.
 *
 * @param none
 *
 * @return none
 *
 */
void usb_host_task_init(void)
{
    unsigned int i;

    TRACE_DEBUG("usb_host_task_init\n\r");

    for( i=0; i<SIZEOF_DATA_STAGE; i++ ) {
        data_stage[i] = 0;
    }

//   Pll_start_auto();
//   Wait_pll_ready();
   Usb_disable();
   Usb_enable();
   Usb_unfreeze_clock();
   Usb_attach();
//jcb   Usb_enable_uvcon_pin();
   Usb_select_host();
   Usb_disable_vbus_hw_control();   // Force Vbus generation without timeout
   Host_enable_device_disconnection_interrupt();
   #if (USB_OTG_FEATURE == ENABLED)
   Usb_enable_id_interrupt();
     #if (OTG_ADEV_SRP_REACTION == VBUS_PULSE)
       Usb_select_vbus_srp_method();
     #else
       Usb_select_data_srp_method();
     #endif
   #endif
   device_state=DEVICE_UNATTACHED;
}

/**
 * @brief Entry point of the USB host management
 *
 * The aim is to manage the device target connection and enumeration
 * depending on the device_state, the function performs the required operations
 * to get the device enumerated and configured
 * Once the device is operationnal, the device_state value is DEVICE_READY
 * This state should be tested by the host task application before performing
 * any applicative requests to the device.
 *
 * @param none
 *
 * @return none
 *
 * \image html host_task.gif
 */
void usb_host_task(void)
{
  U32 dec;
#if (USB_OTG_FEATURE == ENABLED)
  U8  desc_temp;
#endif
  
  //TRACE_DEBUG("host_task ");
  switch (device_state)
  {
     //------------------------------------------------------
     //   DEVICE_UNATTACHED state
     //
     //   - Default init state
     //   - Try to give device power supply
     //
      case DEVICE_UNATTACHED:
         //TRACE_DEBUG("DEVICE_UNATTACHED\n\r");
        for (c=0;c<MAX_EP_NB;c++) {ep_table[c]=0;}// Reset PIPE lookup table with device EP addr
         nb_interface_supported=0;
         Host_clear_device_supported();        // Reset Device status
         Host_clear_configured();
         Host_clear_device_ready();
         Usb_clear_all_event();                // Clear all software events
         new_device_connected=0;
#if (USB_OTG_FEATURE == ENABLED)
         Host_end_session_started_srp();
         Peripheral_is_not_otg_device();
         Usb_host_reject_hnp();
         Usb_disable_role_exchange_interrupt();
         Usb_disable_hnp_error_interrupt();
#endif
         if (Is_usb_id_host())
         {
#if (SOFTWARE_VBUS_CTRL==ENABLE)
           if( Is_usb_bconnection_error_interrupt()||Is_usb_vbus_error_interrupt())
           {
              Usb_ack_bconnection_error_interrupt();
              Usb_ack_vbus_error_interrupt();
              Host_clear_vbus_request();
           }
           #if ((USB_OTG_FEATURE == DISABLED) || (OTG_VBUS_AUTO_WHEN_A_PLUG == ENABLED))
           if (Is_usb_id_host())
           {
             Host_ack_device_connection();
             Usb_enable_manual_vbus();
             Host_vbus_action();
             #if (USB_OTG_FEATURE == ENABLED)
             Init_ta_vbus_rise_counter();
             #endif
           }
           #endif
           #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
           // Handle user requests to turn Vbus ON or OFF
           if (Is_usb_id_host() && Is_user_requested_vbus())
           {
             Ack_user_request_vbus();
             if (Is_usb_vbus_manual_on())
             {
               Usb_disable_vbus();
               Usb_disable_manual_vbus();
               Host_vbus_action();
             }
             else
             {
               Usb_enable_manual_vbus();
               Host_vbus_action();
               Init_ta_vbus_rise_counter();
             }
           }

             #if (OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED)
             if (id_changed_to_host_event == ENABLED)
             {
               id_changed_to_host_event = DISABLED;
               Usb_enable_manual_vbus();
               Init_ta_vbus_rise_counter();
               Host_vbus_action();
             }
             #endif
           #endif

           #if (USB_OTG_FEATURE == ENABLED)
           if (Is_ta_vbus_rise_counter_overflow() && Is_usb_vbus_low() && Is_usb_vbus_manual_on())
           {
             // In the case of an user manual request to turn on Vbus, generate an error message if
             // Vbus not reach Va_vbus_valid before Ta_vbus_rise (rise time verification)
             Usb_disable_vbus();
             Usb_disable_manual_vbus();
             Host_vbus_action();
             //TRACE_DEBUG("Vubs on manual not reach rise\n\r");
             Otg_print_new_event_message(OTGMSG_VBUS_SURCHARGE,OTG_TEMPO_3SEC);
             Otg_print_new_failure_message(OTGMSG_UNSUPPORTED,OTG_TEMPO_4SEC);
           }
           #else
           Usb_disable_vbus_pad();
           Usb_enable_manual_vbus();
           #endif

           if(Is_usb_srp_interrupt())
           {
              Usb_ack_srp_interrupt();

              if (!Is_usb_vbus_manual_on())  // if Vbus was not already delivered, it is really an SRP (OTG B-Device)
              {
                #if (USB_OTG_FEATURE == ENABLED)
                Otg_print_new_event_message(OTGMSG_SRP_RECEIVED,OTG_TEMPO_2SEC);
                Host_session_started_srp();
                Init_ta_srp_counter();
                Srp_received_and_waiting_connect();
                Init_ta_vbus_rise_counter();
                #endif
              }
              device_state=DEVICE_ATTACHED;
              Usb_ack_bconnection_error_interrupt();
              Usb_enable_vbus_pad();
              Usb_enable_vbus();
              Host_vbus_action();
           }
#else
           Usb_enable_vbus();                    // Give at least device power supply!!!
           Host_vbus_action();
           if(Is_usb_vbus_high())
           { 
                device_state=DEVICE_ATTACHED; 
           }     // If VBUS ok goto to device connection expectation
#endif
         }
         #if (USB_OTG_FEATURE == ENABLED)
         else
           if (otg_b_device_state == B_HOST)
           {
             device_state = DEVICE_ATTACHED;
           }
         #endif
      break;

     //------------------------------------------------------
     //   DEVICE_ATTACHED state
     //
     //   - Vbus is on
     //   - Try to detect device connection
     //
      case DEVICE_ATTACHED :
         //TRACE_DEBUG("DEVICE_ATTACHED\n\r");
#if (USB_OTG_FEATURE == ENABLED)
         if (Is_device_connection() || (Is_usb_id_device() && (otg_b_device_state == B_HOST)))
#else
         if (Is_device_connection())     // Device pull-up detected
#endif
         {
            //TRACE_DEBUG("Is_device_connection\n\r");
            Host_ack_device_connection();
            #if (USB_OTG_FEATURE == ENABLED)
            Ack_srp_received_and_connect();   // connection might have been requested by SRP
            #endif
           // Now device is connected, enable disconnection interrupt
            Host_enable_device_disconnection_interrupt();
            Enable_interrupt();
           // Reset device status
            Host_clear_device_supported();
            Host_clear_configured();
            Host_clear_device_ready();
            Host_ack_sof();

// jcb tempo
                // Wait x ms
                TRACE_DEBUG("begin timer\n\r");
                gSystemTick = 0;
//                DelayMS(2257);
                DelayMS(200);
                TRACE_DEBUG("end timer\n\r");
                // Clear bad VBUS error
                Usb_ack_vbus_error_interrupt();
/*
            Host_enable_sof();            // Start Start Of Frame generation
            Host_enable_sof_interrupt();  // SOF will be detected under interrupt
            c = 0;
            while (c<WAIT_100MS)                 // wait 100ms before USB reset
            {
               if (Is_usb_event(EVT_HOST_SOF)) { Usb_ack_event(EVT_HOST_SOF); c++; }  // Count Start Of frame
               if (Is_host_emergency_exit() || Is_usb_bconnection_error_interrupt()) 
               {
                  TRACE_DEBUG("goto error 1\n\r");
                  goto device_attached_error;
               }
               #if (USB_OTG_FEATURE == ENABLED)
               if (Is_usb_device_enabled()) { break; }
               #endif
            }
*/
            Host_disable_device_disconnection_interrupt();

 //jcb      Host_send_reset();  // First USB reset
            Host_send_reset();  // First USB reset
            Host_enable_sof();  // Start Start Of Frame generation
            Host_enable_sof_interrupt();  // SOF will be detected under interrupt

            Usb_ack_event(EVT_HOST_SOF);
            while (Is_host_reset())
            {
              #if (USB_OTG_FEATURE == ENABLED)
              if (Is_usb_device_enabled()) { break; }
              #endif
            } // Active wait of end of reset send
            Host_ack_reset();
            #if (USB_OTG_FEATURE == ENABLED)
            // User can choose the number of consecutive resets sent
            c = 1;
            while (c != OTG_RESET_LENGTH)
            {
              Host_send_reset();
              Usb_ack_event(EVT_HOST_SOF);
              while (Is_host_reset())
              {
                #if (USB_OTG_FEATURE == ENABLED)
                if (Is_usb_device_enabled()) { break; }
                #endif
              }// Active wait of end of reset send
              Host_ack_reset();
              c++;
            }
            #endif

            //Workaround for some bugly devices with powerless pull up
            //usually low speed where data line rise slowly and can be interpretaded as disconnection

            for(c=0;c!=0xFFFF;c++)    // Basic Timeout counter
            {
               if(Is_usb_event(EVT_HOST_SOF))   //If we detect SOF, device is still alive and connected, just clear false disconnect flag
               {
                  if(Is_device_disconnection())
                  {
                      Host_ack_device_connection();
                      Host_ack_device_disconnection();
                      break;
                  }
               }
            }

            Host_enable_device_disconnection_interrupt();
            c = 0;
            while (c<WAIT_100MS)               // wait 100ms after USB reset
            {
               if (Is_usb_event(EVT_HOST_SOF)) { Usb_ack_event(EVT_HOST_SOF); c++; }// Count Start Of frame
               if (Is_host_emergency_exit() || Is_usb_bconnection_error_interrupt()) {
                  TRACE_DEBUG("goto error 2\n\r");
                  goto device_attached_error;
               }
               #if (USB_OTG_FEATURE == ENABLED)
               if (Is_usb_device_enabled()) { break; }
               #endif
            }
            device_state = DEVICE_POWERED;
            c=0;

         }
         #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
         else
         {
             // Handle SRP connection
             if (Is_srp_received_and_waiting_connect())
             {
               // Check if time-out elapsed between SRP and device connection
               if (Is_ta_srp_counter_overflow())
               {
                 Ack_srp_received_and_connect();
                 Clear_all_user_request();
                 device_state = DEVICE_DISCONNECTED;
                 Otg_print_new_failure_message(OTGMSG_DEVICE_NO_RESP,OTG_TEMPO_4SEC);
               }

               if (Is_usb_bconnection_error_interrupt())
               {    // used when a B-Device has sent SRP and connects after the max connection delay accepted by hardware module
                    // vbus is initialized (no drop on output thanks to capacitor)
                  Usb_ack_bconnection_error_interrupt();
                  Usb_disable_vbus();
                  Usb_disable_vbus_pad();
                  Usb_enable_vbus_pad();
                  Usb_enable_vbus_hw_control();
                  Usb_enable_vbus();
               }
             }
             if (Is_ta_vbus_rise_counter_overflow() && Is_usb_vbus_low())
             {
               // In the case of an SRP request to turn on Vbus, generate an error message if
               // Vbus not reach Va_vbus_valid before Ta_vbus_rise (rise time verification)
               Usb_disable_vbus();
               Usb_disable_manual_vbus();
               Host_vbus_action();

               device_state = DEVICE_DISCONNECTED;

             TRACE_DEBUG("Vubs on manual not reach rise 2\n\r");
               Otg_print_new_event_message(OTGMSG_VBUS_SURCHARGE,OTG_TEMPO_3SEC);
               Otg_print_new_failure_message(OTGMSG_UNSUPPORTED,OTG_TEMPO_4SEC);
             }
         }
         #endif

         #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
         // Handle user request to turn Vbus off
         if (Is_usb_id_host() && Is_user_requested_vbus())
         {
           Ack_user_request_vbus();
           Usb_disable_vbus();
           Usb_disable_manual_vbus();
           Host_vbus_action();
           device_state = DEVICE_UNATTACHED;
         }
         #endif

         device_attached_error:
         #if ((USB_OTG_FEATURE == DISABLED) || (OTG_VBUS_AUTO_WHEN_A_PLUG == ENABLED))
        // Device connection error, or vbus pb -> Retry the connection process from the begining
         if( Is_usb_bconnection_error_interrupt()||Is_usb_vbus_error_interrupt()||Is_usb_vbus_low())
         {
           TRACE_DEBUG("HOST device_attached_error\n\r");
           TRACE_DEBUG("Is_usb_bconnection_error_interrupt: 0x%X\n\r", Is_usb_bconnection_error_interrupt());
           TRACE_DEBUG("Is_usb_vbus_error_interrupt: 0x%X\n\r", Is_usb_vbus_error_interrupt());
           TRACE_DEBUG("Is_usb_vbus_low: 0x%X\n\r", Is_usb_vbus_low());
          while(1);  //JCB
           if (Is_usb_id_host())
           {
              Usb_ack_bconnection_error_interrupt();
              Usb_enable_vbus_hw_control();
              device_state=DEVICE_UNATTACHED;
              Usb_disable_vbus();
              Usb_disable_vbus_pad();
              Usb_enable_vbus_pad();
              Usb_ack_vbus_error_interrupt();
              Usb_enable_vbus();
              Usb_disable_vbus_hw_control();
              Host_disable_sof();
              Host_vbus_action();
           }
           else   { device_state = DEVICE_UNATTACHED; }
         }
         #endif

         break;

     //------------------------------------------------------
     //   DEVICE_POWERED state
     //
     //   - Device connection (attach) as been detected,
     //   - Wait 100ms and configure default control pipe
     //
      case DEVICE_POWERED :
         //TRACE_DEBUG("DEVICE_POWERED\n\r");
         //LOG_STR_CODE(log_device_connected);
         Host_device_connection_action();
         if (Is_usb_event(EVT_HOST_SOF))
         {
            Usb_ack_event(EVT_HOST_SOF);
            if (c++ >= WAIT_100MS)                          // Wait 100ms
            {
               device_state = DEVICE_DEFAULT;
               Host_select_pipe(PIPE_CONTROL);
               Host_enable_pipe();
               host_configure_pipe(PIPE_CONTROL, \
                                   TYPE_CONTROL, \
                                   TOKEN_SETUP,  \
                                   EP_CONTROL,   \
                                   SIZE_64,       \
                                   ONE_BANK,     \
                                   0             );
               device_state = DEVICE_DEFAULT;
            }
         }
         break;

     //------------------------------------------------------
     //   DEVICE_DEFAULT state
     //
     //   - Get device descriptor
     //   - Reconfigure Pipe 0 according to Device EP0
     //   - Attribute device address
     //
      case DEVICE_DEFAULT :
         TRACE_DEBUG("DEVICE_DEFAULT\n\r");
        // Get first device descriptor
         #if (USB_OTG_FEATURE == ENABLED)
         Peripheral_is_not_otg_device();    // init status variable
         Init_timeout_bdev_response();      // init B-Device "waiting response" delay (handled by timer interrupt in usb_task.c)
         #endif

         if( CONTROL_GOOD == host_get_device_descriptor_uncomplete())
         {
            c = 0;
            while(c<20)           // wait 20ms before USB reset (special buggly devices...)
            {
               if (Is_usb_event(EVT_HOST_SOF)) { Usb_ack_event(EVT_HOST_SOF); c++; }
               if (Is_host_emergency_exit() || Is_usb_bconnection_error_interrupt())  {break;}
            }
            Host_disable_device_disconnection_interrupt();

            Host_send_reset();          // First USB reset
            Usb_ack_event(EVT_HOST_SOF);
            while (Is_host_reset());    // Active wait of end of reset send
            Host_ack_reset();
            #if (USB_OTG_FEATURE == ENABLED)    // for OTG compliance, reset duration must be > 50ms (delay between reset < 3ms)
            c = 1;
            while (c != OTG_RESET_LENGTH)
            {
              Host_send_reset();
              Usb_ack_event(EVT_HOST_SOF);
              while (Is_host_reset());    // Active wait of end of reset send
              Host_ack_reset();
              c++;
            }
            #endif


            //Workaround for some bugly devices with powerless pull up
            //usually low speed where data line rise slowly and can be interpretaded as disconnection
            for(c=0;c!=0xFFFF;c++)    // Basic Timeout counter
            {
               if(Is_usb_event(EVT_HOST_SOF))   //If we detect SOF, device is still alive and connected, just clear false disconnect flag
               {
                  if(Is_device_disconnection())
                  {
                      Host_ack_device_connection();
                      Host_ack_device_disconnection();
                      break;
                  }
               }
            }
            Host_enable_device_disconnection_interrupt();
            c = 0;
            while(c<200)           // wait 200ms after USB reset
            {
               if (Is_usb_event(EVT_HOST_SOF)) { Usb_ack_event(EVT_HOST_SOF); c++; }
               if (Is_host_emergency_exit() || Is_usb_bconnection_error_interrupt())  {break;}
            }

            Host_select_pipe(PIPE_CONTROL);
            Host_disable_pipe();
            Host_unallocate_memory();
            Host_enable_pipe();
            // Re-Configure the Ctrl Pipe according to the device ctrl EP

            TRACE_DEBUG("Size: 0x%X\n\r", (U16)data_stage[OFFSET_FIELD_MAXPACKETSIZE]);
            TRACE_DEBUG("Size Pipe: 0x%X\n\r", host_determine_pipe_size((U16)data_stage[OFFSET_FIELD_MAXPACKETSIZE]));

            host_configure_pipe(PIPE_CONTROL,                                \
                                TYPE_CONTROL,                                \
                                TOKEN_SETUP,                                 \
                                EP_CONTROL,                                  \
                                host_determine_pipe_size((U16)data_stage[OFFSET_FIELD_MAXPACKETSIZE]),\
                                ONE_BANK,                                    \
                                0                                            );
            // Give an absolute device address
            host_set_address(DEVICE_ADDRESS);
            Host_configure_address(PIPE_CONTROL, DEVICE_ADDRESS);
            device_state = DEVICE_ADDRESSED;
         }
         else
         {
           device_state = DEVICE_ERROR;
         }
         break;

     //------------------------------------------------------
     //   DEVICE_ADDRESSED state
     //
     //   - Check if VID PID is in supported list
     //
      case DEVICE_ADDRESSED :
         TRACE_DEBUG("DEVICE_ADDRESSED\n\r");
         if (CONTROL_GOOD == host_get_device_descriptor())
         {
           // Detect if the device connected belongs to the supported devices table
            if (HOST_TRUE == host_check_VID_PID())
            {
               Host_set_device_supported();
               Host_device_supported_action();
               device_state = DEVICE_CONFIGURED;
               #if (USB_OTG_FEATURE == ENABLED)
                 // In OTG A-HOST state, initiate a HNP if the OTG B-DEVICE has requested this session with a SRP
                 if ((OTG_ENABLE_HNP_AFTER_SRP == ENABLED) && Is_host_session_started_srp() && Is_usb_id_host())
                 {
                    device_state = A_INIT_HNP;
                 }
                 else
                 {
                   if (Is_device_supports_hnp())
                   {
                     if (CONTROL_GOOD != host_set_feature_a_hnp_support())
                     {
                       device_state = A_END_HNP_WAIT_VFALL;   // end session if Device STALLs the request
                     }
                   }
                 }
               #endif
            }
            else
            {
               #if (USB_OTG_FEATURE == ENABLED)
               // In OTG, if the B-DEVICE VID/PID does not match the Target Peripheral List, it is seen as "Unsupported"
               //  - a HNP must be initiated if the device supports it
               //  - an error message must be displayed (and difference must be made between "Std device" and "Hub")
               desc_temp = data_stage[OFFSET_DEV_DESC_CLASS]; // store the device class (for future hub check)
               if (otg_b_device_state != B_HOST)
               {
                 if (Is_host_session_started_srp())
                 {
                   if (CONTROL_GOOD == host_get_configuration_descriptor())
                   {
                     host_check_OTG_features();
                     if (Is_device_supports_hnp())
                     {
                       device_state = A_INIT_HNP;         // unsupported (or test) device will cause a HNP request
                     }
                     else
                     {
                       device_state = A_END_HNP_WAIT_VFALL;
                       if (desc_temp == HUB_CLASS_CODE)
                       {
                         // Display "Hub unsupported" message
                         Otg_print_new_failure_message(OTGMSG_UNSUPPORTED_HUB,OTG_TEMPO_4SEC);
                       }
                       else
                       {
                         // Display "Class unsupported" message
                         TRACE_DEBUG("Class unsupported\n\r");
                         Otg_print_new_failure_message(OTGMSG_UNSUPPORTED,OTG_TEMPO_4SEC);
                       }
                     }
                   }
                   else
                   {
                     device_state = A_END_HNP_WAIT_VFALL;
                   }
                 }
                 else
                 {
                   device_state = A_INIT_HNP;
                 }
               }
               else
               {
                 TRACE_DEBUG("VID/PID does not match the Target Peripheral List\n\r");
                 Otg_print_new_failure_message(OTGMSG_UNSUPPORTED,OTG_TEMPO_4SEC);
                 Set_user_request_disc();   // ask end of session now
                 Set_user_request_suspend();
               }
               #else
                 #if (HOST_STRICT_VID_PID_TABLE==ENABLE)
                    device_state = DEVICE_ERROR;
                    Host_device_not_supported_action();
                 #else
                    device_state = DEVICE_CONFIGURED;
                 #endif
               #endif
            }

         }
         else // Can not get device descriptor
         {  device_state = DEVICE_ERROR; }
         break;

     //------------------------------------------------------
     //   DEVICE_CONFIGURED state
     //
     //   - Configure pipes for the supported interface
     //   - Send Set_configuration() request
     //   - Goto full operating mode (device ready)
     //
      case DEVICE_CONFIGURED :
         TRACE_DEBUG("DEVICE_CONFIGURED\n\r");
         if (CONTROL_GOOD == host_get_configuration_descriptor())
         {
            if (HOST_FALSE != host_check_class()) // Class support OK?
            {
            #if (USB_OTG_FEATURE == ENABLED)
              // Collect information about peripheral OTG descriptor if present
              host_check_OTG_features();
            #endif
            #if (HOST_AUTO_CFG_ENDPOINT==ENABLE)
              host_auto_configure_endpoint();
            #else
               User_configure_endpoint(); // User call here instead of autoconfig
               Host_set_configured();     // Assumes config is OK with user config
            #endif
               if (Is_host_configured())
               {
                  if (CONTROL_GOOD== host_set_configuration(1))  // Send Set_configuration
                  {
                     // host_set_interface(interface_bound,interface_bound_alt_set);
                     // device and host are now fully configured
                     // goto DEVICE READY normal operation
                      device_state = DEVICE_READY;
                     // monitor device disconnection under interrupt
                      Host_enable_device_disconnection_interrupt();
                     // If user host application requires SOF interrupt event
                     // Keep SOF interrupt enable otherwize, disable this interrupt
                  #if (HOST_CONTINUOUS_SOF_INTERRUPT==DISABLE)
                      Host_disable_sof_interrupt();
                  #endif
                      new_device_connected=TRUE;
                      Enable_interrupt();
                      LOG_STR_CODE(log_device_enumerated);
                  }
                  else// Problem during Set_configuration request...
                  {   device_state = DEVICE_ERROR;  }
               }
            }
            else // device class not supported...
            {
                device_state = DEVICE_ERROR;
                LOG_STR_CODE(log_device_unsupported);
                Host_device_class_not_supported_action();
            }
         }
         else // Can not get configuration descriptors...
         {  device_state = DEVICE_ERROR; }
         break;

     //------------------------------------------------------
     //   DEVICE_READY state
     //
     //   - Full standard operating mode
     //   - Nothing to do...
     //
      case DEVICE_READY:     // Host full std operating mode!
         //TRACE_DEBUG("DEVICE_READY\n\r");
         new_device_connected=FALSE;

         // Handles user requests : "stop Vbus" and "suspend"
         #if (USB_OTG_FEATURE == ENABLED)
         if (Is_usb_id_host())
         {
           #if (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED)
           if (Is_user_requested_vbus())
           {
             Ack_user_request_vbus();
             Usb_disable_vbus();
             Usb_disable_manual_vbus();
             Host_vbus_action();
             Clear_all_user_request();
             device_state = A_END_HNP_WAIT_VFALL;
           }
           #endif

           if (Is_user_requested_suspend() || Is_user_requested_hnp())
           {
             // Before entering suspend mode, A-Host must send a SetFeature(b_hnp_enable) if supported by the B-Periph
             Ack_user_request_hnp();
             Ack_user_request_suspend();
             device_state = A_INIT_HNP;
           }
         }
         #endif
         break;

     //------------------------------------------------------
     //   DEVICE_ERROR state
     //
     //   - Error state
     //   - Do custom action call (probably go to default mode...)
     //
      case DEVICE_ERROR :
         TRACE_DEBUG("DEVICE_ERROR\n\r");
      #if (USB_OTG_FEATURE == ENABLED)  // TBD
         device_state=DEVICE_UNATTACHED;
      #elif (HOST_ERROR_RESTART==ENABLE)
         device_state=DEVICE_UNATTACHED;
      #endif
         Host_device_error_action();
         break;

     //------------------------------------------------------
     //   DEVICE_SUSPENDED state
     //
     //   - Host application request to suspend the device activity
     //   - State machine comes here thanks to Host_request_suspend()
     //
      case DEVICE_SUSPENDED :
         TRACE_DEBUG("DEVICE_SUSPENDED\n\r");
         // If OTG device, initiate a HNP process (go to specific state)
         if ((USB_OTG_FEATURE == ENABLED) && Is_peripheral_otg_device())
         {
           device_state = A_INIT_HNP;
         }
         else
         {
           device_state=DEVICE_WAIT_RESUME;    // wait for device resume event
           if(Is_device_supports_remote_wakeup()) // If the connected device supports remote wake up
           {
              if (CONTROL_GOOD != host_set_feature_remote_wakeup())
              {
                device_state = DEVICE_DISCONNECTED;   // stop connexion because device has not accepted the feature
              }
           }

           LOG_STR_CODE(log_going_to_suspend);
           c = Is_host_sof_interrupt_enabled(); //Save current sof interrupt enable state
           Host_disable_sof_interrupt();
           Host_ack_sof();
           Host_disable_sof();           // Stop start of frame generation, this generates the suspend state

           Host_ack_remote_wakeup();
           Host_enable_remote_wakeup_interrupt();
           Host_ack_hwup();
           Host_enable_hwup_interrupt(); // Enable host wake-up interrupt
                                         // (this is the unique USB interrupt able to wake up the CPU core from power-down mode)
           Usb_freeze_clock();
//           Stop_pll();
           Host_suspend_action();              // Custom action here! (for example go to power-save mode...)
         }
         break;

     //------------------------------------------------------
     //   DEVICE_WAIT_RESUME state
     //
     //   - Wait in this state till the host receives an upstream resume from the device
     //   - or the host software request the device to resume
     //
      case DEVICE_WAIT_RESUME :
         TRACE_DEBUG("DEVICE_WAIT_RESUME\n\r");
         if(Is_usb_event(EVT_HOST_REMOTE_WAKEUP)|| Is_host_request_resume())// Remote wake up has been detected
                                                                            // or Local resume request has been received
         {
            if(Is_host_request_resume())       // Not a remote wakeup, but an host application request
            {
               Host_disable_hwup_interrupt();  // Wake up interrupt should be disable host is now wake up !
                  // CAUTION HWUP can be cleared only when USB clock is active
//               Pll_start_auto();               // First Restart the PLL for USB operation
//               Wait_pll_ready();               // Get sure pll is lock
               Usb_unfreeze_clock();           // Enable clock on USB interface
            }
            Host_ack_hwup();                // Clear HWUP interrupt flag
            Host_enable_sof();

            if (Is_usb_event(EVT_HOST_REMOTE_WAKEUP))
            {
              Usb_ack_event(EVT_HOST_REMOTE_WAKEUP);    // Ack software event
              Host_disable_sof_interrupt();
              Host_ack_device_disconnection();
              Host_disable_device_disconnection_interrupt();

              Host_send_resume();     // this other downstream resume is done to ensure min. 20ms of HOST DRIVING RESUME (not Device)
              while (!Is_device_disconnection() && Host_is_resume());
              c = 0;
              Host_ack_sof();
              while (!Is_device_disconnection() && (c != 12))   // wait for min. 10ms of device recovery time
              {
                if (Is_host_sof())
                {
                  Host_ack_sof();
                  c++;
                }
              }
              if (Is_device_disconnection())
              {
                usb_host_task_init();
                device_state = DEVICE_DISCONNECTED;
                Host_ack_remote_wakeup();        // Ack remote wake-up reception
                Host_ack_request_resume();       // Ack software request
                Host_ack_down_stream_resume();   // Ack down stream resume sent
                #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
                Usb_disable_vbus();
                Usb_disable_manual_vbus();
                Host_vbus_action();
                Clear_all_user_request();
                while (Is_usb_vbus_high());
                #endif
              }
              else
              {
                device_state = DEVICE_READY;
                Host_ack_remote_wakeup();        // Ack remote wake-up reception
                Host_ack_request_resume();       // Ack software request
                Host_ack_down_stream_resume();   // Ack down stream resume sent
              }
              Host_enable_device_disconnection_interrupt();
              Host_ack_sof();
            }
            else
            {
              Host_send_resume();                            // Send down stream resume
              //-----------------------
              // Work-around for case of Device disconnection during Suspend
              // The disconnection is never detected and the Resume bit remains high (and RSMEDI flag never set)
              // If the timeout elapses, it implies that the device has disconnected => macro is reset (to reset the Resume bit)
              dec = 0;
              while (dec < 0x4FFFF)   // several hundreds of ms
              {
                if (Is_host_down_stream_resume())   // Wait Down stream resume sent
                {
                  Host_ack_remote_wakeup();        // Ack remote wake-up reception
                  Host_ack_request_resume();       // Ack software request
                  Host_ack_down_stream_resume();   // Ack down stream resume sent
                  if(c) { Host_enable_sof_interrupt(); } // Restore SOF interrupt enable state before suspend
                  device_state=DEVICE_READY;       // Come back to full operating mode
                  LOG_STR_CODE(log_usb_resumed);
                  dec = 0x3FFFFE; // will cause a loop end
                }
                dec++;
              }

              if (dec != 0x3FFFFF)    // if resume failed
              {
                usb_host_task_init();
                device_state = DEVICE_DISCONNECTED;
                #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
                Usb_disable_vbus();
                Usb_disable_manual_vbus();
                Host_vbus_action();
                Clear_all_user_request();
                while (Is_usb_vbus_high());
                #endif
              }
              else
              {
                c = 0;
                Host_ack_sof();
                while (!Is_device_disconnection() && (c != 12))   // wait for min. 10ms of device recovery time
                {
                  if (Is_host_sof())
                  {
                    Host_ack_sof();
                    c++;
                  }
                }
              }
              //-----------------------End of Work Around
            }
         }
         #if ((USB_OTG_FEATURE == ENABLED) && (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED))
         // Handle "stop Vbus" user request
         if (Is_user_requested_vbus() && Is_usb_id_host())
         {
           Ack_user_request_vbus();
           Usb_disable_vbus();
           Usb_disable_manual_vbus();
           Host_vbus_action();
           Clear_all_user_request();
           device_state = A_END_HNP_WAIT_VFALL;
         }
         #endif
         break;

     //------------------------------------------------------
     //   DEVICE_DISCONNECTED state
     //
     //   - Device disconnection has been detected
     //   - Run scheduler in this state at least two times to get sure event is detected by all host application tasks
     //   - Go to DEVICE_DISCONNECTED_ACK state before DEVICE_UNATTACHED, to get sure scheduler calls all app tasks...
     //
      case DEVICE_DISCONNECTED :
         TRACE_DEBUG("DEVICE_DISCONNECTED\n\r");
         device_state = DEVICE_DISCONNECTED_ACK;
         break;

     //------------------------------------------------------
     //   DEVICE_DISCONNECTED_ACK state
     //
     //   - Device disconnection has been detected and managed bu applicatives tasks
     //   - Go to DEVICE_UNATTACHED state
     //
      case DEVICE_DISCONNECTED_ACK :
         TRACE_DEBUG("DEVICE_DISCONNECTED_ACK\n\r");
         host_disable_all_pipe();
         device_state = DEVICE_UNATTACHED;
         #if (USB_OTG_FEATURE == ENABLED)
         if (OTG_VBUS_AUTO_WHEN_A_PLUG == DISABLED)
         {
           Usb_disable_manual_vbus();
           Usb_disable_vbus();
           Host_vbus_action();
           Clear_all_user_request();
         }
         End_session_with_srp();
         Usb_ack_srp_interrupt();
         #endif
         break;

         
#if (USB_OTG_FEATURE == ENABLED)
     //------------------------------------------------------
     //   OTG Specific states : A_PERIPHERAL (A-Host has been turned into a Device after a HNP)
     //
     //   - End session (and stop driving Vbus) when detecting suspend condition
     //   - Disconnect on user request
     //   - Call standard (non-OTG) device task to handle the Endpoint 0 requests
     //
      case A_PERIPHERAL:
         TRACE_DEBUG("A_PERIPHERAL\n\r");
        // End of role exchange : A_PERIPH go into DEVICE_DISCONNECTED mode : stop supplying VBUS
        if (Is_usb_event(EVT_USB_SUSPEND))
        {
          Clear_all_user_request();
          Usb_ack_event(EVT_USB_SUSPEND);
          Usb_disable_wake_up_interrupt();
          Usb_ack_role_exchange_interrupt();
          Usb_select_host();
          Usb_attach();
          Usb_unfreeze_clock();
          otg_b_device_state = B_IDLE;
          device_state = A_END_HNP_WAIT_VFALL;
          Usb_ack_srp_interrupt();
        }
        if (Is_user_requested_disc() || Is_user_requested_vbus())
        {
          Clear_all_user_request();
          Usb_disable_suspend_interrupt();
          Usb_ack_role_exchange_interrupt();
          Usb_select_host();
          Usb_unfreeze_clock();
          otg_b_device_state = B_IDLE;
          device_state = A_END_HNP_WAIT_VFALL;
        }
        if (!Is_device_disconnection_event() && (device_state != A_END_HNP_WAIT_VFALL))
        {
          usb_device_task();
        }
        break;

     //------------------------------------------------------
     //   OTG Specific states : A_INIT_HNP
     //
     //   - Software enters this state when it has been requested to initiate a HNP
     //   - Handle "set feature" commands such as B_HNP_ENABLE or REMOTE_WAKE_UP
     //   - Handle failures
     //
      case A_INIT_HNP:
         TRACE_DEBUG("A_INIT_HNP\n\r");
        Ack_user_request_hnp();
        Ack_user_request_suspend();
        if (Is_peripheral_otg_device() || !Is_host_configured())
        {
          device_state = A_SUSPEND;
          if(Is_device_supports_remote_wakeup() && Is_host_configured()) // If the connected device supports remote wake up
          {
            if (CONTROL_GOOD == host_set_feature_remote_wakeup())
            {
              Host_ack_remote_wakeup();
              Host_enable_remote_wakeup_interrupt();
              Host_ack_hwup();
              Host_enable_hwup_interrupt(); // Enable host wake-up interrupt
            }
            else
            {
              device_state = A_END_HNP_WAIT_VFALL;   // stop connection because device has STALLed the feature
            }
          }

          if (Is_device_supports_hnp() || !Is_host_configured())
          {
            if (CONTROL_GOOD == host_set_feature_b_hnp_enable())
            {
              // B-Device has not STALLed the SetFeature
              Usb_host_accept_hnp();
              Usb_ack_role_exchange_interrupt();
              Usb_ack_hnp_error_interrupt();
              Usb_enable_role_exchange_interrupt();
              Usb_enable_hnp_error_interrupt();
              Host_disable_device_disconnection_interrupt();
              Host_disable_device_connection_interrupt();
            }
            else
            {
              Otg_print_new_failure_message(OTGMSG_DEVICE_NO_RESP,OTG_TEMPO_4SEC);
              device_state = A_END_HNP_WAIT_VFALL;
            }
          }
          Host_ack_remote_wakeup();
          Host_enable_remote_wakeup_interrupt();
          Init_ta_aidl_bdis_counter();
          Host_disable_sof_interrupt();
          Host_ack_sof();
          Host_disable_sof();           // Stop start of frame generation, this generates the suspend state
          Usb_disable_suspend_interrupt();
        }
        else
        {
          device_state = DEVICE_SUSPENDED;
        }
        break;

     //------------------------------------------------------
     //   OTG Specific states : A_SUSPEND
     //
     //   - A-Host enters this state when it has requested the B-DEVICE to start HNP, and have entered Suspend mode
     //   - Detects device silences (with HNP time-out management) and Resume condition
     //
      case A_SUSPEND:
         TRACE_DEBUG("A_SUSPEND\n\r");
        Usb_ack_suspend();
        // HNP is managed by interrupt (HNPERRI/ROLEEXI)
        if (Is_ta_aidl_bdis_counter_overflow())
        {
          device_state = A_END_HNP_WAIT_VFALL;   // stop Vbus = end of current session
        }
        if (Is_usb_event(EVT_HOST_HWUP)|| Is_host_request_resume())
        {
          device_state = DEVICE_WAIT_RESUME;
        }
        break;

     //------------------------------------------------------
     //   OTG Specific states : A_END_HNP_WAIT_VFALL
     //
     //   - A-PERIPH enters this state when it has detected a Suspend from the B-Host
     //   - It stop Vbus delivery and waits line discharge (to avoid spurious SRP detection)
     //
      case A_END_HNP_WAIT_VFALL:
         TRACE_DEBUG("A_END_HNP_WAIT_VFALL\n\r");
        Usb_disable_manual_vbus();
        Usb_disable_vbus();
        usb_configuration_nb = 0;
        Host_vbus_action();
        Clear_all_user_request();
    #if   (OTG_COMPLIANCE_TRICKS == ENABLED)
        device_state = DEVICE_DISCONNECTED;
        usb_host_task_init();
    #else
        if (Is_usb_vbus_low())
        {
          usb_host_task_init();
          Init_ta_vbus_fall_counter();
          device_state = A_TEMPO_VBUS_DISCHARGE;
        }
    #endif
        break;

     //------------------------------------------------------
     //   OTG Specific states : A_TEMPO_VBUS_DISCHARGE
     //
     //   - State entered from A_END_HNP_WAIT_VFALL, when Vbus has just reached the vbus_valid threshold
     //   - In this state we wait long enough (50ms) to be sure that Vbus is not valid on the other device (if still connected)
     //   - When delay is elapsed, go to reset state
      case A_TEMPO_VBUS_DISCHARGE:
         TRACE_DEBUG("A_TEMPO_VBUS_DISCHARGE\n\r");
        if (otg_end_hnp_vbus_delay == 0)
        {
          Host_ack_device_connection();
          Host_ack_device_disconnection();
          Usb_ack_role_exchange_interrupt();
          Usb_ack_srp_interrupt();
          device_state = DEVICE_DISCONNECTED;
        }
        break;
#endif

     //------------------------------------------------------
     //   default state
     //
     //   - Default case: ERROR
     //   - Goto no device state
     //
      default :
          TRACE_DEBUG("default\n\r");
        device_state = DEVICE_UNATTACHED;
         break;
      }
}

//___ F U N C T I O N S   F O R   P O L L I N G   M A N A G E D   D A T A  F L O W S  _________________________

/**
  * @brief This function send nb_data pointed with *buf with the pipe number specified
  *
  * @note This function will activate the host sof interrupt to detect timeout. The
  * interrupt enable sof will be restore.
  *
  * @param pipe
  * @param nb_data
  * @param buf
  *
  * @return status
  */
U8 host_send_data(U8 pipe, U16 nb_data, U8 *buf)
{
   U8 c;
   U8 status=PIPE_GOOD;
   U8 sav_int_sof_enable;
   U8 nak_timeout;
   U16 cpt_nak;
   U8 nb_data_loaded;

   TRACE_DEBUG("host_send_data\n\r");

//jcb big stub !!!
    AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[2] = 0x1022232; // OUT, PIPE 2, 64
//    AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[2] = 0x1021232;
//jcb big stub !!!


   sav_int_sof_enable=Is_host_sof_interrupt_enabled();  // Save state of enable sof interrupt
   Host_enable_sof_interrupt();
   Host_select_pipe(pipe);

   Host_set_token_in();

   Host_set_token_out();
   Host_ack_out_sent();
   while (nb_data != 0)         // While there is something to send...
   {
      Host_unfreeze_pipe();
     // Prepare data to be sent
      c = Host_get_pipe_length();
      if ( (U16)c > nb_data)
      {
         nb_data_loaded = (U8)nb_data;
         c = nb_data;
      }
      else
      {  nb_data_loaded = c; }

      Address_fifochar_endpoint(global_pipe_nb);
      while (c!=0)              // Load Pipe buffer
      {
       //Host_write_byte(*buf++);
         //(((char*)((unsigned int *)AT91C_BASE_OTGHS_EPTFIFO->OTGHS_READEPT0))[dBytes++])=*(buf++);
         pFifo[dBytes++] = *buf;
         buf++;
         c--;
      }
      private_sof_counter=0;    // Reset the counter in SOF detection sub-routine
      cpt_nak=0;
      nak_timeout=0;
      Host_ack_out_sent();
      Host_send_out();
      while (!Is_host_out_sent())
      {
         if (Is_host_emergency_exit())// Async disconnection or role change detected under interrupt
         {
            status=PIPE_DELAY_TIMEOUT;
            Host_reset_pipe(pipe);
            goto host_send_data_end;
         }
         #if (TIMEOUT_DELAY_ENABLE==ENABLE)
         if (private_sof_counter>=250)            // Count 250ms (250sof)
         {
            TRACE_DEBUG("TimeOut Send Data\n\r");
            private_sof_counter=0;
            if (nak_timeout++>=TIMEOUT_DELAY) // Inc timeout and check for overflow
            {
               status=PIPE_DELAY_TIMEOUT;
               Host_reset_pipe(pipe);
               goto host_send_data_end;
            }
         }
         #endif
         if (Is_host_pipe_error()) // Any error ?
         {
            status = Host_error_status();
            Host_ack_all_errors();
            goto host_send_data_end;
         }
         if (Is_host_stall())      // Stall management
         {
            status =PIPE_STALL;
            Host_ack_stall();
            goto host_send_data_end;
         }
         #if (NAK_TIMEOUT_ENABLE==ENABLE)
         if(Is_host_nak_received())  //NAK received
         {
            Host_ack_nak_received();
            if (cpt_nak++>NAK_SEND_TIMEOUT)
            {
               status = PIPE_NAK_TIMEOUT;
               Host_reset_pipe(pipe);
               goto host_send_data_end;
            }
         }
         #endif
      }
      // Here OUT sent
      nb_data -= nb_data_loaded;
      status=PIPE_GOOD;         // Frame correctly sent
      Host_ack_out_sent();
   }
   Host_freeze_pipe();
host_send_data_end:
  // Restore sof interrupt enable state
   if (sav_int_sof_enable==FALSE)   {Host_disable_sof_interrupt();}
  // And return...
   return ((U8)status);
}



/**
  * @brief This function receives nb_data pointed with *buf with the pipe number specified
  *
  * The nb_data parameter is passed as a U16 pointer, thus the data pointed by this pointer
  * is updated with the final number of data byte received.
  *
  * @param pipe
  * @param nb_data
  * @param buf
  *
  * @return status
  */
U8 host_get_data(U8 pipe, U16 *nb_data, U8 *buf)
{
   U8 status=PIPE_GOOD;
   U8 sav_int_sof_enable;
   U8 nak_timeout;
   U16 n,i;
   U16 cpt_nak;

   TRACE_DEBUG("host_get_data[%d]\n\r", pipe);
   n=*nb_data;
   *nb_data=0;




TRACE_DEBUG("HSTPIPCFG[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[2]);
TRACE_DEBUG("HSTPIPINRQ[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPINRQ[2]);
TRACE_DEBUG("HSTPIPERR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPERR[2]);
TRACE_DEBUG("HSTPIPISR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPISR[2]);
TRACE_DEBUG("HSTPIPIMR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPIMR[2]);





//    pipe = 1;
//    AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[1] = 0x1012132; // OUT, PIPE 1, 64

   sav_int_sof_enable=Is_host_sof_interrupt_enabled();
   Host_enable_sof_interrupt();

   Host_select_pipe(pipe);
   Host_continuous_in_mode();
   Host_set_token_in();
   Host_ack_in_received();

   while (n)              // While missing data...
   {
      // start IN request generation
//      Host_unfreeze_pipe();
AT91C_BASE_OTGHS->OTGHS_HSTPIPIDR[global_pipe_nb] = AT91C_OTGHS_FREEZE;
      Host_send_in();
      private_sof_counter=0; // Reset the counter in SOF detection sub-routine
/*
        TRACE_DEBUG("CTRL: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_CTRL);
        TRACE_DEBUG("SR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_SR);
        TRACE_DEBUG("DEVCTRL: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_DEVCTRL);
        TRACE_DEBUG("DEVISR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_DEVISR);
        TRACE_DEBUG("DEVIMR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_DEVIMR);
        TRACE_DEBUG("DEVEPTISR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_DEVEPTISR[0]);
        TRACE_DEBUG("HSTCTRL: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTCTRL);
        TRACE_DEBUG("HSTISR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTISR);
        TRACE_DEBUG("HSTIMR: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTIMR);
        TRACE_DEBUG("HSTPIP: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIP);
        TRACE_DEBUG("HSTPIPCFG[0]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[0]);
        TRACE_DEBUG("HSTPIPCFG[1]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[1]);
        TRACE_DEBUG("HSTPIPCFG[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[2]);
        TRACE_DEBUG("HSTPIPINRQ[0]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPINRQ[0]);
        TRACE_DEBUG("HSTPIPINRQ[1]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPINRQ[1]);
        TRACE_DEBUG("HSTPIPINRQ[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPINRQ[2]);
        TRACE_DEBUG("HSTPIPERR[0]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPERR[0]);
        TRACE_DEBUG("HSTPIPERR[1]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPERR[1]);
        TRACE_DEBUG("HSTPIPERR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPERR[2]);
        TRACE_DEBUG("HSTPIPISR[0]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPISR[0]);
        TRACE_DEBUG("HSTPIPISR[1]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPISR[1]);
        TRACE_DEBUG("HSTPIPISR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPISR[2]);
        TRACE_DEBUG("HSTPIPIMR[0]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPIMR[0]);
        TRACE_DEBUG("HSTPIPIMR[1]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPIMR[1]);
        TRACE_DEBUG("HSTPIPIMR[2]: 0x%X\n\r",AT91C_BASE_OTGHS->OTGHS_HSTPIPIMR[2]);
*/
      nak_timeout=0;
      cpt_nak=0;
      while (!Is_host_in_received())
      {
         if (Is_host_emergency_exit())   // Async disconnection or role change detected under interrupt
         {
            status=PIPE_DELAY_TIMEOUT;
            Host_reset_pipe(pipe);
            goto host_get_data_end;
         }
         #if (TIMEOUT_DELAY_ENABLE==ENABLE)
         if (private_sof_counter>=250)   // Timeout management
         {
            private_sof_counter=0;       // Done in host SOF interrupt
            if (nak_timeout++>=TIMEOUT_DELAY)// Check for local timeout
            {
               status=PIPE_DELAY_TIMEOUT;
               Host_reset_pipe(pipe);
               goto host_get_data_end;
            }
         }
         #endif
         if(Is_host_pipe_error())        // Error management
         {
            status = Host_error_status();
            Host_ack_all_errors();
            goto host_get_data_end;
         }
         if(Is_host_stall())             // STALL management
         {
            status =PIPE_STALL;
            Host_reset_pipe(pipe);
            Host_ack_stall();
            goto host_get_data_end;
         }
         #if (NAK_TIMEOUT_ENABLE==ENABLE)
         if(Is_host_nak_received())  //NAK received
         {
            Host_ack_nak_received();
            if (cpt_nak++>NAK_RECEIVE_TIMEOUT)
            {
               status = PIPE_NAK_TIMEOUT;
               Host_reset_pipe(pipe);
               goto host_get_data_end;
            }
         }
         #endif
      }
      status=PIPE_GOOD;
//      Host_freeze_pipe();
      if (Host_byte_counter()<=n)
      {
         if ((Host_byte_counter() < n)&&(Host_byte_counter()<Host_get_pipe_length()))
         { 
           n = 0;
         }
         else
         { 
           n -= Host_byte_counter();
         }
         (*nb_data) += Host_byte_counter();  // Update nb of byte received

         Address_fifochar_endpoint(global_pipe_nb);
         for (i=Host_byte_counter(); i; i--)
         { 
           *buf = Host_read_byte(); 
           buf++;
         }
      }
      else  // more bytes received than expected
      {     // TODO error code management
         *nb_data += n;
         Address_fifochar_endpoint(global_pipe_nb);
         for (i=n; i; i--)                  // Byte number limited to the initial request (limit tab over pb)
         {
           *buf = Host_read_byte();
           buf++;
         }
         n=0;
      }
      Host_ack_in_received();
   }
   Host_freeze_pipe();
host_get_data_end:
   if (sav_int_sof_enable==FALSE)
   {
      Host_disable_sof_interrupt();
   }
//jcb big stub !!!
//    AT91C_BASE_OTGHS->OTGHS_HSTPIPCFG[2] = 0x1022232; // OUT, PIPE 2, 64
//jcb big stub !!!
   return ((U8)status);
}


//___ F U N C T I O N S   F O R   I N T E R R U P T   M A N A G E D   D A T A   F L O W S  _________________________

#if (USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE)

void reset_it_pipe_str(void)
{
   U8 i;
  TRACE_DEBUG("reset_it_pipe_str\n\r");
   for(i=0;i<MAX_EP_NB;i++)
   {
      it_pipe_str[i].enable=DISABLE;
      it_pipe_str[i].timeout=0;
   }
}

U8 is_any_interrupt_pipe_active(void)
{
   U8 i;
  TRACE_DEBUG("is_any_interrupt_pipe_active\n\r");
   for(i=0;i<MAX_EP_NB;i++)
   {
      if(it_pipe_str[i].enable==ENABLE) return TRUE;
   }
   return FALSE;
}

/**
  * @brief This function receives nb_data pointed with *buf with the pipe number specified
  *
  * The nb_data parameter is passed as a U16 pointer, thus the data pointed by this pointer
  * is updated with the final number of data byte received.
  *
  * @param pipe
  * @param nb_data
  * @param buf
  * @param handle call back function pointer
  *
  * @return status
  */
U8 host_get_data_interrupt(U8 pipe, U16 nb_data, U8 *buf,void(*handle)(U8 status, U16 nb_byte))
{
  TRACE_DEBUG("host_get_data_interrupt\n\r");
   Host_select_pipe(pipe);
   if(it_pipe_str[pipe].enable==ENABLE)
   {
      return HOST_FALSE;
   }
   else
   {
      if(is_any_interrupt_pipe_active()==FALSE)
      {
         g_sav_int_sof_enable=Is_host_sof_interrupt_enabled();
         Host_enable_sof_interrupt();
      }
      it_pipe_str[pipe].enable=ENABLE;
      it_pipe_str[pipe].nb_byte_to_process=nb_data;
      it_pipe_str[pipe].nb_byte_processed=0;
      it_pipe_str[pipe].ptr_buf=buf;
      it_pipe_str[pipe].handle=handle;
      it_pipe_str[pipe].timeout=0;
      it_pipe_str[pipe].nak_timeout=NAK_RECEIVE_TIMEOUT;

      private_sof_counter=0;           // Reset the counter in SOF detection sub-routine
      Host_reset_pipe(pipe);
      Host_enable_stall_interrupt();
      #if (NAK_TIMEOUT_ENABLE==ENABLE)
      Host_enable_nak_interrupt();
      #endif
      Host_enable_error_interrupt();
      Host_enable_receive_interrupt();
      Host_ack_stall();
      Host_ack_nak_received();

      Host_continuous_in_mode();
      Host_set_token_in();
      Host_unfreeze_pipe();
      return HOST_TRUE;
   }
}

/**
  * @brief This function send nb_data pointed with *buf with the pipe number specified
  *
  *
  * @param pipe
  * @param nb_data
  * @param buf
  * @param handle call back function pointer
  *
  * @return status
  */
U8 host_send_data_interrupt(U8 pipe, U16 nb_data, U8 *buf, void(*handle)(U8 status, U16 nb_byte))
{
   U8 i;
   U8 *ptr_buf=buf;

  TRACE_DEBUG("host_send_data_interrupt\n\r");
   Host_select_pipe(pipe);
   if(it_pipe_str[pipe].enable==ENABLE)
   {
      return HOST_FALSE;
   }
   else
   {
      if(is_any_interrupt_pipe_active()==FALSE)
      {
         g_sav_int_sof_enable=Is_host_sof_interrupt_enabled();
         Host_enable_sof_interrupt();
      }
      it_pipe_str[pipe].enable=ENABLE;
      it_pipe_str[pipe].nb_byte_to_process=nb_data;
      it_pipe_str[pipe].nb_byte_processed=0;
      it_pipe_str[pipe].ptr_buf=buf;
      it_pipe_str[pipe].handle=handle;
      it_pipe_str[pipe].timeout=0;
      it_pipe_str[pipe].nak_timeout=NAK_SEND_TIMEOUT;
      it_pipe_str[pipe].nb_byte_on_going=0;

      Host_reset_pipe(pipe);
      Host_unfreeze_pipe();
      // Prepare data to be sent
      i = Host_get_pipe_length();
      if ( i > nb_data)                // Pipe size> remaining data
      {
         i = nb_data;
         nb_data = 0;
      }
      else                             // Pipe size < remaining data
      {
         nb_data -= i;
      }
      it_pipe_str[pipe].nb_byte_on_going+=i;   // Update nb data processed
      Address_fifochar_endpoint(global_pipe_nb);
      while (i!=0)                    // Load Pipe buffer
      {  
        Host_write_byte(*ptr_buf++); i--;
      }
      private_sof_counter=0;          // Reset the counter in SOF detection sub-routine
      it_pipe_str[pipe].timeout=0;    // Refresh timeout counter
      Host_ack_out_sent();
      Host_ack_stall();
      Host_ack_nak_received();

      Host_enable_stall_interrupt();
      Host_enable_error_interrupt();
      #if (NAK_TIMEOUT_ENABLE==ENABLE)
      Host_enable_nak_interrupt();
      #endif
      Host_enable_transmit_interrupt();
      Host_send_out();                // Send the USB frame
      return HOST_TRUE;
   }
}

//! @brief USB pipe interrupt subroutine
//!
//! @param none
//!
//! @return none
#ifdef AVRGCC
 ISR(USB_COM_vect)
#else
//#pragma vector = USB_ENDPOINT_PIPE_vect
__interrupt void usb_pipe_interrupt(void)
#endif
{
   U8 pipe_nb;
   U8 *ptr_buf;
   void  (*fct_handle)(U8 status,U16 nb_byte);
   U16 n;
   U8 i;
   U8 do_call_back=FALSE;

  TRACE_DEBUG("usb_pipe_interrupt\n\r");
   pipe_nb_save = Host_get_selected_pipe();       // Important! Save here working pipe number
   pipe_nb=usb_get_nb_pipe_interrupt();  // work with the correct pipe number that generates the interrupt
   Host_select_pipe(pipe_nb);                        // Select this pipe
   fct_handle=*(it_pipe_str[pipe_nb].handle);

   // Now try to detect what event generate an interrupt...

   if (Is_host_pipe_error())             // Any error ?
   {
      TRACE_DEBUG("host_pipe_error\n\r");
      it_pipe_str[pipe_nb].status = Host_error_status();
      it_pipe_str[pipe_nb].enable=DISABLE;
      Host_stop_pipe_interrupt(pipe_nb);
      Host_ack_all_errors();
      do_call_back=TRUE;
      goto usb_pipe_interrupt_end;
   }

   if (Is_host_stall())                  // Stall handshake received ?
   {
      TRACE_DEBUG("host_stall\n\r");
      it_pipe_str[pipe_nb].status=PIPE_STALL;
      it_pipe_str[pipe_nb].enable=DISABLE;
      Host_stop_pipe_interrupt(pipe_nb);
      do_call_back=TRUE;
      goto usb_pipe_interrupt_end;
   }

   #if (NAK_TIMEOUT_ENABLE==ENABLE)
   if (Is_host_nak_received())           // NAK ?
   {
      Host_ack_nak_received();
      // check if number of NAK timeout error occurs (not for interrupt type pipe)
      if((--it_pipe_str[pipe_nb].nak_timeout==0) && (Host_get_pipe_type()!=TYPE_INTERRUPT))
      {
         it_pipe_str[pipe_nb].status=PIPE_NAK_TIMEOUT;
         it_pipe_str[pipe_nb].enable=DISABLE;
         Host_stop_pipe_interrupt(pipe_nb);
         do_call_back=TRUE;
         goto usb_pipe_interrupt_end;
      }
   }
   #endif

   if (Is_host_in_received())            // Pipe IN reception ?
   {
      TRACE_DEBUG("host_in received\n\r");
      ptr_buf=it_pipe_str[pipe_nb].ptr_buf+it_pipe_str[pipe_nb].nb_byte_processed;       // Build pointer to data buffer
      n=it_pipe_str[pipe_nb].nb_byte_to_process-it_pipe_str[pipe_nb].nb_byte_processed;  // Remaining data bytes
      Host_freeze_pipe();
      if (Host_byte_counter()<=n)
      {
         if ((Host_byte_counter() < n)&&(Host_byte_counter()<Host_get_pipe_length())) //Received less than remaining, but less than pipe capacity
                                                                                      //TODO: error code
         {
            n=0;
         }
         else
         {
            n-=Host_byte_counter();
         }
         it_pipe_str[pipe_nb].nb_byte_processed+=Host_byte_counter();  // Update nb of byte received

         Address_fifochar_endpoint(global_pipe_nb);
         for (i=Host_byte_counter();i;i--)
         { 
           *ptr_buf=Host_read_byte();
           ptr_buf++;
         }
      }
      else  // more bytes received than expected
      {     // TODO error code management
         it_pipe_str[pipe_nb].nb_byte_processed+=n;
         Address_fifochar_endpoint(global_pipe_nb);
         for (i=n;i;i--)                  // Byte number limited to the initial request (limit tab over pb)
         { 
           *ptr_buf=Host_read_byte();
           ptr_buf++;
         }
         n=0;
      }
      Host_ack_in_received();
      if(n>0) //still something to process
      {
         Host_unfreeze_pipe();            // Request another IN transfer
         Host_send_in();
         private_sof_counter=0;           // Reset the counter in SOF detection sub-routine
         it_pipe_str[pipe_nb].timeout=0;  // Reset timeout
         it_pipe_str[pipe_nb].nak_timeout=NAK_RECEIVE_TIMEOUT;

      }
      else //end of transfer
      {
         it_pipe_str[pipe_nb].enable=DISABLE;
         it_pipe_str[pipe_nb].status=PIPE_GOOD;
         Host_stop_pipe_interrupt(pipe_nb);
         do_call_back=TRUE;
      }
   }

   if(Is_host_out_sent())                  // Pipe OUT sent ?
   {
      TRACE_DEBUG("host_out send\n\r");
      Host_ack_out_sent();
      it_pipe_str[pipe_nb].nb_byte_processed+=it_pipe_str[pipe_nb].nb_byte_on_going;
      it_pipe_str[pipe_nb].nb_byte_on_going=0;
      ptr_buf=it_pipe_str[pipe_nb].ptr_buf+it_pipe_str[pipe_nb].nb_byte_processed;       // Build pointer to data buffer
      n=it_pipe_str[pipe_nb].nb_byte_to_process-it_pipe_str[pipe_nb].nb_byte_processed;  // Remaining data bytes
      if(n>0)   // Still data to process...
      {
         Host_unfreeze_pipe();
        // Prepare data to be sent
         i = Host_get_pipe_length();
         if ( i > n)     // Pipe size> remaining data
         {
            i = n;
            n = 0;
         }
         else                // Pipe size < remaining data
         {  n -= i; }
         it_pipe_str[pipe_nb].nb_byte_on_going+=i;   // Update nb data processed
         Address_fifochar_endpoint(global_pipe_nb);
         while (i!=0)                     // Load Pipe buffer
         {
            Host_write_byte(*ptr_buf++); i--;
         }
         private_sof_counter=0;           // Reset the counter in SOF detection sub-routine
         it_pipe_str[pipe_nb].timeout=0;  // Refresh timeout counter
         it_pipe_str[pipe_nb].nak_timeout=NAK_SEND_TIMEOUT;
         Host_send_out();                 // Send the USB frame
      }
      else                                //n==0 Transfer is finished
      {
         it_pipe_str[pipe_nb].enable=DISABLE;    // Tranfer end
         it_pipe_str[pipe_nb].status=PIPE_GOOD;  // Status OK
         Host_stop_pipe_interrupt(pipe_nb);
         do_call_back=TRUE;
      }
   }

usb_pipe_interrupt_end:
   Host_select_pipe(pipe_nb_save);   // Restore pipe number !!!!
   if (is_any_interrupt_pipe_active()==FALSE)    // If no more transfer is armed
   {
      if (g_sav_int_sof_enable==FALSE)
      {
         Host_disable_sof_interrupt();
      }
   }
   if(do_call_back)      // Any callback functions to perform ?
   {
      fct_handle(it_pipe_str[pipe_nb].status,it_pipe_str[pipe_nb].nb_byte_processed);
   }
}
#endif


#endif // USB_HOST_FEATURE ENABLE

