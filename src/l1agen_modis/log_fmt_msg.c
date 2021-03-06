#include "L1A_prototype.h"
#include "PGS_SMF.h"
#include <stdarg.h>
#include <string.h>

void log_fmt_msg ( PGSt_SMF_status  code, const char *routine,
	const char *msg_fmt, ... )
/*
!C*****************************************************************************

!Description:  Function log_fmt_msg writes a SMF log message with a variable
               number of dynamic fields defined by users and a message string 
               associated with the passed in code (mnemonic error/status).

!Input Parameters: 
               code         **  status code generated by message  **
               routine      **  function where the status condition occured ** 
               msg_fmt      **  format specification of variable arguments  **
               ...          **  vaiable argument list.  The argument data
                                type can be int, float, string, etc.  **

!Output Parameters: 
               None

Return Values: 
               None

Externally Defined: 
               PGS_SMF_MAX_MSGBUF_SIZE       (PGS_SMF.h)
               PGS_SMF_MAX_MSG_SIZE          ()

Called By:
               L1A routines

Routines Called:
               PGS_SMF_GetMsgByCode
               PGS_SMF_SetDynamicMsg

!Revision History:    
               revision 1.0 1997/09/17  17:30:00
               Qi Huang/RDC    (qhuang@ltpmail.gsfc.nasa.gov)
               Original development


!Team-unique Header:  
               This software is developed by the MODIS Science
               Data Support Team (SDST) for the National Aeronautics
               and Space Administration (NASA), Goddard Space Flight
               Center (GSFC), under contract NAS5-32373.

!References and Credits:
               None

!Design Notes: 
               None

!END**************************************************************************
*/
{
  /*************************************************************************/
  /*                                                                       */
  /*               Define and Initialize Local Variables                   */
  /*                                                                       */
  /*************************************************************************/

  char  logmsg[PGS_SMF_MAX_MSG_SIZE];         /* string containing complete   */
  char  code_msg[PGS_SMF_MAX_MSG_SIZE];       /* message string associated
                                                 with a specific status code  */
  char  concat_msg[PGS_SMF_MAX_MSGBUF_SIZE];  /* concatenated message string  */
  va_list  args;                              /* varible length argument list */


  /*************************************************************************/
  /*                                                                       */
  /*  Initialize the value of the variable argument list pointer           */
  /*                                                                       */
  /*************************************************************************/

  va_start(args, msg_fmt);


  /*************************************************************************/
  /*                                                                       */
  /*  CALL PGS_SMF_GetMsgByCode to get the message associated with the     */
  /*     code                                                              */
  /*    INPUT:  code                                                       */
  /*    OUTPUT: code_msg                                                   */
  /*    RETURN: PGS_status                                                 */
  /*                                                                       */
  /*************************************************************************/

  PGS_SMF_GetMsgByCode(code, code_msg);


  /*************************************************************************/
  /*                                                                       */
  /*  Put the contents of msg_fmt and args in logmsg                       */
  /*                                                                       */
  /*************************************************************************/

  vsprintf(logmsg, msg_fmt, args);


  /*************************************************************************/
  /*                                                                       */
  /*  Concatenate logmsg and code_msg and put the result into concat_msg   */
  /*                                                                       */
  /*************************************************************************/

  sprintf(concat_msg, "%s: %s", code_msg, logmsg);


  /*************************************************************************/
  /*                                                                       */
  /*  CALL PGS_SMF_SetDynamicMsg to set dynamic message                    */
  /*    INPUT:  code, concat_msg, routine                                  */
  /*    OUTPUT: None                                                       */
  /*    RETURN: PGS_status                                                 */
  /*                                                                       */
  /*************************************************************************/

  PGS_SMF_SetDynamicMsg(code, concat_msg, (char *)routine);


  /*************************************************************************/
  /*                                                                       */
  /*  Terminate use of variable list argument pointer                      */
  /*                                                                       */
  /*************************************************************************/

  va_end(args);


  /*************************************************************************/
  /*                                                                       */
  /*  RETURN                                                               */
  /*                                                                       */
  /*************************************************************************/

  return;

}
