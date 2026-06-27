#!/usr/bin/perl
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: chkcfg.pl - Verify settings in config.h
#

use strict;
use warnings;

my $PATH = "config.h";

sub check_deprecated_settings {
    my @DEPRECATED = qw(VESA_WIDTH VESA_HEIGHT VGA_MODE_12H GUI_THEME UART_MODE WALLPAPER_PATH);

    open(my $fh, "<", $PATH) or return;
    local $/;
    my $content = <$fh>;
    close $fh;

    my @found;
    foreach my $name (@DEPRECATED) {
        push @found, $name if $content =~ /^\s*#define\s+\Q$name\E\b/m;
    }

    return unless @found;

    print STDERR "\n";
    print STDERR "WARNING: $PATH defines settings that are no longer used:\n";
    print STDERR "- $_\n" foreach @found;
    print STDERR "They have been replaced with GRUB cmdline arguments.\n";
    print STDERR "See misc/grub.cfg for details.\n";
    print STDERR "\n";
}

check_deprecated_settings();
