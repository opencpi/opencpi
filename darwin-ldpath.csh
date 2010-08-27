setenv OCPI_ROOT `pwd`
setenv OCPI_HOST_TARGET darwin-i386-bin
setenv DYLD_LIBRARY_PATH \
$OCPI_ROOT/lib/$OCPI_HOST_TARGET{}:$OCPI_ROOT/../ACE_wrappers/$OCPI_HOST_TARGET/ace/.libs:$OCPI_ROOT/../ACE_wrappers/$OCPI_HOST_TARGET/TAO/tao/.libs:$OCPI_ROOT/../ACE_wrappers/$OCPI_HOST_TARGET/TAO/orbsvcs/orbsvcs/.libs:$OCPI_ROOT/../ACE_wrappers/$OCPI_HOST_TARGET/TAO/orbsvcs/IFR_Service/.libs:$OCPI_ROOT/../ACE_wrappers/$OCPI_HOST_TARGET/TAO/TAO_IDL/.libs
