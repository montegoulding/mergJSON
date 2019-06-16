# This Makefile takes advantage of the fact that the LiveCode engine's
# "tests/Makefile" already does pretty much everything required for
# running a test suite.  Rather than duplicating it, it can
# just be included with a few tweaks to its configuration.

# Override the default value of top_srcdir so that it points to the
# top of the engine repository.
top_srcdir = ../livecode

LCS_TESTS_DIR = test-scripts

# Things have now been setup enough that the engine's test Makefile
# can perform the tests without any further configuration.
include $(top_srcdir)/tests/Makefile