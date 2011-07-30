/** @file error and warning output messages

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 
**/

#ifndef _ERR_H_
#define _ERR_H_

//
// Error and Warning outputs
//

void errx (int eval, const char *fmt, ...);
void err  (int eval, const char *fmt, ...);
void warnx(const char *fmt, ...);
void warn (const char *fmt, ...);

#endif