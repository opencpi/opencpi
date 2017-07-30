/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OCPIFPGATIME_INCLUDE__
#define OCPIFPGATIME_INCLUDE__

#include <OcpiOsDataTypes.h>

#define RPLTIME_R32( off ) *((OCPI::OS::uint32_t*)(m_adminBaseAdr+off))
#define RPLTIME_R64( off ) *((OCPI::OS::uint64_t*)(m_adminBaseAdr+off))
#define RPLTIME_W32( off,val ) *((OCPI::OS::uint32_t*)(m_adminBaseAdr+off))=val
#define RPLTIME_W64( off,val ) *((OCPI::OS::uint64_t*)(m_adminBaseAdr+off))=val


namespace  OCPI {
  namespace RPL {

    const OCPI::OS::uint32_t TimeStatusRegOffset  = 0x30;
    const OCPI::OS::uint32_t TimeControlRegOffset = 0x34;
    const OCPI::OS::uint32_t TimeRegMSOffset      = 0x38;     
    const OCPI::OS::uint32_t TimeRegLSOffset      = 0x3c;     
    const OCPI::OS::uint32_t TimeCompRegMSOffset  = 0x40;
    const OCPI::OS::uint32_t TimeCompRegLSOffset  = 0x44;     
    const OCPI::OS::uint32_t TimeRefPPSOffset     = 0x48;

    class Time {
    public:

      Time( void * AdminBaseAdr  )
        {
          m_adminBaseAdr=(OCPI::OS::uint8_t*)AdminBaseAdr;
        }

      /*
       * Register access methods
       */
      inline OCPI::OS::uint32_t getStatusReg()  {return RPLTIME_R32(TimeStatusRegOffset);}
      inline OCPI::OS::uint32_t getControlReg() {return RPLTIME_R32(TimeControlRegOffset);}
      inline void setControlReg( OCPI::OS::uint32_t val ){RPLTIME_W32(TimeControlRegOffset,val);}
      inline OCPI::OS::uint64_t getTime(){return RPLTIME_R64(TimeRegMSOffset);}
      inline void setTime( OCPI::OS::uint64_t val){RPLTIME_W64(TimeRegMSOffset,val);}
      inline OCPI::OS::uint64_t getTimeCompare(){return RPLTIME_R64(TimeCompRegMSOffset);}
      inline OCPI::OS::uint32_t getTimeRefPPS() {return RPLTIME_R32(TimeRefPPSOffset);}

      /*
       * Convienience methods
       */
      inline bool ppsLostS(){return getStatusReg()&(1<<31) ? true : false;}
      inline bool gpsInS(){return getStatusReg()&(1<<30) ? true : false;}
      inline bool ppsInS(){return getStatusReg()&(1<<29) ? true : false;}
      inline bool timeSetS(){return getStatusReg()&(1<<28) ? true : false;}
      inline bool ppsOk(){return getStatusReg()&(1<<27) ? true : false;}
      inline bool ppsLost(){return getStatusReg()&(1<<26) ? true : false;}
      OCPI::OS::uint8_t rollingPPSIn(){return getStatusReg()&0xff;}

      inline void clearStickyBits(){setControlReg(1u<<31);}
      void disableServo(bool d){setControlReg( d?1:0 );}
      void disableGPS(bool d){setControlReg( d?1:0 );}
      void disablePPSIn(bool d){setControlReg( d?1:0 );}

      enum PPSOutControl {
        TimeServerSeconds,
        PPSInputLoopThru,
        LocalXORefDiv2,
        PPSMute
      };
      void driverPPSOut( PPSOutControl );


    private:
      OCPI::OS::uint8_t * m_adminBaseAdr;

    };

  }
}




#endif
