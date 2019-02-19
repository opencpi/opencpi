__Open Component Portability Infrastructure (OpenCPI)__ is an open source software (OSS) framework for developing and executing component-based applications on heterogeneous embedded systems.

This is the source distribution of OpenCPI, which is hosted on
github.com, and is located [here](https://github.com/opencpi/opencpi).

As a framework for both development and execution, OpenCPI supports defining, implementing, building and testing components, as well as executing applications based on those components in the targeted embedded systems.  By targeting heterogeneous systems, the framework supports development and execution across diverse processing technologies including GPPs (general purpose processors), FPGA (field programmable gate arrays), GPUs (graphics processing units) assembled into mixed systems. Component implementations (a.k.a. _workers_) are written in the language commonly used to target the type of processor being used.  Thus workers for GPPs are written in C or C++, workers for FPGAs are written in VHDL or Verlog, and workers for GPUs are written in the OpenCL dialect of C.

A common use of OpenCPI is for software-defined radio applications on platforms that contain both CPU and FPGA computing resources.

**An overview of OpenCPI based on the latest release is available on the website [here](https://www.opencpi.org).**

**The discussion and announcement email list for OpenCPI is `discuss` at lists.opencpi.org.  Subscribe at [lists.opencpi.org](http:lists.opencpi.org).**

# Documentation

The available documentation is [here][doc] with a recommended reading order [here][recommend].  Much of it is oriented toward those using the CentOS6/7 YUM/RPM installation, although all the __development guides__ cover both types of installation, described next.

# Installation

There are two ways to install and use OpenCPI.  The first is recommended for typical users that are
using CentOS6 or CentOS7 Linux and not developing/modifying/patching OpenCPI itself.  Both installation methods require that the user have `sudo` privileges, which may be a sys admin task in some environments

## 1. YUM/RPM Installation:
For CentOS6 or CentOS7 Linux, there is a binary/pre-built RPM installation available using the _`yum`_ command.
  To install OpenCPI this way, use the following commands:
   - `sudo yum install yum-utils epel-release`
   - `sudo yum-config-manager --add-repo=http://opencpi.github.io/repo/opencpi.repo`
   - `sudo yum install 'opencpi*'`

  This installs the latest release.  This will install OpenCPI globally on your system, in standard locations (_e.g._ /usr/share/doc, /usr/lib/debug, etc.).
   For additional information to complete the installation consult the [YUM/RPM Installation Guide][rpminstall].

## 2. Source-based Installation:
For any supported development OS, you can download, build, install, and use OpenCPI starting from source.  To obtain and use OpenCPI this way, you must select a
_tagged release_.  Releases and tags are listed at the OpenCPI github repository [here][releases].  The default git branch (called `master`, accessed without tags) is ___not___ necessarily usable or stable and should not be used except in rare circumstances. (This will probably change soon).  This source installation method downloads, builds and uses the software in a directory of the user's choosing (_e.g._ `~/opencpi`).  Thus multiple versions can be downloaded and coexist, but not execute simultaneously.

Source installations make no global changes to the user's system other than:

   - Installing or updating some required standard packages using the `yum install` or equivalent command.
   - Dynamically/temporarily loading and using the OpenCPI kernel driver in order to test it.

Both these steps require `sudo` privileges.  The installation process is described in detail in the install-from-source [installation guide][ossinstall],
but the steps described here are sufficient if you understand them.
You can either download a tar file for the release, which results in a ~300MB directory before building, or you can download (clone) the git repository with the complete source history, which results in a ~2GB directory before building.

### Downloading sources

#### Obtaining sources via downloading a tar file.
To download the tar file associated with a release, select the release on the OpenCPI repository releases page [here](https://github.com/opencpi/opencpi/releases) and select the tar file to download.
When the tar file is extracted it will create a directory called `opencpi-<release-tag>`, which you should change into.  Some browsers will automatically extract the file, but the first command assumes it does not.
   - `tar xzf opencpi-<release-tag>.gz`
   - `cd opencpi-<release-tag>`

#### Obtaining sources via cloning the OpenCPI git repository.
To download via cloning the entire OpenCPI, first ensure that you have `git` installed on your system.  If `git` is not present, on CentOS systems this is accomplished (installing or updating `git`) by using the command:
   - `sudo yum install git`

After git is installed, you issue these commands to download the repository and enter the directory.
   - `git clone https://github.com/opencpi/opencpi.git`
   - `cd opencpi`
   - `git checkout <release-tag>`

### Build and test OpenCPI for use on the system have downloaded to, by running:

   - `./scripts/install-opencpi.sh`

   This command may take a while, and will require you to provide the `sudo` password twice
   during that process, for the two _global_ actions described earlier.
   If you are not present to provide the password it may fail, but can be rerun.
   It will require internet access to download and build software it needs.
   There are ways around this requirement that are described in the install-from-source [installation guide][ossinstall].
   The testing done by this script only executes software-based components and applications.

### Set up your environment as a user of OpenCPI
   This is done using the `opencpi-setup.sh` script in the
   `cdk` subdirectory.  OpenCPI currently only supports the __bash__ shell.
   There are two ways to perform this step:

   - If you want to manually set up your environment in each shell window as you need it,
     you simply _source_ the script where it lives, under the `cdk` subdirectory.  _E.g._ if OpenCPI was downloaded into the
     `~/opencpi` directory, you would issue the command:

     `source ~/opencpi/cdk/opencpi-setup.sh -s`

   - If you want to set up the environment once on each login, you would add this same line to
     your `~/.profile` file (or `~/.bash_login` or `~/.bash_profile`).  Note that this will only
     take effect when you login, or when you start a new __login shell__ using the `-l` option
     to bash, _e.g._:

     `bash -l`

# License
OpenCPI is Open Source Software, licensed with the LGPL3. See the [license file](LICENSE.txt).

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

  [rpminstall]: <https://opencpi.github.io/RPM_Installation_Guide.pdf>
  [ossinstall]: <https://opencpi.github.io/OpenCPI_Installation.pdf>
  [releases]:   <https://github.com/opencpi/opencpi/releases>
  [recommend]:  <http://opencpi.github.io/Recommended.html>
  [doc]:  <https://opencpi.github.io>
