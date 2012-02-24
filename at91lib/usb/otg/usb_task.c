 //!
//! @file usb_task.c,v
//!
//! Copyright (c) 2006 Atmel.
//!
//! Please read file license.txt for copyright notice.
//!
//! @brief This file manages the USB task either device/host or both.
//!
//! The USB task selects the correct USB task (usb_device task or usb_host task
//! to be executed depending on the current mode available.
//!
//! According to USB_DEVICE_FEATURE and USB_HOST_FEATURE value (located in conf_usb.h file)
//! The usb_task can be configured to support USB DEVICE mode or USB Host mode or both
//! for a dual role device application.
//!
//! This module also contains the general USB interrupt subroutine. This subroutine is used
//! to detect asynchronous USB events.
//!
//! Note:
//!   - The usb_task belongs to the scheduler, the usb_device_task and usb_host do not, they are called
//!     from the general usb_task
//!   - See conf_usb.h file for more details about the configuration of this module
//!
//!
//! @version 1.30 at90usb128-otg-dual_role-toggle-1_0_0 $Id: usb_task.c,v 1.30 2007/02/19 08:21:27 arobert Exp $
//!
//! @todo
//! @bug
//!/

//_____  I N C L U D E S ___________________________________________________

#include "config.h"
#include "conf_usb.h"
#include <utility/trace.h>

#if (USB_OTG_FEATURE == ENABLED)
  //#include "lib_mcu/timer/timer16_drv.h"
  #if (TARGET_BOARD==SPIDER)
  //  #include "lib_board/lcd/lcd_drv.h"
  #endif
#endif

#include "otg_user_task.h"
#include "usb_task.h"
#include "usb/otg/usb_drv.h"
#if ((USB_DEVICE_FEATURE == ENABLED))
#include "usb_descriptors.h"
#endif
//#include "lib_mcu/power/power_drv.h"
//#include "lib_mcu/wdt/wdt_drv.h"
//#include "lib_mcu/pll/pll_drv.h"

#if ((USB_HOST_FEATURE == ENABLED))
   #include "usb/otg/usb_host_task.h"
   #if (USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE)
      extern U8 g_sav_int_sof_enable;
   #endif
#endif

#if ((USB_DEVICE_FEATURE == ENABLED))
   #include "usb/otg/usb_device_task.h"
#endif


#ifndef  USE_USB_PADS_REGULATOR
   #error "USE_USB_PADS_REGULATOR" should be defined as ENABLE or DISABLE in conf_usb.h file
#endif


//jcb Timer16
#define Timer16_select(...)
#define Timer16_set_counter(...)
#define Timer16_get_counter_low() 20

//_____ M A C R O S ________________________________________________________

extern U8 global_pipe_nb;

#define __interrupt
#define Set_otg_custom_timer(ep)   // STUB: todo

#ifndef LOG_STR_CODE
#define LOG_STR_CODE(str)
#else
U8 code log_device_disconnect[]="Device Disconnected";
U8 code log_id_change[]="Pin Id Change";
#endif

//_____ D E F I N I T I O N S ______________________________________________

//!
//! Public : U16 g_usb_event
//! usb_connected is used to store USB events detected upon
//! USB general interrupt subroutine
//! Its value is managed by the following macros (See usb_task.h file)
//! Usb_send_event(x)
//! Usb_ack_event(x)
//! Usb_clear_all_event()
//! Is_usb_event(x)
//! Is_not_usb_event(x)
volatile U16 g_usb_event=0;


#if (USB_DEVICE_FEATURE == ENABLED)
//!
//! Public : (bit) usb_connected
//! usb_connected is set to TRUE when VBUS has been detected
//! usb_connected is set to FALSE otherwise
//! Used with USB_DEVICE_FEATURE == ENABLED only
//!/
extern bit   usb_connected;

//!
//! Public : (U8) usb_configuration_nb
//! Store the number of the USB configuration used by the USB device
//! when its value is different from zero, it means the device mode is enumerated
//! Used with USB_DEVICE_FEATURE == ENABLED only
//!/
extern U8    usb_configuration_nb;

//!
//! Public : (U8) remote_wakeup_feature
//! Store a host request for remote wake up (set feature received)
//!/
U8 remote_wakeup_feature;
#endif


#if (USB_HOST_FEATURE == ENABLED)
//!
//! Private : (U8) private_sof_counter
//! Incremented  by host SOF interrupt subroutime
//! This counter is used to detect timeout in host requests.
//! It must not be modified by the user application tasks.
//!/
volatile U8 private_sof_counter=0;

   #if (USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE)
extern volatile S_pipe_int   it_pipe_str[MAX_EP_NB];
   #endif

#endif

#if ((USB_DEVICE_FEATURE == ENABLED)&& (USB_HOST_FEATURE == ENABLED))
//!
//! Public : (U8) g_usb_mode
//! Used in dual role application (both device/host) to store
//! the current mode the usb controller is operating
//!/
   U8 g_usb_mode=USB_MODE_UNDEFINED;
   U8 g_old_usb_mode;
#endif


#if (USB_OTG_FEATURE == ENABLED)
  //!
  //! Public : (U8) otg_features_supported;
  //!   -> A-Device side : indicates if the B-Device supports HNP and SRP (this is the bmAttributes field of OTG Decriptor)
  //!   -> B-Device side : indicates if the A-Device has enabled the "a_hnp_support" and "b_hnp_enable" features
  volatile U8 otg_features_supported;
  
  //! Public : (U8) otg_user_request;
  //! Store the last request of the user (see usb_device_task.h)
  U8 otg_user_request;
     
  //! Public : (U16) otg_msg_event_delay;
  //! Contains the current display duration of the OTG event message
  U16 otg_msg_event_delay;
  
  //! Public : (U16) otg_msg_failure_delay;
  //! Contains the current display duration of the OTG failure message
  U16 otg_msg_failure_delay;
 
  //! Public : (U16) g_otg_event;
  //! Contains several bits corresponding to differents OTG events (similar to g_usb_event)
  volatile U16 g_otg_event;
   
  //! Public : (U8) otg_device_nb_hnp_retry;
  //! Counts the number of times a HNP fails, before aborting the operations
  U8 otg_device_nb_hnp_retry;

  #if (OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED)
   U8 id_changed_to_host_event;
  #endif
  
  #if (OTG_COMPLIANCE_TRICKS == ENABLED)
   volatile U8 otg_last_sof_received;     // last SOF received in SOF interrupt
   volatile U8 otg_last_sof_stored;       // last SOF value stored in OTG Timer interrupt
   volatile U8 reset_received;            // indicates if a reset has been received from B-Host after a HNP (used in A-Periph mode)
  #endif
  
  //! @brief VBUS Overload management
  //!
  //! Nothing to do ! If the condition is not defined in the board driver file
  //! (i.e. if the board does not support Vbus overcurrent detection),
  //! the macro is defined to false for firmware compatibility
  #ifndef Is_vbus_overcurrent()
    #warning  Is_vbus_overcurrent() must be defined if present on board
    #define   Is_vbus_overcurrent()       (FALSE)
  #endif
   
  //! Private function prototypes
  //! Tasks for OTG Messaging features
  void Otg_message_task_init(void);
  void Otg_message_task(void);
#endif



//_____ D E C L A R A T I O N S ____________________________________________

/**
 * @brief This function initializes the USB process.
 *
 *  Depending on the mode supported (HOST/DEVICE/DUAL_ROLE) the function
 *  calls the coresponding usb mode initialization function
 *
 *  @param none
 *
 *  @return none
 */
void usb_task_init(void)
{
   #if (USB_HOST_FEATURE == ENABLED && USB_DEVICE_FEATURE == ENABLED)
   U8 delay;
   #endif

   #if (USE_USB_PADS_REGULATOR==ENABLE)  // Otherwise assume USB PADs regulator is not used
   Usb_enable_regulator();
   #endif

// ---- DUAL ROLE DEVICE USB MODE ---------------------------------------------
#if (USB_OTG_FEATURE == ENABLED)
   Otg_message_task_init();       // OTG program needs to display event messages to the user
   Otg_timer_init();              // OTG program requires TIMER1 or 3 to handle several OTG specific timings
   otg_b_device_state = B_IDLE;   // init state machines variables
   device_state = DEVICE_UNATTACHED;
   otg_device_nb_hnp_retry = BDEV_HNP_NB_RETRY;
   Clear_all_user_request();
#endif

#if (((USB_DEVICE_FEATURE == ENABLED) && (USB_HOST_FEATURE == ENABLED)) || (USB_OTG_FEATURE == ENABLED))
#if (USB_OTG_FEATURE == ENABLED)
   Usb_enable_uid_pin();
#endif

   // delay=PORTA;
   g_usb_mode=USB_MODE_UNDEFINED;

   if(Is_usb_id_device())
   {
     g_usb_mode=USB_MODE_DEVICE;
     usb_device_task_init();
     #if ((OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED) && (USB_OTG_FEATURE == ENABLED))
     id_changed_to_host_event = DISABLED;
     #endif
   }
   else
   {
     Usb_send_event(EVT_USB_HOST_FUNCTION);
     g_usb_mode=USB_MODE_HOST;
     Usb_ack_id_transition(); // REQUIRED !!! Startup with ID=0, Ack ID pin transistion (default hwd start up is device mode)
     Usb_enable_id_interrupt();
     Enable_interrupt();
     usb_host_task_init();
     #if ((OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED) && (USB_OTG_FEATURE == ENABLED))
     id_changed_to_host_event = ENABLED;
     #endif
   }
   g_old_usb_mode = g_usb_mode;   // Store current usb mode, for mode change detection
// -----------------------------------------------------------------------------

// ---- DEVICE ONLY USB MODE ---------------------------------------------------
#elif ((USB_DEVICE_FEATURE == ENABLED)&& (USB_HOST_FEATURE == DISABLE))
   //jcbUsb_force_device_mode();
   usb_device_task_init();
// -----------------------------------------------------------------------------

// ---- REDUCED HOST ONLY USB MODE ---------------------------------------------
#elif ((USB_DEVICE_FEATURE == DISABLE)&& (USB_HOST_FEATURE == ENABLED))
   //jcbUsb_force_host_mode();
   usb_host_task_init();
#elif ((USB_DEVICE_FEATURE == DISABLE)&& (USB_HOST_FEATURE == DISABLE))
   #error  at least one of USB_DEVICE_FEATURE or USB_HOST_FEATURE should be enabled
#endif
// -----------------------------------------------------------------------------

}

/**
 *  @brief Entry point of the USB mamnagement
 *
 *  Depending on the USB mode supported (HOST/DEVICE/DUAL_ROLE) the function
 *  calls the corresponding usb management function.
 *
 *  @param none
 *
 *  @return none
*/
void usb_task(void)
{

   //TRACE_DEBUG("usb_task ");
// ---- OTG DEVICE ------------------------------------------------------------
//#if (USB_OTG_FEATURE == ENABLED)

       //Usb_force_device_mode();



// ---- DUAL ROLE DEVICE USB MODE ---------------------------------------------
#if ((USB_DEVICE_FEATURE == ENABLED) && (USB_HOST_FEATURE == ENABLED))
   if(Is_usb_id_device())
   { g_usb_mode=USB_MODE_DEVICE;}
   else
   { g_usb_mode=USB_MODE_HOST;}
  // TODO !!! ID pin hot state change
  // Preliminary management: HARDWARE RESET !!!
   #if ( ID_PIN_CHANGE_GENERATE_RESET == ENABLE)
     // Hot ID transition generates wdt reset
//      if((g_old_usb_mode!=g_usb_mode))
      #ifndef  AVRGCC
//         {Wdt_change_16ms(); while(1);   LOG_STR_CODE(log_id_change);}
      #else
//         {Wdt_change_enable(); while(1); LOG_STR_CODE(log_id_change);}
      #endif

   #endif
  g_old_usb_mode=g_usb_mode;   // Store current usb mode, for mode change detection
  // Depending on current usb mode, launch the correct usb task (device or host)
  
   #if (USB_OTG_FEATURE == ENABLED)  
     // Configure OTG timers
     Set_otg_custom_timer(VBUSRISE_70MS);
     Set_otg_custom_timer(VBUSPULSE_40MS);
     Set_otg_custom_timer(VFALLTMOUT_131MS);
     Set_otg_custom_timer(SRPMINDET_100US);
   #endif
   
   switch(g_usb_mode)
   {
      case USB_MODE_DEVICE:
         usb_device_task();
         break;

      case USB_MODE_HOST:
         #if (OTG_ADEV_SRP_REACTION == VBUS_PULSE)
           Usb_select_vbus_srp_method();
         #else
           Usb_select_data_srp_method();
         #endif
         usb_host_task();
         // Handle Vbus overcurrent error (auto-disabled if not supported or not defined in board driver file)
         #if (USB_OTG_FEATURE == ENABLED)
         if (Is_vbus_overcurrent())
         {
           Otg_print_new_event_message(OTGMSG_VBUS_SURCHARGE,OTG_TEMPO_3SEC);
         }
         #endif
         break;

      case USB_MODE_UNDEFINED:  // No break !
      default:
         break;
   }
// -----------------------------------------------------------------------------

// ---- DEVICE ONLY USB MODE ---------------------------------------------------
#elif ((USB_DEVICE_FEATURE == ENABLED)&& (USB_HOST_FEATURE == DISABLED))
   usb_device_task();
// -----------------------------------------------------------------------------

// ---- REDUCED HOST ONLY USB MODE ---------------------------------------------
#elif ((USB_DEVICE_FEATURE == DISABLED)&& (USB_HOST_FEATURE == ENABLED))
   usb_host_task();
// -----------------------------------------------------------------------------

//! ---- ERROR, NO MODE ENABLED -------------------------------------------------
#elif ((USB_DEVICE_FEATURE == DISABLE)&& (USB_HOST_FEATURE == DISABLE))
   #error  at least one of USB_DEVICE_FEATURE or USB_HOST_FEATURE should be enabled
   #error  otherwise the usb task has nothing to do ...
#endif
// -----------------------------------------------------------------------------

#if (USB_OTG_FEATURE == ENABLED)
   Otg_message_task();
#endif
}



//! @brief USB interrupt subroutine
//!
//! This function is called each time a USB interrupt occurs.
//! The following USB DEVICE events are taken in charge:
//! - VBus On / Off
//! - Start Of Frame
//! - Suspend
//! - Wake-Up
//! - Resume
//! - Reset
//! - Start of frame
//!
//! The following USB HOST events are taken in charge:
//! - Device connection
//! - Device Disconnection
//! - Start Of Frame
//! - ID pin change
//! - SOF (or Keep alive in low speed) sent
//! - Wake up on USB line detected
//!
//! The following USB HOST events are taken in charge:
//! - HNP success (Role Exchange)
//! - HNP failure (HNP Error)
//!
//! For each event, the user can launch an action by completing
//! the associate define (See conf_usb.h file to add action upon events)
//!
//! Note: Only interrupts events that are enabled are processed
//!
//! @param none
//!
//! @return none
#ifdef AVRGCC
 ISR(USB_GEN_vect)
#else
//#pragma vector = USB_GENERAL_vect
__interrupt void usb_general_interrupt(void)
#endif
{
   //TRACE_DEBUG("usb_general_interrupt\n\r");
   #if (USB_HOST_PIPE_INTERRUPT_TRANSFER == ENABLE)
   U8 i;
   U8 save_pipe_nb;
   #endif

// ---------- DEVICE events management -----------------------------------
// -----------------------------------------------------------------------
#if ((USB_DEVICE_FEATURE == ENABLED) || (USB_OTG_FEATURE == ENABLED))
  //- VBUS state detection
   if (Is_usb_vbus_transition() && Is_usb_vbus_interrupt_enabled() && Is_usb_id_device())
   {
      Usb_ack_vbus_transition();
      if (Is_usb_vbus_high())
      {
         usb_connected = TRUE;
         Usb_vbus_on_action();
         Usb_send_event(EVT_USB_POWERED);
         Usb_enable_reset_interrupt();
         usb_start_device();
         Usb_attach();
      }
      else
      {
         TRACE_DEBUG("VBUS low\n\r");
         Usb_detach();
         #if  (USB_OTG_FEATURE == ENABLED)
          Usb_device_stop_hnp();
          Usb_select_device();
          Clear_all_user_request();
         #endif
         Usb_vbus_off_action();
         usb_connected = FALSE;
         usb_configuration_nb = 0;
         Usb_send_event(EVT_USB_UNPOWERED);
      }
   }
  // - Device start of frame received
   if (Is_usb_sof() && Is_sof_interrupt_enabled())
   {
      //TRACE_DEBUG("SOF\n\r");
      Usb_ack_sof();
      Usb_sof_action();
      #if (USB_OTG_FEATURE == ENABLED)
      sof_seen_in_session = TRUE;
        #if (OTG_COMPLIANCE_TRICKS == ENABLED)
        otg_last_sof_received = UDFNUML;  // store last frame number received
        #endif
      #endif
   }
  // - Device Suspend event (no more USB activity detected)
   if (Is_usb_suspend() && Is_suspend_interrupt_enabled())
   {
    //jcb  TRACE_DEBUG("Device Suspend event \n\r");
      #if (USB_OTG_FEATURE == ENABLED)
      // 1st : B-PERIPH mode ?
      if (Is_usb_id_device())
      {
        // HNP Handler
        TRACE_DEBUG("HNP Handler\n\r");
        if (Is_host_requested_hnp() // "b_hnp_enable" feature received
            && (Is_session_started_with_srp() || Is_user_requested_hnp() || (OTG_B_DEVICE_AUTORUN_HNP_IF_REQUIRED == ENABLED)))
        {
          if (otg_device_nb_hnp_retry == 0)
          {
            otg_features_supported &= ~OTG_B_HNP_ENABLE;
          }
          else
          {
            Ack_user_request_hnp();
            Usb_ack_hnp_error_interrupt();
            Usb_ack_role_exchange_interrupt();
            Usb_enable_role_exchange_interrupt();
            Usb_enable_hnp_error_interrupt();
            Usb_device_initiate_hnp();
            otg_device_nb_hnp_retry--;
          }
          Usb_ack_suspend();
        }
        else
        {
          // Remote wake-up handler
          TRACE_DEBUG("Remote wake-up handler\n\r");
          if ((remote_wakeup_feature == ENABLED) && (usb_configuration_nb != 0))
          {
            
            Usb_disable_suspend_interrupt();
            Usb_ack_wake_up();
            Usb_enable_wake_up_interrupt();
            // After that user can execute "Usb_initiate_remote_wake_up()" to initiate a remote wake-up
            // Note that the suspend interrupt flag SUSPI must still be set to enable upstream resume
            // So the SUSPE enable bit must be cleared to avoid redundant interrupt
            // ****************
            // Please note also that is Vbus is lost during an upstream resume (Host disconnection),
            // the RMWKUP bit (used to initiate remote wake up and that is normally cleared by hardware when sent)
            // remains set after the event, so that a good way to handle this feature is :
            //            Usb_initiate_remote_wake_up();
            //            while (Is_usb_pending_remote_wake_up())
            //            {
            //              if (Is_usb_vbus_low())
            //              {
            //                // Emergency action (reset macro, etc.) if Vbus lost during resuming
            //                break;
            //              }
            //            }
            //            Usb_ack_remote_wake_up_start();
            // ****************
          }
          else
          {
            // No remote wake-up supported
            Usb_send_event(EVT_USB_SUSPEND);
            Usb_suspend_action();
            Usb_ack_suspend();
            Usb_ack_wake_up();                  // clear wake up to detect next event
            Usb_enable_wake_up_interrupt();
            Usb_freeze_clock();
          }
        }
      }
      else
      {
        // A-PERIPH mode (will cause a session end, handled in usb_host_task.c)
        Usb_send_event(EVT_USB_SUSPEND);
        Usb_suspend_action();
        Usb_ack_suspend();
      }
      #else
      // Remote wake-up handler
      if ((remote_wakeup_feature == ENABLED) && (usb_configuration_nb != 0))
      {
        TRACE_DEBUG("Remote wake-up handler\n\r");
        Usb_disable_suspend_interrupt();
        Usb_ack_wake_up();
        Usb_enable_wake_up_interrupt();
        Usb_suspend_action();
        Usb_freeze_clock();
        // After that user can execute "Usb_initiate_remote_wake_up()" to initiate a remote wake-up
        // Note that the suspend interrupt flag SUSPI must still be set to enable upstream resume
        // So the SUSPE enable bit must be cleared to avoid redundant interrupt
        // ****************
        // Please note also that is Vbus is lost during an upstream resume (Host disconnection),
        // the RMWKUP bit (used to initiate remote wake up and that is normally cleared by hardware when sent)
        // remains set after the event, so that a good way to handle this feature is :
        //            Usb_unfreeze_clock();
        //            Usb_initiate_remote_wake_up();
        //            while (Is_usb_pending_remote_wake_up())
        //            {
        //              if (Is_usb_vbus_low())
        //              {
        //                // Emergency action (reset macro, etc.) if Vbus lost during resuming
        //                break;
        //              }
        //            }
        //            Usb_ack_remote_wake_up_start();
        // ****************
      }
      else
      {
        // No remote wake-up supported
        Usb_send_event(EVT_USB_SUSPEND);
        Usb_suspend_action();
        Usb_ack_suspend();
        Usb_ack_wake_up();                  // clear wake up to detect next event
        Usb_enable_wake_up_interrupt();
        Usb_freeze_clock();
      }
      #endif
    }
   
  // - Wake up event (USB activity detected): Used to resume
   if (Is_usb_wake_up() && Is_swake_up_interrupt_enabled())
   {
      TRACE_DEBUG("W\n\r");
      Usb_unfreeze_clock();
      Usb_ack_wake_up();
      Usb_disable_wake_up_interrupt();
      Usb_ack_suspend();
      Usb_enable_suspend_interrupt();
      Usb_wake_up_action();
      Usb_send_event(EVT_USB_WAKE_UP);
   }
  // - Resume state bus detection
   if (Is_usb_resume() && Is_resume_interrupt_enabled())
   {
      TRACE_DEBUG("Resume state bus detect\n\r");
      Usb_disable_wake_up_interrupt();
      Usb_ack_resume();
      Usb_disable_resume_interrupt();
      Usb_resume_action();
      Usb_send_event(EVT_USB_RESUME);
   }
  // - USB bus reset detection
   if (Is_usb_reset()&& Is_reset_interrupt_enabled())
   {
      TRACE_DEBUG_WP("B\n\r");
      Usb_ack_reset();
      usb_init_device();
      #if (USB_OTG_FEATURE == ENABLED)
        if (Is_usb_id_host())
        {
          usb_configure_endpoint(EP_CONTROL,      \
                                 TYPE_CONTROL,    \
                                 DIRECTION_OUT,   \
                                 SIZE_8,          \
                                 ONE_BANK,        \
                                 NYET_DISABLED);
        }
        #if (OTG_COMPLIANCE_TRICKS == ENABLED)
          // First initialization is important to be synchronized
          // A reset must first have been received
          if (device_state == A_PERIPHERAL)
          {
            otg_last_sof_received = UDFNUML;
            otg_last_sof_stored   = UDFNUML;
            Usb_ack_sof();
            Usb_enable_sof_interrupt();
            reset_received = TRUE;
            Timer16_select(OTG_USE_TIMER);    // reinitialize timer
            Timer16_set_counter(0);
          }
        #endif
      #endif
      Usb_reset_action();
      Usb_send_event(EVT_USB_RESET);
   }

// ---------- OTG events management ------------------------------------
// ---------------------------------------------------------------------
#if (USB_OTG_FEATURE == ENABLED)
  // - OTG HNP Success detection
   if (Is_usb_role_exchange_interrupt() && Is_role_exchange_interrupt_enabled())
   {
     TRACE_DEBUG("OTG HNP detect\n\r");
     Usb_ack_role_exchange_interrupt();
     Host_ack_device_connection();
     Host_ack_device_disconnection();
     Otg_send_event(EVT_OTG_HNP_SUCCESS);
     End_session_with_srp();
     Clear_otg_features_from_host();
     if (Is_usb_id_host())
     {
       // HOST (A- or B-) mode
       if ((device_state != A_PERIPHERAL) && (device_state != A_END_HNP_WAIT_VFALL))
       {
         // Current mode is A-HOST, device will take the A-PERIPHERAL role
         otg_b_device_state = B_PERIPHERAL;
         device_state = A_PERIPHERAL;
         usb_connected = FALSE;
         usb_configuration_nb = 0;
         Usb_select_device();
         Usb_attach();
         Usb_unfreeze_clock();
         Usb_disable_role_exchange_interrupt();
         Usb_disable_hnp_error_interrupt();
         Usb_device_stop_hnp();
         Usb_ack_suspend();
         Usb_ack_reset();
         #if (OTG_COMPLIANCE_TRICKS == ENABLED)
           Timer16_select(OTG_USE_TIMER);     // 4 first lines compensating test error TD4.5-2,9ms
           Timer16_set_counter(0);
           Usb_freeze_clock();                // USB clock can be freezed to slow down events and condition detection
           while (Timer16_get_counter_low() != 20);
           Usb_unfreeze_clock();
           reset_received = FALSE;
           Usb_disable_sof_interrupt();       // will be set in the next OTG Timer IT (mandatory)
         #endif
         Usb_enable_suspend_interrupt();
         Usb_enable_reset_interrupt();
         usb_configure_endpoint(EP_CONTROL,    \
                                TYPE_CONTROL,  \
                                DIRECTION_OUT, \
                                SIZE_8,        \
                                ONE_BANK,      \
                                NYET_DISABLED);
       }
     }
     else
     {  // In B_HOST mode, the HNPREQ bit must not be cleared because it releases the bus in suspend mode (and sof can't start)
       if ((otg_b_device_state != B_HOST) && (otg_b_device_state != B_END_HNP_SUSPEND))
       {
         // Current mode is B-PERIPHERAL, device will go into B-HOST role
         End_session_with_srp();
         Clear_otg_features_from_host();
         otg_b_device_state = B_HOST;
         device_state = DEVICE_ATTACHED;
         usb_connected = FALSE;
         usb_configuration_nb = 0;
         Usb_select_host();
         Host_enable_sof();     // start Host (sof)
         Host_send_reset();     // send the first RESET
         Host_disable_reset_interrupt();
         while (Host_is_reset());
         i = 2;
         while (i != 1)
         {
           i++;
         };
         while (i < 5)  // generates 4 others RESET
         {
           while (Host_is_reset());
           Host_send_reset();
           Host_ack_reset();
           i++;
         }

         Usb_disable_role_exchange_interrupt();
         Usb_disable_hnp_error_interrupt();
         Clear_all_user_request();
       }
     }
   }
  // - OTG HNP Failure detection
   if (Is_usb_hnp() && Is_usb_hnp_error_interrupt()&& Is_hnp_error_interrupt_enabled())
   {
     TRACE_DEBUG("OTG HNP failure\n\r");
     Usb_device_stop_hnp();
     Usb_disable_role_exchange_interrupt();
     Usb_disable_hnp_error_interrupt();
     Usb_ack_hnp_error_interrupt();
     if (Is_usb_id_device())
     {
       Otg_send_event(EVT_OTG_HNP_ERROR);
       Clear_all_user_request();
     }
   }
#endif
#endif// End DEVICE FEATURE MODE

// ---------- HOST events management -----------------------------------
// ---------------------------------------------------------------------
#if (((USB_HOST_FEATURE == ENABLED) && (USB_DEVICE_FEATURE == ENABLED)) || (USB_OTG_FEATURE == ENABLED))
  // - ID pin change detection
   if(Is_usb_id_transition()&&Is_usb_id_interrupt_enabled())
   {
      TRACE_DEBUG("ID pin change\n\r");
      Usb_device_stop_hnp();
      #if (USB_OTG_FEATURE == ENABLED)
      Clear_all_user_request();
      #endif
      if(Is_usb_id_device())
      { g_usb_mode=USB_MODE_DEVICE;}
      else
      { g_usb_mode=USB_MODE_HOST;}
      Usb_ack_id_transition();
      if( g_usb_mode != g_old_usb_mode) // Basic Debounce
      {
         if(Is_usb_id_device()) // Going into device mode
         {
            Usb_send_event(EVT_USB_DEVICE_FUNCTION);
            #if (USB_OTG_FEATURE == ENABLED)
              otg_b_device_state = B_IDLE;
            #endif
            device_state = DEVICE_UNATTACHED;
            #if ((OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED) && (USB_OTG_FEATURE == ENABLED))
              id_changed_to_host_event = DISABLED;
            #endif
         }
         else                   // Going into host mode
         {
           #if (USB_OTG_FEATURE == ENABLED)
             otg_b_device_state = B_IDLE;
           #endif
           device_state = DEVICE_UNATTACHED;
           Usb_send_event(EVT_USB_HOST_FUNCTION);
           #if ((OTG_VBUS_AUTO_AFTER_A_PLUG_INSERTION == ENABLED) && (USB_OTG_FEATURE == ENABLED))
           id_changed_to_host_event = ENABLED;
           #endif
          }
         Usb_id_transition_action();
         LOG_STR_CODE(log_id_change);
         #if ( ID_PIN_CHANGE_GENERATE_RESET == ENABLE)
        // Hot ID transition generates wdt reset
            #ifndef  AVRGCC
//               Wdt_change_16ms(); while(1);
            #else
//               Wdt_change_enable(); while(1);
            #endif
         #endif
      }
   }
#endif
#if ((USB_HOST_FEATURE == ENABLED) || (USB_OTG_FEATURE == ENABLED))
  // - The device has been disconnected
   if(Is_device_disconnection() && Is_host_device_disconnection_interrupt_enabled())
   {
      TRACE_DEBUG("device disconnect\n\r");
      host_disable_all_pipe();
      Host_ack_device_disconnection();
      device_state=DEVICE_DISCONNECTED;
      Usb_send_event(EVT_HOST_DISCONNECTION);
      LOG_STR_CODE(log_device_disconnect);
      Host_device_disconnection_action();
      #if (USB_OTG_FEATURE == ENABLED)
       Clear_all_user_request();
      #endif
   }
  // - Device connection
   if(Is_device_connection() && Is_host_device_connection_interrupt_enabled())
   {
      TRACE_DEBUG("device connect\n\r");
      Host_ack_device_connection();
      host_disable_all_pipe();
      Host_device_connection_action();
   }
  // - Host Start of frame has been sent
   if (Is_host_sof() && Is_host_sof_interrupt_enabled())
   {
      //TRACE_DEBUG("Host SOF ");
      Host_ack_sof();
      Usb_send_event(EVT_HOST_SOF);
      private_sof_counter++;

      // delay timeout management for interrupt tranfer mode in host mode
      #if ((USB_HOST_PIPE_INTERRUPT_TRANSFER==ENABLE) && (TIMEOUT_DELAY_ENABLE==ENABLE))
      if (private_sof_counter>=250)   // Count 1/4 sec
      {
         private_sof_counter=0;
         for(i=0;i<MAX_EP_NB;i++)
         {
            if(it_pipe_str[i].enable==ENABLE)
            {
               save_pipe_nb=Host_get_selected_pipe();
               Host_select_pipe(i);
               if((++it_pipe_str[i].timeout>TIMEOUT_DELAY) && (Host_get_pipe_type()!=TYPE_INTERRUPT))
               {
                  it_pipe_str[i].enable=DISABLE;
                  it_pipe_str[i].status=PIPE_DELAY_TIMEOUT;
                  Host_stop_pipe_interrupt(i);
                  if (is_any_interrupt_pipe_active()==FALSE)    // If no more transfer is armed
                  {
                     if (g_sav_int_sof_enable==FALSE)
                     {
                        Host_disable_sof_interrupt();
                     }
                  }
                  it_pipe_str[i].handle(PIPE_DELAY_TIMEOUT,it_pipe_str[i].nb_byte_processed);
               }
               Host_select_pipe(save_pipe_nb);
            }
         }
      }
      #endif  // (USB_HOST_PIPE_INTERRUPT_TRANSFER==ENABLE) && (TIMEOUT_DELAY_ENABLE==ENABLE))
      Host_sof_action();
   }
  // - Host Wake-up has been received
   if (Is_host_hwup() && Is_host_hwup_interrupt_enabled())
   {
      TRACE_DEBUG("Host wake up\n\r");
      Host_disable_hwup_interrupt();  // Wake up interrupt should be disable host is now wake up !
      Host_disable_remote_wakeup_interrupt();
      // CAUTION HWUP can be cleared only when USB clock is active (not frozen)!
//      Pll_start_auto();               // First Restart the PLL for USB operation
//      Wait_pll_ready();               // Get sure pll is lock
      Usb_unfreeze_clock();           // Enable clock on USB interface
      Host_enable_sof();              // start sending SOF
      Host_ack_hwup();                // Clear HWUP interrupt flag
      Host_ack_remote_wakeup();
      Usb_send_event(EVT_HOST_HWUP);  // Send software event
      Usb_send_event(EVT_HOST_REMOTE_WAKEUP);
      Host_hwup_action();             // Map custom action
      #if (USB_OTG_FEATURE == ENABLED)
        if (Is_usb_hnp())
        {
          Usb_host_reject_hnp();
          Usb_disable_hnp_error_interrupt();
          Usb_disable_role_exchange_interrupt();
        }
      #endif
     Host_send_resume();
   }

   // Remote Wake Up has been received
   if (Is_host_remote_wakeup_interrupt_enabled() && Is_host_remote_wakeup())
   {
     TRACE_DEBUG("Remote wake up\n\r");
     Host_disable_remote_wakeup_interrupt();
     Host_disable_hwup_interrupt();
     Host_ack_remote_wakeup();
     Host_ack_hwup();                // Clear HWUP interrupt flag
//     Pll_start_auto();               // First Restart the PLL for USB operation
//     Wait_pll_ready();               // Get sure pll is lock
     Usb_unfreeze_clock();           // Enable clock on USB interface
     Host_enable_sof();     // start sending SOF
     Usb_send_event(EVT_HOST_REMOTE_WAKEUP);
     Usb_send_event(EVT_HOST_HWUP);  // Send software event
     #if (USB_OTG_FEATURE == ENABLED)
       if (Is_usb_hnp())
       {
         Usb_host_reject_hnp();
         Usb_disable_hnp_error_interrupt();
         Usb_disable_role_exchange_interrupt();
       }
     #endif
     Host_send_resume();
   }
#endif // End HOST FEATURE MODE

}


#if (USB_OTG_FEATURE == ENABLED)

// ---------- OTG Timings management -----------------------------------
// ---------------------------------------------------------------------
//! @brief OTG TIMER interrupt subroutine
//!
//! This function is called each time a OTG Timer interrupt occurs (every 2ms @ 8 MHz)
//! Function decrements the variables required by OTG program
//!
//! @param none
//!
//! @return none
#ifdef AVRGCC
  #if (OTG_USE_TIMER == TIMER16_1)
    ISR(TIMER1_COMPA_vect)
  #else
    ISR(TIMER3_COMPA_vect)
  #endif
#else
/*
  #if (OTG_USE_TIMER == TIMER16_1)
    #pragma vector = TIMER1_COMPA_vect
  #else
    //#pragma vector = TIMER3_COMPA_vect
  #endif
*/
__interrupt void otg_timer_interrupt(void)
#endif
{
   TRACE_DEBUG("otg_timer_interrupt\n\r");
//#if (USE_TIMER16 == BOTH_TIMER16)
//  U8 tmr_sel_save = timer16_selected;     // save timer currently selected
//#endif

//  Timer16_select(OTG_USE_TIMER);
//  Timer16_clear_compare_a_it();

  //! OTG Messaging timer
#if ((OTG_MESSAGING_OUTPUT == OTGMSG_ALL) || (OTG_MESSAGING_OUTPUT == OTGMSG_FAIL))
  if ((Get_failure_msg_delay() != 0x0000) && (Get_failure_msg_delay() != 0xFFFF))   { Decrement_failure_msg_delay(); }
  #if (OTG_MESSAGING_OUTPUT == OTGMSG_ALL)
    if ((Get_event_msg_delay() != 0x0000) && (Get_event_msg_delay() != 0xFFFF))       { Decrement_event_msg_delay(); }
  #endif
#endif

  //! Increments Tb_Srp counter if needed
  if (Is_srp_sent_and_waiting_answer())         { otg_tb_srp_cpt++; }

  //! Increments T_vbus_wait_connect if needed
  if (Is_srp_received_and_waiting_connect())    { otg_ta_srp_wait_connect++; }

  //! Decrements Ta_aidl_bdis timer if needed (A_suspend state)
  if ((device_state == A_SUSPEND) && (otg_ta_aidl_bdis_tmr > 1))
                                                { otg_ta_aidl_bdis_tmr--; }

  //! Decrements Timeout_bdev_respond timer if needed
  if ((device_state == DEVICE_DEFAULT) && (!Is_timeout_bdev_response_overflow()))
                                                { otg_timeout_bdev_respond--; }

  //! Decrements Ta_vbus_rise timer if needed
  if (!Is_ta_vbus_rise_counter_overflow())      { otg_ta_vbus_rise--; }
  
  //! Decrements Ta_vbus_fall timer if needed  
  if (!Is_ta_vbus_fall_counter_overflow())      { otg_end_hnp_vbus_delay--; }
  
  
  
  //! Needed for compliance only
  #if (OTG_COMPLIANCE_TRICKS == ENABLED)
 
  if (device_state == A_PERIPHERAL)
  {
    if (Is_sof_interrupt_enabled() && (reset_received == TRUE))
    {
      if (otg_last_sof_stored != otg_last_sof_received)
      {
        // No SOF is missing
        otg_last_sof_received = otg_last_sof_stored;
      }
      else
      {
        // SOF seems to be missing..
        Usb_freeze_clock();
        Usb_disable_sof_interrupt();
        reset_received = FALSE;
        while (Timer16_get_counter_low() != 20);  // overflow set to 62 in usb_task.h
        Usb_unfreeze_clock();
      }
      otg_last_sof_received = UDFNUML;
      otg_last_sof_stored = UDFNUML;
    }
  }
  #endif
  
//#if (USE_TIMER16 == BOTH_TIMER16)
//  Timer16_select(tmr_sel_save);        // restore timer that was selected before entering IT
//#endif
}


// ---------- OTG Messaging management ---------------------------------
// ---------------------------------------------------------------------
//! @brief OTG Messaging task initialization
//!
//! Initializes variables and screen to prepare next messages to be handled
//! First version of this function works on SPIDER Board and needs for the 3 lower lines of the LCD
//!
//! @param none
//!
//! @return none
void Otg_message_task_init(void)
{
   TRACE_DEBUG("Otg_message_task_init\n\r");
  Otg_messaging_init();
  otg_msg_event_delay = 0;
  otg_msg_failure_delay = 0;
}


//! @brief OTG Messaging main task
//!
//! OTG specifies that user must be kept informed of several events
//! This task allows user to display two kinds of messages : EVENT or FAILURE
//! For each new message, it can specify if the message remains displayed all the time or only during a specified delay
//!
//! @param none
//!
//! @return none
void Otg_message_task(void)
{
  // Check if an OTG message must be erased (if it was set up for a specified delay)
#if ((OTG_MESSAGING_OUTPUT == OTGMSG_ALL) || (OTG_MESSAGING_OUTPUT == OTGMSG_FAIL))
  if (Get_failure_msg_delay() == 0)  { Otg_clear_failure_message(); }
  #if (OTG_MESSAGING_OUTPUT == OTGMSG_ALL)
    if (Get_event_msg_delay() == 0)    { Otg_clear_event_message();   }
  #endif
#endif
}
#endif




extern void suspend_action(void)
{
   TRACE_DEBUG("suspend_action\n\r");
   Enable_interrupt();
//   Enter_power_down_mode();
}

extern void host_suspend_action(void)
{
   TRACE_DEBUG("host_suspend_action\n\r");
   //Enter_power_down_mode();  //For example...
}

void otg_not_supported_device(void)
{
   TRACE_DEBUG("otg_not_supported_device\n\r");
  Otg_send_event(EVT_OTG_DEV_UNSUPPORTED);
}

