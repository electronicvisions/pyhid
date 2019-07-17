//-----------------------------------------------------------------
//
// Copyright (c) 2014 TU-Dresden  All rights reserved.
//
// Unless otherwise stated, the software on this site is distributed
// in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. THERE IS NO WARRANTY FOR THE SOFTWARE,
// TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE
// STATED IN WRITING THE COPYRIGHT HOLDERS PROVIDE THE SOFTWARE
// "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE
// RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE IS WITH YOU.
// SHOULD THE SOFTWARE PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL
// NECESSARY SERVICING, REPAIR OR CORRECTION. IN NO EVENT UNLESS
// REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY
// COPYRIGHT HOLDER, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
// GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT
// OF THE USE OR INABILITY TO USE THE SOFTWARE (INCLUDING BUT NOT
// LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES
// SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE SOFTWARE TO
// OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER HAS BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//-----------------------------------------------------------------

// Company           :   TU-Dresden
// Author            :   Stephan Hartmann
// E-Mail            :   hartmann@iee.et.tu-dresden.de
//
// Filename          :   hid_libusb.hpp
// Project Name      :   PyHID
// Description       :   HID using libusb-1.0
//-----------------------------------------------------------------
// Version   |  Author    |  Date         |  Comment
// ----------------------------------------------------------------
// 0.1       | hartmann   | 19 Jun 2013   |  initial version
// ----------------------------------------------------------------
#ifndef __HID_LIBUSB_HPP__
#define __HID_LIBUSB_HPP__

#include <stdint.h>
#include <cstddef>
#include <string>

#define HID_LIBUSB_INVALID_ARGS   -1000
#define HID_LIBUSB_NO_DEVICE      -1001
#define HID_LIBUSB_NO_DEVICE_OPEN -1002
#define HID_LIBUSB_READ_ERROR     -1003
#define HID_LIBUSB_NO_UDEV        -1004
#define HID_LIBUSB_UDEV_MON_ERROR -1005
#define HID_LIBUSB_UDEV_TIMEOUT   -1006
#define HID_LIBUSB_NO_LIBUSB      -1007

typedef struct hid_device_info
{
  uint16_t                uiVendorID;
  uint16_t                uiProductID;
  char                   *szSerial;
  uint16_t                uiReleaseNumber;
  char                   *szManufacturer;
  char                   *szProduct;
  int32_t                 iInterfaceNumber;
  uint16_t                uiBusNumber;
  uint16_t                uiDeviceAddress;
  struct hid_device_info *pNext;
} hid_device_info_t;

typedef struct input_report
{
  uint8_t             *puiData;
  size_t               uiLength;
  struct input_report *pNext;
} input_report_t;

#include <pthread.h>
#include <libusb.h>

class libusb_wrapper
{
private:
  typedef int     (*libusbInit_t)(libusb_context **);
  typedef void    (*libusbExit_t)(libusb_context *);
  typedef ssize_t (*libusbGetDeviceList_t)(libusb_context *, libusb_device ***);
  typedef void    (*libusbFreeDeviceList_t)(libusb_device **, int);
  typedef int     (*libusbGetDeviceDescriptor_t)(libusb_device *,
                                                 libusb_device_descriptor *);
  typedef int     (*libusbGetActiveConfigDescriptor_t)(
                              libusb_device *,
                              struct libusb_config_descriptor **);
  typedef int     (*libusbGetConfigDescriptor_t)(
                              libusb_device *, uint8_t,
                              struct libusb_config_descriptor **);
  typedef void    (*libusbFreeConfigDescriptor_t)(
                              struct libusb_config_descriptor *);
  typedef int     (*libusbOpen_t)(libusb_device *, libusb_device_handle **);
  typedef void    (*libusbClose_t)(libusb_device_handle *);
  typedef uint8_t (*libusbGetBusNumber_t)(libusb_device *);
  typedef uint8_t (*libusbGetDeviceAddress_t)(libusb_device *);
  typedef int     (*libusbAttachKernelDriver_t)(libusb_device_handle *,
                                                int);
  typedef int     (*libusbDetachKernelDriver_t)(libusb_device_handle *,
                                                int);
  typedef int     (*libusbKernelDriverActive_t)(libusb_device_handle *,
                                                int);
  typedef int     (*libusbClaimInterface_t)(libusb_device_handle *, int);
  typedef int     (*libusbReleaseInterface_t)(libusb_device_handle *, int);
  typedef int     (*libusbGetStringDescriptorAscii_t)(libusb_device_handle *,
                                                      uint8_t,
                                                      unsigned char *, int);
  typedef int     (*libusbHandleEvents_t)(libusb_context *);
  typedef struct libusb_transfer * (*libusbAllocTransfer_t)(int);
  typedef void    (*libusbFreeTransfer_t)(struct libusb_transfer *);
  typedef int     (*libusbSubmitTransfer_t)(struct libusb_transfer *);
  typedef int     (*libusbCancelTransfer_t)(struct libusb_transfer *);
  typedef int     (*libusbControlTransfer_t)(libusb_device_handle *,
                                             uint8_t, uint8_t, uint16_t,
                                             uint16_t, unsigned char *,
                                             uint16_t, unsigned int);
  typedef int     (*libusbInterruptTransfer_t)(libusb_device_handle *,
                                               unsigned char,
                                               unsigned char *, int,
                                               int *, unsigned int);
  typedef const char * (*libusbStrerror_t)(enum libusb_error);

  static const char *usbi_errors[];
  static const char *libusb_strerror(enum libusb_error);

  void *m_pLib;
  void closeUSBLib();
  libusb_wrapper();
public:
  ~libusb_wrapper();
  static libusb_wrapper &getInstance();
  bool loadUSBLib();

  libusbInit_t                      libusbInit;
  libusbExit_t                      libusbExit;
  libusbGetDeviceList_t             libusbGetDeviceList;
  libusbFreeDeviceList_t            libusbFreeDeviceList;
  libusbGetDeviceDescriptor_t       libusbGetDeviceDescriptor;
  libusbGetActiveConfigDescriptor_t libusbGetActiveConfigDescriptor;
  libusbGetConfigDescriptor_t       libusbGetConfigDescriptor;
  libusbFreeConfigDescriptor_t      libusbFreeConfigDescriptor;
  libusbOpen_t                      libusbOpen;
  libusbClose_t                     libusbClose;
  libusbGetBusNumber_t              libusbGetBusNumber;
  libusbGetDeviceAddress_t          libusbGetDeviceAddress;
  libusbAttachKernelDriver_t        libusbAttachKernelDriver;
  libusbDetachKernelDriver_t        libusbDetachKernelDriver;
  libusbKernelDriverActive_t        libusbKernelDriverActive;
  libusbClaimInterface_t            libusbClaimInterface;
  libusbReleaseInterface_t          libusbReleaseInterface;
  libusbGetStringDescriptorAscii_t  libusbGetStringDescriptorAscii;
  libusbHandleEvents_t              libusbHandleEvents;
  libusbAllocTransfer_t             libusbAllocTransfer;
  libusbFreeTransfer_t              libusbFreeTransfer;
  libusbSubmitTransfer_t            libusbSubmitTransfer;
  libusbCancelTransfer_t            libusbCancelTransfer;
  libusbControlTransfer_t           libusbControlTransfer;
  libusbInterruptTransfer_t         libusbInterruptTransfer;
  libusbStrerror_t                  libusbStrerror;
};

class hid_libusb
{
private:

  typedef class hid_libusb self_type_t;

  static libusb_context  *m_pContext;
  libusb_device_handle   *m_pDeviceHandle;
  input_report_t         *m_pInputReports;
  bool                    m_bShutdownThread;
  pthread_barrier_t       m_Barrier;
  pthread_cond_t          m_Condition;
  pthread_mutex_t         m_Mutex;
  pthread_t               m_Thread;
  size_t                  m_uiMaxPacketSize;
  int32_t                 m_iInputEndpoint;
  int32_t                 m_iOutputEndpoint;
  int32_t                 m_iInterface;
  bool                    m_bOpenDevice;
  bool                    m_bDetachedKernel;
  char                   *m_szUdevPath;
  char                    m_szVendorID[5];
  char                    m_szProductID[5];
  char                    m_szBusNum[4];
  char                    m_szDevAddr[4];
  hid_device_info_t      *m_pDevices;
  struct udev            *m_pUdev;
  struct libusb_transfer *m_pTransfer;

  static char *getUSBString(libusb_device_handle *, const uint8_t);
  static void readCallback(struct libusb_transfer *);
  static void *readThread(void *);
  static void cleanupMutex(void *);
  static void freeHID();
  bool findUdevPath();
  int returnData(uint8_t *, size_t);

public:

  hid_libusb();
  virtual ~hid_libusb();
  virtual int enumerateHID(const uint16_t, const uint16_t);
  virtual void freeHIDEnumeration();
  virtual int writeHID(const uint8_t *, size_t, const bool bFeature = false);
  virtual int readHID(uint8_t *puiData, size_t uiLength,
                      int iMilliseconds = -1);
  virtual int readFeature(uint8_t *puiData, size_t uiLength,
                          int iMilliseconds = 0);
  virtual int openHID(const uint16_t, const uint16_t, const char *szSerial = 0);
  virtual void closeHID();
  virtual int openHIDDevice(const hid_device_info_t *);
  virtual int waitDeviceReAdd(const uint16_t uiTimeout = 0);
  static void getErrorString(const int, std::string &);
};

#endif
