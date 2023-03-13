#!/usr/bin/perl

# Copyright (c) 2018, MIPI Alliance, Inc. 
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#  Contributors:
#   norbert.schulz@intel.com - initial API and implementation
#
use strict;
use warnings;
use File::Spec;
use XML::Simple;
use File::Find;
use Getopt::Long;
use Pod::Usage;
use String::Escape;
use bigint qw/hex/;

use Data::Dumper;

# =============================================================================
# Globals
# =============================================================================
my ( undef, undef, $TOOL ) = File::Spec->splitpath( $0 );
my $INDENTATION = 2;
my $SRCLOCATION = 1;
my $CONFIG = ();
my $FILE_COUNTER = 0;
my $MSG_STRUCT = ();
my $FILE_STRUCT = ();
my $USED_IDS = ();
my %OPTIONS = ();


# =============================================================================
# Main functions.
# =============================================================================

sub main {
    read_options ();

    find({
        wanted => \&file_handler,
        preprocess => \&filter_file_list }, @{$OPTIONS{src}});

    sub file_handler {
        my $file = $File::Find::name;
        if (-f $_ && !is_file_existing ($file)) {
            _i ("Parsing: ".$file."\n");
            add_file ($file);
            my $count = parse_file ($_);
            _i ("Parsing finished: ".$file.", found $count call(s)\n");
        }
    }

    #
    # Filters the file list before the file_handler functions will be
    # called by File::find.
    #
    sub filter_file_list {
        my @filtered_list = ();
        # Filter files with wanted extensions.
        foreach my $pattern (@{$CONFIG->{SrcFilePatterns}->{SrcFilePattern}}) {
            push (@filtered_list, glob($pattern->{Pattern}));
        }
        # Add sub directories to scan whole trees.
        foreach my $dir (grep { -d } @_) {
            if ($dir ne '.' && $dir ne '..') {
                push (@filtered_list, $dir);
            }
        }
        return @filtered_list;
    }

    if (!defined ($FILE_STRUCT)) {
        _e ("No files found, which match the provided patterns!\n");
    }

    if (!defined ($MSG_STRUCT)) {
        _e ("No messages found!\n");
    }

    my $xml = generate_xml ($MSG_STRUCT, $FILE_STRUCT);

    _i ("Writing XML file: ".$OPTIONS{catalog}."\n");
    open (CATALOG, ">".$OPTIONS{catalog})
        or _e ("Could not open file (w): $!\n");
    print CATALOG $xml;
    close (CATALOG);
    _i ("Writing XML file finished\n");
}

#
# generate_xml (<msg-data-structure>, <file-data-structure>)
#
# Description:
# Generate SyS-T catalog XML structure based on read message
# calls and scanned files.
#
sub generate_xml {
    my $msg_struct = $_[0];
    my $file_struct = $_[1];

    _i ("Generating XML structure\n");

    my $output =  load_template($OPTIONS{template});

    # build sources section
    #
    if (defined $file_struct) {
        if ($SRCLOCATION) {
            my $source_list = '<syst:SourceFiles>'."\n";
            foreach my $file_id (sort {$a <=> $b} keys %{$file_struct}) {
                $source_list.= ' ' x ($INDENTATION*2);
                $source_list.= '<syst:File ID="'.$file_id.'">';
                $source_list.= '<![CDATA['.$file_struct->{$file_id}.']]>';
                $source_list.= '</syst:File>'."\n"
            }
            $source_list.= ' ' x $INDENTATION;
            $source_list.= '</syst:SourceFiles>'."\n";

            $output  =~ s/<syst:SourceFiles\/>/$source_list/sm;
        }

        if (defined $msg_struct->{"32"}) {
            if (index($output, '<syst:Catalog32/>') != -1) {
                my $section = generate_catalog_section($msg_struct, 32);
                $output =~ s/<syst:Catalog32\/>/$section/sm;
            } else {
                _w('32 bit catalog messages found, but template file '.
                   'is missing the insertion pattern "<syst:Catalog32/>"'."\n");
            }
        }
        if (defined $msg_struct->{"64"}) {
            if (index($output, '<syst:Catalog64/>') != -1) {
                my $section = generate_catalog_section($msg_struct, 64);
                $output =~ s/<syst:Catalog64\/>/$section/sm;
            } else {
                _w('64 bit catalog messages found, but template file '.
                   'is missing the insertion pattern "<Messages IdSize="64Bit"/>"'."\n");
            }
        }
    } else {
        _w( "no source files to parse found." );
    }

    _i ("Generating XML structure finished\n");

    return $output;
}

#
# generate_catalog_section (<found hits>, 32|64)
#
# Description
sub generate_catalog_section {
    my $hits = $_[0];
    my $size = $_[1]+0;

    my $data = $hits->{$size};
    my $output ="";

    $output.= '<syst:Catalog'.$size.'>'."\n";

    foreach my $file_id (sort keys %{$data}) {
        foreach my $line_id (sort {$a <=> $b} keys %{$data->{$file_id}}) {
            foreach my $msg_id (sort {$a <=> $b} keys %{$data->{$file_id}->{$line_id}}) {
                my $msg = $data->{$file_id}->{$line_id}->{$msg_id};
                my $idval= "";

                if ($size == 32) {
                    $idval = sprintf("0x%08x", $msg_id);
                } else {
                    $idval = sprintf("0x%016x", $msg_id);
                }
                $output.= ' ' x ($INDENTATION*2);
                $output.= '<syst:Format ID="'.$idval.'"';
                if ($SRCLOCATION) {
                    $output.= ' File="'.$file_id.'" Line="'.$line_id .'"';
                }
                $output.= '><![CDATA['.String::Escape::unbackslash($msg).']]>';
                $output.= '</syst:Format>'."\n";
            }
        }
    }
    $output.= ' ' x $INDENTATION;
    $output.= '</syst:Catalog'.$size.'>'."\n";

    return $output;
}

#
# add_file (<file>)
#
# Description:
# Add a file to the file data structure and increment global
# file counter.
#
sub add_file {
    my $file     = $_[0];

    $FILE_COUNTER++;

    if ($SRCLOCATION) {
        _i ("Add $file with file id $FILE_COUNTER to file catalog\n");
    }

    if (defined($FILE_STRUCT->{$FILE_COUNTER})) {
        _e ("File with file id $FILE_COUNTER already exists!\n");
    }
    $FILE_STRUCT->{$FILE_COUNTER} = $file;
}

#
# is_file_existing (<file>)
#
# Description:
# Check if file is already existing in the file data structure
#
sub is_file_existing {
    return (grep {$FILE_STRUCT->{$_} eq $_[0]} keys %{$FILE_STRUCT}) > 0;
}

#
# load_template (<file>)
#
# Description:
# Parse the given file as the catalog template file where the catalog
# messages get added into.
#
sub load_template {
    my $file         = $_[0];
    local $/ = undef;
    open (FILE, $file) or _e ("Could not open input template file ".$file.": $!\n");
    my $content = <FILE>;
    close FILE;

    _i("Loaded template collateral file ".$file."\n");

    return $content;
}

#
# parse_file (<file>)
#
# Description:
# Parse the given file to extract SyS-T catalog trace calls. Results will be
# stored into global catalog data structure.
#
sub parse_file {
    my $file         = $_[0];

    open (FILE, $file) or _e ("Could not open file ".$file.": $!\n");
    my @content = <FILE>;
    close FILE;

    my $add_count = 0;

    # loop over the input lines
    #
    for (my $i = 0; $i < $#content+1; $i++) {
        my $calltype= undef;

        for my $idsize ( "32", "64" ) {

            my $calltype ="Catalog".$idsize;
            my $callset = $CONFIG->{CatalogCalls}->{$calltype}->{CatalogCall};

             foreach my $function_name (keys %{$callset})
             {
                if (!defined ($callset->{$function_name}->{IdParamIdx})) {
                    _e ("Configuration: 'IdParamIdx' is not defined for function ".
                            "call: $function_name\n");
                }
                if (!defined ($callset->{$function_name}->{StringParamIdx})) {
                    _e ("Configuration: 'StringParamIdx' is not defined for ".
                            "function call: $function_name\n");
                }

                # find the current function call
                if ($content[$i] =~ /\b${function_name}\b/) {
                    my $call = strip_comments ($content[$i]);
                    my $line_no_start = $i+1;
                    my $line_no_end   = $i+1;

                    if (!strip_whitespaces ($call)) {
                        _i ("Catalog instrumentation call \@ ".
                                "$file:$line_no_start will be skipped.\n");
                        next;
                    }
                    _vi ("function call start: $function_name at line ".
                        $line_no_start."\n");

                    # try to find the end of the function call, which is might
                    # not be at the same line.
                    while ($content[$i] !~ /\)(\s*|\t*);/) {
                        my $tmp = strip_comments ($content[++$i]);
                        $call.=$tmp;
                    }
                    $line_no_end = $i+1;

                    _vi ("functon call end: $function_name at line ".
                        $line_no_end."\n");

                    # remove new line and extract arguments
                    $call =~ s/\n//g;
                    $call =~ m/${function_name}\b(\s*|\t*)\((.*)\)(\s*|\t*);/;

                    my $arguments = $2;

                    # now split the arguments by character
                    my @array = split(//, $arguments);
                    my $current_arg_no = 0;
                    my $inside_string = 0;
                    my $possible_end_of_call = 0;
                    my $bracket_count = 0;
                    my $found_quotes = 0;
                    my @function_args = ();
                    my $opening_quote = "";

                    # Iterate argument list character by character
                    for (my $i = 0; $i < $#array+1; $i++) {
                        # if a closing bracket will be found not inside the format
                        # string it may be the end of the call.
                        if ($array[$i] eq ")" && !$inside_string) {
                            $possible_end_of_call ^= 1;
                            if ($bracket_count == 0) {
                                last;
                            }
                            $bracket_count--;
                        }
                        # if opening bracket was found, reset possible end marker
                        if ($array[$i] eq "(" && !$inside_string) {
                            $bracket_count++;
                            $possible_end_of_call = 0;
                        }

                        # if a semicolon was found, not inside the format string
                        # and one of the previous characters was a closing bracket,
                        # it's the end of the call.
                        if ($array[$i] eq ";" && !$inside_string
                                && $possible_end_of_call) {
                            _d ("End of call detected at line $line_no_start\n");
                            last;
                        }

                        # Find the argument separators
                        if ($array[$i] eq "," && !$inside_string) {
                            $current_arg_no++;
                            next;
                        }

                        # If a format string was found, do not search
                        # for argument separators.
                        if ($array[$i] eq "\"" || $array[$i] eq "'") {
                            $found_quotes = 1;
                            # check if the previous character escaped the current
                            # one.
                            if ($i >= 0 && $array[$i-1] ne "\\") {
                                # Within a string, quotes match, so reset control variables.
                                if ($inside_string && $opening_quote eq $array[$i]) {
                                    $opening_quote = "";
                                    $inside_string ^= 1;
                                    next;
                                # Not within a string, start quote empty. Begin of a string.
                                } elsif (!$inside_string && $opening_quote eq "") {
                                    $opening_quote = $array[$i];
                                    $inside_string ^= 1;
                                    next;
                                }
                            }
                        }

                        # Skip spaces outside of a string
                        if (!$inside_string && $found_quotes && ($array[$i] eq " " || $array[$i] eq "\t")) {
                            next;
                        }

                        # Add argument to array.
                        $function_args[$current_arg_no].=$array[$i];
                    }

                    # remove leading and trailing whitespaces from the arguments
                    map {
                        $_ = strip_whitespaces ($_);
                    } @function_args;

                    my $str_idx = $callset->{$function_name}->{StringParamIdx};
                    my $file = (defined ($FILE_STRUCT->{$FILE_COUNTER}) ?
                                    $FILE_STRUCT->{$FILE_COUNTER} : $FILE_COUNTER);

                    my $message_id = undef;
                    my $message_str = $function_args[$str_idx-1];

                    my $algorithm = 'fromIdParam';
                    if (defined( $callset->{$function_name}->{Algorithm})) {
                        $algorithm =  $callset->{$function_name}->{Algorithm};
                    }
                    _d("Algorithm = $algorithm\n");

                    my $id_idx  =  $callset->{$function_name}->{IdParamIdx};
                    _d("IdParamIdx = $id_idx\n");
                    if ($algorithm eq 'hash65599') {
                        $message_id = hash_x65599 ($message_str, $function_args[$id_idx-1]);
                    } else {
                        $message_id = to_value($function_args[$id_idx-1]);
                    }

                    _d ("StringParamIdx = $str_idx\n");
                    _d ("IdSize = $idsize\n");
                    _d ("Found call: ".strip_whitespaces ($call)."\n");
                    _d ("Arguments: ".strip_whitespaces ($arguments)."\n");

                    if (!$found_quotes) {
                        _e ("Catalog instrumentation call \@ ".
                                "$file:$line_no_start ".
                                "could not be parsed. ".
                                "No valid format string found.\n");
                    }
                    if (!is_dec($message_id) && !is_hex($message_id)) {
                        _e ("Catalog instrumentation call \@ ".
                                "$file:$line_no_start '$message_id' ".
                                "is not decimal or hexadecimal.\n");
                    }

                    if (exists($USED_IDS->{$message_id})) {
                        if ($algorithm eq 'hash65599') {
                            _e ("Catalog instrumentation call \@ ".
                                    "$file:$line_no_start hash based ID '$message_id' ".
                                    "already exists. Change the offset value.\n");
                        } else {
                            _e ("Catalog instrumentation call \@ ".
                                    "$file:$line_no_start with ID '$message_id' ".
                                    "also exists \@ ".
                                    $FILE_STRUCT->{$USED_IDS->{$message_id}->{file}}.
                                    ":".
                                    $USED_IDS->{$message_id}->{line}.".\n");
                        }
                    }
                    $MSG_STRUCT->{$idsize}->{$FILE_COUNTER}->{$line_no_start}->{$message_id} = $message_str;

                    $USED_IDS->{$message_id} ={
                        file => $FILE_COUNTER,
                        line => $line_no_start
                    };
                    $add_count++;

                    _d ("-" x 79 . "\n");
                }
            }
        }
    }
    close (FILE);

    return $add_count;
}


# =============================================================================
# Helper functions.
# =============================================================================


#
# read_options_from_config_file ()
#
# Description:
# Read options from config file and store them into global data structure.
# The function also checks if the resp. option was overruled by a command
# line switch.
#
sub read_options_from_config_file {
    $CONFIG = read_config ($OPTIONS{config});

    if (!defined ($CONFIG->{CatalogConfigs})
            || !defined ($CONFIG->{CatalogConfigs}->{CatalogConfig})) {
        return;
    }

    my %new_options = ();
    foreach my $config_record (@{$CONFIG->{CatalogConfigs}->{CatalogConfig}}) {
        my $option = $config_record->{option};
        my $value = $config_record->{value};
        if (!defined($OPTIONS{$option})) {
            if ($option eq "guid" || $option eq "src") {
                if (!defined($new_options{$option})) {
                    @{$new_options{$option}} = ();
                }
                push (@{$new_options{$option}}, $value);
            } else {
                $new_options{$option} = $value;
            }
        } else {
            _vi ("Config file option '$option' overruled by ".
                    "command line option\n");
        }
    }

    # Merge the two hashes.
    @OPTIONS{keys %new_options} = values %new_options;
}

#
# read_options ()
#
# Description:
# Read options from command line an store into global data structure.
#
sub read_options {
    GetOptions (\%OPTIONS,
        'verbose|v',
        'debug|d',
        'src=s@',
        'catalog|cf=s',
        'config|c=s',
        'template|tpl=s',
        'nolocation|nl',
        'help|h'
    ) or pod2usage(-exitval => 0, -verbose => 2, -noperldoc => 1);

    if (defined($OPTIONS{help})) {
        pod2usage(-exitval => 0, -verbose => 2, -noperldoc => 1);
    }

    if (!defined($OPTIONS{config})) {
        _e ("-config option is missing\n");
    } elsif (!-f $OPTIONS{config}) {
        _e ("Specified config is not a file\n");
    }

    read_options_from_config_file ();

    if (defined($OPTIONS{src})) {
        map {
            _e ("Not existing or not a directory: $_\n") if !-e $_ || !-d $_;
        } @{$OPTIONS{src}};
    } else {
        _e ("-src option is missing\n");
    }

    if (!defined($OPTIONS{catalog})) {
        _e ("-catalog option is missing\n");
    }

    if (!defined($OPTIONS{template})) {
        _e ("-template option is missing\n");
    }

    if (defined($OPTIONS{indentation})) {
        $INDENTATION = $OPTIONS{indentation};
    }

    if (defined($OPTIONS{nolocation}) and
        (uc($OPTIONS{nolocation}) eq "TRUE") or ($OPTIONS{nolocation} == 1)
        )
    {
        $SRCLOCATION = 0;
    }
}

sub parse_guid {
    my $guid_expr = $_[0];
    my $ret = ();

    if ($guid_expr =~ /^(.*);(.*)$/) {
        $ret->{guid} = $1;
        $ret->{mask} = $2;
    } else {
        $ret->{guid} = $guid_expr;
    }

    return $ret;
}

#
# strip_comments (<string>)
#
# Description:
# Strip C-style comments.
#
sub strip_comments {
    my $str = $_[0];
    $str =~ s/\/\/.*//;
    $str =~ s/\/\*.*\*\///g;

    return $str;
}

#
# strip_whitespaces (<string>)
#
# Description:
# Strip leading and trailing whitespaces/tabs.
#
sub strip_whitespaces {
    my $str = $_[0];
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;
    $str =~ s/^\t+//;
    $str =~ s/\t+$//;

    return $str;
}

#
# read_config (<configuration-file>)
#
# Description:
# Read configuration and return data structure. An error
# message will be printed for every missing required section.
#
sub read_config {
    my $config_file = $_[0];

    my $xml_ref = XMLin($config_file,
        KeyAttr => {
            'CatalogCall' => 'Name'
        },
        ForceArray => [
            'CatalogCall',
            'SrcFilePattern',
            'CatalogConfig'
        ]
    );

    if (!defined ($xml_ref->{SrcFilePatterns})
            || !defined ($xml_ref->{SrcFilePatterns}->{SrcFilePattern})) {
        _e ("The specified config file does not contain a correct ".
                "SrcFilePatterns/SrcFilePattern structure\n");
    }

    if (!defined ($xml_ref->{CatalogCalls}) ||
        (!defined ($xml_ref->{CatalogCalls}->{Catalog32}->{CatalogCall}) &&
         !defined ($xml_ref->{CatalogCalls}->{Catalog64}->{CatalogCall}))
        )
    {
        _e ("The specified config file does not contain a correct ".
                "CatalogCalls/CatalogCall structure\n");
    }

    return $xml_ref;
}

#
# hash_x65599 (<string>, <offset>)
#
# Description:
# Calculates a x65599 hash for the provided string and offset value.
#
sub hash_x65599 {
    my $string  = String::Escape::unbackslash($_[0]);
    my $offset    = int($_[1]);

    my $hash = 0;
    my $tail256 = substr($string, -256);

    foreach my $char (split(//, $tail256 )) {
        $hash = uint ($hash * 65599 + ord($char));
    }

    return $hash + $offset;
}

#
# uint (<number>)
#
# Description:
# Mimics the unsigned int data type in Perl. Required for the x65599 hash
# calculation.
#
sub uint {
    return unpack('I', pack('I', $_[0]));
}

#
# to_value (<hex or decimal number>)
#
# Description:
# convert C-Language value to perl hex value
#
sub to_value {
    my $input = $_[0];
    my $val=0;

    # Remove possible LL or ULL from the hex number in the C file.
    $input =~ s/u?ll$//gi;
    if (is_hex($input)) {
        $val = hex($input);
    } else {
        $val = $input+0;
    }

    return $val;
}


#
# is_hex (<number>)
#
# Description:
# Checks if a number is a hex value.
#
sub is_hex {
    return ($_[0] =~ /^0x[0-9a-f]+$/i);
}

#
# is_dec (<number>)
#
# Description:
# Checks if a number is a decimal value.
#
sub is_dec {
    return ($_[0] =~ /^\d+$/);
}

#
# _e (<msg>)
#
# Description:
# Print error message on STDERR and exit with exist status 1.
#
sub _e {
    _print (*STDERR, "[ERROR]", $_[0]);
    exit (1);
}

#
# _w (<msg>)
#
# Description:
# Print warning message on STDOUT.
#
sub _w {
    _print (*STDOUT, "[WARNING]", $_[0]);
}

#
# _i (<msg>)
#
# Description:
# Print general message on STDOUT.
#
sub _i {
    _print (*STDOUT, "", $_[0]);
}

#
# _vi (<msg>)
#
# Description:
# Print verbose message on STDOUT.
#
sub _vi {
    _i ($_[0]) if $OPTIONS{verbose};
}

#
# _d (<msg>)
#
# Description:
# Print debug message on STDOUT.
#
sub _d {
    _print (*STDOUT, "[DEBUG]", $_[0]) if $OPTIONS{debug};
}

#
# _print (<msg>)
#
# Description:
# General print handler.
#
sub _print {
    my $handle  = $_[0];
    my $prefix  = $_[1];
    my $msg     = $_[2];

    print $handle "$TOOL: ".($prefix ne "" ? $prefix.": " : ""). $msg;
}

main ();
exit (0);

__END__
=head1 NAME

B<syst_cgen.pl> -- A SyS-T collateral generation script.

=head1 DESCRIPTION

Generate a SyS-T colleteral file with catalog call information from source
files. The script scans for pre-configured C-style macro calls and extracts
the message, its ID and the line number and puts it into a SyS-T collateral
XML structure.

=head1 USAGE

syst_cgen.pl

           -config <config file>
           [-template <catalog template file>]
           [-src <src-dir-1>] [-src <src-dir-2> [...]] [-o <xml-file>]
           [-verbose] [-debug]

  -src <src-dir>                Search path for the sources.
  -catalog|-cf <dest-file>      Destination catalog XML file.
  -template|-tpl <catalog-file> Catalog template to be extended with messages.
  -config|c <config-file>       Catalog generation config file.
  -nolocation|-nl               Supress source location generation
  -verbose|v                    Switch on verbose messages.
  -debug|d                      Switch on debug messages.


=head1 CONFIGURATION

The catalog configuration file specifies the parameters of the script, e.g.
the macro call names, the argument structure of the calls, and file extensions,
which should be scanned.

=head2 EXAMPLE CATALOG CONFIGURATION
   <CatalogGenerator>
       <CatalogConfigs>
           <CatalogConfig option="catalog" value="generated_catalog.xml" />
           <CatalogConfig option="template" value="template.xml" />
           <CatalogConfig option="indentation" value="4" />
           <CatalogConfig option="nolocation" value="true|false" />
           <CatalogConfig option="src" value="." />
       </CatalogConfigs>
       <SrcFilePatterns>
           <SrcFilePattern Pattern="*.{cpp,c,h}" />
       </SrcFilePatterns>
       <CatalogCalls>
           <Catalog32>
               <CatalogCall Name="MIPI_SYST_HASH" Algorithm="hash65599" IdParamIdx="2" StringParamIdx="1" />

               <CatalogCall Name="MIPI_SYST_CATPRINTF32" IdParamIdx="3" StringParamIdx="4"    />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_0" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_1" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_2" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_3" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_4" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_5" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF32_6" IdParamIdx="3" StringParamIdx="4"  />
           </Catalog32>
           <Catalog64>
               <CatalogCall Name="MIPI_SYST_CATPRINTF64" IdParamIdx="3" StringParamIdx="4" />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_0" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_1" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_2" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_3" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_4" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_5" IdParamIdx="3" StringParamIdx="4"  />
               <CatalogCall Name="MIPI_SYST_CATPRINTF64_6" IdParamIdx="3" StringParamIdx="4"  />
           </Catalog64>
       </CatalogCalls>
   </CatalogGenerator>

=head2 SECTION SrcFileExtensions

   <SrcFilePatterns>
      <SrcFilePattern Pattern="<pattern>" />
   </SrcFilePatterns>

This sections defines the file extensions, which will be scanned by the script.

=head2 SECTION CatalogCalls

   <CatalogCalls>
      <Catalog64 or Catalog32>
         <CatalogCall Name="<macro-name>" IdParamIdx="<id-argument-index>"
            StringParamIdx="<format-string-index>" IdSize="<id-size>" />
      </Catalog64 or /Catalog32>
   </CatalogCalls>

This section describes the used macro calls. It describes how the macro is
named and where the arguments of intereset can be found by the script. There
are different sections for 32 bit and 64 bit wide catalog calls.

=head2 SECTION CatalogConfigs

    <CatalogConfigs>
        <CatalogConfig option="<option-name>" value="<value>" />
    </CatalogConfigs>

This section specifies pre-defined options for the catalog generation script.
The options will be overruled by the respective command line options.

=cut
