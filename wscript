## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
from waflib import Logs, Utils, Options, TaskGen, Task
from waflib.Errors import WafError

import wutils

REQUIRED_BOOST_LIBS = ['graph', 'thread', 'unit_test_framework',
                       'system', 'random', 'date_time', 'iostreams', 'regex', 'program_options', 'chrono', 'filesystem']

def required_boost_libs(conf):
    conf.env.REQUIRED_BOOST_LIBS += REQUIRED_BOOST_LIBS

def options(opt):
    opt.load(['version'], tooldir=['%s/.waf-tools' % opt.path.abspath()])
    opt.load(['doxygen', 'sphinx_build', 'type_traits', 'compiler-features', 'cryptopp', 'sqlite3'],
             tooldir=['%s/ndn-cxx/.waf-tools' % opt.path.abspath()])
    opt.add_option('--with-dash',
                   help=('libdash path - assuming ../AMuSt-libdash/libdash/ if not provided'
                         '  - should contain the following subfolders: libdash/include/, build/bin/ '
                         '  - should contain the following files: header files, shared object (libdash.so)'),
                   default=False, dest='with_dash')

def configure(conf):
    conf.load(['doxygen', 'sphinx_build', 'type_traits', 'compiler-features', 'version', 'cryptopp', 'sqlite3'])

    conf.env['ENABLE_NDNSIM']=False

    if not os.environ.has_key('PKG_CONFIG_PATH'):
        os.environ['PKG_CONFIG_PATH'] = ':'.join([
            '/usr/local/lib/pkgconfig',
            '/usr/local/lib64/pkgconfig',
            '/usr/local/lib32/pkgconfig',
            '/opt/local/lib/pkgconfig'])

    conf.check_cxx(lib='pthread', uselib_store='PTHREAD', define_name='HAVE_PTHREAD', mandatory=False)
    conf.check_sqlite3(mandatory=True)
    conf.check_cryptopp(mandatory=True, use='PTHREAD')


 
    # check for libdash
    conf.env['ENABLE_DASH'] = False
    lib_to_check = 'libdash.so'
    libdash_default_dir = "../AMuSt-libdash/libdash/" # according to tutorial

    if Options.options.with_dash:
        conf.msg("Checking AMuSt-libdash location", ("%s (given)" % Options.options.with_dash))
        libdash_dir = os.path.abspath(os.path.join(Options.options.with_dash, "build/bin/"))
        if os.path.exists(os.path.join(libdash_dir, lib_to_check)):
            conf.env['WITH_DASH'] = os.path.abspath(Options.options.with_dash)
        else:
            # Add this module to the list of modules that won't be built
            # if they are enabled.
            Logs.error ("AMuSt-ndnSIM requires AMuSt-libdash. Please consult the manual and documentation for details.")
            Logs.error ("Could not find " +  lib_to_check  + " in " + os.path.join(libdash_dir, lib_to_check))
            conf.env['MODULES_NOT_BUILT'].append('dash')
            conf.report_optional_feature("libdash", "AMuSt-LIBDASH Integration",  False, "Not found in " + str(Options.options.with_dash) + " (see option --with-dash)")
            return
    else:
        # No user specified '--with-dash' option, try to guess
        # we have built it, so it should be in ../libdash/libdash/
        libdash_dir = os.path.abspath(os.path.join(libdash_default_dir, "build/bin/"))

        if os.path.exists(os.path.join(libdash_dir, lib_to_check)):
            conf.msg("Checking for LIBDASH location", ("%s (guessed)" % libdash_default_dir))
            conf.env['WITH_DASH'] = os.path.abspath(libdash_default_dir)
        else:
            # Add this module to the list of modules that won't be built
            # if they are enabled.
            Logs.error ("AMuSt-ndnSIM requires AMuSt-libdash. Please consult the manual and documentation for details.")
            Logs.error ("Could not find " +  lib_to_check  + " in " + os.path.join(libdash_dir, lib_to_check))
            conf.env['MODULES_NOT_BUILT'].append('dash')
            conf.report_optional_feature("libdash", "AMuSt-LIBDASH Integration",  False, "Not found in " + str(libdash_default_dir) + " (see option --with-dash)")
            return
    # end if - libdash should be found now


    test_code = '''
#include "libdash.h"

int main()
{
  return 0;
}
'''

    conf.env.append_value('NS3_MODULE_PATH',os.path.abspath(os.path.join(conf.env['WITH_DASH'], 'build/bin/')))

    conf.env['INCLUDES_DASH'] = os.path.join(conf.env['WITH_DASH'], "libdash/include/")
    conf.env['LIBPATH_DASH'] = [ os.path.join(conf.env['WITH_DASH'], "build/bin/") ]

    conf.env['DASH'] = conf.check(fragment=test_code, lib='dash', libpath=conf.env['LIBPATH_DASH'], use='DASH', mandatory=False)

    if conf.env['DASH']:
        conf.env['ENABLE_DASH'] = True
        conf.env.append_value('CXXDEFINES', 'NS3_LIBDASH')
        conf.report_optional_feature("libdash", "LIBDASH Integration", True, "Found in " + str(os.path.join(conf.env['WITH_DASH'])))
    else:
        Logs.error ("amus-ndnSIM requires libdash. Please consult the manual and documentation for details.")
        Logs.error ("   failed to compile test_code for libdash")
        Logs.error ("   Please ensure that libdash.h is available in " + str(conf.env['INCLUDES_DASH']))
        conf.env['MODULES_NOT_BUILT'].append('libdash')
        conf.report_optional_feature("libdash", "LIBDASH Integration",  False, "Not found in " + str(os.path.join(conf.env['WITH_DASH']) + ", (see option --with-dash)"))
        return


    if not conf.env['LIB_BOOST']:
        conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                     "Required boost libraries not found")
        Logs.error ("ndnSIM will not be build as it requires boost libraries of version at least 1.48")
        conf.env['MODULES_NOT_BUILT'].append('ndnSIM')
        return
    else:
        present_boost_libs = []
        for boost_lib_name in conf.env['LIB_BOOST']:
            if boost_lib_name.startswith("boost_"):
                boost_lib_name = boost_lib_name[6:]
            if boost_lib_name.endswith("-mt"):
                boost_lib_name = boost_lib_name[:-3]
            present_boost_libs.append(boost_lib_name)

        missing_boost_libs = [lib for lib in REQUIRED_BOOST_LIBS if lib not in present_boost_libs]

        if missing_boost_libs != []:
            conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                         "ndnSIM requires boost libraries: %s" % ' '.join(missing_boost_libs))
            conf.env['MODULES_NOT_BUILT'].append('ndnSIM')

            Logs.error ("ndnSIM will not be build as it requires boost libraries: %s" % ' '.join(missing_boost_libs))
            Logs.error ("Please upgrade your distribution or install custom boost libraries (http://ndnsim.net/faq.html#boost-libraries)")
            return

        boost_version = conf.env.BOOST_VERSION.split('_')
        if int(boost_version[0]) < 1 or int(boost_version[1]) < 48:
            conf.report_optional_feature("ndnSIM", "ndnSIM", False,
                                         "ndnSIM requires at least boost version 1.48")
            conf.env['MODULES_NOT_BUILT'].append('ndnSIM')

            Logs.error ("ndnSIM will not be build as it requires boost libraries of version at least 1.48")
            Logs.error ("Please upgrade your distribution or install custom boost libraries (http://ndnsim.net/faq.html#boost-libraries)")
            return

    conf.env['ENABLE_NDNSIM']=True;
    conf.env['MODULES_BUILT'].append('ndnSIM')

    conf.report_optional_feature("ndnSIM", "ndnSIM", True, "")

    conf.write_config_header('../../ns3/ndnSIM/ndn-cxx/ndn-cxx-config.hpp', define_prefix='NDN_CXX_', remove=False)
    conf.write_config_header('../../ns3/ndnSIM/NFD/config.hpp', remove=False)

def build(bld):
    (base, build, split) = bld.getVersion('NFD')
    bld(features="subst",
        name="version-NFD",
        source='NFD/version.hpp.in', target='../../ns3/ndnSIM/NFD/version.hpp',
        install_path=None,
        VERSION_STRING=base,
        VERSION_BUILD="%s-ndnSIM" % build,
        VERSION=int(split[0]) * 1000000 + int(split[1]) * 1000 + int(split[2]),
        VERSION_MAJOR=split[0], VERSION_MINOR=split[1], VERSION_PATCH=split[2])

    (base, build, split) = bld.getVersion('ndn-cxx')
    bld(features="subst",
        name="version-ndn-cxx",
        source='ndn-cxx/src/version.hpp.in', target='../../ns3/ndnSIM/ndn-cxx/version.hpp',
        install_path=None,
        VERSION_STRING=base,
        VERSION_BUILD="%s-ndnSIM" % build,
        VERSION=int(split[0]) * 1000000 + int(split[1]) * 1000 + int(split[2]),
        VERSION_MAJOR=split[0], VERSION_MINOR=split[1], VERSION_PATCH=split[2])

    deps = ['core', 'network', 'point-to-point', 'topology-read', 'mobility', 'internet']
    if 'ns3-visualizer' in bld.env['NS3_ENABLED_MODULES']:
        deps.append('visualizer')

    if bld.env.ENABLE_EXAMPLES:
        deps += ['point-to-point-layout', 'csma', 'applications', 'wifi']

    ndnCxxSrc = bld.path.ant_glob('ndn-cxx/src/**/*.cpp',
                                  excl=['ndn-cxx/src/**/*-osx.cpp',
                                        'ndn-cxx/src/util/dummy-client-face.cpp'])

    nfdSrc = bld.path.ant_glob(['%s/**/*.cpp' % dir for dir in ['NFD/core', 'NFD/daemon', 'NFD/rib']],
                               excl=['NFD/core/network-interface.cpp',
                                     'NFD/daemon/main.cpp',
                                     'NFD/daemon/nfd.cpp',
                                     'NFD/daemon/face/ethernet*',
                                     'NFD/daemon/face/multicast-udp*',
                                     'NFD/daemon/face/tcp*',
                                     'NFD/daemon/face/udp*',
                                     'NFD/daemon/face/unix-stream*',
                                     'NFD/daemon/face/websocket*',
                                     'NFD/rib/nrd.cpp'])

    module = bld.create_ns3_module('ndnSIM', deps)
    module.module = 'ndnSIM'
    module.features += ' ns3fullmoduleheaders ndncxxheaders'
    module.use += ['version-ndn-cxx', 'version-NFD', 'BOOST', 'CRYPTOPP', 'SQLITE3', 'RT', 'PTHREAD', 'DASH']
    module.includes = ['../..', '../../ns3/ndnSIM/NFD', './NFD/core', './NFD/daemon', './NFD/rib', '../../ns3/ndnSIM', '../../ns3/ndnSIM/ndn-cxx']
    module.export_includes = ['../../ns3/ndnSIM/NFD', './NFD/core', './NFD/daemon', './NFD/rib', '../../ns3/ndnSIM']

    headers = bld(features='ns3header')
    headers.module = 'ndnSIM'
    headers.source = ["ndn-all.hpp"]

    if not bld.env['ENABLE_NDNSIM']:
        bld.env['MODULES_NOT_BUILT'].append('ndnSIM')
        return

    module_dirs = ['apps', 'helper', 'model', 'utils']
    module.source = bld.path.ant_glob(['%s/**/*.cpp' % dir for dir in module_dirs],
                                      excl=['model/ip-faces/*']) + ndnCxxSrc + nfdSrc

    module_dirs = ['NFD/core', 'NFD/daemon', 'NFD/rib', 'apps', 'helper', 'model', 'utils']
    module.full_headers = bld.path.ant_glob(['%s/**/*.hpp' % dir for dir in module_dirs])
    module.full_headers += bld.path.ant_glob('NFD/common.hpp')

    module.ndncxx_headers = bld.path.ant_glob(['ndn-cxx/src/**/*.hpp'],
                                              excl=['src/**/*-osx.hpp', 'src/detail/**/*'])
    if bld.env['ENABLE_BRITE']:
        module.includes.append(os.path.abspath(os.path.join(bld.env['WITH_BRITE'],'.')))

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    if bld.env.ENABLE_TESTS:
        bld.recurse('tests')

    bld.ns3_python_bindings()

@TaskGen.feature('ns3fullmoduleheaders')
@TaskGen.after_method('process_rule')
def apply_ns3fullmoduleheaders(self):
    # ## get all of the ns3 headers
    ns3_dir_node = self.bld.path.find_or_declare("ns3")

    mode = getattr(self, "mode", "install")

    for src_node in set(self.full_headers):
        dst_node = ns3_dir_node.find_or_declare(src_node.path_from(self.bld.path.find_dir('src')))
        assert dst_node is not None

        relpath = src_node.parent.path_from(self.bld.path.find_dir('src'))

        task = self.create_task('ns3header')
        task.mode = getattr(self, 'mode', 'install')
        if task.mode == 'install':
            self.bld.install_files('${INCLUDEDIR}/%s%s/ns3/%s' % (wutils.APPNAME, wutils.VERSION, relpath),
                                   [src_node])
            task.set_inputs([src_node])
            task.set_outputs([dst_node])
        else:
            task.header_to_remove = dst_node

@TaskGen.feature('ndncxxheaders')
@TaskGen.after_method('process_rule')
def apply_ndnsim_moduleheaders(self):
    # ## get all of the ns3 headers
    ndncxx_dir_node = self.bld.path.find_or_declare("ns3/ndnSIM/ndn-cxx")

    mode = getattr(self, "mode", "install")

    for src_node in set(self.ndncxx_headers):
        dst_node = ndncxx_dir_node.find_or_declare(src_node.path_from(self.bld.path.find_dir('src/ndnSIM/ndn-cxx/src')))
        assert dst_node is not None

        relpath = src_node.parent.path_from(self.bld.path.find_dir('src/ndnSIM/ndn-cxx/src'))

        task = self.create_task('ns3header')
        task.mode = getattr(self, 'mode', 'install')
        if task.mode == 'install':
            self.bld.install_files('${INCLUDEDIR}/%s%s/ns3/ndnSIM/ndn-cxx/%s' % (wutils.APPNAME, wutils.VERSION, relpath),
                                   [src_node])
            task.set_inputs([src_node])
            task.set_outputs([dst_node])
        else:
            task.header_to_remove = dst_node
