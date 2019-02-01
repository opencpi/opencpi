# OpenCPI Release to Support Zynq UltraScale+
**This is a custom release of OpenCPI by Geon Technologies, LLC with support for ZynqUltraScale+.**

**Follow the instructions directly below, as the standard OpenCPI installation instructions will not work as-is.**

**For installation and setup, follow the instructions in [ZCU102 Getting Started Guide](https://github.com/Geontech/bsp_zcu1xx/blob/release_1.4_zynq_ultra/pdfs/ZCU102_Getting_Started_Guide.pdf**

**or [ZCU111 Getting Started Guide](https://github.com/Geontech/bsp_zcu1xx/blob/release_1.4_zynq_ultra/pdfs/ZCU111_Getting_Started_Guide.pdf)**

**and follow the corresponding [Reading List](https://github.com/Geontech/bsp_zcu1xx).**


# Standard OpenCPI Information
**[Overview of OpenCPI](http://opencpi.github.io/Overview.pdf) **
This is the source distribution of OpenCPI, which is hosted on github.com.

**An [Overview of OpenCPI](http://opencpi.github.io/Overview.pdf) is available.**

Documentation
---
All documentation is in the `doc/` subdirectory available [here](doc/). **This includes a recommended reading order list** with links to various PDFs.

Installation
---
#### YUM / RPM

**This is the recommended usage for most users** and the most likely to be supported.

On CentOS 6 or 7 systems:
 - `sudo yum install yum-utils epel-release`
 - `sudo yum-config-manager --add-repo=http://opencpi.github.io/repo/opencpi-v1.4.0.repo`
 - `sudo yum install 'opencpi*'`

For additional information, consult the [YUM/RPM Installation Guide][rpminstall].

#### Build from Source
- These steps perform a source installation and build, in a user-controlled location.
- `sudo yum install git` (ensure `git` is installed on the system)
- Enter the following command in a directory where `opencpi` will be cloned/downloaded.
- `git clone https://github.com/opencpi/opencpi.git`
- `cd opencpi` (enter the directory where the OpenCPI git repository was cloned)
- `scripts/install-opencpi.sh` (root permission *not* required) will:
  - first install some standard prerequisites using `sudo yum install`
  - download/build others directly in the `prerequisites` subdirectory under `opencpi/`.
  - build the framework and built-in projects from source
  - if a development system, run tests on the resulting built system


For additional information, consult the [traditional Installation Guide][ossinstall].

License
---
OpenCPI is Open Source Software, licensed with the LGPL3.  See `LICENSE.txt`.

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

  [rpminstall]: <http://opencpi.github.io/RPM_Installation_Guide.pdf>
  [ossinstall]: <http://opencpi.github.io/OpenCPI_Installation.pdf>
