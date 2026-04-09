#/** @file
#  This is a very simple shell script to test how the interpreter parses the parameters.
#
#  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#**/
echo -on
set Var_EFCF356F_228C_47C2_AD0C_3B5DAC9A8CFA      ValueOfGuid
set Sharp_E8528E46_A008_4221_8DE0_D5AB42A9C580    ^#
set Quote_E95DEE8B_E3AA_4155_9ED5_6916394104FC    ^"
set Var_ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE
alias ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE   ShellCTestApp

#
# '^' should escape all special characters (including space)
#     but has no impact to non-special characters
#
ShellCTestApp ^^
ShellCTestApp ^#
ShellCTestApp ^%Var_EFCF356F_228C_47C2_AD0C_3B5DAC9A8CFA%
ShellCTestApp ^"
ShellCTestApp ^ 1
ShellCTestApp ^ 
ShellCTestApp ^1
ShellCTestApp ^^^"
ShellCTestApp ^^^

#
# '#' should be processed before %% replacement, and inside '"'
#
ShellCTestApp #%Var_EFCF356F_228C_47C2_AD0C_3B5DAC9A8CFA%
#ShellCTestApp "#"
ShellCTestApp %Sharp_E8528E46_A008_4221_8DE0_D5AB42A9C580%

#
# '%' should be processed before grouping parameters
#
ShellCTestApp "%Var_EFCF356F_228C_47C2_AD0C_3B5DAC9A8CFA% 2%Quote_E95DEE8B_E3AA_4155_9ED5_6916394104FC%

#
# alias should be processed after %% replacement
#
%Var_ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE%

#
# '"' should be stripped, space inside '"' should be kept, 
#
ShellCTestApp "p   1"
ShellCTestApp "p"1
ShellCTestApp "p   1"e"x"""

set -d Var_EFCF356F_228C_47C2_AD0C_3B5DAC9A8CFA
set -d Sharp_E8528E46_A008_4221_8DE0_D5AB42A9C580
set -d Quote_E95DEE8B_E3AA_4155_9ED5_6916394104FC
set -d Var_ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE
alias -d ShellCTestApp_EE6E8BC6_71A6_44A5_BED3_D8F901105CDE
echo -off