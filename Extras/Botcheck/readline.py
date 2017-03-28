#!/usr/bin/env python

# The motivation for this script is to be able to read a line of text with
# a timeout, while handling both CR and LF as line ending. 

import sys
import select
import optparse
import os
from datetime import datetime, timedelta

parser = optparse.OptionParser(
    usage = "Usage: %prog [options]",
    description = "Read a line of text and print it",
    option_list = [
        optparse.make_option("-t", "--timeout", type = "int", help = "Timeout to read a line in seconds")])

options, args = parser.parse_args()

timeout = timedelta(seconds = options.timeout) if options.timeout else timedelta.max
returnValue = 0
chars = []

unbufferedStdin = os.fdopen(sys.stdin.fileno(), "rb", 0)

startDatetime = datetime.now()
while True:
    readable, writeable, exceptions = select.select([unbufferedStdin], [], [], 1)
    if unbufferedStdin in readable:
        timeoutCount = 0
        c = unbufferedStdin.read(1)
        if not c in ("\r", "\n"):
            chars.append(c)
        else:
            break

    if datetime.now() - startDatetime > timeout:
        returnValue = 128 # Same as bash read when it times out
        break

print "".join(chars) # Send the read string back on stdout
sys.exit(returnValue)
