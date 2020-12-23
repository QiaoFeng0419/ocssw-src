# Introduction

This contains science processing software used within OBPG. It is provided here with NO SUPPORT. For fully supported code, please use the binaries distributed with SeaDAS <https://seadas.gsfc.nasa.gov/>

OCSSW_bash.env (OCSSW.env for csh) is provided for setting needed environment variables to build the software.

## REQUIREMENTS:

- gcc (4.5 or higher)
- g++ (matching gcc version)
- gfortran (matching gcc version)
- zlib
- csh
- tcsh
- unzip
- git
- cmake
- bison
- flex
- libX11-devel
- wget

_additional known OS-specific requirements_

- Ubuntu 16.04

  - zlib1g-dev

- CentOS 7

  - glibc-static
  - libgfortran-static
  - zlib-static
  - gmp-static
  - libjpeg-turbo-static
  - libstdc++-static

- Mac:

  - libX11 can be installed by XQuartz

## Building the code

CMake is used for the build system with a CMakeLists.txt file in each directory.

Here are the steps needed to build using bash, cmake and make:

1. Get a copy of the ocssw source git repository

  ```
  $ git clone https://oceandata.sci.gsfc.nasa.gov/ocssw/ocssw-src.git [optional target directory name]
  ```

  _Note: OBPG staff developers with access to the deveopment repository can do the following instead:_

  ```
  git clone https://oceandata.sci.gsfc.nasa.gov/rcs/obpg/ocssw.git [optional target directory name]

  or when on OBPG internal network, with ssh key in gitlab:
  git clone git@oceanrcs101:obpg/ocssw.git

  cd ocssw
  git submodule init
  git submodule update
  ```

2. Setup environment variables (usually in ~/.bashrc)

  ```
  $ export CC=gcc
  $ export CXX=g++
  $ export FC=gfortran
  $ export OCSSWROOT=$HOME/ocssw
  $ export OCSSW_DEBUG=1  #set to 0 if not debugging
  $ source $OCSSWROOT/OCSSW_bash.env
  ```

3. Grab the third party library sources and put them in opt

  ```
  $ cd $OCSSWROOT/opt
  $ wget https://oceandata.sci.gsfc.nasa.gov/ocssw/opt-src.tar
  $ tar -xvf opt-src.tar
  ```

  _Note: OBPG staff developers can use the_ `get_lib3_src.sh` _script to obtain these_

4. Build and install the third party libs  

  _Note: You may want to remove $OCSSWROOT/opt/bin, $OCSSWROOT/opt/lib, and $OCSSWROOT/opt/include_


  ```
  $ cd $OCSSWROOT/opt/src
  $ export OCSSW_DEBUG=0 #probably do not want to debug these
  $ # you may want to remove $OCSSWROOT/opt/bin, $OCSSWROOT/opt/lib, and $OCSSWROOT/opt/include prior to:
  $ ./BuildIt
  ```

5. Create build directory and run cmake

  CMake allows for out-of-source-tree builds. Make a directory for this and run cmake within it, e.g.:

  ```
  $ cd <ocssw source directory> (e.g. $OCSSWROOT/ocssw-src)
  $ mkdir build
  $ cd build
  $ cmake ..
  ```

  _Note: There are a few options you can give to cmake to control what extra software is built:_

  ```
  BUILD_VIIRS_L1     - Build VIIRS L1 code
                 (not yet open source)
  BUILD_HISTORICAL   - Build the historical code
                 (e.g. old seawifs binaries no longer needed)
  BUILD_MISC         - Build non-essential code
  BUILD_AHMAD_FRASER - Build the AF radiative transfer code
                 (not distributed)
  BUILD_ALL          - Build everything we got

  The "cmake .." command is run like this to use one or more of these options:

  cmake .. -DBUILD_HISTORICAL=ON -DBUILD_MISC=ON

  Please be aware that not all the code required for these options
  is available in the externally accessible source repository
  ```

6. Build the OCSSW binaries

  ```
  $ cd <ocssw build directory> (e.g. $OCSSWROOT/ocssw-src/build)
  $ export OCSSW_DEBUG=1 #could be set to 0 if not debugging
  $ make -j 20
  ```

7. Install binaries into $OCSSWROOT/bin

  ```
  $ make install
  ```

8. Grab all the data trees

  See <https://oceandata.sci.gsfc.nasa.gov/ocssw/> for manual installation or use install_ocssw.py or the SeaDAS GUI

  _Note: OBPG staff developers can use the following:_

  ```
  $ cd <cmake build directory>
  $ make update   #This command will take a LONG time
  ```

## IDE setup

### NetBeans

Here are the steps needed to use NetBeans for programming and debugging:

1. download, build and install OCSSW as described above. Make sure to set `OCSSW_DEBUG=0` before building 3rd party libs and setting `OCSSW_DEBUG=1` before building OCSSW code.

2. setup the correct compilers

  ```
  menu Tools -> Options

  select C/C++

  select Build Tools tab

  Base Directory = /usr/bin
  C Compiler = /usr/bin/gcc-5
  C++ Compiler = /usr/bin/g++-5
  Fortran Compiler = /usr/bin/gfortran-5
  Assembler = /usr/bin/as
  Make Command = /usr/bin/make
  Debugger Command = /usr/bin/gdb
  QMake Command = /usr/bin/qmake
  CMake Command = /usr/bin/cmake

  click OK
  ```

3. load the project into netbeans

  ```
  menu File -> New Project  

  select C/C++
  select C/C++ Project with Existing Sources

  click Next

  Folder that contains existing source = $OCSSWROOT
  (substitute the real dir for $OCSSWROOT)

  Build Host = localhost
  Tool Collection = Default GNU
  Configuration Mode = Custom

  click Next

  check Pre-Build Step is Required
  Run in Folder = $OCSSWROOT/build
  select Predefined Command
  Script type = CMake
  Script = $OCSSWROOT/CMakeLists.txt
  Arguments = (the default is fine)

  click Next

  (Build Actions)

  click Next

  (Source Files)
  Source File Folders = $OCSSWROOT
  (Exclude Pattern)
  ^(nbproject|.git|opt|share|test|var)$

  click Next

  (Code Assistance Configuration)

  click Next

  (Project Name and Location)

  click Finish
  ```

4. Setup a run configuration

  ```
  menu Run -> Set Project Configuration -> Customize...

  click Manage Configurations...
  click New

  New Name = l2gen (or whatever you want)

  select l2gen
  click Set Active

  click OK

  select Categories: Build -> Make
  Working Directory = build
  Build Command = ${MAKE} -j 20
  Clean Command = ${MAKE} clean
  Build Result = build/src/l2gen/l2gen

  select Categories: Run
  Run Command = "${OUTPUT_PATH}" par=S2002079035435.L2_MLAC_OC.subpix.par
  Run Directory = $OCSSWROOT/test/l2gen/seawifs
  Environment =
  Build First = checked
  Console Type = Internal Terminal
  External Terminal Type = Default

  click OK
  ```

  Now you can Run or Debug

5. Setup OBPG standard code formatting style

  ```
  select menu Tools->Options
  select "Editor" icon on top
  select "Formatting" tab
  Make sure "Language: All Languages"
  Check "Expand Tabs to Spaces"
  Number of spaces = 4
  for each language (C, C++, C/C++ Headers) change
  select Style = K&R
  under "Braces Placement" heading
  Function Declaration = Same Line
  ```

### Eclipse

Here are the steps needed to use eclipse for programming and debugging:

1. download, build and install OCSSW as described above. Make sure to set `OCSSW_DEBUG=0` before building 3rd party libs and setting `OCSSW_DEBUG=1` before building OCSSW code.

2. load the project into eclipse

  ```
  menu File -> New -> C++ Project Project Name:
  make up a name for the project

  uncheck Use Default
  Location Location = $OCSSWROOT (substitute your val for $OCSSWROOT)

  Project type:
  Makefile Project -> Empty Project

  Toolchains = "Linux GCC" for Linux, "MacOSX GCC" for mac

  Next Advanced Settings (button)

  C/C++ Build (list item)

  Builder Settings (tab)
  uncheck "Use default build command"
  Build Command = make -j 20
  uncheck "Generate Makefiles automatically"
  Build Location = ${workspace_loc:/ocssw/build}

  Click on the OK Button.
  Click on the Finish Button.
  Wait a long time for the C/C++ Indexer to finish...
  ```

3. make a run configuration to run l2gen

  ```
  menu Run -> Run Configurations...
  select C/C++ Application
  click the icon with a plus sign on it
  Name = l2gen-run

  Main tab C/C++ Application = build/src/l2gen/l2gen

  Arguments tab Program arguments = "par=S1997262224952.L2_LAC.par"
  uncheck Use default
  Working directory = $OCSSWROOT/test/l2gen (substitute your value)

  click Apply
  click Run
  ```

4. Setup OBPG standard code formatting style

  ```
  menu Window -> Preferences General -> Editors -> Text Editors Display tab
  width = 4
  check "Insert spaces for tabs"

  C/C++ -> Code Style -> Formatter click "New..." button
  Profile name = OCSSW Initialize settings with following profile: K&R [built-in]
  click OK

  Indentation tab Tab policy:
  Spaces only
  Indentation size = 4
  Tab size = 4

  Braces tab Function declaration: Same Line

  Line Wrapping tab check "Never join already wrapped lines"

  click OK
  click Apply
  click OK
  ```
