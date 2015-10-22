add_executable(ctestunittests
	assertfailurestests.c
	assertsuccesstests.c
	ctestunittests.c
	simpletestsuiteonetest.c
	simpletestsuitetwotests.c
	testfunctioncleanuptests.c
	testfunctioninitializetests.c
	testsuiteinitializecleanuptests.c
)

target_link_libraries(ctestunittests ctest)