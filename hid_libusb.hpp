//-----------------------------------------------------------------
//
// Copyright (c) 2013 TU-Dresden  All rights reserved.
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
  virtual bool enumerateHID(const uint16_t, const uint16_t);
  virtual void freeHIDEnumeration();
  virtual int writeHID(const uint8_t *, size_t, const bool bFeature = false);
  virtual int readHID(uint8_t *puiData, size_t uiLength,
                      int iMilliseconds = -1);
  virtual int readFeature(uint8_t *puiData, size_t uiLength,
                          int iMilliseconds = 0);
  virtual int openHID(const uint16_t, const uint16_t, const char *szSerial = 0);
  virtual void closeHID();
  virtual int openHIDDevice(const hid_device_info_t *);
  virtual bool waitDeviceReAdd(const uint16_t uiTimeout = 0);
};

#endif
