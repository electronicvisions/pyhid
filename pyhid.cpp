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

inline static Py::List::size_type getArgCount(const Py::Tuple args,
                                              const Py::Dict kwds)
{
  return args.length() + kwds.length();
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
  hid_libusb m_hidDevice;

  Py::Object pyhidaccess_openHID(const Py::Tuple &args, const Py::Dict &kwds)
  {
    ::checkArgCount("openHID", 2, args, kwds);

    Py::List keys(2);
    keys[0] = Py::String("vid");
    keys[1] = Py::String("pid");

    Py::Dict d(kwds);
    ::mergeArgs("openHID", keys, args, d);

    Py::Long vid(d["vid"]);
    Py::Long pid(d["pid"]);

    if ( m_hidDevice.openHID(long(vid), long(pid)) < 0 )
      throw Py::IOError("openHID() failed to open HID device.");

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
      throw Py::IOError("writeHID() failed to write to HID device.");

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
      throw Py::IOError("writeFeature() failed to write to HID device.");

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
        throw Py::IOError("readHID() failed to read from HID device.");
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

    int result = m_hidDevice.readFeature(puiData, long(size));

    if ( result < 0 )
      {
        delete [] puiData;
        throw Py::IOError("readFeature() failed to read from HID device.");
      }

    Py::List list(result);
    for ( Py::List::size_type i = 0; i < (unsigned int)(result); ++i )
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

    bool bResult = m_hidDevice.waitDeviceReAdd(long(timeout));

    if ( !bResult )
      throw Py::IOError("waitDeviceReAdd() Failed to wait for device.");

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

  static void init_type(void)
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
      return m_value;
    return genericGetAttro(name_);
  }

  int setattro(const Py::String &name_, const Py::Object &value)
  {
    std::string name(name_.as_std_string("utf-8"));
    if ( name == "value" )
      {
        m_value = value;
        return 0;
      }

    return genericSetAttro(name_, value);
  }
};

class pyhid_module : public Py::ExtensionModule<pyhid_module>
{
public:
  pyhid_module() : Py::ExtensionModule<pyhid_module>("pyhid")
  {
    pyhidaccess::init_type();
    initialize("Python HID access module");
    Py::Dict d(moduleDictionary());
    Py::Object x(pyhidaccess::type());
    d["pyhidaccess"] = x;
  }

  virtual ~pyhid_module()
  {
  }
};

#if defined( _WIN32 )
 #define EXPORT_SYMBOL __declspec( dllexport )
#else
 #define EXPORT_SYMBOL
#endif

static pyhid_module *pPyHID;

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
