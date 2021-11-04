# This should be safe to leave on for nearly all targets with a UNIX-like
# build environment (including MinGW), as the scripts that attempt get the
# SVN information will silently fail, leaving these variables empty.
#
# If this does cause problems, then just disable this .mak file, and let
# Makefile.common zero out the revision and dirty count.
SVN_REV := $(shell ./buildcfg/svn_rev.sh)
SVN_DTY := $(shell ./buildcfg/svn_dty.sh)
