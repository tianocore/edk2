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
# Due to the script wrapping required to process the OpenSSL
# configuration data, each native architecture must be processed
# individually by the maintainer (in addition to the standard version):
#   ./process_files.pl
#   ./process_files.pl X64
#   ./process_files.pl [Arch]
#
# Follow the command below to update the INF file:
# 1. OpensslLib.inf ,OpensslLibCrypto.inf and OpensslLibFull.inf
# ./process_files.pl
# 2. OpensslLibAccel.inf and OpensslLibFullAccel.inf
# ./process_files.pl X64
# ./process_files.pl X64Gcc
# ./process_files.pl IA32
# ./process_files.pl IA32Gcc

use strict;
use Cwd;
use File::Copy;
use File::Basename;
use File::Path qw(make_path remove_tree);
use Text::Tabs;

my $comment_character;

#
# OpenSSL perlasm generator script does not transfer the copyright header
#
sub copy_license_header
{
    my @args = split / /, shift;    #Separate args by spaces
    my $source = $args[1];          #Source file is second (after "perl")
    my $target = pop @args;         #Target file is always last
    chop ($target);                 #Remove newline char

    my $temp_file_name = "license.tmp";
    open (my $source_file, "<" . $source) || die $source;
    open (my $target_file, "<" . $target) || die $target;
    open (my $temp_file, ">" . $temp_file_name) || die $temp_file_name;

    #Add "generated file" warning
    $source =~ s/^..//;             #Remove leading "./"
    print ($temp_file "$comment_character WARNING: do not edit!\r\n");
    print ($temp_file "$comment_character Generated from $source\r\n");
    print ($temp_file "$comment_character\r\n");

    #Copy source file header to temp file
    while (my $line = <$source_file>) {
        next if ($line =~ /#!/);    #Ignore shebang line
        $line =~ s/#/$comment_character/;            #Fix comment character for assembly
        $line =~ s/\s+$/\r\n/;      #Trim trailing whitepsace, fixup line endings
        print ($temp_file $line);
        last if ($line =~ /http/);  #Last line of copyright header contains a web link
    }
    print ($temp_file "\r\n");
    #Retrieve generated assembly contents
    while (my $line = <$target_file>) {
        $line =~ s/\s+$/\r\n/;      #Trim trailing whitepsace, fixup line endings
        print ($temp_file expand ($line));  #expand() replaces tabs with spaces
    }

    close ($source_file);
    close ($target_file);
    close ($temp_file);

    move ($temp_file_name, $target) ||
        die "Cannot replace \"" . $target . "\"!";
}

#
# Find the openssl directory name for use lib. We have to do this
# inside of BEGIN. The variables we create here, however, don't seem
# to be available to the main script, so we have to repeat the
# exercise.
#
my $inf_file;
my $OPENSSL_PATH;
my $uefi_config;
my $extension;
my $compile;
my $arch;
my @inf;

BEGIN {
    $inf_file = "OpensslLib.inf";
    $uefi_config = "UEFI";
    $arch = shift;

    if (defined $arch) {
        if (uc ($arch) eq "X64") {
            $arch = "X64";
            $uefi_config = "UEFI-x86_64";
            $extension = "nasm";
            $compile = "MSFT";
            $comment_character = ";";
        } elsif (uc ($arch) eq "X64GCC") {
            $arch = "X64Gcc";
            $uefi_config = "UEFI-x86_64-GCC";
            $extension = "S";
            $compile = "GCC";
            $comment_character = "#";
        } elsif (uc ($arch) eq "IA32") {
            $arch = "IA32";
            $uefi_config = "UEFI-x86";
            $extension = "nasm";
            $compile = "MSFT";
            $comment_character = ";";
        } elsif (uc ($arch) eq "IA32GCC") {
            $arch = "IA32Gcc";
            $uefi_config = "UEFI-x86-GCC";
            $extension = "S";
            $compile = "GCC";
            $comment_character = "#";
        } else {
            die "Unsupported architecture \"" . $arch . "\"!";
        }
        $inf_file = "OpensslLibAccel.inf";
        if ($extension eq "nasm") {
            if (`nasm -v 2>&1`) {
                #Presence of nasm executable will trigger inclusion of AVX instructions
                die "\nCannot run assembly generators with NASM in path!\n\n";
            }
        }

        # Prepare assembly folder
        if (-d $arch) {
            opendir my $dir, $arch ||
                die "Cannot open assembly folder \"" . $arch . "\"!";
            while (defined (my $file = readdir $dir)) {
                if (-d "$arch/$file") {
                    next if $file eq ".";
                    next if $file eq "..";
                    remove_tree ("$arch/$file", {safe => 1}) ||
                       die "Cannot clean assembly folder \"" . "$arch/$file" . "\"!";
                }
            }

        } else {
            mkdir $arch ||
                die "Cannot create assembly folder \"" . $arch . "\"!";
        }
    }

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
                "--config=../UefiAsm.conf",
                "$uefi_config",
                "no-afalgeng",
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
use configdata qw/%config/;
use configdata qw/%target/;

#
# Collect build flags from configdata
#
my $flags = "";
foreach my $f (@{$config{lib_defines}}) {
    $flags .= " -D$f";
}

my @cryptofilelist = ();
my @sslfilelist = ();
my @ecfilelist = ();
my @asmfilelist = ();
my @asmbuild = ();
foreach my $product ((@{$unified_info{libraries}},
                      @{$unified_info{engines}})) {
    foreach my $o (@{$unified_info{sources}->{$product}}) {
        foreach my $s (@{$unified_info{sources}->{$o}}) {
            # No need to add unused files in UEFI.
            # So it can reduce porting time, compile time, library size.
            next if $s =~ "crypto/bio/b_print.c";
            next if $s =~ "crypto/rand/randfile.c";
            next if $s =~ "crypto/store/";
            next if $s =~ "crypto/err/err_all.c";
            next if $s =~ "crypto/aes/aes_ecb.c";

            if ($unified_info{generate}->{$s}) {
                if (defined $arch) {
                    my $buildstring = "perl";
                    foreach my $arg (@{$unified_info{generate}->{$s}}) {
                        if ($arg =~ ".pl") {
                            $buildstring .= " ./openssl/$arg";
                        } elsif ($arg =~ "PERLASM_SCHEME") {
                            $buildstring .= " $target{perlasm_scheme}";
                        } elsif ($arg =~ "LIB_CFLAGS") {
                            $buildstring .= "$flags";
                        }
                    }
                    ($s, my $path, undef) = fileparse($s, qr/\.[^.]*/);
                    $buildstring .= " ./$arch/$path$s.$extension";
                    make_path ("./$arch/$path");
                    push @asmbuild, "$buildstring\n";
                    push @asmfilelist, "  $arch/$path$s.$extension  |$compile\r\n";
                }
                next;
            }
            if ($s =~ "/ec/" || $s =~ "/sm2/") {
                push @ecfilelist, '  $(OPENSSL_PATH)/' . $s . "\r\n";
                next;
            }
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
  if ($_ =~ "/ec/" || $_ =~ "/sm2/") {
    push @ecfilelist, '  $(OPENSSL_PATH)/' . $_ . "\r\n";
    next;
  }
  push @cryptofilelist, '  $(OPENSSL_PATH)/' . $_ . "\r\n";
}


#
# Generate assembly files
#
if (@asmbuild) {
    print "\n--> Generating assembly files ... ";
    foreach my $buildstring (@asmbuild) {
        system ("$buildstring");
        copy_license_header ($buildstring);
    }
    print "Done!";
}

#
# Update OpensslLib.inf with autogenerated file list
#
my @new_inf = ();
my $subbing = 0;
print "\n--> Updating $inf_file ... ";
foreach (@inf) {
    if ($_ =~ "DEFINE OPENSSL_FLAGS_CONFIG") {
        push @new_inf, "  DEFINE OPENSSL_FLAGS_CONFIG    =" . $flags . "\r\n";
        next;
    }
    if ( $_ =~ "# Autogenerated files list starts here" ) {
        push @new_inf, $_, @cryptofilelist, @sslfilelist;
        $subbing = 1;
        next;
    }
    if (defined $arch) {
        my $arch_asmfile_flag = "# Autogenerated " . $arch . " files list starts here";
        if ($_ =~ $arch_asmfile_flag) {
            push @new_inf, $_, @asmfilelist;
            $subbing = 1;
            next;
        }
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

if (!defined $arch) {
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
}

#
# Update OpensslLibFull.inf with autogenerated file list
#
if (!defined $arch) {
    $inf_file = "OpensslLibFull.inf";
} else {
    $inf_file = "OpensslLibFullAccel.inf";
}
# Read the contents of the inf file
@inf = ();
@new_inf = ();
open( FD, "<" . $inf_file ) ||
    die "Cannot open \"" . $inf_file . "\"!";
@inf = (<FD>);
close(FD) ||
    die "Cannot close \"" . $inf_file . "\"!";
$subbing = 0;
print "\n--> Updating $inf_file ... ";
foreach (@inf) {
    if ( $_ =~ "# Autogenerated files list starts here" ) {
        push @new_inf, $_, @cryptofilelist, @sslfilelist, @ecfilelist;
        $subbing = 1;
        next;
    }
    if (defined $arch) {
        my $arch_asmfile_flag = "# Autogenerated " . $arch . " files list starts here";
        if ($_ =~ $arch_asmfile_flag) {
            push @new_inf, $_, @asmfilelist;
            $subbing = 1;
            next;
        }
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

