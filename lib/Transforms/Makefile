##===- lib/Transforms/Makefile -----------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..
PARALLEL_DIRS = Utils Instrumentation Scalar InstCombine IPO Hello

include $(LEVEL)/Makefile.config

# Some targets don't support plugins
ifdef $(TARGET_HAS_DYNAMIC_LIBS)
  PARALLEL_DIRS := $(filter-out Hello, $(PARALLEL_DIRS))
endif

include $(LEVEL)/Makefile.common
