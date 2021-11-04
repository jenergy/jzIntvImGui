
# If there's a .svn directory, assume this is a Subversion workarea, and
# estimate the number of added/modified/conflicted files.
if [ -r ../.svn ] ; then
    echo $( (svn status -q || :) | grep -v '^[ ?]' | wc -l )
fi
