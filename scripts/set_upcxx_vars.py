#!/usr/bin/python
'''
Set up the environment variables for using UPC++
'''

import sys
import os
import re
from optparse import OptionParser

VERBOSE=0

def process_line(line):
    if (line.startswith('#')):
        if (VERBOSE): print "Comment: " + line
        return

    assign_pos = line.find("=")
    if (assign_pos != -1):
        line = line.replace('(', '{')
        line = line.replace(')', '}')
        line = re.sub('\s+', ' ', line)
        print line[0:assign_pos-1].replace(' ','') + '="' + line[assign_pos+1:-1].lstrip() + '"'

def process_gasnet_mak(fn):
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
            process_line(full_line)
            full_line = ""
        
def main():
    ## Parse command options
    parser = OptionParser()
    parser.add_option("-f", "--file", action="store", type="string", dest="mak_file", help="upcxx mak file to read", default="")

    (options, args) = parser.parse_args()

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
            process_gasnet_mak(gasnet_mak_file)
        else:
            process_line(line)


    print "export UPCXX_LDLIBS"
    print "export UPCXX_LDFLAGS"
    print "export UPCXX_CXXFLAGS"
    
if __name__ == '__main__':
        main()
        
