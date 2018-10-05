There are multiple tests for the capture_v2 worker. The top level tests allows for four different tests:
1: Test sending no data.
2: Test making only metadata full.
3: Test making data full.
4: Test sending a multiple zlms (with different opcodes), a single word message, filling data and
filling up metadata. For configurations where there are at least six metadata records.

In the capture_v2-test.xml, there is a test property called testScenario that takes the values 1, 2, 3 or 4 to choose which one of the four tests to run.

The generate.py script generates input data depending on which testScenario was chosen and the verify.py scripts determines how to verify the data based on the testScenario.

There are also some custom tests that tests things not currently supported by the test suite. They test stopping on an opcode other than 0 (need to be able to set done=capture_v2
in the app xml to be able to test this) and testing the worker with no output connected.

These custom tests are built and run manually by running the run_test.sh script.

The generate.py and verify.py scripts in the custom test subdirectories are simpler versions than the ones in the top level capture_v2 test directory or they are specific to the custom test.
