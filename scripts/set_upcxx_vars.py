#!/usr/bin/python
'''
Set up the environment variables for using UPC++
'''

import sys
import os
import re
from optparse import OptionParser

VERBOSE=0

def process_line(line, is_csh):
    if (line.startswith('#')):
        if (VERBOSE): print "Comment: " + line
        return

    assign_pos = line.find("=")
    if (assign_pos != -1):
        line = line.replace('(', '{')
        line = line.replace(')', '}')
        line = re.sub('\s+', ' ', line)
        if (line.startswith('GASNET_LDFLAGS_OVERRIDE')):
            line = re.sub('-O[0-9]', '', line) ## remove "-Ox" flag
        if (is_csh):
            print("set "),
            
        print line[0:assign_pos-1].replace(' ','') + '="' + line[assign_pos+1:-1].lstrip() + '"'

def process_gasnet_mak(fn, is_csh):
    if (VERBOSE):
        print("gasnet mak file: %s" % fn)

    try:
        mak_file=open(fn, 'r')
    except:
        sys.stderr.write('Failed to open gasnet makefile: %s\n' % fn)
        sys.exit(0)

    full_line = ""    
    for line in mak_file:
        full_line += line
        if (full_line.rfind('\\') != -1):
            full_line = full_line.strip('\\\n\r')
            ## print "full_line %s" % full_line
        else:    
            process_line(full_line, is_csh)
            full_line = ""

def prelude_for_csh():
    gasnet_vars = ["CONDUIT_INCLUDES",\
                   "CONDUIT_LIBDIRS",\
                   "CONDUIT_LDFLAGS",\
                   "CONDUIT_LIBS",\
                   "CONDUIT_OPT_CFLAGS",\
                   "CONDUIT_MISC_CFLAGS",\
                   "CONDUIT_MISC_CPPFLAGS",\
                   "CONDUIT_DEFINES",\
                   "MANUAL_CFLAGS",\
                   "MANUAL_LDFLAGS",\
                   "MANUAL_LIBS"]
    suffixes = ["", "_SEQ", "_PAR"]
    for var in gasnet_vars:
        for suffix in suffixes:
            print "set " + var + suffix + "= "

def main():
    ## Parse command options
    parser = OptionParser()
    parser.add_option("-f", "--file", action="store", type="string", dest="mak_file", help="upcxx mak file to read", default="")
    parser.add_option("-c", "--csh", action="store_true", dest="is_csh", help="generate setenv file for csh", default=False)

    (options, args) = parser.parse_args()

    if (options.is_csh):
        prelude_for_csh()
    
    if (VERBOSE):
        print("upcxx mak file: %s" % options.mak_file)

    try:
        mak_file=open(options.mak_file, 'r')
    except:
        sys.stderr.write('Failed to open upcxx makefile: %s\n' % fn)
        sys.exit(0)

    for line in mak_file:
        if (line.startswith("include")):
            gasnet_mak_file = line.lstrip('include').replace(' ','').rstrip('\n\r')
            process_gasnet_mak(gasnet_mak_file, options.is_csh)
        else:
            process_line(line, options.is_csh)
    
    if (options.is_csh):
        print 'setenv UPCXX_LDLIBS "${UPCXX_LDLIBS}"'
        print 'setenv UPCXX_LDFLAGS "${UPCXX_LDFLAGS}"'
        print 'setenv UPCXX_CXXFLAGS "${UPCXX_CXXFLAGS}"'
        print 'set path = ($path $UPCXX_DIR/bin)'
    else:
        print 'export UPCXX_LDLIBS'
        print 'export UPCXX_LDFLAGS'
        print 'export UPCXX_CXXFLAGS'
        print 'export PATH=$PATH:$UPCXX_DIR/bin'

if __name__ == '__main__':
        main()
        
