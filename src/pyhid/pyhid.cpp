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
// Filename          :   pyhid.cpp
// Project Name      :   PyHID
// Description       :   Python Wrapper for C++ Class
//-----------------------------------------------------------------
// Version   |  Author    |  Date         |  Comment
// ----------------------------------------------------------------
// 0.1       | hartmann   | 19 Jun 2013   |  initial version
// ----------------------------------------------------------------
#include "CXX/Objects.hxx"
#include "CXX/Extensions.hxx"
#include "pyhid.hpp"
#include <sstream>
#include <string>

static pyhid_module *pPyHID;

#include "hid_libusb.hpp"

#ifdef _MSC_VER
 #pragma warning(disable: 4786)
#endif

namespace Py
{
  class IOError: public StandardError
  {
  public:
    IOError (const std::string& reason)
      : StandardError()
    {
      PyErr_SetString (_Exc_IOError(), reason.c_str());
    }
  };
}

class HIDError
{
private:
  Py::String m_message;
  Py::Int    m_code;
  Py::Object m_exception_arg;
public:
  HIDError(const int err, const std::string& reason)
    : m_message(Py::String(reason)),
      m_code(Py::Int(err)),
      m_exception_arg()
  {
    Py::Tuple arg_list(2);
    arg_list[0] = m_message;
    arg_list[1] = m_code;
    m_exception_arg = arg_list;
  }

  HIDError(const HIDError &other)
    : m_message(other.m_message),
      m_code(other.m_code),
      m_exception_arg(other.m_exception_arg)
  {
  }

  Py::Object &pythonExceptionArg()
  {
    return m_exception_arg;
  }

};

inline static Py::List::size_type getArgCount(const Py::Tuple args,
                                              const Py::Dict kwds)
{
  return args.length() + kwds.length();
}

static void genErrorMessage(const char *szFunc,
                            const char *szMsg,
                            const int iError,
                            std::string &szError)
{
  if ( !szFunc && !szMsg )
    hid_libusb::getErrorString(iError, szError);
  else
    {
      std::string message;
      hid_libusb::getErrorString(iError, message);
      if ( szFunc )
        {
          szError = szFunc;
          szError += "() ";
        }
      if ( szMsg )
        {
          szError += szMsg;
          szError += "(";
        }
      szError += message;
      if ( szMsg )
        szError += ")";
    }
}

static void addDefaultArg(const std::string &funcname,
                          const unsigned int expectedArgCount,
                          const std::string &argname,
                          const Py::Object &defaultval,
                          const Py::Tuple &args,
                          const Py::Dict &kwds)
{
  if ( getArgCount(args, kwds) >= expectedArgCount )
    return;
  if ( kwds.hasKey(argname) )
    return;
  kwds[argname] = defaultval;
}

static void checkArgCount(const std::string &funcname,
                          const unsigned int expectedArgCount,
                          const Py::Tuple &args,
                          const Py::Dict &kwds)
{
  const Py::List::size_type argCount = getArgCount(args, kwds);
  if ( argCount != expectedArgCount )
    {
      std::ostringstream message;
      message << funcname << "() takes exactly "<< expectedArgCount <<
        " arguments (" << argCount << " given)";
      throw Py::TypeError(message.str());
    }
}

static void mergeArgs(const std::string &funcname, const Py::List &keys,
                      const Py::Tuple &args, Py::Dict &kwds)
{
  for ( Py::List::size_type i = 0; i < args.length(); ++i )
    {
      if ( kwds.hasKey(keys[i]) )
        {
          std::ostringstream message;
          message << funcname << "() got multiple values for " <<
            "keyword argument '" << keys[i] << "'";
          throw Py::TypeError(message.str());
        }
      kwds[keys[i]] = args[i];
    }

  Py::List names(kwds.keys());
  for ( Py::List::size_type i = 0; i < names.length(); ++i )
    {
      Py::String str1(names[i]);
      bool bFoundKey = false;
      for ( Py::List::size_type j = 0; j < keys.length(); ++j )
        {
          Py::String str2(keys[j]);
          if ( str1.as_std_string("utf-8") == str2.as_std_string("utf-8") )
            {
              bFoundKey = true;
              break;
            }
        }
      if ( !bFoundKey )
        {
          std::ostringstream message;
          message << funcname << "() got an unexpected keyword argument '" <<
            str1 << "'";
          throw Py::TypeError(message.str());
        }
    }
}

class pyhidaccess: public Py::PythonClass<pyhidaccess>
{
private :
  Py::String m_value;

protected :

  hid_libusb m_hidDevice;

  Py::Object pyhidaccess_openHID(const Py::Tuple &args, const Py::Dict &kwds)
  {
    ::addDefaultArg("openHID", 3, "serial", Py::None(), args, kwds);
    ::checkArgCount("openHID", 3, args, kwds);

    Py::List keys(3);
    keys[0] = Py::String("vid");
    keys[1] = Py::String("pid");
    keys[2] = Py::String("serial");

    Py::Dict d(kwds);
    ::mergeArgs("openHID", keys, args, d);

    Py::Long vid(d["vid"]);
    Py::Long pid(d["pid"]);

    int iResult;
    if (Py::Object(d["serial"]).isNone()) {
        iResult = m_hidDevice.openHID(long(vid), long(pid));
    } else {
        // python-CXX has no unsigned long, fall back to hex string
        Py::String serial(d["serial"]);
        unsigned long lSerial = std::stoul(serial, nullptr, 16);
        std::ostringstream oss;

        // uppercase is required for comparison in openHID()
        oss << std::uppercase << std::hex << lSerial;

        iResult = m_hidDevice.openHID(long(vid), long(pid), oss.str().c_str());
    }
    if ( iResult < 0 )
    {
        std::string szError;
        genErrorMessage("openHID", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
    }

    return Py::None();
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_openHID)

  Py::Object pyhidaccess_writeHID(const Py::Tuple &args, const Py::Dict &kwds)
  {
    ::checkArgCount("writeHID", 1, args, kwds);

    Py::List keys(1);
    keys[0] = Py::String("data");

    Py::Dict d(kwds);
    ::mergeArgs("writeHID", keys, args, d);

    Py::List data(d["data"]);

    uint8_t *puiData = new uint8_t[data.length()];
    for ( Py::List::size_type i = 0; i < data.length(); ++i )
      {
        Py::Long value(data[i]);
        puiData[i] = long(value);
      }

    int iResult = m_hidDevice.writeHID(puiData, data.length());
    delete [] puiData;

    if ( iResult < 0 )
      {
        std::string szError;
        genErrorMessage("writeHID", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
      }
    return Py::Long(iResult);
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_writeHID)

  Py::Object pyhidaccess_writeFeature(const Py::Tuple &args,
                                      const Py::Dict &kwds)
  {
    ::checkArgCount("writeFeature", 2, args, kwds);

    Py::List keys(2);
    keys[0] = Py::String("id");
    keys[1] = Py::String("data");

    Py::Dict d(kwds);
    ::mergeArgs("writeFeature", keys, args, d);

    Py::Long id(d["id"]);
    Py::List data(d["data"]);
    uint8_t *puiData = new uint8_t[data.length() + 1];
    for ( Py::List::size_type i = 1; i < ( data.length() + 1 ); ++i )
      {
        Py::Long value(data[i - 1]);
        puiData[i] = long(value);
      }
    puiData[0] = long(id);

    int iResult = m_hidDevice.writeHID(puiData, data.length() + 1, true);
    delete [] puiData;

    if ( iResult < 0 )
      {
        std::string szError;
        genErrorMessage("writeFeature", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
      }

    return Py::Long(iResult);
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_writeFeature)

  Py::Object pyhidaccess_readHID(const Py::Tuple &args, const Py::Dict &kwds)
  {
    ::addDefaultArg("readHID", 2, "timeout", Py::Long(-1), args, kwds);
    ::checkArgCount("readHID", 2, args, kwds);

    Py::List keys(2);
    keys[0] = Py::String("size");
    keys[1] = Py::String("timeout");

    Py::Dict d(kwds);
    ::mergeArgs("readHID", keys, args, d);

    Py::Long size(d["size"]);
    Py::Long timeout(d["timeout"]);

    uint8_t *puiData = new uint8_t[long(size)];
    int iResult = m_hidDevice.readHID(puiData, long(size),
                                      long(timeout));

    if ( iResult < 0 )
      {
        delete [] puiData;

        std::string szError;
        genErrorMessage("readHID", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
      }

    Py::List list(iResult);
    for ( Py::List::size_type i = 0; i < (unsigned int)(iResult); ++i )
      list[i] = Py::Long(puiData[i]);

    delete [] puiData;

    return list;
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_readHID)


  Py::Object pyhidaccess_readFeature(const Py::Tuple &args,
                                     const Py::Dict &kwds)
  {
    ::checkArgCount("readFeature", 2, args, kwds);

    Py::List keys(2);
    keys[0] = Py::String("id");
    keys[1] = Py::String("size");

    Py::Dict d(kwds);
    ::mergeArgs("readFeature", keys, args, d);

    Py::Long id(d["id"]);
    Py::Long size(d["size"]);

    uint8_t *puiData = new uint8_t[long(size)];
    puiData[0] = long(id);

    int iResult = m_hidDevice.readFeature(puiData, long(size));

    if ( iResult < 0 )
      {
        delete [] puiData;

        std::string szError;
        genErrorMessage("readFeature", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
      }

    Py::List list(iResult);
    for ( Py::List::size_type i = 0; i < (unsigned int)(iResult); ++i )
      list[i] = Py::Long(puiData[i]);

    delete [] puiData;

    return list;
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_readFeature)

  Py::Object pyhidaccess_waitDeviceReAdd(const Py::Tuple &args,
                                         const Py::Dict &kwds)
  {
    ::addDefaultArg("waitDeviceReAdd", 1, "timeout", Py::Long(0), args, kwds);

    Py::List keys(1);
    keys[0] = Py::String("timeout");

    Py::Dict d(kwds);
    ::mergeArgs("waitDeviceReAdd", keys, args, d);

    Py::Long timeout(d["timeout"]);

    if ( long(timeout) < 0 || long(timeout) > 0xFFFF )
      throw Py::ValueError("waitDeviceReAdd() Timeout is out of range.");

    int iResult = m_hidDevice.waitDeviceReAdd(long(timeout));

    if ( iResult )
      {
        std::string szError;
        genErrorMessage("waitDeviceReAdd", 0, iResult, szError);
        throw Py::Exception(pPyHID->m_HidError,
                            HIDError(iResult, szError).pythonExceptionArg());
      }

    return Py::None();
  }
  PYCXX_KEYWORDS_METHOD_DECL(pyhidaccess, pyhidaccess_waitDeviceReAdd)

  Py::Object pyhidaccess_closeHID(void)
  {
    m_hidDevice.closeHID();
    return Py::None();
  }
  PYCXX_NOARGS_METHOD_DECL(pyhidaccess, pyhidaccess_closeHID)

  public :
  pyhidaccess(Py::PythonClassInstance *pSelf, Py::Tuple &args, Py::Dict &kwds) :
  Py::PythonClass<pyhidaccess>::PythonClass(pSelf, args, kwds),
    m_value("default value")
  {
  }

  virtual ~pyhidaccess()
  {
    m_hidDevice.closeHID();
  }

  static void init_type()
  {
    behaviors().name("pyhidaccess");
    behaviors().doc("Raw HID access via Python");
    behaviors().supportGetattro();
    behaviors().supportSetattro();
    PYCXX_ADD_KEYWORDS_METHOD(openHID, pyhidaccess_openHID,
                              "Open HID device");
    PYCXX_ADD_KEYWORDS_METHOD(writeHID, pyhidaccess_writeHID,
                              "Write data to HID device");
    PYCXX_ADD_KEYWORDS_METHOD(writeFeature, pyhidaccess_writeFeature,
                              "Write feature report to HID device");
    PYCXX_ADD_KEYWORDS_METHOD(readHID, pyhidaccess_readHID,
                              "Read data from HID device");
    PYCXX_ADD_KEYWORDS_METHOD(readFeature, pyhidaccess_readFeature,
                              "Read feature data from HID device");
    PYCXX_ADD_KEYWORDS_METHOD(waitDeviceReAdd, pyhidaccess_waitDeviceReAdd,
                              "Wait until a known device reattaches");
    PYCXX_ADD_NOARGS_METHOD(closeHID, pyhidaccess_closeHID,
                            "Close HID device" );
    behaviors().readyType();
  }

  Py::Object getattro(const Py::String &name_)
  {
    std::string name(name_.as_std_string("utf-8"));
    if ( name == "value" )
      return this->m_value;
    return genericGetAttro(name_);
  }

  int setattro(const Py::String &name_, const Py::Object &value)
  {
    std::string name(name_.as_std_string("utf-8"));
    if ( name == "value" )
      {
        this->m_value = value;
        return 0;
      }
    return genericSetAttro(name_, value);
  }
};

pyhid_module::pyhid_module() : Py::ExtensionModule<pyhid_module>("pyhid"),
                               m_HidError()
{
  m_HidError.init(*this, "HIDError");

  pyhidaccess::init_type();
  initialize("Python HID access module");
  Py::Dict d(moduleDictionary());
  Py::Object x(pyhidaccess::type());
  d["pyhidaccess"] = x;
  d["HIDError"] = m_HidError;

  d["HID_LIBUSB_INVALID_ARGS"]    = Py::Int(HID_LIBUSB_INVALID_ARGS);
  d["HID_LIBUSB_NO_DEVICE"]       = Py::Int(HID_LIBUSB_NO_DEVICE);
  d["HID_LIBUSB_NO_DEVICE_OPEN"]  = Py::Int(HID_LIBUSB_NO_DEVICE_OPEN);
  d["HID_LIBUSB_READ_ERROR"]      = Py::Int(HID_LIBUSB_READ_ERROR);
  d["HID_LIBUSB_NO_UDEV"]         = Py::Int(HID_LIBUSB_NO_UDEV);
  d["HID_LIBUSB_UDEV_MON_ERROR"]  = Py::Int(HID_LIBUSB_UDEV_MON_ERROR);
  d["HID_LIBUSB_UDEV_TIMEOUT"]    = Py::Int(HID_LIBUSB_UDEV_TIMEOUT);
  d["LIBUSB_SUCCESS"]             = Py::Int(LIBUSB_SUCCESS);
  d["LIBUSB_ERROR_IO"]            = Py::Int(LIBUSB_ERROR_IO);
  d["LIBUSB_ERROR_INVALID_PARAM"] = Py::Int(LIBUSB_ERROR_INVALID_PARAM);
  d["LIBUSB_ERROR_ACCESS"]        = Py::Int(LIBUSB_ERROR_ACCESS);
  d["LIBUSB_ERROR_NO_DEVICE"]     = Py::Int(LIBUSB_ERROR_NO_DEVICE);
  d["LIBUSB_ERROR_NOT_FOUND"]     = Py::Int(LIBUSB_ERROR_NOT_FOUND);
  d["LIBUSB_ERROR_BUSY"]          = Py::Int(LIBUSB_ERROR_BUSY);
  d["LIBUSB_ERROR_TIMEOUT"]       = Py::Int(LIBUSB_ERROR_TIMEOUT);
  d["LIBUSB_ERROR_OVERFLOW"]      = Py::Int(LIBUSB_ERROR_OVERFLOW);
  d["LIBUSB_ERROR_PIPE"]          = Py::Int(LIBUSB_ERROR_PIPE);
  d["LIBUSB_ERROR_INTERRUPTED"]   = Py::Int(LIBUSB_ERROR_INTERRUPTED);
  d["LIBUSB_ERROR_NO_MEM"]        = Py::Int(LIBUSB_ERROR_NO_MEM);
  d["LIBUSB_ERROR_NOT_SUPPORTED"] = Py::Int(LIBUSB_ERROR_NOT_SUPPORTED);
  d["LIBUSB_ERROR_OTHER"]         = Py::Int(LIBUSB_ERROR_OTHER);
}

pyhid_module::~pyhid_module()
{
}

#if defined( _WIN32 )
 #define EXPORT_SYMBOL __declspec( dllexport )
#else
 #define EXPORT_SYMBOL
#endif

#if defined( PY3 )

extern "C" EXPORT_SYMBOL PyObject *PyInit_pyhid()
{
#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)
  Py::InitialisePythonIndirectPy::Interface();
#endif

  pPyHID = new pyhid_module;
  return pPyHID->module().ptr();
}

extern "C" EXPORT_SYMBOL PyObject *PyInit_pyhid_d()
{
  return PyInit_pyhid();
}

#else

extern "C" EXPORT_SYMBOL void initpyhid()
{
#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)
  Py::InitialisePythonIndirectPy::Interface();
#endif

  pPyHID = new pyhid_module;
}

extern "C" EXPORT_SYMBOL void initpyhid_d()
{
  initpyhid();
}
#endif
