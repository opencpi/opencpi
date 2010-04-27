


#ifndef CPIFPGATIME_INCLUDE__
#define CPIFPGATIME_INCLUDE__

#include <CpiOsDataTypes.h>

#define RPLTIME_R32( off ) *((CPI::OS::uint32_t*)(m_adminBaseAdr+off))
#define RPLTIME_R64( off ) *((CPI::OS::uint64_t*)(m_adminBaseAdr+off))
#define RPLTIME_W32( off,val ) *((CPI::OS::uint32_t*)(m_adminBaseAdr+off))=val
#define RPLTIME_W64( off,val ) *((CPI::OS::uint64_t*)(m_adminBaseAdr+off))=val


namespace  CPI {
  namespace RPL {

    const CPI::OS::uint32_t TimeStatusRegOffset  = 0x30;
    const CPI::OS::uint32_t TimeControlRegOffset = 0x34;
    const CPI::OS::uint32_t TimeRegMSOffset      = 0x38;     
    const CPI::OS::uint32_t TimeRegLSOffset      = 0x3c;     
    const CPI::OS::uint32_t TimeCompRegMSOffset  = 0x40;
    const CPI::OS::uint32_t TimeCompRegLSOffset  = 0x44;     
    const CPI::OS::uint32_t TimeRefPPSOffset     = 0x48;

    class Time {
    public:

      Time( void * AdminBaseAdr  )
	{
	  m_adminBaseAdr=(CPI::OS::uint8_t*)AdminBaseAdr;
	}

      /*
       * Register access methods
       */
      inline CPI::OS::uint32_t getStatusReg()  {return RPLTIME_R32(TimeStatusRegOffset);}
      inline CPI::OS::uint32_t getControlReg() {return RPLTIME_R32(TimeControlRegOffset);}
      inline void setControlReg( CPI::OS::uint32_t val ){RPLTIME_W32(TimeControlRegOffset,val);}
      inline CPI::OS::uint64_t getTime(){return RPLTIME_R64(TimeRegMSOffset);}
      inline void setTime( CPI::OS::uint64_t val){RPLTIME_W64(TimeRegMSOffset,val);}
      inline CPI::OS::uint64_t getTimeCompare(){return RPLTIME_R64(TimeCompRegMSOffset);}
      inline CPI::OS::uint32_t getTimeRefPPS() {return RPLTIME_R32(TimeRefPPSOffset);}

      /*
       * Convienience methods
       */
      inline bool ppsLostS(){return getStatusReg()&(1<<31) ? true : false;}
      inline bool gpsInS(){return getStatusReg()&(1<<30) ? true : false;}
      inline bool ppsInS(){return getStatusReg()&(1<<29) ? true : false;}
      inline bool timeSetS(){return getStatusReg()&(1<<28) ? true : false;}
      inline bool ppsOk(){return getStatusReg()&(1<<27) ? true : false;}
      inline bool ppsLost(){return getStatusReg()&(1<<26) ? true : false;}
      CPI::OS::uint8_t rollingPPSIn(){return getStatusReg()&0xff;}

      inline void clearStickyBits(){setControlReg(1<<31);}
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
      CPI::OS::uint8_t * m_adminBaseAdr;

    };

  }
}




#endif
