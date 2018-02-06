
To run the best fit test type, on three different test cases
> make run

To run the test mabually type
> ./bestfit <thoughput> <distortion> <memusage>

  Where:
  throughput is frames per second
  distortion is in %
  memusage is in bytes

Example:
  ./bestfit 9999 .001 100

This example selects implementation #1 since it is the only implementation that has the ability to meet the distortion requirements.

The examples application uses the following expressions for its test case.

      OA::PVString("__ocpi__exp-required", "throughput > Appthroughput"),
      OA::PVString("__ocpi__exp-scored 40", "distortion <= Appdistortion"),
      OA::PVString("__ocpi__exp-scored 10", "memoryusage <= Appmemusage"),

Appthroughput, Appdisortion and Appmemusage are the parameters passed in at the command line.  

Each implementation has a set of properties that are used to advertise its capabilities.  Here are the property values for each worker.


		    throughput		    memoryusage	     distortion
impl1:		    10000		    500		     .001
impl2:		    25000		    5000	     .002
impl3:		    50000		    50000	     .003



