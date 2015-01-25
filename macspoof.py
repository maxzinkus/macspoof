#!/usr/bin/env python3
import subprocess
from getopt import getopt, GetoptError
from os import geteuid
from random import randrange
from re import match
from sys import argv, exit
from time import sleep

version = "macspoof v2.3 (beta) by Max Zinkus <maxzinkus@gmail.com>"

def macspoof():
    try:
        opts = getopt(argv[1:], "hVqi:m:", 
                ['help', 'version', 'quiet', 'interface=', 'mac='])[0]
    except GetoptError as err:
        print(str(err).capitalize() + ".")
        print("Do macspoof -h for usage information.")
        return 1
    interface = None
    spoof_addr = None
    quiet = False
    for opt, arg in opts:
        if opt == '-h' or opt == '--help':
            print(helppage)
            return 0
        elif opt == '-V' or opt == '--version':
            print(version)
            return 0
        elif opt == '-q' or opt == '--quiet':
            quiet = True
        elif opt == '-i' or opt == '--interface':
            interface = arg
        elif opt == '-m' or opt == '--mac':
            spoof_addr = arg
    if subprocess.check_output(['uname', '-s']).strip() != b'Darwin' and not quiet:
        print("Warning: Non-Darwin operating system detected, proceed with caution.")
    if not interface:
        if not quiet:
            print("No interface specified.")
            print("Do macspoof -h for usage information.")
        return 1
    if geteuid() != 0:
        if not quiet:
            print("macspoof needs to be run as root.")
        return 1
    if not validate_interface(interface):
        if not quiet:
            print("Invalid interface.")
        return 1
    if spoof_addr is not None:
        if not validate_addr(spoof_addr):
            if not quiet:
                print("Invalid address given.")
                print("Do macspoof -h for usage information.")
            return 1
    else:
        spoof_addr = generate_mac()
    startmac = subprocess.check_output(['ifconfig', interface, 'ether'])[-19:-2]
    sleep(0.5)
    subprocess.call(['ifconfig', interface, 'ether', spoof_addr])
    sleep(0.5)
    subprocess.call(['ifconfig', interface, 'down'])
    sleep(0.5)
    subprocess.call(['ifconfig', interface, 'up'])
    sleep(0.5)
    if startmac == subprocess.check_output(['ifconfig', interface, 'ether'])[-19:-2]:
        if not quiet:
            print("NIC refused update, you may wish to try again.")
        return 1
    return 0

def validate_interface(interface):
    pattern = "^([0-9a-zA-Z]){3}$"
    if not match(pattern, interface):
        return False
    try:
        subprocess.check_call(['ifconfig', '--', interface],
                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        return False
    return True

def validate_addr(addr):
    pattern = "([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})"
    if match(pattern, addr):
        return True
    return False

def generate_mac():
    mac = []
    while len(mac) <= 16:
        byte = "%x" % randrange(16)
        mac.append(byte)
        if len(mac) in [2, 5, 8, 11, 14]:
            mac.append(':')
    return ''.join(mac)

helppage = """NAME
    macspoof - Spoof your Media Access Control address

SYNOPSIS
    As root (EUID 0):
    macspoof -i <id> [-m <mac>]

DESCRIPTION
    This tool changes the Media Access Control (MAC) address reported by your 
    Network Interface Controller (NIC) until the NIC resets (i.e. system 
    reboot). If no MAC address is specified with -m or --mac, a random
    address will be generated and used. While the NIC reads this address out
    of ROM at boot, it can be spoofed by reassignment such as that supported
    in some ifconfig implementations. Note that upon success, data link layer
    connections will likely need to be renegotiated. This tool needs to be run
    as root (unless any of -h, --help, -V, or --version are passed) in order
    to configure the NIC via ifconfig. This software is in beta. Further, it is
    designed for and has only be tested on Mac OS X (specifically >=10.8).
    Results may vary by system. Please do not use this in a manner inconsistent
    with any policy or law, including but not limited to circumvention of MAC
    address filtering. No warranty whatsoever is offered for this software.

OPTIONS
    -h, --help                              Print usage info and exit
    -V, --version                           Print version info and exit

    
    -i, --interface <id>    Interface identifier (See below: INTERFACE INFO)
    -m, --mac <mac>         Spoof to given address (See below: MAC ADDRESS INFO)

    -q, --quiet             Suppress error messages

INTERFACE INFO
    On most *nix systems, ifconfig will output information about NIC
    interfaces. Here, interface identifier is the ifconfig name of a NIC
    interface. For example, on most Mac OS X systems, the wifi interface is
    ``en1'', and on many Linux systems the wifi interface is ``wlan0'' or
    ``en0''.

MAC ADDRESS INFO
    A valid MAC address consists of six colon-delimited hexadecimal bytes, for
    example:
        50:ca:18:8a:11:de
        de:ad:be:ef:ca:fe
    Capitalization of non-digit characters in hex digits is system-specific,
    however in testing, ifconfig handles the disparity. If none is specified,
    macspoof will generate a random valid address to spoof to.

AUTHOR
    Max Zinkus <maxzinkus@gmail.com>
"""

if __name__ == '__main__':
    exit(macspoof())
