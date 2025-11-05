#!/usr/bin/perl
#
# write out configdata.pm as json
#
use strict;
use warnings;
use JSON;

BEGIN {
        my $openssldir = shift;
        push @INC, $openssldir;
}
use configdata qw/%config %target %unified_info/;

my %data;
$data{'config'} = \%config;
$data{'target'} = \%target;
$data{'unified_info'} = \%unified_info;
print encode_json(\%data)
