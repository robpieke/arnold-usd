# vim: filetype=python
# Copyright 2019 Autodesk, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import glob
import re
import shutil
import sys, os
import platform
from SCons.Script import PathVariable
import SCons

# Disable warning about Python 2.6 being deprecated
SetOption('warn', 'no-python-version')

# Local helper tools
sys.path = [os.path.abspath(os.path.join('tools'))] + sys.path

from utils import system, configure
from utils.system import IS_WINDOWS, IS_LINUX, IS_DARWIN
from utils.build_tools import *

# Allowed compilers
if IS_WINDOWS:
    ALLOWED_COMPILERS = ['msvc', 'icc']
    arnold_default_api_lib = os.path.join('$ARNOLD_PATH', 'lib')
else:
    ALLOWED_COMPILERS = ['gcc', 'clang']
    arnold_default_api_lib = os.path.join('$ARNOLD_PATH', 'bin')

# Scons doesn't provide a string variable
def StringVariable(key, help, default):
    # We always get string values, so it's always valid and trivial to convert
    return (key, help, default, lambda k, v, e: True, lambda s: s)

# Custom variables definitions
vars = Variables('custom.py')
vars.AddVariables(
    PathVariable('BUILD_DIR', 'Directory where temporary build files are placed by scons', 'build', PathVariable.PathIsDirCreate),
    PathVariable('REFERENCE_DIR', 'Directory where the test reference images are stored.', 'testsuite', PathVariable.PathIsDirCreate),
    EnumVariable('MODE', 'Set compiler configuration', 'opt', allowed_values=('opt', 'debug', 'profile')),
    EnumVariable('WARN_LEVEL', 'Set warning level', 'none', allowed_values=('strict', 'warn-only', 'none')),
    EnumVariable('COMPILER', 'Set compiler to use', ALLOWED_COMPILERS[0], allowed_values=ALLOWED_COMPILERS),
    PathVariable('SHCXX', 'C++ compiler used for generating shared-library objects', None),
    EnumVariable('CXX_STANDARD', 'C++ standard for gcc/clang.', '11', allowed_values=('11', '14', '17', '20')),
    PathVariable('ARNOLD_PATH', 'Arnold installation root', os.getenv('ARNOLD_PATH', None), PathVariable.PathIsDir),
    PathVariable('ARNOLD_API_INCLUDES', 'Where to find Arnold API includes', os.path.join('$ARNOLD_PATH', 'include'), PathVariable.PathIsDir),
    PathVariable('ARNOLD_API_LIB', 'Where to find Arnold API static libraries', arnold_default_api_lib, PathVariable.PathIsDir),
    PathVariable('ARNOLD_BINARIES', 'Where to find Arnold API dynamic libraries and executables', os.path.join('$ARNOLD_PATH', 'bin'), PathVariable.PathIsDir),
    PathVariable('ARNOLD_PYTHON', 'Where to find Arnold python bindings', os.path.join('$ARNOLD_PATH', 'python'), PathVariable.PathIsDir),  
    PathVariable('USD_PATH', 'USD installation root', os.getenv('USD_PATH', None)),
    PathVariable('USD_INCLUDE', 'Where to find USD includes', os.path.join('$USD_PATH', 'include'), PathVariable.PathIsDir),
    PathVariable('USD_LIB', 'Where to find USD libraries', os.path.join('$USD_PATH', 'lib'), PathVariable.PathIsDir),
    PathVariable('USD_BIN', 'Where to find USD binaries', os.path.join('$USD_PATH', 'bin'), PathVariable.PathIsDir),   
    EnumVariable('USD_BUILD_MODE', 'Build mode of USD libraries', 'monolithic', allowed_values=('shared_libs', 'monolithic', 'static')),
    StringVariable('USD_LIB_PREFIX', 'USD library prefix', 'lib'),
    # 'static'  will expect a static monolithic library "libusd_m". When doing a monolithic build of USD, this 
    # library can be found in the build/pxr folder
    PathVariable('BOOST_INCLUDE', 'Where to find Boost includes', os.path.join('$USD_PATH', 'include', 'boost-1_61'), PathVariable.PathIsDir),
    PathVariable('BOOST_LIB', 'Where to find Boost libraries', '.', PathVariable.PathIsDir),
    BoolVariable('BOOST_ALL_NO_LIB', 'Disable automatic linking of boost libraries on Windows.', False),
    PathVariable('PYTHON_INCLUDE', 'Where to find Python includes (pyconfig.h)', os.getenv('PYTHON_INCLUDE', None)),
    PathVariable('PYTHON_LIB', 'Where to find Python libraries (python27.lib) ', os.getenv('PYTHON_LIB', None)),
    PathVariable('TBB_INCLUDE', 'Where to find TBB headers.', os.getenv('TBB_INCLUDE', None)),
    PathVariable('TBB_LIB', 'Where to find TBB libraries', os.getenv('TBB_LIB', None)),
    BoolVariable('TBB_STATIC', 'Whether we link against a static TBB library', False),
    # Google test dependency
    PathVariable('GOOGLETEST_PATH', 'Google Test installation root', '.', PathVariable.PathAccept),
    PathVariable('GOOGLETEST_INCLUDE', 'Where to find Google Test includes', os.path.join('$GOOGLETEST_PATH', 'include'), PathVariable.PathAccept),
    PathVariable('GOOGLETEST_LIB', 'Where to find Google Test libraries', os.path.join('$GOOGLETEST_PATH', 'lib64' if IS_LINUX else 'lib'), PathVariable.PathAccept),
    BoolVariable('ENABLE_UNIT_TESTS', 'Whether or not to enable C++ unit tests. This feature requires Google Test.', False),
    EnumVariable('TEST_ORDER', 'Set the execution order of tests to be run', 'reverse', allowed_values=('normal', 'reverse')),
    EnumVariable('SHOW_TEST_OUTPUT', 'Display the test log as it is being run', 'single', allowed_values=('always', 'never', 'single')),
    EnumVariable('USE_VALGRIND', 'Enable Valgrinding', 'False', allowed_values=('False', 'True', 'Full')),
    BoolVariable('UPDATE_REFERENCE', 'Update the reference log/image for the specified targets', False),
    PathVariable('PREFIX', 'Directory to install under', '.', PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_PROCEDURAL', 'Directory to install the procedural under.', os.path.join('$PREFIX', 'procedural'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_RENDER_DELEGATE', 'Directory to install the render delegate under.', os.path.join('$PREFIX', 'plugin'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_NDR_PLUGIN', 'Directory to install the ndr plugin under.', os.path.join('$PREFIX', 'plugin'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_USD_IMAGING_PLUGIN', 'Directory to install the usd imaging plugin under.', os.path.join('$PREFIX', 'plugin'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_SCENE_DELEGATE', 'Directory to install the scene delegate under.', os.path.join('$PREFIX', 'plugin'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_HEADERS', 'Directory to install the headers under.', os.path.join('$PREFIX', 'include'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_SCHEMAS', 'Directory to install the schemas under.', os.path.join('$PREFIX', 'schema'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_BIN', 'Directory to install the binaries under.', os.path.join('$PREFIX', 'bin'), PathVariable.PathIsDirCreate),
    PathVariable('PREFIX_DOCS', 'Directory to install the documentation under.', os.path.join('$PREFIX', 'docs'), PathVariable.PathIsDirCreate),
    BoolVariable('SHOW_PLOTS', 'Display timing plots for the testsuite. gnuplot has to be found in the environment path.', False),
    BoolVariable('BUILD_SCHEMAS', 'Whether or not to build the schemas and their wrapper.', True),
    BoolVariable('BUILD_RENDER_DELEGATE', 'Whether or not to build the hydra render delegate.', True),
    BoolVariable('BUILD_NDR_PLUGIN', 'Whether or not to build the node registry plugin.', True),
    BoolVariable('BUILD_USD_IMAGING_PLUGIN', 'Whether or not to build the usdImaging plugin.', True),
    BoolVariable('BUILD_USD_WRITER', 'Whether or not to build the arnold to usd writer tool.', True),
    BoolVariable('BUILD_PROCEDURAL', 'Whether or not to build the arnold procedural.', True),
    BoolVariable('BUILD_SCENE_DELEGATE', 'Whether or not to build the arnold scene delegate.', False),
    BoolVariable('BUILD_TESTSUITE', 'Whether or not to build the testsuite.', True),
    BoolVariable('BUILD_DOCS', 'Whether or not to build the documentation.', True),
    BoolVariable('PROC_SCENE_FORMAT', 'Whether or not to build the procedural with a scene format plugin.', True),
    BoolVariable('DISABLE_CXX11_ABI', 'Disable the use of the CXX11 abi for gcc/clang', False),
    StringVariable('BOOST_LIB_NAME', 'Boost library name pattern', 'boost_%s'),
    StringVariable('TBB_LIB_NAME', 'TBB library name pattern', '%s'),
    StringVariable('USD_MONOLITHIC_LIBRARY', 'Name of the USD monolithic library', 'usd_ms'),
    StringVariable('PYTHON_LIB_NAME', 'Name of the python library', 'python27'),
    StringVariable('USD_PROCEDURAL_NAME', 'Name of the usd procedural.', 'usd'),
    ('TEST_PATTERN', 'Glob pattern of tests to be run', 'test_*'),
    ('KICK_PARAMS', 'Additional parameters for kick', '-v 6')
)

if IS_WINDOWS:
    vars.Add(EnumVariable('MSVC_VERSION', 'Version of MS Visual Studio to use', '14.0', allowed_values=('8.0', '9.0', '10.0', '11.0', '14.0', '14.1', '14.2')))
else:
    vars.Add(BoolVariable('RPATH_ADD_ARNOLD_BINARIES', 'Add Arnold binaries to the RPATH', False))

if IS_DARWIN:
    vars.Add(('SDK_VERSION', 'Version of the Mac OSX SDK to use', '')) # use system default
    vars.Add(PathVariable('SDK_PATH', 'Root path to installed OSX SDKs', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs'))
    vars.Add(('MACOS_VERSION_MIN', 'Minimum compatibility with Mac OSX', '10.11'))

# Create the scons environment
env = Environment(variables = vars, ENV = os.environ, tools = ['default', 'doxygen'])

BUILD_DIR = env.subst(env['BUILD_DIR'])
REFERENCE_DIR = env.subst(env['REFERENCE_DIR'])

## Tells SCons to store all file signatures in the database
## ".sconsign.<SCons_version>.dblite" instead of the default ".sconsign.dblite".
SConsignFile(os.path.join(BUILD_DIR, '.sconsign.%s' % (SCons.__version__)))

# We are disabling unit tests on MacOS for now.
if IS_DARWIN:
    env['ENABLE_UNIT_TESTS'] = False

def get_optional_env_var(env_name):
    return env.subst(env[env_name]) if env_name in env else None

USD_BUILD_MODE        = env['USD_BUILD_MODE']

BUILD_SCHEMAS            = env['BUILD_SCHEMAS']
BUILD_RENDER_DELEGATE    = env['BUILD_RENDER_DELEGATE'] if USD_BUILD_MODE != 'static' else False
BUILD_NDR_PLUGIN         = env['BUILD_NDR_PLUGIN'] if USD_BUILD_MODE != 'static' else False
BUILD_USD_IMAGING_PLUGIN = env['BUILD_USD_IMAGING_PLUGIN'] if BUILD_SCHEMAS else False
BUILD_SCENE_DELEGATE     = env['BUILD_SCENE_DELEGATE'] if USD_BUILD_MODE != 'static' else False
BUILD_USD_WRITER         = env['BUILD_USD_WRITER']
BUILD_PROCEDURAL         = env['BUILD_PROCEDURAL']
BUILD_TESTSUITE          = env['BUILD_TESTSUITE']
BUILD_DOCS               = env['BUILD_DOCS']

USD_LIB_PREFIX        = env['USD_LIB_PREFIX']

env['USD_LIB_AS_SOURCE'] = None
# There are two possible behaviors with USD_LIB_PREFIX, if it starts with 'lib'
# then we have to remove it, since gcc and clang automatically substitutes it on
# non windows platforms. If the prefix does not start with lib, then we have to
# force scons to properly link against libs named in a non-standard way.
if not IS_WINDOWS:
    if USD_LIB_PREFIX.startswith('lib'):
        USD_LIB_PREFIX = USD_LIB_PREFIX[3:]
    else:
        # Scons needs this variable, so we can pass uniquely named shared
        # objects to link against. This is only required on osx and linux.
        env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1
        env['USD_LIB_AS_SOURCE'] = True

env['USD_LIB_PREFIX'] = USD_LIB_PREFIX

# Forcing the build of the procedural when the testsuite is enabled.
if BUILD_TESTSUITE:
    BUILD_PROCEDURAL = True

ARNOLD_PATH         = env.subst(env['ARNOLD_PATH'])
ARNOLD_API_INCLUDES = env.subst(env['ARNOLD_API_INCLUDES'])
ARNOLD_API_LIB      = env.subst(env['ARNOLD_API_LIB'])
ARNOLD_BINARIES     = env.subst(env['ARNOLD_BINARIES'])

if not IS_WINDOWS and env['RPATH_ADD_ARNOLD_BINARIES']:
    env['RPATH'] = ARNOLD_BINARIES

env['ARNOLD_BINARIES'] = ARNOLD_BINARIES

PREFIX                    = env.subst(env['PREFIX'])
PREFIX_PROCEDURAL         = env.subst(env['PREFIX_PROCEDURAL'])
PREFIX_RENDER_DELEGATE    = env.subst(env['PREFIX_RENDER_DELEGATE'])
PREFIX_NDR_PLUGIN         = env.subst(env['PREFIX_NDR_PLUGIN'])
PREFIX_USD_IMAGING_PLUGIN = env.subst(env['PREFIX_USD_IMAGING_PLUGIN'])
PREFIX_SCENE_DELEGATE     = env.subst(env['PREFIX_SCENE_DELEGATE'])
PREFIX_HEADERS            = env.subst(env['PREFIX_HEADERS'])
PREFIX_SCHEMAS            = env.subst(env['PREFIX_SCHEMAS'])
PREFIX_BIN                = env.subst(env['PREFIX_BIN'])
PREFIX_DOCS               = env.subst(env['PREFIX_DOCS'])

USD_PATH = env.subst(env['USD_PATH'])
USD_INCLUDE = env.subst(env['USD_INCLUDE'])
USD_LIB = env.subst(env['USD_LIB'])
USD_BIN = env.subst(env['USD_BIN'])

# Storing values after expansion
env['USD_PATH'] = USD_PATH
env['USD_INCLUDE'] = USD_INCLUDE
env['USD_LIB'] = USD_LIB
env['USD_BIN'] = USD_BIN

# these could be supplied by linux / osx
BOOST_INCLUDE = get_optional_env_var('BOOST_INCLUDE')
BOOST_LIB = get_optional_env_var('BOOST_LIB')
PYTHON_INCLUDE = get_optional_env_var('PYTHON_INCLUDE')
PYTHON_LIB = get_optional_env_var('PYTHON_LIB')
TBB_INCLUDE = get_optional_env_var('TBB_INCLUDE')
TBB_LIB = get_optional_env_var('TBB_LIB')
if env['ENABLE_UNIT_TESTS']:
    GOOGLETEST_INCLUDE = env.subst(env['GOOGLETEST_INCLUDE'])
    GOOGLETEST_LIB = env.subst(env['GOOGLETEST_LIB'])
else:
    GOOGLETEST_INCLUDE = None
    GOOGLETEST_LIB = None

env['PYTHON_LIBRARY'] = File(env['PYTHON_LIB_NAME']) if os.path.isabs(env['PYTHON_LIB_NAME']) else env['PYTHON_LIB_NAME']

if env['COMPILER'] == 'clang':
   env['CC']  = 'clang'
   env['CXX']  =  'clang++'

# force compiler to match SHCXX
if env['SHCXX'] != '$CXX':
   env['CXX'] = env['SHCXX']

# Get Arnold version
env['ARNOLD_VERSION'] = get_arnold_version(ARNOLD_API_INCLUDES)

if env['PROC_SCENE_FORMAT']:
    env['ARNOLD_HAS_SCENE_FORMAT_API'] = get_arnold_has_scene_format_api(ARNOLD_API_INCLUDES)
else:
    env['ARNOLD_HAS_SCENE_FORMAT_API'] = 0

# Get USD Version
header_info = get_usd_header_info(USD_INCLUDE) 
env['USD_VERSION'] = header_info['USD_VERSION']
env['USD_HAS_PYTHON_SUPPORT'] = header_info['USD_HAS_PYTHON_SUPPORT']
env['USD_HAS_UPDATED_COMPOSITOR'] = header_info['USD_HAS_UPDATED_COMPOSITOR']
env['USD_HAS_FULLSCREEN_SHADER'] = header_info['USD_HAS_FULLSCREEN_SHADER']

if env['COMPILER'] in ['gcc', 'clang'] and env['SHCXX'] != '$CXX':
   env['GCC_VERSION'] = os.path.splitext(os.popen(env['SHCXX'] + ' -dumpversion').read())[0]


print("Building Arnold-USD:")
print(" - Build mode: '{}'".format(env['MODE']))
print(" - Host OS: '{}'".format(system.os))
print(" - Arnold version: '{}'".format(env['ARNOLD_VERSION']))
#print(" - Environment:")
#for k, v in os.environ.items():
#    print("     {} = {}".format(k,v))


# Platform definitions
if IS_DARWIN:
    env.Append(CPPDEFINES = Split('_DARWIN'))
elif IS_LINUX:
    env.Append(CPPDEFINES = Split('_LINUX'))
elif IS_WINDOWS:
    env.Append(CPPDEFINES = Split('_WINDOWS _WIN32 WIN32 _USE_MATH_DEFINES'))
    env.Append(CPPDEFINES = Split('_WIN64'))
    if env['TBB_LIB_NAME'] != '%s':
        env.Append(CPPDEFINES = Split('__TBB_NO_IMPLICIT_LINKAGE=1'))
    if env['BOOST_ALL_NO_LIB']:
        env.Append(CPPDEFINES = Split('BOOST_ALL_NO_LIB HBOOST_ALL_NO_LIB'))

# Adding USD paths to environment for the teststuite
dylib = 'PATH' if IS_WINDOWS else ('DYLD_LIBRARY_PATH' if IS_DARWIN else 'LD_LIBRARY_PATH')
env_separator = ';' if IS_WINDOWS else ':'

env.AppendENVPath(dylib, USD_LIB, envname='ENV', sep=env_separator, delete_existing=1)
env.AppendENVPath(dylib, USD_BIN, envname='ENV', sep=env_separator, delete_existing=1)
env.AppendENVPath(dylib, ARNOLD_BINARIES, envname='ENV', sep=env_separator, delete_existing=1)
env.AppendENVPath('PYTHONPATH', os.path.join(USD_LIB, 'python'), envname='ENV', sep=env_separator, delete_existing=1)
env.AppendENVPath('PXR_PLUGINPATH_NAME', os.path.join(USD_PATH, 'plugin', 'usd'), envname='ENV', sep=env_separator, delete_existing=1)
os.environ['PATH'] = env['ENV']['PATH']
os.putenv('PATH', os.environ['PATH'])
os.environ['PYTHONPATH'] = env['ENV']['PYTHONPATH']
os.putenv('PYTHONPATH', os.environ['PYTHONPATH'])
os.environ['PXR_PLUGINPATH_NAME'] = env['ENV']['PXR_PLUGINPATH_NAME']
os.putenv('PXR_PLUGINPATH_NAME', os.environ['PXR_PLUGINPATH_NAME'])

env['ENV']['ARNOLD_PATH'] = os.path.abspath(ARNOLD_PATH)
env['ENV']['ARNOLD_BINARIES'] = os.path.abspath(ARNOLD_BINARIES)
env['ENV']['PREFIX_BIN'] = os.path.abspath(PREFIX_BIN)
env['ENV']['PREFIX_PROCEDURAL'] = os.path.abspath(PREFIX_PROCEDURAL)

# Compiler settings
if env['COMPILER'] in ['gcc', 'clang']:
    env.Append(CCFLAGS = Split('-fno-operator-names -std=c++{}'.format(env['CXX_STANDARD'])))
    if IS_DARWIN:
        env.Append(LINKFLAGS = '-Wl,-undefined,error')
        env_dict = env.Dictionary()
        # Minimum compatibility with Mac OSX "env['MACOS_VERSION_MIN']"
        env.Append(CCFLAGS   = ['-mmacosx-version-min={MACOS_VERSION_MIN}'.format(**env_dict)])
        env.Append(LINKFLAGS = ['-mmacosx-version-min={MACOS_VERSION_MIN}'.format(**env_dict)])
        env.Append(CCFLAGS   = ['-isysroot','{SDK_PATH}/MacOSX{SDK_VERSION}.sdk/'.format(**env_dict)])
        env.Append(LINKFLAGS = ['-isysroot','{SDK_PATH}/MacOSX{SDK_VERSION}.sdk/'.format(**env_dict)])
    else:
        env.Append(LINKFLAGS = '-Wl,--no-undefined')
        if env['DISABLE_CXX11_ABI']:
            env.Append(CPPDEFINES = [{'_GLIBCXX_USE_CXX11_ABI' : 0}])
    # Warning level
    if env['WARN_LEVEL'] == 'none':
        env.Append(CCFLAGS = Split('-w'))
    else:
        env.Append(CCFLAGS = Split('-Wall -Wsign-compare -Wno-deprecated-register -Wno-undefined-var-template -Wno-unused-local-typedef'))
        if env['WARN_LEVEL'] == 'strict':
            env.Append(CCFLAGS = Split('-Werror'))

    # Optimization flags
    if env['MODE'] == 'opt' or env['MODE'] == 'profile':
        env.Append(CCFLAGS = Split('-O3'))

    # Debug and profile flags
    if env['MODE'] == 'debug' or env['MODE'] == 'profile':
        env.ParseFlags('-DDEBUG')
        env.Append(CCFLAGS = Split('-g'))
        env.Append(LINKFLAGS = Split('-g'))
        env.Append(CCFLAGS = Split('-O0'))

    # Linux profiling
    if system.os == 'linux' and env['MODE'] == 'profile':
        env.Append(CCFLAGS = Split('-pg'))
        env.Append(LINKFLAGS = Split('-pg'))

# msvc settings
elif env['COMPILER'] == 'msvc':
    env.Append(CCFLAGS=Split('/EHsc'))
    env.Append(LINKFLAGS=Split('/Machine:X64'))
    env.Append(CCFLAGS=Split('/D "NOMINMAX"'))
    # Optimization/profile/debug flags
    if env['MODE'] == 'opt':
        env.Append(CCFLAGS=Split('/O2 /Oi /Ob2 /MD'))
        env.Append(CPPDEFINES=Split('NDEBUG'))
    elif env['MODE'] == 'profile':
        env.Append(CCFLAGS=Split('/Ob2 /MD /Zi'))
    else:  # debug mode
        env.Append(CCFLAGS=Split('/Od /Zi /MD'))
        env.Append(LINKFLAGS=Split('/DEBUG'))

# Add include and lib paths to Arnold
env.Append(CPPPATH = [ARNOLD_API_INCLUDES, USD_INCLUDE])
env.Append(LIBPATH = [ARNOLD_API_LIB, ARNOLD_BINARIES, USD_LIB])

# Add optional include and library paths. These are the standard additional
# libraries required when using USD.
env.Append(CPPPATH = [p for p in [BOOST_INCLUDE, PYTHON_INCLUDE, TBB_INCLUDE, GOOGLETEST_INCLUDE] if p is not None])
env.Append(LIBPATH = [p for p in [BOOST_LIB, PYTHON_LIB, TBB_LIB, GOOGLETEST_LIB] if p is not None])

env['ROOT_DIR'] = os.getcwd()

# including common headers
env.Append(CPPPATH = [os.path.join(env['ROOT_DIR'], 'common')])
env['COMMON_SRC'] = [os.path.join(env['ROOT_DIR'], 'common', src) for src in find_files_recursive(os.path.join(env['ROOT_DIR'], 'common'), ['.cpp'])]

# Configure base directory for temp files
BUILD_BASE_DIR = os.path.join(BUILD_DIR, '%s_%s' % (system.os, 'x86_64'), '%s_%s' % (env['COMPILER'], env['MODE']), 'usd-%s_arnold-%s' % (env['USD_VERSION'], env['ARNOLD_VERSION']))

env['BUILD_BASE_DIR'] = BUILD_BASE_DIR
# Build target
if os.path.isabs(BUILD_BASE_DIR):
    env['BUILD_ROOT_DIR'] = BUILD_BASE_DIR
else:
    env['BUILD_ROOT_DIR'] = os.path.join(env['ROOT_DIR'], BUILD_BASE_DIR)

if os.path.isabs(REFERENCE_DIR):
    env['REFERENCE_DIR_ROOT'] = REFERENCE_DIR
else:
    env['REFERENCE_DIR_ROOT'] = os.path.join(env['ROOT_DIR'], REFERENCE_DIR)

# Propagate any "library path" environment variable to scons
# if system.os == 'linux':
#     add_to_library_path(env, '.')
#     if os.environ.has_key('LD_LIBRARY_PATH'):
#         add_to_library_path(env, os.environ['LD_LIBRARY_PATH'])
#     os.environ['LD_LIBRARY_PATH'] = env['ENV']['LD_LIBRARY_PATH']
# elif system.os == 'darwin':
#     if os.environ.has_key('DYLD_LIBRARY_PATH'):
#         add_to_library_path(env, os.environ['DYLD_LIBRARY_PATH'])
# elif system.os == 'windows':
#     add_to_library_path(env, os.environ['PATH'])
#     os.environ['PATH'] = env['ENV']['PATH']

# SCons scripts to build
procedural_script = os.path.join('procedural', 'SConscript')
procedural_build = os.path.join(BUILD_BASE_DIR, 'procedural')

cmd_script = os.path.join('cmd', 'SConscript')
cmd_build = os.path.join(BUILD_BASE_DIR, 'cmd')

schemas_script = os.path.join('schemas', 'SConscript')
schemas_build = os.path.join(BUILD_BASE_DIR, 'schemas')

translator_script = os.path.join('translator', 'SConscript')
translator_build = os.path.join(BUILD_BASE_DIR, 'translator')

renderdelegate_script = os.path.join('render_delegate', 'SConscript')
renderdelegate_build = os.path.join(BUILD_BASE_DIR, 'render_delegate')
renderdelegate_plug_info = os.path.join('render_delegate', 'plugInfo.json')

ndrplugin_script = os.path.join('ndr', 'SConscript')
ndrplugin_build = os.path.join(BUILD_BASE_DIR, 'ndr')
ndrplugin_plug_info = os.path.join('ndr', 'plugInfo.json')

usdimagingplugin_script = os.path.join('usd_imaging', 'SConscript')
usdimagingplugin_build = os.path.join(BUILD_BASE_DIR, 'usd_imaging')
usdimagingplugin_plug_info = os.path.join('usd_imaging', 'plugInfo.json')

scenedelegate_script = os.path.join('scene_delegate', 'SConscript')
scenedelegate_build = os.path.join(BUILD_BASE_DIR, 'scene_delegate')
scenedelegate_plug_info = os.path.join('scene_delegate', 'plugInfo.json')

testsuite_build = os.path.join(BUILD_BASE_DIR, 'testsuite')

usd_input_resource_folder = os.path.join(USD_LIB, 'usd')

# Define targets
# Target for the USD procedural

if BUILD_PROCEDURAL or BUILD_USD_WRITER:
    TRANSLATOR = env.SConscript(translator_script,
        variant_dir = translator_build,
        duplicate = 0, exports = 'env')

    SConscriptChdir(0)
else:
    TRANSLATOR = None

if BUILD_PROCEDURAL or BUILD_RENDER_DELEGATE or BUILD_NDR_PLUGIN or BUILD_USD_IMAGING_PLUGIN:
    ARNOLDUSD_HEADER = env.Command(os.path.join(BUILD_BASE_DIR, 'arnold_usd.h'), 'arnold_usd.h.in', configure.configure_header_file) 
else:
    ARNOLDUSD_HEADER = None

# Define targets
# Target for the USD procedural
if BUILD_PROCEDURAL:
    PROCEDURAL = env.SConscript(procedural_script,
        variant_dir = procedural_build,
        duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(PROCEDURAL, TRANSLATOR[0])
    Depends(PROCEDURAL, ARNOLDUSD_HEADER)

    if env['USD_BUILD_MODE'] == 'static':
        # For static builds of the procedural, we need to copy the usd 
        # resources to the same path as the procedural
        usd_target_resource_folder = os.path.join(os.path.dirname(os.path.abspath(str(PROCEDURAL[0]))), 'usd')
        if os.path.exists(usd_input_resource_folder) and not os.path.exists(usd_target_resource_folder):
            shutil.copytree(usd_input_resource_folder, usd_target_resource_folder)

else:
    PROCEDURAL = None

if BUILD_SCHEMAS:
    SCHEMAS = env.SConscript(schemas_script,
        variant_dir = schemas_build,
        duplicate = 0, exports = 'env')
    SConscriptChdir(0)
else:
    SCHEMAS = None

if BUILD_USD_WRITER:
    ARNOLD_TO_USD = env.SConscript(cmd_script, variant_dir = cmd_build, duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(ARNOLD_TO_USD, TRANSLATOR[0])
    if env['USD_BUILD_MODE'] == 'static':
        # For static builds of the writer, we need to copy the usd 
        # resources to the same path as the procedural
        usd_target_resource_folder = os.path.join(os.path.dirname(os.path.abspath(str(ARNOLD_TO_USD[0]))), 'usd')
        if os.path.exists(usd_input_resource_folder) and not os.path.exists(usd_target_resource_folder):
            shutil.copytree(usd_input_resource_folder, usd_target_resource_folder)
else:
    ARNOLD_TO_USD = None

if BUILD_RENDER_DELEGATE:
    RENDERDELEGATE = env.SConscript(renderdelegate_script, variant_dir = renderdelegate_build, duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(RENDERDELEGATE, ARNOLDUSD_HEADER)
else:
    RENDERDELEGATE = None

if BUILD_NDR_PLUGIN:
    NDRPLUGIN = env.SConscript(ndrplugin_script, variant_dir = ndrplugin_build, duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(NDRPLUGIN, ARNOLDUSD_HEADER)
else:
    NDRPLUGIN = None

if BUILD_USD_IMAGING_PLUGIN:
    USDIMAGINGPLUGIN = env.SConscript(usdimagingplugin_script, variant_dir = usdimagingplugin_build, duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(USDIMAGINGPLUGIN, ARNOLDUSD_HEADER)
else:
    USDIMAGINGPLUGIN = None

if BUILD_SCENE_DELEGATE:
    SCENEDELEGATE = env.SConscript(scenedelegate_script, variant_dir = scenedelegate_build, duplicate = 0, exports = 'env')
    SConscriptChdir(0)
    Depends(SCENEDELEGATE, ARNOLDUSD_HEADER)
else:
    SCENEDELEGATE = None

#Depends(PROCEDURAL, SCHEMAS)

if BUILD_DOCS:
    docs_output = os.path.join(BUILD_BASE_DIR, 'docs')
    env['DOXYGEN_TAGS'] = {
        'OUTPUT_DIRECTORY': docs_output
    }
    DOCS = env.Doxygen(source='docs/Doxyfile', target=docs_output)
else:
    DOCS = None

# Generating plugInfo.json files so we have the right platform specific
# extension.

plugInfos = [
    renderdelegate_plug_info,
    ndrplugin_plug_info,
    scenedelegate_plug_info,
]

for plugInfo in plugInfos:
    env.Command(target=plugInfo, source=['%s.in' % plugInfo],
                action=configure.configure_plug_info)

if BUILD_USD_IMAGING_PLUGIN:
    env.Command(target=usdimagingplugin_plug_info,
                source=['{}.in'.format(usdimagingplugin_plug_info)], action=configure.configure_usd_maging_plug_info)

if RENDERDELEGATE:
    Depends(RENDERDELEGATE, renderdelegate_plug_info)

if SCENEDELEGATE:
    Depends(SCENEDELEGATE, scenedelegate_plug_info)

if BUILD_TESTSUITE:
    env['USD_PROCEDURAL_PATH'] = os.path.abspath(str(PROCEDURAL[0]))
    # Target for the test suite
    TESTSUITE = env.SConscript(os.path.join('testsuite', 'SConscript'),
        variant_dir = testsuite_build,
        exports     = ['env'],
        duplicate   = 0)
    SConscriptChdir(1)
    '''
    This is currently causing issues when running the testsuite (see #746).
    We're disabling it for now, so devs will need to first build the repo 
    and then run the testsuite. This will also allow to run the tests on 
    a prebuilt library
    Depends(TESTSUITE, PROCEDURAL)
    if env['ENABLE_UNIT_TESTS']:
        if RENDERDELEGATE:
            Depends(TESTSUITE, RENDERDELEGATE)
        if NDRPLUGIN:
            Depends(TESTSUITE, NDRPLUGIN)
    '''
else:
    TESTSUITE = None

for target in [RENDERDELEGATE, PROCEDURAL, SCHEMAS, ARNOLD_TO_USD, RENDERDELEGATE, DOCS, TESTSUITE, NDRPLUGIN, USDIMAGINGPLUGIN]:
    if target:
        env.AlwaysBuild(target)

if TESTSUITE:
    env.Alias('testsuite', TESTSUITE)
env.Alias('install', PREFIX)

# Install compiled dynamic library
if PROCEDURAL:
    INSTALL_PROC = env.Install(PREFIX_PROCEDURAL, PROCEDURAL)
    INSTALL_PROC += env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd'), ARNOLDUSD_HEADER)
    if env['USD_BUILD_MODE'] == 'static':
        INSTALL_PROC += env.Install(PREFIX_PROCEDURAL, usd_input_resource_folder)
    env.Alias('procedural-install', INSTALL_PROC)

if ARNOLD_TO_USD:
    INSTALL_ARNOLD_TO_USD = env.Install(PREFIX_BIN, ARNOLD_TO_USD)
    if env['USD_BUILD_MODE'] == 'static':
        INSTALL_ARNOLD_TO_USD += env.Install(PREFIX_BIN, usd_input_resource_folder)
    env.Alias('writer-install', INSTALL_ARNOLD_TO_USD)

if RENDERDELEGATE:
    if IS_WINDOWS:
        INSTALL_RENDERDELEGATE = env.Install(PREFIX_RENDER_DELEGATE, RENDERDELEGATE)
    else:
        INSTALL_RENDERDELEGATE = env.InstallAs(os.path.join(PREFIX_RENDER_DELEGATE, 'hdArnold%s' % system.LIB_EXTENSION), RENDERDELEGATE)
    INSTALL_RENDERDELEGATE += env.Install(os.path.join(PREFIX_RENDER_DELEGATE, 'hdArnold', 'resources'), [os.path.join('render_delegate', 'plugInfo.json')])
    INSTALL_RENDERDELEGATE += env.Install(PREFIX_RENDER_DELEGATE, ['plugInfo.json'])
    INSTALL_RENDERDELEGATE += env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd', 'render_delegate'), env.Glob(os.path.join('render_delegate', '*.h')))
    env.Alias('delegate-install', INSTALL_RENDERDELEGATE)

if NDRPLUGIN:
    if IS_WINDOWS:
        INSTALL_NDRPLUGIN = env.Install(PREFIX_NDR_PLUGIN, NDRPLUGIN)
    else:
        INSTALL_NDRPLUGIN = env.InstallAs(os.path.join(PREFIX_NDR_PLUGIN, 'ndrArnold%s' % system.LIB_EXTENSION), NDRPLUGIN)
    INSTALL_NDRPLUGIN += env.Install(os.path.join(PREFIX_NDR_PLUGIN, 'ndrArnold', 'resources'), [os.path.join('ndr', 'plugInfo.json')])
    INSTALL_NDRPLUGIN += env.Install(PREFIX_NDR_PLUGIN, ['plugInfo.json'])
    INSTALL_NDRPLUGIN += env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd', 'ndr'), env.Glob(os.path.join('ndr', '*.h')))
    env.Alias('ndrplugin-install', INSTALL_NDRPLUGIN)

if USDIMAGINGPLUGIN:
    if IS_WINDOWS:
        INSTALL_USDIMAGINGPLUGIN = env.Install(PREFIX_USD_IMAGING_PLUGIN, USDIMAGINGPLUGIN)
    else:
        INSTALL_USDIMAGINGPLUGIN = env.InstallAs(os.path.join(PREFIX_USD_IMAGING_PLUGIN, 'usdImagingArnold%s' % system.LIB_EXTENSION), USDIMAGINGPLUGIN)
    INSTALL_USDIMAGINGPLUGIN += env.Install(os.path.join(PREFIX_USD_IMAGING_PLUGIN, 'usdImagingArnold', 'resources'), [os.path.join('usd_imaging', 'plugInfo.json')])
    INSTALL_USDIMAGINGPLUGIN += env.Install(PREFIX_USD_IMAGING_PLUGIN, ['plugInfo.json'])
    INSTALL_USDIMAGINGPLUGIN += env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd', 'usd_imaging'), env.Glob(os.path.join('usd_imaging', '*.h')))
    env.Alias('usdimagingplugin-install', INSTALL_USDIMAGINGPLUGIN)

if SCENEDELEGATE:
    if IS_WINDOWS:
        INSTALL_SCENEDELEGATE = env.Install(PREFIX_SCENE_DELEGATE, SCENEDELEGATE)
    else:
        INSTALL_SCENEDELEGATE = env.InstallAs(os.path.join(PREFIX_SCENE_DELEGATE, 'imagingArnold%s' % system.LIB_EXTENSION), SCENEDELEGATE)
    INSTALL_SCENEDELEGATE += env.Install(os.path.join(PREFIX_SCENE_DELEGATE, 'imagingArnold', 'resources'), [os.path.join('scene_delegate', 'plugInfo.json')])
    INSTALL_SCENEDELEGATE += env.Install(PREFIX_SCENE_DELEGATE, ['plugInfo.json'])
    INSTALL_SCENEDELEGATE += env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd', 'scene_delegate'), env.Glob(os.path.join('scene_delegate', '*.h')))
    env.Alias('delegate-install', INSTALL_SCENEDELEGATE)

if ARNOLDUSD_HEADER:
    INSTALL_ARNOLDUSDHEADER = env.Install(os.path.join(PREFIX_HEADERS, 'arnold_usd'), ARNOLDUSD_HEADER)
    env.Alias('arnoldusdheader-install', INSTALL_ARNOLDUSDHEADER)

# This follows the standard layout of USD plugins / libraries.
if SCHEMAS:
    INSTALL_SCHEMAS = env.Install(os.path.join(PREFIX_SCHEMAS), ['plugInfo.json'])
    INSTALL_SCHEMAS += env.Install(os.path.join(PREFIX_SCHEMAS, 'usdArnold', 'resources', 'usdArnold'), [SCHEMAS[0]])
    INSTALL_SCHEMAS += env.Install(os.path.join(PREFIX_SCHEMAS, 'usdArnold', 'resources'), [SCHEMAS[1], SCHEMAS[2]])
    env.Alias('schemas-install', INSTALL_SCHEMAS)

if DOCS:
    INSTALL_DOCS = env.Install(PREFIX_DOCS, DOCS)
    env.Alias('docs-install', INSTALL_DOCS)

# We don't need to install the license if the prefix is left to its default #553
if PREFIX != '.':
    INSTALL_LICENSE = env.Install(PREFIX, 'LICENSE.md')
    env.Alias('license-install', INSTALL_LICENSE)

Default(PREFIX)
