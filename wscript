#!/usr/bin/env python
import os
from waflib.extras.test_base import summary


def depends(ctx):
    pass

def options(opt):
    opt.load('compiler_cxx')
    opt.load('python')
    opt.load('test_base')

def configure(conf):
    conf.load('compiler_cxx')
    conf.load('test_base')
    conf.load('python')
    conf.load('genpybind')
    conf.check_python_headers()
    conf.check_cfg(package='libusb-1.0', args=['--cflags', '--libs'], uselib_store='USB1')


def build(bld):
    bld(target = 'pyhid_inc',
        includes = 'include',
        export_includes = 'include',
    )

    bld.shlib(
        target          = 'hid_libusb',
        features        = 'cxx',
        source          = 'src/pyhid/hid_libusb.cpp',
        use             = 'pyhid_inc USB1',
        install_path    = '${PREFIX}/lib',
    )

    bld(
        features = 'genpybind cxx cxxshlib pyext',
        target = 'pyhid',
        source = 'include/pyhid/genpybind.h',
        use = 'hid_libusb pyhid_inc',
    )

    bld.add_post_fun(summary)
