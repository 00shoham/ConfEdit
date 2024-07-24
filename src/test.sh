#!/bin/sh

#./api-test -apitest config insertline\
#           -arg filename config\
#           -arg linenumber 10\
#           -arg text "#Hi, mom!"

#./api-test -apitest config replaceline\
#           -arg filename config\
#           -arg linenumber 10\
#           -arg text "#Hello, mother."

#./api-test -apitest config deleteline\
#           -arg filename config\
#           -arg linenumber 10
#

#./api-test -apitest config appendline\
#           -arg filename config\
#           -arg text "#Hi, mom!"

#touch /var/confedit/junk.ini
#./api-test -apitest config replacefile\
#           -arg filename junk\
#           -array contents '[' One Two Three '' '#comment' 'VAR=VALUE with spaces' '' '#include '' \"hi\"' ']'

#./api-test -apitest config replacefile\
#           -arg filename junk\
#           -array contents '[' One Two Three '#comment' 'VAR=VALUE' ']'

./api-test -apitest config readstr\
           -arg filename config
