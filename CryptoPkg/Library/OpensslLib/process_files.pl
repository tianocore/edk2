#!/usr/bin/perl -w
#
# This script runs the OpenSSL Configure script, then processes the
# resulting file list into our local OpensslLib[Crypto].inf and also
# takes copies of opensslconf.h and dso_conf.h.
#
# This only needs to be done once by a developer when updating to a
# new version of OpenSSL (or changing options, etc.). Normal users
# do not need to do this, since the results are stored in the EDK2
# git repository for them.
#
use strict;
use Cwd;
use File::Copy;

#
# Find the openssl directory name for use lib. We have to do this
# inside of BEGIN. The variables we create here, however, don't seem
# to be available to the main script, so we have to repeat the
# exercise.
#
my $inf_file;
my $OPENSSL_PATH;
my @inf;

BEGIN {
    $inf_file = "OpensslLib.inf";

    # Read the contents of the inf file
    open( FD, "<" . $inf_file ) ||
        die "Cannot open \"" . $inf_file . "\"!";
    @inf = (<FD>);
    close(FD) ||
        die "Cannot close \"" . $inf_file . "\"!";

    foreach (@inf) {
        if (/DEFINE\s+OPENSSL_PATH\s*=\s*([a-z]+)/) {

            # We need to run Configure before we can include its result...
            $OPENSSL_PATH = $1;

            my $basedir = getcwd();

            chdir($OPENSSL_PATH) ||
                die "Cannot change to OpenSSL directory \"" . $OPENSSL_PATH . "\"";

            # Configure UEFI
            system(
                "./Configure",
                "UEFI",
                "no-afalgeng",
                "no-asm",
                "no-async",
                "no-autoerrinit",
                "no-autoload-config",
                "no-bf",
                "no-blake2",
                "no-camellia",
                "no-capieng",
                "no-cast",
                "no-chacha",
                "no-cms",
                "no-ct",
                "no-deprecated",
                "no-des",
                "no-dgram",
                "no-dsa",
                "no-dynamic-engine",
                "no-ec",
                "no-ec2m",
                "no-engine",
                "no-err",
                "no-filenames",
                "no-gost",
                "no-hw",
                "no-idea",
                "no-md4",
                "no-mdc2",
                "no-pic",
                "no-ocb",
                "no-poly1305",
                "no-posix-io",
                "no-rc2",
                "no-rc4",
                "no-rfc3779",
                "no-rmd160",
                "no-scrypt",
                "no-seed",
                "no-sock",
                "no-srp",
                "no-ssl",
                "no-stdio",
                "no-threads",
                "no-ts",
                "no-ui",
                "no-whirlpool",
                # OpenSSL1_1_1b doesn't support default rand-seed-os for UEFI
                # UEFI only support --with-rand-seed=none
                "--with-rand-seed=none"
                ) == 0 ||
                    die "OpenSSL Configure failed!\n";

            # Generate opensslconf.h per config data
            system(
                "perl -I. -Mconfigdata util/dofile.pl " .
                "include/openssl/opensslconf.h.in " .
                "> include/openssl/opensslconf.h"
                ) == 0 ||
                    die "Failed to generate opensslconf.h!\n";

            # Generate dso_conf.h per config data
            system(
                "perl -I. -Mconfigdata util/dofile.pl " .
                "include/crypto/dso_conf.h.in " .
                "> include/crypto/dso_conf.h"
                ) == 0 ||
                    die "Failed to generate dso_conf.h!\n";

            chdir($basedir) ||
                die "Cannot change to base directory \"" . $basedir . "\"";

            push @INC, $1;
            last;
        }
    }
}

#
# Retrieve file lists from OpenSSL configdata
#
use configdata qw/%unified_info/;

my @cryptofilelist = ();
my @sslfilelist = ();
foreach my $product ((@{$unified_info{libraries}},
                      @{$unified_info{engines}})) {
    foreach my $o (@{$unified_info{sources}->{$product}}) {
        foreach my $s (@{$unified_info{sources}->{$o}}) {
            next if ($unified_info{generate}->{$s});
            next if $s =~ "crypto/bio/b_print.c";

            # No need to add unused files in UEFI.
            # So it can reduce porting time, compile time, library size.
            next if $s =~ "crypto/rand/randfile.c";
            next if $s =~ "crypto/store/";
            next if $s =~ "crypto/err/err_all.c";
            next if $s =~ "crypto/aes/aes_ecb.c";

            if ($product =~ "libssl") {
                push @sslfilelist, '  $(OPENSSL_PATH)/' . $s . "\r\n";
                next;
            }
            push @cryptofilelist, '  $(OPENSSL_PATH)/' . $s . "\r\n";
        }
    }
}


#
# Update the perl script to generate the missing header files
#
my @dir_list = ();
for (sort keys %{$unified_info{dirinfo}}){
  push @dir_list,$_;
}

my $dir = getcwd();
my @files = ();
my @headers = ();
chdir ("openssl");
foreach(@dir_list){
  @files = glob($_."/*.h");
  push @headers, @files;
}
chdir ($dir);

foreach (@headers){
  if(/ssl/){
    push @sslfilelist, '  $(OPENSSL_PATH)/' . $_ . "\r\n";
    next;
  }
  push @cryptofilelist, '  $(OPENSSL_PATH)/' . $_ . "\r\n";
}


#
# Update OpensslLib.inf with autogenerated file list
#
my @new_inf = ();
my $subbing = 0;
print "\n--> Updating OpensslLib.inf ... ";
foreach (@inf) {
    if ( $_ =~ "# Autogenerated files list starts here" ) {
        push @new_inf, $_, @cryptofilelist, @sslfilelist;
        $subbing = 1;
        next;
    }
    if ( $_ =~ "# Autogenerated files list ends here" ) {
        push @new_inf, $_;
        $subbing = 0;
        next;
    }

    push @new_inf, $_
        unless ($subbing);
}

my $new_inf_file = $inf_file . ".new";
open( FD, ">" . $new_inf_file ) ||
    die $new_inf_file;
print( FD @new_inf ) ||
    die $new_inf_file;
close(FD) ||
    die $new_inf_file;
rename( $new_inf_file, $inf_file ) ||
    die "rename $inf_file";
print "Done!";

#
# Update OpensslLibCrypto.inf with auto-generated file list (no libssl)
#
$inf_file = "OpensslLibCrypto.inf";

# Read the contents of the inf file
@inf = ();
@new_inf = ();
open( FD, "<" . $inf_file ) ||
    die "Cannot open \"" . $inf_file . "\"!";
@inf = (<FD>);
close(FD) ||
    die "Cannot close \"" . $inf_file . "\"!";

$subbing = 0;
print "\n--> Updating OpensslLibCrypto.inf ... ";
foreach (@inf) {
    if ( $_ =~ "# Autogenerated files list starts here" ) {
        push @new_inf, $_, @cryptofilelist;
        $subbing = 1;
        next;
    }
    if ( $_ =~ "# Autogenerated files list ends here" ) {
        push @new_inf, $_;
        $subbing = 0;
        next;
    }

    push @new_inf, $_
        unless ($subbing);
}

$new_inf_file = $inf_file . ".new";
open( FD, ">" . $new_inf_file ) ||
    die $new_inf_file;
print( FD @new_inf ) ||
    die $new_inf_file;
close(FD) ||
    die $new_inf_file;
rename( $new_inf_file, $inf_file ) ||
    die "rename $inf_file";
print "Done!";

#
# Copy opensslconf.h and dso_conf.h generated from OpenSSL Configuration
#
print "\n--> Duplicating opensslconf.h into Include/openssl ... ";
system(
    "perl -pe 's/\\n/\\r\\n/' " .
    "< " . $OPENSSL_PATH . "/include/openssl/opensslconf.h " .
    "> " . $OPENSSL_PATH . "/../../Include/openssl/opensslconf.h"
    ) == 0 ||
    die "Cannot copy opensslconf.h!";
print "Done!";

print "\n--> Duplicating dso_conf.h into Include/crypto ... ";
system(
    "perl -pe 's/\\n/\\r\\n/' " .
    "< " . $OPENSSL_PATH . "/include/crypto/dso_conf.h" .
    "> " . $OPENSSL_PATH . "/../../Include/crypto/dso_conf.h"
    ) == 0 ||
    die "Cannot copy dso_conf.h!";
print "Done!\n";

print "\nProcessing Files Done!\n";

exit(0);

