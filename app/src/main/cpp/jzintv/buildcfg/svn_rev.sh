
# If there's a .svn directory, assume this is a Subversion workarea, and
# get the current SVN revision number.  Otherwise, this script outputs nothing.
if [ -r ../.svn ] ; then
    (svnversion | sed -e 's/[^:]*://g' -e 's/[^0-9].*//g') || :
fi
