This is the source distribution of OpenCPI, which is hosted on github.com.

**An [Overview of OpenCPI](http://opencpi.github.io/Overview.pdf) is available.**

Documentation
---
All documentation is in the `doc/` subdirectory available [here](doc/). **This includes a recommended reading order list** with links to various PDFs.

Installation
---
#### YUM / RPM
This is for CentOS 6 or 7 systems:
 - `sudo yum install yum-utils`
 - `sudo yum-config-manager --add-repo=http://opencpi.github.io/repo/opencpi-v1.4.0.repo`
 - `sudo yum install 'opencpi-*' 'ocpi-prereq-*'`

For additional information, consult the [YUM/RPM Installation Guide][rpminstall].

#### Build from Source
- These steps perform a source installation and build, in a user-controlled location.
- `sudo yum install git` (ensure `git` is installed on the system)
- Enter the following command in a directory where `opencpi` will be cloned/downloaded.
- `git clone https://github.com/opencpi/opencpi.git`
- `cd opencpi` (enter the directory where the OpenCPI git repository was cloned)
- This next command will:
--- first install some standard prerequisites using `sudo yum install`
--- download/build others directly in the `prerequisites` subdirectory under `opencpi/`.
--- build the framework and built-in projects from source
--- if a development system, run tests on the resulting built system
- `scripts/install-opencpi.sh` (root permission *not* required)

For additional information, consult the [traditional Installation Guide][ossinstall].

License
---
OpenCPI is Open Source Software, licensed with the LGPL3.  See `LICENSE.txt`.

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

  [rpminstall]: <http://opencpi.github.io/RPM_Installation_Guide.pdf>
  [ossinstall]: <http://opencpi.github.io/OpenCPI_Installation.pdf>
