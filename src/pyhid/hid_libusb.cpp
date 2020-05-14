//-----------------------------------------------------------------
//
// Copyright (c) 2015 TU-Dresden  All rights reserved.
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
// Filename          :   hid_libusb.cpp
// Project Name      :   PyHID
// Description       :   HID using libusb-1.0
//-----------------------------------------------------------------
// Version   |  Author    |  Date         |  Comment
// ----------------------------------------------------------------
// 0.1       | hartmann   | 19 Jun 2013   |  initial version
// ----------------------------------------------------------------
#include "pyhid/hid_libusb.hpp"

#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdexcept>

libusb_context *hid_libusb::m_pContext = 0;

const char *libusb_wrapper::usbi_errors[] =
  {
    "Success",
    "Input/Output Error",
    "Invalid parameter",
    "Access denied (insufficient permissions)",
    "No such device (it may have been disconnected)",
    "Entity not found",
    "Resource busy",
    "Operation timed out",
    "Overflow",
    "Pipe error",
    "System call interrupted (perhaps due to signal)",
    "Insufficient memory",
    "Operation not supported or unimplemented on this platform",
    "Other error"
  };

libusb_wrapper::libusb_wrapper() : m_pLib(0),
                                   libusbInit(0),
                                   libusbExit(0),
                                   libusbGetDeviceList(0),
                                   libusbFreeDeviceList(0),
                                   libusbGetDeviceDescriptor(0),
                                   libusbGetActiveConfigDescriptor(0),
                                   libusbGetConfigDescriptor(0),
                                   libusbFreeConfigDescriptor(0),
                                   libusbOpen(0),
                                   libusbClose(0),
                                   libusbGetBusNumber(0),
                                   libusbGetDeviceAddress(0),
                                   libusbAttachKernelDriver(0),
                                   libusbDetachKernelDriver(0),
                                   libusbKernelDriverActive(0),
                                   libusbClaimInterface(0),
                                   libusbReleaseInterface(0),
                                   libusbGetStringDescriptorAscii(0),
                                   libusbHandleEvents(0),
                                   libusbAllocTransfer(0),
                                   libusbFreeTransfer(0),
                                   libusbSubmitTransfer(0),
                                   libusbCancelTransfer(0),
                                   libusbControlTransfer(0),
                                   libusbInterruptTransfer(0),
                                   libusbStrerror(0)
{
}

libusb_wrapper &libusb_wrapper::getInstance()
{
  static libusb_wrapper myLibUSBWrapper;
  return myLibUSBWrapper;
}

const char* libusb_wrapper::libusb_strerror(enum libusb_error errcode)
{
  int errcode_index = -errcode;

  const int max_index = ( sizeof(libusb_wrapper::usbi_errors) /
                          sizeof(char *) );
  if ( (errcode_index < 0 ) || ( errcode_index >= max_index ) )
    errcode_index = max_index - 1;

  return libusb_wrapper::usbi_errors[errcode_index];
}

bool libusb_wrapper::loadUSBLib()
{
  if ( this->m_pLib )
    return true;

  this->m_pLib = ::dlopen("libusb-1.0.so.0", RTLD_LAZY);
  if ( !this->m_pLib )
    return false;

  this->libusbInit                      = (libusbInit_t)
    ::dlsym(this->m_pLib, "libusb_init");
  this->libusbExit                      = (libusbExit_t)
    ::dlsym(this->m_pLib, "libusb_exit");
  this->libusbGetDeviceList             = (libusbGetDeviceList_t)
    ::dlsym(this->m_pLib, "libusb_get_device_list");
  this->libusbFreeDeviceList            = (libusbFreeDeviceList_t)
    ::dlsym(this->m_pLib, "libusb_free_device_list");
  this->libusbGetDeviceDescriptor       = (libusbGetDeviceDescriptor_t)
    ::dlsym(this->m_pLib, "libusb_get_device_descriptor");
  this->libusbGetActiveConfigDescriptor = (libusbGetActiveConfigDescriptor_t)
    ::dlsym(this->m_pLib, "libusb_get_active_config_descriptor");
  this->libusbGetConfigDescriptor       = (libusbGetConfigDescriptor_t)
    ::dlsym(this->m_pLib, "libusb_get_config_descriptor");
  this->libusbFreeConfigDescriptor      = (libusbFreeConfigDescriptor_t)
    ::dlsym(this->m_pLib, "libusb_free_config_descriptor");
  this->libusbOpen                      = (libusbOpen_t)
    ::dlsym(this->m_pLib, "libusb_open");
  this->libusbClose                     = (libusbClose_t)
    ::dlsym(this->m_pLib, "libusb_close");
  this->libusbGetBusNumber              = (libusbGetBusNumber_t)
    ::dlsym(this->m_pLib, "libusb_get_bus_number");
  this->libusbGetDeviceAddress          = (libusbGetDeviceAddress_t)
    ::dlsym(this->m_pLib, "libusb_get_device_address");
  this->libusbAttachKernelDriver        = (libusbAttachKernelDriver_t)
    ::dlsym(this->m_pLib, "libusb_detach_kernel_driver");
  this->libusbDetachKernelDriver        = (libusbDetachKernelDriver_t)
    ::dlsym(this->m_pLib, "libusb_detach_kernel_driver");
  this->libusbKernelDriverActive        = (libusbKernelDriverActive_t)
    ::dlsym(this->m_pLib, "libusb_kernel_driver_active");
  this->libusbClaimInterface            = (libusbClaimInterface_t)
    ::dlsym(this->m_pLib, "libusb_claim_interface");
  this->libusbReleaseInterface          = (libusbReleaseInterface_t)
    ::dlsym(this->m_pLib, "libusb_release_interface");
  this->libusbGetStringDescriptorAscii  = (libusbGetStringDescriptorAscii_t)
    ::dlsym(this->m_pLib, "libusb_get_string_descriptor_ascii");
  this->libusbHandleEvents              = (libusbHandleEvents_t)
    ::dlsym(this->m_pLib, "libusb_handle_events");
  this->libusbAllocTransfer             = (libusbAllocTransfer_t)
    ::dlsym(this->m_pLib, "libusb_alloc_transfer");
  this->libusbFreeTransfer              = (libusbFreeTransfer_t)
    ::dlsym(this->m_pLib, "libusb_free_transfer");
  this->libusbSubmitTransfer            = (libusbSubmitTransfer_t)
    ::dlsym(this->m_pLib, "libusb_submit_transfer");
  this->libusbCancelTransfer            = (libusbCancelTransfer_t)
    ::dlsym(this->m_pLib, "libusb_cancel_transfer");
  this->libusbControlTransfer           = (libusbControlTransfer_t)
    ::dlsym(this->m_pLib, "libusb_control_transfer");
  this->libusbInterruptTransfer         = (libusbInterruptTransfer_t)
    ::dlsym(this->m_pLib, "libusb_interrupt_transfer");
  this->libusbStrerror                  = (libusbStrerror_t)
    ::dlsym(this->m_pLib, "libusb_strerror");

  if ( ! this->libusbStrerror )
    this->libusbStrerror = libusb_wrapper::libusb_strerror;

  const bool bValid = ( this->libusbInit && this->libusbExit &&
                        this->libusbGetDeviceList &&
                        this->libusbFreeDeviceList &&
                        this->libusbGetDeviceDescriptor &&
                        this->libusbGetActiveConfigDescriptor &&
                        this->libusbGetConfigDescriptor &&
                        this->libusbFreeConfigDescriptor &&
                        this->libusbOpen && this->libusbClose &&
                        this->libusbGetBusNumber &&
                        this->libusbGetDeviceAddress &&
                        this->libusbAttachKernelDriver &&
                        this->libusbDetachKernelDriver &&
                        this->libusbKernelDriverActive &&
                        this->libusbClaimInterface &&
                        this->libusbReleaseInterface &&
                        this->libusbGetStringDescriptorAscii &&
                        this->libusbHandleEvents && this->libusbAllocTransfer &&
                        this->libusbFreeTransfer &&
                        this->libusbSubmitTransfer &&
                        this->libusbCancelTransfer &&
                        this->libusbControlTransfer &&
                        this->libusbInterruptTransfer );

  if ( !bValid )
    this->closeUSBLib();

  return bValid;
}

void libusb_wrapper::closeUSBLib()
{
  if ( this->m_pLib )
    ::dlclose(this->m_pLib);

  this->m_pLib                          = 0;
  this->libusbInit                      = 0;
  this->libusbExit                      = 0;
  this->libusbGetDeviceList             = 0;
  this->libusbFreeDeviceList            = 0;
  this->libusbGetDeviceDescriptor       = 0;
  this->libusbGetActiveConfigDescriptor = 0;
  this->libusbGetConfigDescriptor       = 0;
  this->libusbFreeConfigDescriptor      = 0;
  this->libusbOpen                      = 0;
  this->libusbClose                     = 0;
  this->libusbGetBusNumber              = 0;
  this->libusbGetDeviceAddress          = 0;
  this->libusbAttachKernelDriver        = 0;
  this->libusbDetachKernelDriver        = 0;
  this->libusbKernelDriverActive        = 0;
  this->libusbClaimInterface            = 0;
  this->libusbReleaseInterface          = 0;
  this->libusbGetStringDescriptorAscii  = 0;
  this->libusbHandleEvents              = 0;
  this->libusbAllocTransfer             = 0;
  this->libusbFreeTransfer              = 0;
  this->libusbSubmitTransfer            = 0;
  this->libusbCancelTransfer            = 0;
  this->libusbControlTransfer           = 0;
  this->libusbInterruptTransfer         = 0;
  this->libusbStrerror                  = 0;
}

libusb_wrapper::~libusb_wrapper()
{
  this->closeUSBLib();
}

hid_libusb::hid_libusb() : m_pDeviceHandle(0),
                           m_pInputReports(0),
                           m_bShutdownThread(false),
                           m_uiMaxPacketSize(0),
                           m_iInputEndpoint(0),
                           m_iOutputEndpoint(0),
                           m_iInterface(0),
                           m_bOpenDevice(false),
                           m_bDetachedKernel(false),
                           m_szUdevPath(0),
                           m_szVendorID(),
                           m_szProductID(),
                           m_szBusNum(),
                           m_szDevAddr(),
                           m_pDevices(0),
                           m_pUdev(udev_new()),
                           m_pTransfer(0)
{
}

hid_libusb::~hid_libusb()
{
  this->closeHID();
  this->freeHIDEnumeration();
  if ( this->m_pUdev )
    udev_unref(this->m_pUdev);
}

char *hid_libusb::getUSBString(libusb_device_handle *pDevHandle,
                               const uint8_t uiIdx)
{
  char szBuf[512];

  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();
  int iLen = libusbWrapper.libusbGetStringDescriptorAscii(pDevHandle,
                                                          uiIdx,
                                                          (unsigned char*)szBuf,
                                                          sizeof(szBuf)
                                                          );
  if ( iLen < 0 )
    return 0;

  char* szResult = new char[iLen+1];
  memcpy(szResult, szBuf, iLen);
  szResult[iLen] = '\0';
  return szResult;
}

bool hid_libusb::findUdevPath()
{
  if ( this->m_szUdevPath )
    delete [] this->m_szUdevPath;

  this->m_szUdevPath = 0;

  if ( !this->m_pUdev )
    return false;

  struct udev_enumerate *pEnumerate = udev_enumerate_new(this->m_pUdev);
  if ( udev_enumerate_add_match_subsystem(pEnumerate, "usb")  < 0 )
    {
      udev_enumerate_unref(pEnumerate);
      return false;
    }

  if ( udev_enumerate_scan_devices(pEnumerate) < 0 )
    {
      udev_enumerate_unref(pEnumerate);
      return false;
    }

  struct udev_list_entry *pDevices = udev_enumerate_get_list_entry(pEnumerate);

  struct udev_list_entry *pDevListEntry;
  udev_list_entry_foreach(pDevListEntry, pDevices)
    {
      const char *szPath = udev_list_entry_get_name(pDevListEntry);
      struct udev_device *pDev = udev_device_new_from_syspath(this->m_pUdev,
                                                              szPath);

      const char *szVID = udev_device_get_sysattr_value(pDev, "idVendor");
      const char *szPID = udev_device_get_sysattr_value(pDev, "idProduct");
      const char *szBus = udev_device_get_property_value(pDev, "BUSNUM");
      const char *szDev  = udev_device_get_property_value(pDev, "DEVNUM");

      if ( szVID && szPID && szBus && szDev )
        {
          if ( !strcmp(szVID, this->m_szVendorID) &&
               !strcmp(szPID, this->m_szProductID) &&
               !strcmp(szBus, this->m_szBusNum) &&
               !strcmp(szDev, this->m_szDevAddr) )
            {
              this->m_szUdevPath = new char[strlen(szPath)+1];
              strcpy(this->m_szUdevPath, szPath);
              break;
            }
        }

      udev_device_unref(pDev);
    }
  udev_enumerate_unref(pEnumerate);

  return true;
}

int hid_libusb::returnData(uint8_t *puiData, size_t uiLength)
{
  input_report_t *pReport = this->m_pInputReports;
  size_t uiLen = ( uiLength < pReport->uiLength ) ?
    uiLength : pReport->uiLength;

  if ( uiLen > 0 && puiData )
    memcpy(puiData, pReport->puiData, uiLen);
  this->m_pInputReports = pReport->pNext;

  delete [] pReport->puiData;
  delete pReport;

  return uiLen;
}

void hid_libusb::readCallback(struct libusb_transfer *pTransfer)
{
  self_type_t *pThis = static_cast<self_type_t *>(pTransfer->user_data);

  if ( pTransfer->status == LIBUSB_TRANSFER_COMPLETED )
    {
      input_report_t *pReport = new input_report_t;
      pReport->puiData = new uint8_t[pTransfer->actual_length];
      memcpy(pReport->puiData, pTransfer->buffer, pTransfer->actual_length);
      pReport->uiLength = pTransfer->actual_length;
      pReport->pNext = 0;

      pthread_mutex_lock(&pThis->m_Mutex);

      if ( !pThis->m_pInputReports )
        {
          pThis->m_pInputReports = pReport;
          pthread_cond_signal(&pThis->m_Condition);
        }
      else
        {
          input_report_t *pCurrent = pThis->m_pInputReports;
          int iNumQueued = 0;
          while ( pCurrent->pNext )
            {
              pCurrent = pCurrent->pNext;
              iNumQueued++;
            }
          pCurrent->pNext = pReport;
          if ( iNumQueued > 30 )
            pThis->returnData(0, 0);
        }
      pthread_mutex_unlock(&pThis->m_Mutex);
    }
  else if ( pTransfer->status == LIBUSB_TRANSFER_CANCELLED ||
            pTransfer->status == LIBUSB_TRANSFER_NO_DEVICE )
    {
      pThis->m_bShutdownThread = true;
      return;
    }

  int iResult = libusb_wrapper::getInstance().libusbSubmitTransfer(pTransfer);
  if ( iResult )
    pThis->m_bShutdownThread = true;
}

void *hid_libusb::readThread(void *pParam)
{
  self_type_t *pThis = static_cast<self_type_t *>(pParam);
  uint8_t *puiBuf;
  const size_t uiLength = pThis->m_uiMaxPacketSize;

  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();

  puiBuf = new uint8_t[uiLength];
  pThis->m_pTransfer = libusbWrapper.libusbAllocTransfer(0);
  libusb_fill_interrupt_transfer(pThis->m_pTransfer,
                                 pThis->m_pDeviceHandle,
                                 pThis->m_iInputEndpoint,
                                 puiBuf,
                                 uiLength,
                                 self_type_t::readCallback,
                                 pThis,
                                 5000
                                 );

  libusbWrapper.libusbSubmitTransfer(pThis->m_pTransfer);

  pthread_barrier_wait(&pThis->m_Barrier);

  while ( ! pThis->m_bShutdownThread )
    {
      int iResult = libusbWrapper.libusbHandleEvents(self_type_t::m_pContext);
      if ( iResult < 0 )
        {
          if ( iResult != LIBUSB_ERROR_BUSY &&
               iResult != LIBUSB_ERROR_TIMEOUT &&
               iResult != LIBUSB_ERROR_OVERFLOW &&
               iResult != LIBUSB_ERROR_INTERRUPTED )
            break;
        }
    }

  if ( ! libusbWrapper.libusbCancelTransfer(pThis->m_pTransfer) )
    libusbWrapper.libusbHandleEvents(self_type_t::m_pContext);

  pthread_mutex_lock(&pThis->m_Mutex);
  pthread_cond_broadcast(&pThis->m_Condition);
  pthread_mutex_unlock(&pThis->m_Mutex);

  delete [] puiBuf;

  return 0;
}

void hid_libusb::cleanupMutex(void *pParam)
{
  self_type_t *pThis = static_cast<self_type_t *>(pParam);
  pthread_mutex_unlock(&pThis->m_Mutex);
}

int hid_libusb::enumerateHID(const uint16_t uiVendorID,
                             const uint16_t uiProductID)
{
  libusb_device **ppList;
  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();

  if ( ! self_type_t::m_pContext )
    {
      if ( !libusbWrapper.loadUSBLib() )
        return HID_LIBUSB_NO_LIBUSB;
      const int iResult = libusbWrapper.libusbInit(&self_type_t::m_pContext);
      if ( iResult )
        return iResult;
      atexit(self_type_t::freeHID);
    }

  ssize_t iDeviceCount = libusbWrapper.libusbGetDeviceList(
                            self_type_t::m_pContext, &ppList);
  if ( iDeviceCount < 0 )
    return iDeviceCount;

  if ( this->m_pDevices )
    this->freeHIDEnumeration();

  hid_device_info_t *pCurrent = 0;

  int i = 0;
  libusb_device *pDev;
  while ( ( pDev = ppList[i++] ) )
    {
      struct libusb_device_descriptor desc;
      int iResult = libusbWrapper.libusbGetDeviceDescriptor(pDev, &desc);

      if ( desc.bDeviceClass != LIBUSB_CLASS_PER_INTERFACE &&
           desc.bDeviceClass != LIBUSB_CLASS_VENDOR_SPEC )
        continue;

      const uint16_t uiDeviceVID = desc.idVendor;
      const uint16_t uiDevicePID = desc.idProduct;

      struct libusb_config_descriptor *pConfDesc = 0;
      iResult = libusbWrapper.libusbGetActiveConfigDescriptor(pDev, &pConfDesc);
      if ( iResult < 0 )
        libusbWrapper.libusbGetConfigDescriptor(pDev, 0, &pConfDesc);
      if ( pConfDesc )
        {
          for ( int j = 0; j < pConfDesc->bNumInterfaces; j++ )
            {
              const struct libusb_interface *pInterface;
              pInterface = &pConfDesc->interface[j];
              for ( int k = 0; k < pInterface->num_altsetting; k++ )
                {
                  const struct libusb_interface_descriptor *pInterfaceDesc;
                  pInterfaceDesc = &pInterface->altsetting[k];
                  if ( pInterfaceDesc->bInterfaceClass == LIBUSB_CLASS_HID )
                    {
                      if ( ( !uiVendorID || uiVendorID == uiDeviceVID ) &&
                           ( !uiProductID || uiProductID == uiDevicePID ) )
                        {
                          hid_device_info_t *pNext;

                          pNext = new hid_device_info_t;
                          if ( pCurrent )
                            pCurrent->pNext = pNext;
                          else
                            this->m_pDevices = pNext;
                          pCurrent = pNext;

                          pCurrent->pNext = 0;
                          pCurrent->szManufacturer = 0;
                          pCurrent->szSerial = 0;
                          pCurrent->szProduct = 0;
                          pCurrent->uiBusNumber =
                            libusbWrapper.libusbGetBusNumber(pDev);
                          pCurrent->uiDeviceAddress =
                            libusbWrapper.libusbGetDeviceAddress(pDev);

                          libusb_device_handle *pDevHandle;
                          iResult = libusbWrapper.libusbOpen(pDev, &pDevHandle);
                          if ( iResult >= 0 )
                            {
                              if (desc.iSerialNumber > 0 )
                                pCurrent->szSerial =
                                  self_type_t::getUSBString(pDevHandle,
                                                            desc.iSerialNumber);
                              if (desc.iManufacturer > 0 )
                                pCurrent->szManufacturer =
                                  self_type_t::getUSBString(pDevHandle,
                                                            desc.iManufacturer);
                              if ( desc.iProduct > 0 )
                                pCurrent->szProduct =
                                  self_type_t::getUSBString(pDevHandle,
                                                            desc.iProduct);

                              libusbWrapper.libusbClose(pDevHandle);
                            }
                          pCurrent->uiVendorID = uiDeviceVID;
                          pCurrent->uiProductID = uiDevicePID;
                          pCurrent->uiReleaseNumber = desc.bcdDevice;
                          pCurrent->iInterfaceNumber =
                            pInterfaceDesc->bInterfaceNumber;
                        }
                    }
                }
            }
          libusbWrapper.libusbFreeConfigDescriptor(pConfDesc);
        }
    }
  libusbWrapper.libusbFreeDeviceList(ppList, 1);

  return 0;
}

void hid_libusb::freeHIDEnumeration()
{
  hid_device_info_t *pDevice = this->m_pDevices;
  while ( pDevice )
    {
      if ( pDevice->szSerial )
        delete [] pDevice->szSerial;
      if ( pDevice->szManufacturer )
        delete [] pDevice->szManufacturer;
      if ( pDevice->szProduct )
        delete [] pDevice->szProduct;
      hid_device_info *pNext = pDevice->pNext;
      delete pDevice;
      pDevice = pNext;
    }
  this->m_pDevices = 0;
}

int hid_libusb::writeHID(const uint8_t *puiData, size_t uiLength,
                         const bool bFeature)
{
  if ( ! this->m_bOpenDevice )
    return HID_LIBUSB_NO_DEVICE_OPEN;

  const uint8_t uiReportNumber = puiData[0];
  bool bSkippedReportID = false;
  if ( !uiReportNumber && !bFeature )
    {
      puiData++;
      uiLength--;
      bSkippedReportID = true;
    }

  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();

  if ( this->m_iOutputEndpoint <= 0 || bFeature )
    {
      const uint16_t uiRequestType = ( bFeature ) ?
        0x300 : ( 0x200 | uiReportNumber );
      int iResult = libusbWrapper.libusbControlTransfer(
                          this->m_pDeviceHandle,
                          LIBUSB_REQUEST_TYPE_CLASS |
                          LIBUSB_RECIPIENT_INTERFACE |
                          LIBUSB_ENDPOINT_OUT,
                          0x09,
                          uiRequestType,
                          this->m_iInterface,
                          (unsigned char *)puiData,
                          uiLength,
                          1000);

      if ( iResult < 0 )
        return iResult;

      if ( bSkippedReportID )
        return uiLength++;
      return uiLength;
    }

  int iActualLength;
  int iResult = libusbWrapper.libusbInterruptTransfer(this->m_pDeviceHandle,
                                                      this->m_iOutputEndpoint,
                                                      (unsigned char*)puiData,
                                                      uiLength,
                                                      &iActualLength,
                                                      1000);

  if ( iResult < 0 )
    return iResult;

  if ( bSkippedReportID )
    iActualLength++;

  return iActualLength;
}

int hid_libusb::readHID(uint8_t *puiData, size_t uiLength,
                        int iMilliseconds)
{
  if ( ! this->m_bOpenDevice )
    return HID_LIBUSB_NO_DEVICE_OPEN;

  int iBytesRead = HID_LIBUSB_READ_ERROR;

  pthread_mutex_lock(&this->m_Mutex);
  pthread_cleanup_push(&self_type_t::cleanupMutex, this);

  if ( this->m_pInputReports )
    iBytesRead = this->returnData(puiData, uiLength);
  else if ( !this->m_bShutdownThread )
    {
      if ( iMilliseconds == -1 )
        {
          while ( !this->m_pInputReports && !this->m_bShutdownThread )
            pthread_cond_wait(&this->m_Condition, &this->m_Mutex);
          if ( this->m_pInputReports )
            iBytesRead = this->returnData(puiData, uiLength);
        }
      else if ( iMilliseconds > 0 )
        {
          struct timespec ts;
          clock_gettime(CLOCK_REALTIME, &ts);
          ts.tv_sec += iMilliseconds / 1000;
          ts.tv_nsec += ( iMilliseconds % 1000 ) * 1000000;
          if ( ts.tv_nsec >= 1000000000L )
            {
              ts.tv_sec++;
              ts.tv_nsec -= 1000000000L;
            }
          while ( !this->m_pInputReports && !this->m_bShutdownThread )
            {
              int iResult = pthread_cond_timedwait(&this->m_Condition,
                                                   &this->m_Mutex, &ts);
              if ( !iResult )
                {
                  if ( this->m_pInputReports)
                    {
                      iBytesRead = this->returnData(puiData, uiLength);
                      break;
                    }
                }
              else if ( iResult == ETIMEDOUT )
                {
                  iBytesRead = 0;
                  break;
                }
              else
                break;
            }
        }
      else
        iBytesRead = 0;
    }
  pthread_mutex_unlock(&this->m_Mutex);
  pthread_cleanup_pop(0);

  return iBytesRead;
}

int hid_libusb::readFeature(uint8_t *puiData, size_t uiLength,
                            int iMilliseconds)
{
  if ( ! this->m_bOpenDevice )
    return HID_LIBUSB_NO_DEVICE_OPEN;

  int iResult = libusb_wrapper::getInstance().libusbControlTransfer(
                       this->m_pDeviceHandle,
                       LIBUSB_ENDPOINT_IN |
                       LIBUSB_REQUEST_TYPE_CLASS |
                       LIBUSB_RECIPIENT_INTERFACE,
                       0x01,
                       0x0300 | puiData[0],
                       this->m_iInterface,
                       puiData,
                       uiLength,
                       iMilliseconds);

  return iResult;
}

int hid_libusb::openHID(const uint16_t vid, const uint16_t pid,
                        std::string const& serial)
{
  int iResult = this->enumerateHID(vid, pid);
  if ( iResult < 0 )
    return iResult;

  hid_device_info_t *pDevice = this->m_pDevices;
  while ( pDevice )
    {
      if ( !serial.empty() )
        {
          if ( !strcmp(serial.c_str(), pDevice->szSerial) )
            break;
        }
      else
        break;
      pDevice = pDevice->pNext;
    }

  iResult = HID_LIBUSB_NO_DEVICE;
  if ( pDevice )
    iResult = this->openHIDDevice(pDevice);

  return iResult;
}

void hid_libusb::closeHID()
{
  if ( ! this->m_bOpenDevice )
    return;

  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();

  this->m_bShutdownThread = true;
  if ( this->m_pTransfer )
    {
      libusbWrapper.libusbCancelTransfer(this->m_pTransfer);
      pthread_join(this->m_Thread, 0);
      libusbWrapper.libusbFreeTransfer(this->m_pTransfer);
      this->m_pTransfer = 0;
    }

  if ( this->m_pDeviceHandle )
    {
      libusbWrapper.libusbReleaseInterface(this->m_pDeviceHandle,
                                           this->m_iInterface);
      if ( this->m_bDetachedKernel )
        libusbWrapper.libusbAttachKernelDriver(this->m_pDeviceHandle,
                                               this->m_iInterface);
      libusbWrapper.libusbClose(this->m_pDeviceHandle);
      this->m_pDeviceHandle = 0;
    }

  pthread_barrier_destroy(&this->m_Barrier);
  pthread_cond_destroy(&this->m_Condition);
  pthread_mutex_destroy(&this->m_Mutex);

  this->m_bOpenDevice = false;
  this->m_bDetachedKernel = false;
}

int hid_libusb::openHIDDevice(const hid_device_info_t *pDeviceToOpen)
{
  if ( ! pDeviceToOpen )
    return HID_LIBUSB_INVALID_ARGS;

  libusb_wrapper &libusbWrapper = libusb_wrapper::getInstance();

  if ( ! self_type_t::m_pContext )
    {
      if ( !libusbWrapper.loadUSBLib() )
        return HID_LIBUSB_NO_LIBUSB;
      int iResult = libusbWrapper.libusbInit(&self_type_t::m_pContext);
      if ( iResult < 0 )
        return iResult;
      atexit(self_type_t::freeHID);
    }

  this->closeHID();

  this->m_bShutdownThread = false;
  this->m_iInputEndpoint = 0;
  this->m_iOutputEndpoint = 0;

  pthread_mutex_init(&this->m_Mutex, 0);
  pthread_cond_init(&this->m_Condition, 0);
  pthread_barrier_init(&this->m_Barrier, NULL, 2);

  libusb_device **ppList;
  libusbWrapper.libusbGetDeviceList(self_type_t::m_pContext, &ppList);

  libusb_device *pDev;
  int d = 0;
  bool bGoodOpen = false;
  int iResult = HID_LIBUSB_NO_DEVICE;
  while ( ( pDev = ppList[d++] ) )
    {
      struct libusb_device_descriptor desc;
      libusbWrapper.libusbGetDeviceDescriptor(pDev, &desc);

      if ( desc.idVendor != pDeviceToOpen->uiVendorID ||
           desc.idProduct != pDeviceToOpen->uiProductID )
        continue;

      struct libusb_config_descriptor *pConfDesc = 0;
      if ( libusbWrapper.libusbGetActiveConfigDescriptor(pDev, &pConfDesc) < 0 )
        continue;

      for (int j = 0; j < pConfDesc->bNumInterfaces; j++ )
        {
          const struct libusb_interface *pInterface = &pConfDesc->interface[j];
          for ( int k = 0; k < pInterface->num_altsetting; k++ )
            {
              const struct libusb_interface_descriptor *pInterfaceDesc;
              pInterfaceDesc = &pInterface->altsetting[k];

              if ( pInterfaceDesc->bInterfaceClass == LIBUSB_CLASS_HID ||
                   pInterfaceDesc->bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC )
                {
                  const uint16_t uiBusNumber =
                    libusbWrapper.libusbGetBusNumber(pDev);
                  const uint16_t uiDeviceAddress =
                    libusbWrapper.libusbGetDeviceAddress(pDev);
                  if ( pDeviceToOpen->uiBusNumber == uiBusNumber &&
                       pDeviceToOpen->uiDeviceAddress == uiDeviceAddress &&
                       pDeviceToOpen->iInterfaceNumber ==
                       pInterfaceDesc->bInterfaceNumber )
                    {
                      iResult = libusbWrapper.libusbOpen(
                                      pDev, &this->m_pDeviceHandle);
                      if ( iResult < 0 )
                        break;
                      bGoodOpen = true;
                      this->m_bDetachedKernel = false;

                      iResult = libusbWrapper.libusbKernelDriverActive(
                                      this->m_pDeviceHandle,
                                      pInterfaceDesc->bInterfaceNumber);
                      if ( iResult < 0 )
                        {
                          libusbWrapper.libusbClose(this->m_pDeviceHandle);
                          bGoodOpen = false;
                          break;
                        }
                      if ( iResult == 1 )
                        {
                          iResult = libusbWrapper.libusbDetachKernelDriver(
                                          this->m_pDeviceHandle,
                                          pInterfaceDesc->bInterfaceNumber);
                          if ( iResult < 0 )
                            {
                              libusbWrapper.libusbClose(this->m_pDeviceHandle);
                              bGoodOpen = false;
                              break;
                            }
                          this->m_bDetachedKernel = true;
                        }

                      iResult = libusbWrapper.libusbClaimInterface(
                                      this->m_pDeviceHandle,
                                      pInterfaceDesc->bInterfaceNumber);
                      if ( iResult < 0 )
                        {
                          if ( this->m_bDetachedKernel )
                            libusbWrapper.libusbAttachKernelDriver(
                                  this->m_pDeviceHandle,
                                  pInterfaceDesc->bInterfaceNumber);
                          libusbWrapper.libusbClose(this->m_pDeviceHandle);
                          bGoodOpen = false;
                          break;
                        }

                      this->m_iInterface    = pInterfaceDesc->bInterfaceNumber;
                      snprintf(this->m_szVendorID, 5, "%04x", desc.idVendor);
                      snprintf(this->m_szProductID, 5, "%04x", desc.idProduct);
                      snprintf(this->m_szBusNum, 4, "%03d", uiBusNumber);
                      snprintf(this->m_szDevAddr, 4, "%03d", uiDeviceAddress);

                      for ( int i = 0; i < pInterfaceDesc->bNumEndpoints; i++ )
                        {
                          const struct libusb_endpoint_descriptor *pEndpoint;
                          pEndpoint = &pInterfaceDesc->endpoint[i];

                          const bool bIsInterrupt =
                            ( pEndpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK )
                            == LIBUSB_TRANSFER_TYPE_INTERRUPT;
                          const bool bIsOutput =
                            ( pEndpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK )
                            == LIBUSB_ENDPOINT_OUT;
                          const bool bIsInput =
                            ( pEndpoint->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK )
                            == LIBUSB_ENDPOINT_IN;
                          if ( this->m_iInputEndpoint == 0 && bIsInterrupt &&
                               bIsInput )
                            {
                              this->m_iInputEndpoint = pEndpoint->bEndpointAddress;
                              this->m_uiMaxPacketSize = pEndpoint->wMaxPacketSize;
                            }
                          if ( this->m_iOutputEndpoint == 0 &&
                               bIsInterrupt && bIsOutput )
                            this->m_iOutputEndpoint = pEndpoint->bEndpointAddress;
                        }
                      pthread_create(&this->m_Thread,
                                     0,
                                     self_type_t::readThread, this);
                      pthread_barrier_wait(&this->m_Barrier);
                      break;
                    }
                }
            }
        }
      libusbWrapper.libusbFreeConfigDescriptor(pConfDesc);
    }

  libusbWrapper.libusbFreeDeviceList(ppList, 1);

  if ( bGoodOpen )
    {
      this->findUdevPath();
      this->m_bOpenDevice = true;
      return 0;
    }

  pthread_barrier_destroy(&this->m_Barrier);
  pthread_cond_destroy(&this->m_Condition);
  pthread_mutex_destroy(&this->m_Mutex);

  return iResult;
}

void hid_libusb::freeHID()
{
  if ( self_type_t::m_pContext )
    libusb_wrapper::getInstance().libusbExit(self_type_t::m_pContext);
}

int hid_libusb::waitDeviceReAdd(const uint16_t uiTimeout)
{
  if ( !this->m_szUdevPath || !this->m_pUdev )
    return HID_LIBUSB_NO_UDEV;

  struct udev_monitor *pMon = udev_monitor_new_from_netlink(this->m_pUdev,
                                                            "udev");
  if ( !pMon )
    return HID_LIBUSB_UDEV_MON_ERROR;

  if ( udev_monitor_filter_add_match_subsystem_devtype(pMon, "usb", 0) < 0 )
    {
      udev_monitor_unref(pMon);
      return 0;
    }
  udev_monitor_enable_receiving(pMon);

  int iFd = udev_monitor_get_fd(pMon);

  uint16_t uiTimeToRun = uiTimeout;
  while ( 1 )
    {
      struct timeval tv;
      tv.tv_sec = uiTimeToRun / 1000;
      tv.tv_usec = ( uiTimeToRun % 1000 ) * 1000;

      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(iFd, &fds);

      struct timeval tv_start;
      gettimeofday(&tv_start, 0);

      int iResult = select(iFd+1, &fds, 0, 0, &tv);

      if ( iResult > 0 && FD_ISSET(iFd, &fds) )
        {
          struct udev_device *pDev = udev_monitor_receive_device(pMon);
          if ( pDev )
            {
              if ( !strcmp(udev_device_get_action(pDev), "add") )
                {
                  const char *szPath = udev_device_get_syspath(pDev);
                  if ( !strcmp(szPath, this->m_szUdevPath) )
                    return 0;
                }
            }
        }
      else
        return false;

      struct timeval tv_end;
      gettimeofday(&tv_end, 0);
      long int iTime = ( tv_end.tv_sec - tv_start.tv_sec ) * 1000 +
        ( tv_end.tv_usec - tv_start.tv_usec ) / 1000;
      if ( iTime <= 0 )
        iTime = 1;

      if ( uiTimeToRun )
        {
          if ( iTime >= uiTimeToRun )
            return HID_LIBUSB_UDEV_TIMEOUT;
          uiTimeToRun -= iTime;
        }
    }

  return 0;
}

void hid_libusb::getErrorString(const int iError,
                                std::string &szError)
{
  if ( iError >= LIBUSB_ERROR_OTHER )
    {
      szError = "libusb: ";
      szError += libusb_wrapper::getInstance().libusbStrerror(
                        static_cast<libusb_error>(iError));
      szError += ".";
    }
  else
    {
      szError = "hid: ";
      switch ( iError )
        {
        case HID_LIBUSB_INVALID_ARGS :
          szError += "Invalid arguments supplied.";
          break;
        case HID_LIBUSB_NO_DEVICE :
          szError += "No matching HID device found.";
          break;
        case HID_LIBUSB_NO_DEVICE_OPEN :
          szError += "No HID device open to perform requested operation.";
          break;
        case HID_LIBUSB_READ_ERROR :
          szError += "Generic HID read error occured.";
          break;
        case HID_LIBUSB_NO_UDEV :
          szError += "Udev or the device Udev path is not available.";
          break;
        case HID_LIBUSB_UDEV_MON_ERROR :
          szError += "Failed to create an Udev monitor.";
          break;
        case HID_LIBUSB_UDEV_TIMEOUT :
          szError += "Waiting for Udev timed out.";
          break;
        case HID_LIBUSB_NO_LIBUSB :
          szError += "Failed to load libusb-1.0.so.0.";
          break;
        default :
          szError += "Unknown error.";
        }
    }
}

int hid_libusb::writeHID(std::vector<uint8_t> const& data)
{
	int ret = writeHID(data.data(), data.size());
	if (ret < 0) {
		std::string message;
		getErrorString(ret, message);
		throw std::runtime_error(message);
	}
	return ret;
}


std::vector<uint8_t> hid_libusb::readHID(size_t const size, int const timeout)
{
	std::vector<uint8_t> data(size);
	int ret = readHID(data.data(), size, timeout);
	if (ret < 0) {
		std::string message;
		getErrorString(ret, message);
		throw std::runtime_error(message);
	} else {
		data.resize(ret);
	}
	return data;
}
