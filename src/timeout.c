/*****************************************************************************/
/*                                                                           */
/* File: timeout.c                                                           */
/*                                                                           */
/*****************************************************************************/

#include "cf.defs.h"
#include "cf.extern.h"

/*************************************************************************/
  
void SetTimeOut(int timeout)
 
{
ALARM_PID = -1;
signal(SIGALRM,(void *)TimeOut);
alarm(timeout);
}

/*************************************************************************/
  
void TimeOut()
 
{
alarm(0);

if (ALARM_PID != -1)
   {
   Verbose("%s: Time out of process %d\n",VPREFIX,ALARM_PID);
   kill(ALARM_PID,cfterm);
   kill(ALARM_PID,cfkill);
   }
else
   {
   Verbose("%s: Time out\n",VPREFIX);
   }
}

/*************************************************************************/

void DeleteTimeOut()

{
}
