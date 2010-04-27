#include <CpiUtilRefCounter.h>
#include <CpiOsAssert.h>

CPI::Util::Misc::RefCounter::RefCounter()
:refCount(1)
{

}
CPI::Util::Misc::RefCounter::~RefCounter()
{
	cpiAssert( refCount == 0 );
}
int CPI::Util::Misc::RefCounter::incRef()
{
	return refCount++;
}
int CPI::Util::Misc::RefCounter::decRef()
{
	refCount--;
	if ( refCount == 0 ) {
		delete this;
		return 0;
	}
	return refCount;
}


