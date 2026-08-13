#!/usr/bin/env perl
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: cproto.pl - Script for auto-generating header files with prototypes
#

my @SOURCE_DIRS = ("apps", "build", "kernel", "feat", "lib", "gui");

sub uniq {
    my (@list) = @_;
    my %hash = map { $_ => 1 } @list;
    my @ret = keys(%hash);
    return @ret;
}

sub slurp {
    my ($path) = @_;
    open(my $fh, "<", $path) or return "";
    binmode($fh);
    local $/;
    my $content = <$fh>;
    close($fh);
    return defined($content) ? $content : "";
}

sub process_file {
    my ($file) = @_;
    my @protos;

    open(my $fh, "<", $file) or die "Cannot open $file: $!";
    my $content = do { local $/; <$fh> };
    close($fh);

    $content =~ s/\r\n/\n/g;

    while ($content =~ /^global ([^{=;]*)[{=;]/mg) {
        my $decl = $1;
        $decl =~ s/ _unsd//g;
        $decl =~ s/\n/ /g;
        $decl =~ s/\s+/ /g;
        $decl =~ s/^\s+|\s+$//g;
        $decl =~ s/\* /*/g;
        push @protos, "extern $decl;";
    }

    return @protos;
}

sub main {
    foreach my $dir (@SOURCE_DIRS) {
        my @files = glob("$dir/*.c $dir/*.C");
        @files = map(lc, @files);
        @files = uniq(@files);
        @files = sort(@files);

        my @lines;

        foreach my $file (@files) {
            my @protos = process_file($file);
            next unless @protos;
            push @lines, "/* $file */";
            push @lines, @protos;
        }

        my $header = "include/p_${dir}.h";
        my $content = join("\n", @lines) . "\n";

        if (slurp($header) eq $content) {
            print "$0: $header unchanged\n";
            next;
        }

        open(my $fh, ">", $header) or die "Cannot open $header: $!";
        binmode($fh);
        print $fh $content;
        close($fh);

        print "$0: $header updated\n";
    }
}

main();
