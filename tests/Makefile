all:
	make -C	facilities/test_os -f Makefile.ocpi
	make -C	facilities/test_logger -f Makefile.ocpi
	make -C framework/framework -f Makefile.ocpi

clean:
	make -C	facilities/test_os -f Makefile.ocpi clean
	make -C	facilities/test_logger -f Makefile.ocpi clean
	make -C framework/framework -f Makefile.ocpi clean

run:
	framework/framework/$(OCPI_OUT_DIR)/ocpi_unit_test

runv:
	valgrind framework/framework/$(OCPI_OUT_DIR)/ocpi_unit_test

