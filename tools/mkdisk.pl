#!/usr/bin/env perl
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkdisk.pl - Make bootable disk image
#

sub slurp {
    my ($path) = @_;
    open(my $f, $path) or die "Cannot read $path\n";
    binmode $f;
    local $/;
    my $data = <$f>;
    close $f;
    return $data;
}

sub spit {
    my ($path, $data) = @_;
    open(my $f, ">", $path) or die "Cannot write $path: $!\n";
    binmode $f;
    print $f $data;
    close $f or die "Write error on $path\n";
}

sub main {
    my ($boot_path, $kernel_path, $disk_path) = @ARGV;
    die "Usage: mkdisk.pl <boot-bin> <kernel-bin> <disk-image>\n" unless defined $disk_path;

    my $kernel = slurp($kernel_path);
    my $kernel_sectors = int((length($kernel) + 511) / 512);
    my $kernel_padding = "\0" x ($kernel_sectors * 512 - length($kernel));

    my $boot = slurp($boot_path);
    my $boot_sectors = int((length($boot) + 511) / 512);
    my $boot_padding = "\0" x ($boot_sectors * 512 - length($boot));
    my $stage2_sectors = $boot_sectors - 1;

    substr($boot, 5, 2, pack("v", $stage2_sectors));
    substr($boot, 512, 2, pack("v", $kernel_sectors));

    my $stage1 = substr($boot, 0, 512);
    my $image = $stage1 . $boot . $boot_padding . $kernel . $kernel_padding;

    spit($disk_path, $image);
}

main();
