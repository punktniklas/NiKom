#!/usr/bin/env python

# The motivation for this script is to be able to read a line of text with
# a timeout, while handling both CR and LF as line ending. 

import sys
import select
import optparse
import os

parser = optparse.OptionParser(
    usage = "Usage: %prog [options]",
    description = "Read a line of text and print it",
    option_list = [
        optparse.make_option("-t", "--timeout", type = "int", help = "Read timeout in seconds")])

options, args = parser.parse_args()

returnValue = 0
timeoutCount = 0
chars = []

unbufferedStdin = os.fdopen(sys.stdin.fileno(), "rb", 0)

while True:
    readable, writeable, exceptions = select.select([unbufferedStdin], [], [], 1)
    if unbufferedStdin in readable:
        timeoutCount = 0
        c = unbufferedStdin.read(1)
        if not c in ("\r", "\n"):
            chars.append(c)
        else:
            break
    else:
        timeoutCount += 1
        if options.timeout and options.timeout <= timeoutCount: 
            returnValue = 128 # Same as bash read when it times out 
            break

print "".join(chars) # Send the read string back on stdout
sys.exit(returnValue)
