#!/usr/bin/perl
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# PROGRAM NAME: configure_sig_gen.pl <IP address of sig gen> <command>]
# Send command to signal generator
# Based on Keysight sample program perl.txt

use IO::Socket;

my $total = $#ARGV + 1;

#If two commands are passed, it is on or off
die "Incorrect number of arguments., Usage is configure_sig_gen.pl <IP address of sig gen> <command>\n" unless $total eq 2;

my $instrumentName = $ARGV[0];
# Get socket
$sock = new IO::Socket::INET ( PeerAddr => $instrumentName,
			       PeerPort => 5025,
			       Proto => 'tcp',
    );
die "Socket Could not be created, Reason: $!\n" unless $sock;

my $command = $ARGV[1];
# Set freq
print "Sending command $command \n";
print $sock "$command\n";

# Wait for completion
#print "Waiting for source to settle...\n";
print $sock "*opc?\n";
my $response = <$sock>;
chomp $response; # Removes newline from response
if ($response ne "1")
{
 die "Bad response to '*OPC?' from instrument!\n";
} 
