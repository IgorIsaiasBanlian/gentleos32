#!/usr/bin/env perl
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: sectors.pl - Print file size in 512B sectors
#

my $path = $ARGV[0] or die "Usage: sectors.pl <file>\n";
my $size = -s $path;
die "Cannot read $path\n" unless defined $size;

print int(($size + 511) / 512), "\n";
