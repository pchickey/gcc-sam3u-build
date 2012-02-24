//! @file usb_host_enum.h,v
//!
//! Copyright (c) 2004 Atmel.
//!
//! Use of this program is subject to Atmel's End User License Agreement.
//! Please read file license.txt for copyright notice.
//!
//! @brief USB host enumeration process header file
//!
//! This file contains the USB pipe 0 management routines corresponding to
//! the standard enumeration process (refer to chapter 9 of the USB
//! specification.
//!
//! @version 1.12 at90usb128-otg-dual_role-toggle-1_0_0 $Id: usb_host_enum.h,v 1.12 2007/02/13 10:14:12 arobert Exp $
//!
//! @todo
//! @bug

#ifndef _USB_HOST_ENUM_H_
#define _USB_HOST_ENUM_H_

//_____ I N C L U D E S ____________________________________________________


#include "usb/otg/usb_task.h"

//_____ M A C R O S ________________________________________________________

#ifndef SIZEOF_DATA_STAGE
   #error SIZEOF_DATA_STAGE should be defined in conf_usb.h
#endif

#if (SIZEOF_DATA_STAGE<0xFF)     //! Optimize descriptor offset index according to data_stage[] size
   #define T_DESC_OFFSET   U8    //! U8 is enought and faster
#else
   #define T_DESC_OFFSET   U16   //! U16 required !
#endif

#ifndef MAX_EP_PER_INTERFACE
   #define MAX_EP_PER_INTERFACE 4
#endif

#define BIT_SELF_POWERED   6  // offset
#define BIT_REMOTE_WAKEUP  5  // offset

#define BIT_SRP_SUPPORT    0  // offset
#define BIT_HNP_SUPPORT    1  // offset


//_____ S T A N D A R D    D E F I N I T I O N S ___________________________

//! @defgroup host_enum USB host enumeration functions module
//! @{

//! Usb Setup Data
typedef struct
{
   U8      bmRequestType;        //!< Characteristics of the request
   U8      bRequest;             //!< Specific request
   U16     wValue;               //!< field that varies according to request
   U16     wIndex;               //!< field that varies according to request
   U16     wLength;              //!< Number of bytes to transfer if Data
   U8      uncomplete_read;      //!< 1 = only one read
}  S_usb_setup_data;


typedef struct
{
   U8  interface_nb;
   U8  altset_nb;
   U16 class;
   U16 subclass;
   U16 protocol;
   U8  nb_ep;
   U8  ep_addr[MAX_EP_PER_INTERFACE];
} S_interface;

extern  S_usb_setup_data usb_request;
extern  U8 data_stage[SIZEOF_DATA_STAGE];
extern  U8 device_status;

#define REQUEST_TYPE_POS         0
#define REQUEST_POS              1
#define VALUE_HIGH_POS           2
#define VALUE_LOW_POS            3
#define INDEX_HIGH_POS           4
#define INDEX_LOW_POS            5
#define LENGTH_HIGH_POS          6
#define LENGTH_LOW_POS           7
#define UNCOMPLETE_READ_POS      8
#define DATA_ADDR_HIGH_POS       9
#define DATA_ADDR_LOW_POS       10

#define CONTROL_GOOD             0
#define CONTROL_DATA_TOGGLE   0x01
#define CONTROL_DATA_PID      0x02
#define CONTROL_PID           0x04
#define CONTROL_TIMEOUT       0x08
#define CONTROL_CRC16         0x10
#define CONTROL_STALL         0x20
#define CONTROL_NO_DEVICE     0x40


//!< Set of defines for offset in data stage
#define OFFSET_FIELD_MAXPACKETSIZE     7
#define OFFSET_FIELD_MSB_VID           9
#define OFFSET_FIELD_LSB_VID           8
#define OFFSET_FIELD_MSB_PID           11
#define OFFSET_FIELD_LSB_PID           10

#define OFFSET_DESCRIPTOR_LENGHT       0
#define OFFSET_FIELD_DESCRIPTOR_TYPE   1
#define OFFSET_FIELD_TOTAL_LENGHT      2
#define OFFSET_FIELD_BMATTRIBUTES      7
#define OFFSET_FIELD_MAXPOWER          8

#define OFFSET_FIELD_OTG_FEATURES      2

//! OFFSET for INTERFACE DESCRIPTORS
#define OFFSET_FIELD_NB_INTERFACE      4
#define OFFSET_FIELD_CLASS             5
#define OFFSET_FIELD_SUB_CLASS         6
#define OFFSET_FIELD_PROTOCOL          7

#define OFFSET_FIELD_INTERFACE_NB      2
#define OFFSET_FIELD_ALT               3
#define OFFSET_FIELS_NB_OF_EP          4

#define OFFSET_FIELD_EP_ADDR           2
#define OFFSET_FIELD_EP_TYPE           3
#define OFFSET_FIELD_EP_SIZE           4
#define OFFSET_FIELD_EP_INTERVAL       6

//! defines for Hub detection
#define OFFSET_DEV_DESC_CLASS          4    // offset for the CLASS field in the Device Descriptor
#define HUB_CLASS_CODE                 9    // value of Hub CLASS


#define HOST_FALSE                     0
#define HOST_TRUE                      1

U8 host_send_control(U8*);

/**
 * host_clear_endpoint_feature
 *
 * @brief this function send a clear endpoint request
 *
 * @param U8 ep (the target endpoint nb)
 *
 * @return status
 */
#define host_clear_endpoint_feature(ep)   (usb_request.bmRequestType = 0x02,\
                                           usb_request.bRequest      = CLEAR_FEATURE,\
                                           usb_request.wValue        = FEATURE_ENDPOINT_HALT << 8,\
                                           usb_request.wIndex        = ep,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
/**
 * host_get_configuration
 *
 * @brief this function send a get configuration request
 *
 * @param none
 *
 * @return status
 */
#define host_get_configuration()          (usb_request.bmRequestType = 0x80,\
                                           usb_request.bRequest      = GET_CONFIGURATION,\
                                           usb_request.wValue        = 0,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 1,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
/**
 * host_set_configuration
 *
 * @brief this function send a set configuration request
 *
 * @param U8 configuration numer to activate
 *
 * @return status
 */
#define host_set_configuration(cfg_nb)    (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_CONFIGURATION,\
                                           usb_request.wValue        = cfg_nb,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
/**
 * host_set_interface
 *
 * @brief this function send a set interface request
 * to specify a specific alt setting for an interface
 *
 * @param U8 interface_nb (the interface)
 *        U8 alt_setting (the alternate setting to activate)
 *
 * @return status
 */
#define host_set_interface(interface_nb,alt_setting)        (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_INTERFACE,\
                                           usb_request.wValue        = alt_setting,\
                                           usb_request.wIndex        = interface_nb,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

/**
 * host_get_device_descriptor_uncomplete
 *
 * @brief this function send a get device desriptor request.
 * The descriptor table received is stored in data_stage array.
 * The received descriptors is limited to the control pipe lenght
 *
 *
 * @param none
 *
 *
 * @return status
 */
#define host_get_device_descriptor_uncomplete()  (usb_request.bmRequestType = 0x80,\
                                           usb_request.bRequest      = GET_DESCRIPTOR,\
                                           usb_request.wValue        = DEVICE_DESCRIPTOR << 8,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 64,\
                                           usb_request.uncomplete_read = TRUE,\
                                           host_send_control(data_stage))

/**
 * host_get_device_descriptor
 *
 * @brief this function send a get device desriptor request.
 * The descriptor table received is stored in data_stage array.
 *
 *
 * @param none
 *
 *
 * @return status
 */
#define host_get_device_descriptor()      (usb_request.bmRequestType = 0x80,\
                                           usb_request.bRequest      = GET_DESCRIPTOR,\
                                           usb_request.wValue        = DEVICE_DESCRIPTOR << 8,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 18,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
/**
 * host_get_configuration_descriptor
 *
 * @brief this function send a get device configuration request.
 * The configuration descriptor table received is stored in data_stage array.
 *
 *
 * @param none
 *
 *
 * @return status
 */
#define host_get_configuration_descriptor()  (usb_request.bmRequestType = 0x80,\
                                           usb_request.bRequest      = GET_DESCRIPTOR,\
                                           usb_request.wValue        = CONFIGURATION_DESCRIPTOR << 8,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 255,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

#define host_get_descriptor_uncomplete()  (usb_request.bmRequestType = 0x80,\
                                           usb_request.bRequest      = GET_DESCRIPTOR,\
                                           usb_request.wValue        = 0,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 64,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
/**
 * host_set_address
 *
 * @brief this function send a set address request.
 *
 *
 * @param U8 address (the addr attributed to the device)
 *
 *
 * @return status
 */
#define host_set_address(addr)            (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_ADDRESS,\
                                           usb_request.wValue        = (U16)addr,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

/**
 * host_set_feature_remote_wakeup
 *
 * @brief this function send a set feature device remote wakeup
 *
 * @param none
 *
 * @return status
 */
#define host_set_feature_remote_wakeup()   (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_FEATURE,\
                                           usb_request.wValue        = 1,\
                                           usb_request.wIndex        = 1,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

#if (USB_OTG_FEATURE == ENABLED)
/**
 * host_set_feature_a_hnp_support
 *
 * @brief This function send a set feature "a_hnp_support" to tell to B-Device that A-Device support HNP
 *
 * @param none
 *
 * @return status
 */
#define host_set_feature_a_hnp_support()   (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_FEATURE,\
                                           usb_request.wValue        = 4,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

/**
 * host_set_feature_b_hnp_enable
 *
 * @brief This function send a set feature "b_hnp_enable" to make B-Device initiating a HNP
 *
 * @param none
 *
 * @return status
 */
#define host_set_feature_b_hnp_enable()    (usb_request.bmRequestType = 0x00,\
                                           usb_request.bRequest      = SET_FEATURE,\
                                           usb_request.wValue        = 3,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
#endif


/**
 * host_ms_get_max_lun
 *
 * @brief this function send the mass storage specific request "get max lun"
 *
 *
 * @param none
 *
 *
 * @return status
 */
#define host_ms_get_max_lun()             (usb_request.bmRequestType = 0xA1,\
                                           usb_request.bRequest      = MS_GET_MAX_LUN,\
                                           usb_request.wValue        = 0,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 1,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

/**
 * Get_VID
 *
 * @brief this function returns the VID of the device connected
 *
 * @param none
 *
 *
 * @return U16 (VID value)
 */
#define Get_VID()      (device_VID)

/**
 * Get_PID
 *
 * @brief this function returns the PID of the device connected
 *
 * @param none
 *
 *
 * @return U16 (PID value)
 */
#define Get_PID()      (device_PID)

/**
 * Get_maxpower
 *
 * @brief this function returns the maximum power consumption ot hte connected device (unit is 2mA)
 *
 * @param none
 *
 *
 * @return U8 (maxpower value)
 */
#define Get_maxpower()      (maxpower)

/**
 * @brief this function returns the USB class associated to the specified interface
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U16 (CLASS code)
 */
#define Get_class(s_interface)      (interface_supported[s_interface].class)

/**
 * @brief this function returns the USB subclass associated to the specified interface
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U16 (SUBCLASS code)
 */
#define Get_subclass(s_interface)      (interface_supported[s_interface].subclass)

/**
 * @brief this function returns the USB protocol associated to the specified interface
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U16 (protocol code)
 */
#define Get_protocol(s_interface)      (interface_supported[s_interface].protocol)

/**
 * @brief this function returns endpoint address associated to the specified interface and
 * endpoint number in this interface.
 *
 * @param U8 s_interface: the supported interface number
 * @param U8 n_ep: the endpoint number in this interface
 *
 * @return U8 (endpoint address)
 */
#define Get_ep_addr(s_interface,n_ep)      (interface_supported[s_interface].ep_addr[n_ep])

/**
 * @brief this function returns number of endpoints associated to
 * a supported interface.
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U8 (number of enpoints)
 */
#define Get_nb_ep(s_interface)      (interface_supported[s_interface].nb_ep)

/**
 * @brief this function returns number of the alternate setting field associated to
 * a supported interface.
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U8 (number of alt setting value)
 */
#define Get_alts_s(s_interface)      (interface_supported[s_interface].altset_nb)

/**
 * @brief this function returns number of the interface number associated to
 * a supported interface.
 *
 * @param U8 s_interface: the supported interface number
 *
 * @return U8 (number of the interface)
 */
#define Get_interface_number(s_interface)      (interface_supported[s_interface].interface_nb)

/**
 * @brief this function returns the number of interface supported in the device connected
 *
 * @param none
 *
 * @return U8 : The number of interface
 */
#define Get_nb_supported_interface()      (nb_interface_supported)

/**
 * @brief this function returns true if the device connected is self powered
 *
 * @param none
 *
 * @return U8 : The number of interface
 */
#define Is_device_self_powered()      ((bmattributes & (1<<BIT_SELF_POWERED)) ? TRUE : FALSE)

/**
 * @brief this function returns true if the device supports remote wake_up
 *
 * @param none
 *
 * @return U8 : The number of interface
 */
#define Is_device_supports_remote_wakeup()  ((bmattributes & (1<<BIT_REMOTE_WAKEUP)) ? TRUE : FALSE)

/**
 * @brief this function returns true if the device supports SRP
 *
 * @param none
 *
 * @return U8 : SRP status
 */
#define Is_device_supports_srp()    ((otg_features_supported & (1<<BIT_SRP_SUPPORT)) ? TRUE : FALSE)

/**
 * @brief this function returns true if the device supports HNP
 *
 * @param none
 *
 * @return U8 : HNP status
 */
#define Is_device_supports_hnp()    ((otg_features_supported & (1<<BIT_HNP_SUPPORT)) ? TRUE : FALSE)


extern U8 nb_interface_supported;
extern S_interface interface_supported[MAX_INTERFACE_SUPPORTED];
extern U16 device_PID;
extern U16 device_VID;
extern U8 bmattributes;
extern U8 maxpower;

U8 host_check_VID_PID(void);
#if (USB_OTG_FEATURE == ENABLED)
  U8 host_check_OTG_features(void);
#endif
U8 host_check_class  (void);
U8 host_auto_configure_endpoint();
T_DESC_OFFSET get_interface_descriptor_offset(U8 interface, U8 alt);
U8 host_get_hwd_pipe_nb(U8 ep_addr);

//! @}

#endif  // _USB_HOST_ENUM_H_

