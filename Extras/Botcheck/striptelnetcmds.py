#!/usr/bin/env python

# This script strips telnet command data from a line of text on stdin and
# prints it on stdout.

import sys

state = "Normal"
chars = []

for c in sys.stdin.readline():
    if "Normal" == state:
        if "\xff" == c:
            state = "Command"
        else:
            chars.append(c)
    elif "Command" == state:
        command = c
        if "\xff" == command:
            # Command followed by command should output command
            chars.append("\xff")
            state = "Normal"
        elif "\xfa" == command:
            state = "SubOption"
        elif command in ("\xfb", "\xfc", "\xfd", "\xfe"):
            # WILL, WON'T, DO and DON'T is followed by a byte of option code
            state = "OptionCode"
        else:
            state = "Normal"
    elif "SubOption" == state:
        # Bytes following a sub-option til next command are part of it
        if "\xff" == c:
            state = "Command"
    elif "OptionCode" == state:
        state = "Normal"

sys.stdout.write("".join(chars))

