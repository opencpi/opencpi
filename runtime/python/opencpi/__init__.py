# This file will run when "import opencpi" is done.
# We always load the (single) aci swig module now.
# In the future if we have more subpackages we can use one of the variety of techniques
# for lazy import of subpackages.

# This file is only functional when exported, and with swig libraries collocated.
import sys
old=sys.getdlopenflags()
if sys.platform != 'darwin':
   import ctypes
   sys.setdlopenflags(old|ctypes.RTLD_GLOBAL)
import aci
sys.setdlopenflags(old)

