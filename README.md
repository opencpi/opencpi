This is the source distribution of OpenCPI, which is hosted on github.com.

**An [Overview of OpenCPI](http://opencpi.github.io/Overview.pdf) is available.**

Documentation
---
All documentation is in the `/doc/` subdirectory available [here](doc/). **This includes a recommended reading order list** with links to various PDFs.

Installation
---
#### RPM
This is for CentOS 6 or 7 systems:
 - `sudo curl -o /etc/yum.repos.d/opencpi-v1.3.1.repo http://opencpi.github.io/repo/opencpi-v1.3.1.repo`
 - `sudo yum install 'opencpi-*' 'ocpi-prereq-*'`

For additional information, consult the [RPM Installation Guide][rpminstall].

#### Build from Source
 - `scripts/install-opencpi.sh` (root permission *not* required)

License
---
OpenCPI is Open Source Software, licensed with the LGPL3.  See `LICENSE.txt`.

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

  [rpminstall]: <http://opencpi.github.io/RPM_Installation_Guide.pdf>
