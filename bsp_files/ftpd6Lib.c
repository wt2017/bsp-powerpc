/* ftpd6Lib.c - File Transfer Protocol (FTP) Server library */
/*
 * Copyright (c) 2001-2006 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
/*
 * Copyright (c) 1985,1989,1993,1994 The Regents of the University of
 * California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
modification history
--------------------
02x,08mar06,mwv  st_size is now a INT64 for display (SPR#118569) 
02w,03jan06,dlk  Treat ECONNABORTED/ENOBUFS errnos from accept() as non-fatal.
02v,02jan05,dlk  Decrease stack usage in directory listing code path
		 (SPR #116334).
02u,02jan05,dlk  Add ftpd6CwdStat variable and '-c' option to allow disabling
                 cwd directory check (SPR #114250).
02t,11oct05,vvv  renamed globals to avoid clashes with other modules (SPR
                 #110674)
02s,06oct05,rp   create directory with rwx permissions for all SPR 113341
02r,26sep05,rp   fixed issues found by static analysis tool
02q,25sep05,vvv  renamed debug to avoid conflict with other globals (SPR
                 #112767)
02p,13sep05,vvv  added delay to allow IPv6 address to initialize before
                 bringing up server
02o,22aug05,pad  Added '0' as mode for mkdir() call when compiled for RTP.
02n,18may05,rp   allow cwd to root directory SPR#109402
02m,05may05,nee  updated copyright information
                 removed redundant header inclusions
02l,15apr05,rp   fixed coverity bugs
02k,13apr05,rp   handle wildcard in pathnames (SPR #103337)
02j,06apr05,vvv  completed fix made in 02h (fixed for both NLST and LIST)
02i,30mar05,vvv  fixed docs (SPR #107187)
02h,29mar05,vvv  fixed directory listing for "/" (SPR #106817)
02g,25mar05,rp   cd only to a directory
02g,25feb05,niq  use reentrant getopt_r
02f,15feb05,vvv  fixed warnings
02e,16feb05,aeg  added various headers (SPR #106381).
02d,27jan05,dlk  Fix formatting of file length, avoid "%qd".
                 In send_data(), default blksize to BUFSIZ.
02c,26jan05,vvv  added backward-compatibility API for ftpdLib
02b,21jan05,vvv  osdep.h cleanup
02a,18jan05,syy  Enable the server to be v4-, v6-only or v4/v6 dual
01z,03jan05,syy  Enable build for v4-only also. Port to RTP.
01y,28dec04,syy  Moved macros related to path library to ftpd6Ext.h
01x,06dec04,syy  Fixed SPR#104035: add support for guest login; also reduced
                 task name length.
01w,23nov04,syy  Fix to SPR#104698: fixed directory validation in "cwd"
01v,08nov04,syy  Doc changes
01u,27oct04,vvv  fixed docs (SPR #102813)
01t,30aug04,dlk  Replace LOG_ERROR() with log_err(), etc..
01s,17sep04,vvv  fixed references to rt11 for VxWorks 6.x
01r,14sep04,niq  virtual stack related backwards compatibility changes
01q,24aug04,ram  SPR#87288 include copyright header file
01p,23aug04,vvv  merged from COMP_WN_IPV6_BASE6_ITER5_TO_UNIFIED_PRE_MERGE
01o,03aug04,kc   Fixed virtualization issue for ftp server init.
01n,08jul04,vvv  fixed warnings
01m,22apr04,asr  Fix compile warnings for PENTIUM2/gnu
01l,19apr04,asr  changed to use wdStart instead of wdSemStart
01k,17mar04,spm  merged from Orion for MSP 3.0 baseline
01j,20nov03,niq  osdep.h cleanup
01h,06nov03,rlm  Ran batch header update for header re-org.
01g,12apr03,ijm  fixed diab compiler warnings
01f,06aug02,ant  coding conventions, xxxxnames[] fixed, lostconn() deleted
01e,18jul02,ant  changed typo of the task name "tFtpd6Task"
01d,06jun02,ant  function renamefrom changed
01c,08may02,ppp  Merging the teamf1 T202 changes into clarinet
01b,23apr02,ant  changes in functions send_file_list and ftpd6DirListGet
01a,14sep01,ant  written + port from FreeBSD
*/

/*
DESCRIPTION
This library implements the server side of the File Transfer Protocol (FTP),
which provides remote access to the file systems available on a target.
The protocol is defined in RFC 959. This implementation supports all commands
required by that specification, as well as several additional commands(RFC 1639,
RFC 2428).

USER INTERFACE
During system startup, the ftpd6Init() routine creates a control connection
at the predefined FTP server port which is monitored by the primary FTP
task. Each FTP session established is handled by a secondary server task
created as necessary. The server accepts the following commands:

 USER, PASS, QUIT, PORT, PASV, TYPE, STRU, MODE, RETR, STOR, APPE,
 REST, RNFR, RNTO, DELE, CWD, LIST, NLST, SITE, HELP, NOOP, STAT, 
 MKD, RMD, PWD, CDUP, STOU, SYST, SIZE, MDTM, LPRT, LPSV, EPRT,
 EPSV, IDLE, XCWD, XPWD, XMKD, XRMD, XCUP. 

The ftpd6Delete() routine will disable the FTP server until restarted. 
It reclaims all system resources used by the server tasks and cleanly 
terminates all active sessions.

To use this feature, include the following component:
INCLUDE_FTP6_SERVER

This library is RTP-ready and can be compiled for IPv4-only, as well as for
IPv4/v6 dual stacks. When used for IPv4-only stack, FTPD6_OPTIONS_STRING must
be re-defined to use "-4", as the default value is set to "-6". When used for
RTP, make sure INCLUDE_POSIX_TIMER is defined in the kernel, as it is required
to support POSIX timers used by this library.

In case of VIRTUAL_STACK the FTP6 server service is not immediately
initialized during system start up. The INCLUDE_FTP6_SERVER only announces the 
server to the virtual stack manager as disabled. To enable the server in 
one stack instance vsComponentStatusSet and vsInitRtnsCall need to be called 
for this instance. 
The server can only be started in one virtual stack.

INCLUDE FILES
ftpd6Lib.h
*/

/* includes */

#include <vxWorks.h>
#include <fioLib.h>
#include <stdlib.h>
#include <selectLib.h>
#include "ftpd6Ext.h"
#include <ftpd6Lib.h>
#include <unistd.h>
#include <ctype.h>

#ifdef _WRS_KERNEL
#include <private/iosLibP.h>
#include <pathLib.h>
#else
#include <strings.h>
#include <pathUtilLib.h>
#endif /* _WRS_KERNEL */

#ifdef VIRTUAL_STACK
#include <netinet/vsLib.h> 
#include <netinet/vsData.h> 
#endif /* VIRTUAL_STACK */

#ifdef _WRS_VXWORKS_5_X
#ifdef __cplusplus

inline int &__errnoRef()
    {
    return (*__errno());
    }

#define errno   __errnoRef()

#else

#define errno    (*__errno())

#endif  /* __cplusplus */
#endif /* _WRS_VXWORKS_5_X */

/* defines */

#define FTPD6_MAXARGS       50
#define FTPD6_MAXCMDLEN     512 

/* wrapper for KAME-special getnameinfo() */
#ifndef   NI_WITHSCOPEID
#define   NI_WITHSCOPEID    0
#endif
#define   MAXGLOBARGS       16384

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */

#define    SWAITMAX                   90    /* wait at most 90 seconds */
#define    SWAITINT                    5    /* interval between retries */
#define    FTP_DATA_PORT              20
#define    FTPD_WORK_TASK_NAME_LEN    40

#define LOGCMD(cdir, cmd, file) \
  if (ftpd6Logging > 1) \
    {if (file == NULL) {printf("%s %s\n",cmd, cdir);} \
     else {printf ("%s %s%s\n", cmd, *(file) == '/' ? "" : cdir, file);} }

#define LOGCMD2(cdir, cmd, file1, file2) \
  if (ftpd6Logging > 1) \
  {printf ("%s %s%s %s%s\n", cmd, *(file1) == '/' ? "" : cdir, file1, *(file2) == '/' ? "" : cdir, file2);}
#define LOGBYTES(cdir, cmd, file, cnt) \
  if (ftpd6Logging > 1) \
    {if (cnt == (off_t)-1) {printf ("%s %s%s\n", cmd, *(file) == '/' ? "" : cdir, file);} \
     else {printf ("%s %s%s = %lu bytes\n", cmd, (*(file) == '/') ? "" : cdir, file, cnt);} }

/* globals */

int   ftpd6Debug = 0;           /* debug flag */
int   ftpd6Logging  = 0;             /* extra logging flag */
int   ftpd6Paranoid = 1;             /* be extra careful about security */
int   ftpd6Readonly = 0;             /* Server is in readonly mode.*/
int   ftpd6Noepsv   = 0;             /* EPSV command is disabled.*/
int   ftpd6MaxTimeout = 7200;        /* don't allow idle time to be set beyond
					2 hours */
BOOL  ftpd6CwdStat = TRUE;	/* cwd() does check that path is directory */

BOOL  ftpd6SecurityEnabled = FALSE;   /* security disabled by default */

char *ftpd6hostname;            /* symbolic name of this machine */

#ifdef VIRTUAL_STACK
/* define variables to be set to values from cdf params in the configlette*/
char   *ftpd6optionsstring   = "-6";
FUNCPTR ftpd6loginrtn        = NULL;
int     ftpd6ssize           = 0;

VS_REG_ID ftpd6RegistrationNum = 0;

/* variable indicating if Ftp6 server is already started; later set to vsnum */
LOCAL int ftpd6vsnum         = -1;

#endif /*VIRTUAL_STACK*/

char *typenames[] =
    {
    "",
    "ascii",
    "ebcdic",
    "image", 
    "tenex",
    };

char *formnames[] =
    {
    "",
    "non-print",
    "telnet format controls", 
    "carriage control (ASA)",
    };

char *strunames[] =
    {
    "",
    "file",
    "record structure", 
    "page structure",
    };

char *modenames[] =
    {
    "",
    "stream",
    "block", 
    "compressed",
    };

/* forward declarations */
FUNCPTR ftpd6LoginVerifyRtn;           /* function which check user password */
int     ftps6CurrentClients;           /* number of current Client sessions */ 
int     ftpd6TaskPriority         = 56;
int     ftpd6WorkTaskStackSize    = 12000;
int     ftpd6TimeoutTaskStackSize = 12000;
int     ctl_sock;                      /* socket descriptor of the server */
int     ftpd6WorkTaskPriority     = 252;
int     ftps6MaxClients           = 4; /* max number of sessions */

#ifdef _WRS_KERNEL
int 	ftpd6TaskOptions	= VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
int 	ftpd6WorkTaskOptions	= VX_SUPERVISOR_MODE | VX_UNBREAKABLE; 
#undef main
#else
int 	ftpd6TaskOptions	= 0;
int 	ftpd6WorkTaskOptions	= 0; 
#endif /* _WRS_KERNEL */

/* externals */
extern int     vsnprintf (char *, size_t, const char *, va_list);

/* locals */

LOCAL BOOL     ftps6Active = FALSE;   /* Server started? */
LOCAL BOOL     ftps6ShutdownFlag;     /* Server halt requested? */
LOCAL LIST     ftps6SessionList;
LOCAL SEM_ID   ftps6MutexSem;
LOCAL SEM_ID   ftps6SignalSem;
LOCAL int      ftpd6TaskId     = -1;
LOCAL int      ftpd6NumTasks;
LOCAL char     ftpd6WorkTaskName [FTPD_WORK_TASK_NAME_LEN];
LOCAL int      swaitmax = SWAITMAX;      /* above */
LOCAL int      swaitint = SWAITINT;      /* above */
LOCAL union    sockunion server_addr;    /* server address */
LOCAL int      restricted_data_ports = 1;
LOCAL int      timeout  = 900;      /* timeout after 15 minutes of inactivity */


/* forward declarations */

static FILE *    dataconn (FTPD6_SESSION_DATA *, char *, off_t, char *);
static FILE *    getdatasock (FTPD6_SESSION_DATA *, char *);
static char *    gunique (FTPD6_SESSION_DATA *, char *);
static int       receive_data (FTPD6_SESSION_DATA *, FILE *, FILE *);
static void      send_data (FTPD6_SESSION_DATA *, FILE *, FILE *,
                            off_t, off_t, int);

#ifndef VIRTUAL_STACK
LOCAL void     ftpd6Task (int, char *, int);
LOCAL STATUS   ftpd6WorkTask (FTPD6_SESSION_DATA *);
#else
LOCAL void     ftpd6Task (int, char *, int, int stacknum);
LOCAL STATUS   ftpd6WorkTask (FTPD6_SESSION_DATA *, int stacknum);
#endif /*VIRTUAL_STACK*/

LOCAL BOOL ftpDirListPattern (char * pat, char * name);
LOCAL FTPD6_SESSION_DATA *ftpd6SessionAdd (void);
LOCAL void     ftpd6SessionDelete (FTPD6_SESSION_DATA *);
LOCAL void     ftpd6SockFree (int *);

#ifdef VIRTUAL_STACK
/*******************************************************************************
*
* ftpd6InstInit - initialize the ftp6 server
*
* This routine is a wrapper function for ftpd6Init. It is only called in case
* of virtual stack. In that case ftpd6Init can not be called directly from 
* the configlette as it is not clear for which virtual stack it should be
* called. Instead this function is called from the vsManager.
* It is acting exactly as the configlette for nonvirtual stack.  
*
* RETURNS: OK, or ERROR if initialization fails
*
* \NOMANUAL
*/

STATUS ftpd6InstInit
    (
    void *noparams    /* default config params */
    )
    {
    return ftpd6Init (ftpd6optionsstring, ftpd6loginrtn, ftpd6ssize);
    }

/*******************************************************************************
*
* ftpd6Destructor - destructor for ftp6 server 
*
* This routine is only called to reset the ftpdvsnum to -1 in case the virtual 
* stack in which ftp6 server  was running is deleted. All resources are freed in
* response to an error at the socket.
* It should not be called as it does not make much sense to delete the
* management stack.
*
* RETURNS: OK, or ERROR if initialization fails
*
* \NOMANUAL
*/

STATUS ftpd6Destructor
    (
    VSNUM vsnum        /* stack number of the instance where ftpd6 runs */
    )
    {
    if (ftpd6vsnum == vsnum)
       {
       /* 
        * Don't call ftpd6Delete() when the whole stack instance, where ftp6
        * server is running, goes down. ftpd6Delete() is called directly in 
        * ftpd6Task() in that case. Call it when only ftp6 server application 
        * goes down (e.g. vssglde() - single destructor call). 
        */
       if (vsStatusGet() == VS_RUNNING)
           ftpd6Delete();
       
       ftpd6vsnum = -1;  /* this allows the start of ftp server in a new vs*/
       }
    
    return (OK);
    /* else do nothing, it is another virtual stack instance */
    }


#endif /*VIRTUAL_STACK*/

#ifndef _WRS_KERNEL
/*******************************************************************************
*
* ftpd6TimerHandler - Handler required by POSIX timer.
*
* ERRNO:
*
*   EINVAL: semId given is NULL;
*   others: semGive failed.
*
* \NOMANUAL
*
*/
void ftpd6TimerHandler
    (
    timer_t   timerid,	  /* expired timer ID */
    int       semId       /* user argument    */
    )
    {
    if (semId != NULL)
        semGive ((SEM_ID) semId);
    else
        errnoSet (EINVAL);
    }

#endif /* !_WRS_KERNEL */

/*******************************************************************************
*
* ftpd6Init - initialize the FTP server task
*
* This routine installs the password verification routine indicated by
* <pLoginRtn> and establishes a control connection for the primary FTP
* server task, which it then creates. It is called automatically during
* system startup if INCLUDE_FTP6_SERVER is defined. The primary server task 
* supports simultaneous client sessions, up to the limit specified by the 
* global variable 'ftps6MaxClients'. The default value allows a maximum of 
* four simultaneous connections. The <stackSize> argument specifies the stack 
* size for the primary server task. It is set to the value specified in the 
* 'ftpd6WorkTaskStackSize' global variable by default. The <ftpdoptions> 
* argument specifies the options of the Server as follows:
*
* \is
* \i '-d' 
* Enables debugging (messages to the system message logger). 
* \i '-E'
* Disables Extended Passive Mode (EPSV). 
* \i '-l'
* Extra logging (messages to the system message logger) 
* \i '-r'
* Read only mode. It is allowed only to read from Server.
* The following commands are disabled: 'STOR', 'APPE', 'DELE', 'RNTO', 'MKD', 
* 'RMD', 'SITE', 'CHMOD', 'STOU', and 'RNFR'. 
* \i '-R'
* Turn off extra careful about security. It checks a value of an 
* address and a port number sent with command PORT. Where:
* \cs
* data_dest.su_port < IPPORT_RESERVED
* data_dest.su_sin.sin_addr == his_addr.su_sin.sin_addr  
* \ce
* The sent address must be the same as an address of a control 
* connection address.
* \i '-T' <maxtimeout>
* Default maxtimeout = 7200s. Don't allow idle time to be set 
* beyond 2 hours (command IDLE).
* \i '-t' <timeout>
* Default timeout = 900s. Timeout after 15 minutes of 
* inactivity, file ftpd6Parse.c.
* \i '-U'
* Restricted data port sets the option IP_PORTRANGE to IPV6_PORTRANGE_DEFAULT.
* This is done while creating a data connection with a Client, passive mode. 
* Default value is IP_PORTRANGE_HIGH.  
* \i '-a' <bindname>
* Sets the local IP address of the Server to <bindname>, otherwise one is 
* automatically chosen.
* \i '-4'
* Enable version 4.
* \i '-6'
* Enable version 6.
*
* If no parameter 4 or 6 specified, version 4 addresses are checked first and
* then version 6 addresses. If IPv6 support is not built in, the server will
* only enable version 4 in this case.
* \i '-c <checkDirFlag>'
* Disable (<checkDirFlag> = 0) or enable (<checkDirFlag> != 0) directory
* checking. By default, the FTP server checks that the path specified
* in CWD or XCWD command exists and is a directory (as reported by stat()).
* In VxWorks, some paths which appear to be directories, are not in fact
* directories. For instance, if one has created a memDrv device named
* "/test/snark", one may successfully open "/test/snark/0" as a file; however
* neither "/test/snark" nor "/test" is a directory.  Nevertheless, it may
* be convenient for the FTP client's user to cd to "/test/snark" and then
* get "0". To allow this and similar cases, specify a zero argument to -c.
* \ie
* 
* RETURNS:
* OK if server started, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS ftpd6Init
    (
    char *    ftpdoptions,    /* options: debug mode, logging, etc. */
    FUNCPTR   pLoginRtn,      /* user verification routine, or NULL */
    int       stackSize       /* task stack size, or 0 for default  */
    )    
    {
    int     ch;    /* tos, addrlen;*/
    char *  bindname = NULL;
    int     family = AF_UNSPEC;
    int     enable_v4 = 1;
    int     argc;
    char *  tempArgv [FTPD6_MAXARGS + 2];
    char ** argv = tempArgv;
    GETOPT  getoptState;

    int     ftpd6StackNum = 0;

#ifdef VIRTUAL_STACK
    if ((ftpd6vsnum != -1) && (ftpd6vsnum != myStackNum))
        {
        errnoSet (EALREADY);
        return (ERROR);
        }

    /* Save stack number to prevent startup in other virtual stacks. */
    ftpd6StackNum = ftpd6vsnum = myStackNum;

#endif /* VIRTUAL_STACK */

    if (ftps6Active)
        return (OK);

    /* Initialize the globals. Needed for when restarting the server. */
    ftpd6Debug = 0;        /* debug flag */
    ftpd6Logging   = 0;        /* extra logging flag */
    restricted_data_ports = 1;
    ftpd6Paranoid  = 1;        /* be extra careful about security */
    ftpd6Readonly  = 0;        /* Server is in readonly mode. */
    ftpd6Noepsv    = 0;        /* EPSV command is disabled. */
    timeout   = 900;      /* timeout after 15 minutes of inactivity */
    ftpd6MaxTimeout = 7200;  /* don't allow idle time to be set beyond 2 hours */

    ftpd6LoginVerifyRtn = pLoginRtn; 

    ftps6ShutdownFlag = FALSE;
    ftps6CurrentClients = 0;

    if (getOptServ (ftpdoptions, "ftpd6Init",  &argc, argv,
            FTPD6_MAXARGS + 2) != OK)
    {
    log_err (FTPD_LOG, "too many arguments.");
    goto ftpd6Init_err;
    }

    getoptInit (&getoptState);

    /* process options */

    while ((ch = getopt_r(argc, argv, "dlEURrt:T:va:46c:",
			  &getoptState)) != -1) 
        {
        switch (ch)
            {
            case 'd':
                ftpd6Debug++;
                break;

            case 'E':
                ftpd6Noepsv = 1;
                break;

            case 'l':
                ftpd6Logging++;    /* > 1 == extra logging */
                break;

            case 'r':
                ftpd6Readonly = 1;
                break;

            case 'R':
                ftpd6Paranoid = 0;
                break;

            case 'T':
                ftpd6MaxTimeout = atoi(getoptState.optarg);
                if (timeout > ftpd6MaxTimeout)
                    timeout = ftpd6MaxTimeout;
                break;

            case 't':
                timeout = atoi(getoptState.optarg);
                if (ftpd6MaxTimeout < timeout)
                    ftpd6MaxTimeout = timeout;
                break;

            case 'U':
                restricted_data_ports = 0;
                break;
                
            case 'a':
                bindname = getoptState.optarg;
                break;

            case 'v':
                ftpd6Debug = 1;
                break;
                
            case '4':
                enable_v4 = 1;
                if (family == AF_UNSPEC)
                    family = AF_INET;
                break;
#ifdef INET6
            case '6':
                enable_v4 = 0;                
                family = AF_INET6;
                break;
#endif
	    case 'c':
		ftpd6CwdStat = (atoi (getoptState.optarg) != 0);
		break;

            default:
                log_warning (FTPD_LOG, "unknown flag -%c ignored",
                             getoptState.optopt);
                break;
            }     /*switch*/
        }     /*while*/

    /* Create data structures for managing client connections. */

    lstInit (&ftps6SessionList);
    ftps6MutexSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (ftps6MutexSem == NULL)
        goto ftpd6Init_err;

    ftps6SignalSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);    
    if (ftps6SignalSem == NULL)     
        goto ftpd6Init_err1;

    /* Create a FTP server task to receive client requests. */

    ftpd6TaskId = taskSpawn("tFtp6d", ftpd6TaskPriority, ftpd6TaskOptions,
                            stackSize == 0 ? ftpd6WorkTaskStackSize : stackSize,
                            (FUNCPTR) ftpd6Task, family, (int) bindname,
			    enable_v4, ftpd6StackNum, 0, 0, 0, 0, 0, 0);

    if (ftpd6TaskId == ERROR) 
        {
        log_err (FTPD_LOG, "ERROR: ftpd6Task cannot be created");
        free (ftpd6hostname);
        goto ftpd6Init_err2;
        }

    ftps6Active = TRUE;
    log_info (FTPD_LOG, "ftpd6Task created");  
    return (OK);

ftpd6Init_err2:
    semDelete (ftps6SignalSem);
ftpd6Init_err1:
    semDelete (ftps6MutexSem);
ftpd6Init_err:
    return (ERROR);
    }    

/*******************************************************************************
*
* ftpd6Delete - terminate the FTP server task
*
* This routine halts the FTP server and closes the control connection. All
* client sessions are removed after completing any commands in progress.
* When this routine executes, no further client connections will be accepted
* until the server is restarted. This routine is not reentrant and must not
* be called from interrupt level.
*
* NOTE: If any file transfer operations are in progress when this routine is
* executed, the transfers will be aborted, possibly leaving incomplete files
* on the destination host.
*
* RETURNS: OK if shutdown completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* INTERNAL
* This routine is synchronized with the deletion routine for a client session
* so that the exit of the client tasks can be detected. It also shares a
* critical section with the creation of client sessions to prevent orphaned
* tasks, which would occur if a session were added after this routine had
* shut down all known clients.
*/

STATUS ftpd6Delete (void)
    {
    BOOL serverActive = FALSE;
    FTPD6_SESSION_DATA *pData;

    if (! ftps6Active)    /* Automatic success if server is not running. */
        return (OK);

    /*
     * Remove the FTP server task to prevent additional sessions from starting.
     * The exclusion semaphore guarantees a stable list of active clients.
     */
    semTake (ftps6MutexSem, WAIT_FOREVER); 

    /* 
     * Do not delete the tFtp6d if ftpd6Delete() has been called from that 
     * task.
     */
    
    if (taskIdSelf() != ftpd6TaskId)
        {
        taskDelete (ftpd6TaskId);
        ftpd6TaskId = -1;
        }

    if (ftps6CurrentClients != 0)
        serverActive = TRUE;

    /*
     * Set the shutdown flag so that any secondary server tasks will exit
     * as soon as possible. Once the FTP server has started, this routine is
     * the only writer of the flag and the secondary tasks are the only
     * readers. To avoid unnecessary blocking, the secondary tasks do not
     * guard access to this flag when checking for a pending shutdown during
     * normal processing. Those tasks do protect access to this flag during
     * their cleanup routine to prevent a race condition which would result
     * in incorrect use of the signalling semaphore.
     */

    ftps6ShutdownFlag = TRUE;

    /* 
     * Close the command sockets of any active sessions to prevent further 
     * activity. If the session is waiting for a command from a socket,
     * the close will trigger the session exit.
     */
 
    pData = (FTPD6_SESSION_DATA *)lstFirst (&ftps6SessionList);
    while (pData != NULL)
        {
        ftpd6SockFree (&pData->cmdSock);
        pData = (FTPD6_SESSION_DATA *)lstNext (&pData->node);
        }

    semGive (ftps6MutexSem);   

    /* Wait for all secondary tasks to exit. */

    if (serverActive)
        {
        /*
         * When a shutdown is in progress, the cleanup routine of the last
         * client task to exit gives the signalling semaphore.
         */

        semTake (ftps6SignalSem, WAIT_FOREVER);
        }

    /*
     * Remove the original socket - this occurs after all secondary tasks
     * have exited to avoid prematurely closing their control sockets.
     */
    
    ftpd6SockFree (&ctl_sock);

    /* Remove the protection and signalling semaphores and list of clients. */

    lstFree (&ftps6SessionList);    /* Sanity check - should already be empty. */

    semDelete (ftps6MutexSem);

    semDelete (ftps6SignalSem);

    /* Set security to the default - disabled */
    ftpd6SecurityEnabled = FALSE;

    guestPathReset ();

    free (ftpd6hostname);    
    
    ftps6Active = FALSE;
    
#ifdef VIRTUAL_STACK
    ftpd6vsnum = -1;  /* this allows the start of ftp server in a new vs*/
#endif /* VIRTUAL_STACK */

    return (OK);
    }

/*******************************************************************************
*
* ftpd6Task - FTPv6 server daemon task
*
* This routine monitors the FTP control port for incoming requests from clients
* and processes each request by spawning a secondary server task after 
* establishing the control connection. If the maximum number of connections is
* reached, it returns the appropriate error to the requesting client. The 
* routine is the entry point for the primary FTPv6 server task and should only
* be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL:
* The server task is deleted by the server shutdown routine. Adding a newly
* created client session to the list of active clients is performed atomically
* with respect to the shutdown routine. However, accepting control connections
* is not a critical section, since closing the initial socket used in the
* listen() call also closes any later connections which are still open.
*
* \NOMANUAL
*/

#ifndef VIRTUAL_STACK
LOCAL void ftpd6Task
    (
    int    family,
    char * bindname,
    int    enable_v4
    )
#else
LOCAL void ftpd6Task     
    (
    int    family,
    char * bindname,
    int    enable_v4,
    int    stackNum    /* stack number of the instance where ftpd6 runs */
    ) 
#endif /* VIRTUAL_STACK */
    {
    int addrlen, newSock;
    FAST FTPD6_SESSION_DATA *pSlot;
    union sockunion         addr;
    int ftpd6StackNum = 0;
    struct addrinfo hints, *res;
    int error, on = 1;

#ifdef VIRTUAL_STACK
    ftpd6StackNum = stackNum; 
    
    if (vsMyStackNumSet (stackNum) == ERROR)
        {
        log_err (FTPD_LOG, "vsMyStackNumSet failed");
        return;
        }
#endif /* VIRTUAL_STACK */    

#ifdef INET6
#ifdef _WRS_KERNEL
    /*
     * The following 2 second delay has been added temporarily to give
     * enough time for DAD to complete on the IPv6 address on the boot 
     * interface during boot.
     *
     * The delay is required only when v6 is included and it is required
     * in the kernel only.
     */

    taskDelay (2 * sysClkRateGet());
#endif
#endif

    /* Setup control connection for client requests. */

    memset (&hints, 0, sizeof (hints));

#ifndef INET6
    hints.ai_family = family == AF_UNSPEC ? AF_INET : family;
#else    
    hints.ai_family = family == AF_UNSPEC ? AF_INET6 : family;    
#endif    
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    
    error = getaddrinfo(bindname, "ftp", &hints, &res);
    if (error) 
        {
        if (family == AF_UNSPEC) 
            {
            hints.ai_family = AF_UNSPEC;
            error = getaddrinfo(bindname, "ftp", &hints, &res);
            }
        }
    if (error) 
        {
        log_err (FTPD_LOG, "%s", gai_strerror(error));
        if (error == EAI_SYSTEM)
            log_err (FTPD_LOG | LOG_ERRNO, ""); 
        goto ftpd6Task_err;
        }
    
    if (res->ai_addr == NULL) 
        {
        log_err (FTPD_LOG, "getaddrinfo failed");
        freeaddrinfo (res);
        goto ftpd6Task_err;
        } 
    else
        family = res->ai_addr->sa_family;
    
    /* Open a socket, bind it to the FTP port, and start listening. */
     
    ctl_sock = socket(family, SOCK_STREAM, 0);
    if (ctl_sock < 0) 
        {
        log_err (FTPD_LOG | LOG_ERRNO, "control socket");
        freeaddrinfo(res);
        goto ftpd6Task_err;
        }
    
    if (setsockopt(ctl_sock, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&on, sizeof(on)) < 0)
        log_err (FTPD_LOG | LOG_ERRNO, "control setsockopt");
#ifdef IPV6_V6ONLY
    if (family == AF_INET6 && enable_v4 == 0) 
        {
        if (setsockopt(ctl_sock, IPPROTO_IPV6, IPV6_V6ONLY,
                       (char *)&on, sizeof (on)) < 0)
            log_err (FTPD_LOG | LOG_ERRNO,
                     "control setsockopt(IPV6_V6ONLY)");
        }
#endif /* IPV6_V6ONLY */

    memcpy(&server_addr, res->ai_addr, res->ai_addr->sa_len);
    freeaddrinfo(res);

    if (family == AF_INET) 
        server_addr.su_len = sizeof(struct sockaddr_in);
#ifdef INET6    
    else if (family == AF_INET6)
        server_addr.su_len = sizeof(struct sockaddr_in6);
#endif /* INET6 */
    
    if (bind(ctl_sock, (struct sockaddr *)&server_addr, server_addr.su_len) < 0) 
        {
        log_err (FTPD_LOG | LOG_ERRNO, "control bind");
        goto ftpd6Task_err1;
        }
    
    if (listen(ctl_sock, 32) < 0) 
        {
        log_err (FTPD_LOG | LOG_ERRNO, "control listen");
        goto ftpd6Task_err1;
        }

    if ((ftpd6hostname = (char *)malloc(MAXHOSTNAMELEN)) == NULL) 
        {
        log_err (FTPD_LOG, "ERROR: Unable to allocate memory.");
        goto ftpd6Task_err1;
        }
    
    (void) gethostname(ftpd6hostname, MAXHOSTNAMELEN - 1);
    ftpd6hostname [MAXHOSTNAMELEN - 1] = EOS;
    ftpd6NumTasks = 0;
    
    /* The following loop halts if this task is deleted. */

    FOREVER
        {
	/* Wait for a new incoming connection. */
    
        addrlen = server_addr.su_len;
    
        newSock = accept(ctl_sock, (struct sockaddr *)&addr, &addrlen);
        if (newSock < 0)
            {
	    /*
	     * ECONNABORTED is returned if the client resets
	     * the completed connection before accept() returns it.
	     * ENOBUFS might also be returned in some cases; we
	     * treat this as non-fatal also. In either case,
	     * the resources associated with the client connection
	     * will be cleaned up by the lower-level code.
	     */
	    if (errno == ECONNABORTED || errno == ENOBUFS)
		continue;

            log_err (FTPD_LOG | LOG_ERRNO, "in accept()");
        
            /* terminate the FTPv6 server task */
        
            ftpd6Delete(); 
            break;
            }

        /*
         * Register a new FTP client session. This process is a critical
         * section with the server shutdown routine. If an error occurs,
         * the reply must be sent over the control connection to the peer
         * before the semaphore is released. Otherwise, this task could
         * be deleted and no response would be sent, possibly causing
         * the new client to hang indefinitely.
         */

        semTake (ftps6MutexSem, WAIT_FOREVER);
    
        /* SO_KEEPALIVE ?? */

        log_info (FTPD_LOG, "accepted a new client connection");

        /* Create a new session entry for this connection, if possible. */

        pSlot = ftpd6SessionAdd ();
        if (pSlot == NULL)     /* Maximum number of connections reached. */
            {
            /* Send transient failure report to client. */

            replyNopSlot(newSock, 421,
			 "Session limit reached, closing control connection.");
            log_err (FTPD_LOG, "cannot get a new connection slot");
            close (newSock);
            semGive (ftps6MutexSem);
            continue;
            }

        pSlot->cmdSock = newSock;

        /* Save the control address and assign the default data address. */

        bcopy ( (char *)&addr, (char *)&pSlot->data_dest, sizeof(addr));
        bcopy ( (char *)&addr, (char *)&pSlot->his_addr, sizeof(addr));
        pSlot->data_dest.su_port = htons (FTP_DATA_PORT);

        /* Create a task name. */

        sprintf (ftpd6WorkTaskName, "tFtp6dT%d", ftpd6NumTasks);

        /* Spawn a secondary task to process FTP requests for this session. */

        log_info (FTPD_LOG, "creating a new server task %s...",
                  ftpd6WorkTaskName);

        if (taskSpawn (ftpd6WorkTaskName, ftpd6WorkTaskPriority,
                       ftpd6WorkTaskOptions, ftpd6WorkTaskStackSize,
                       ftpd6WorkTask, (int) pSlot,
                       ftpd6StackNum, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) 
            {
            /* Send transient failure report to client. */
         
            reply(pSlot, 421,  "Service not available, closing control connection.");
            ftpd6SessionDelete (pSlot);
            log_err (FTPD_LOG, "cannot create a new work task");
            semGive (ftps6MutexSem);
            continue;
            }

        log_info (FTPD_LOG, "done.");
    
        /* Session added - end of critical section with shutdown routine. */

        semGive (ftps6MutexSem);
        }     /*end FOREVER*/
    
    /* Fatal error - update state of server. */

    ftps6Active = FALSE;
    
    ftpd6TaskId = -1;

ftpd6Task_err1:
    ftpd6SockFree (&ctl_sock);
ftpd6Task_err:
    semDelete (ftps6SignalSem);
    semDelete (ftps6MutexSem);

    return;
    }         

/*******************************************************************************
*
* ftpd6WorkTask - main protocol processor for the FTPv6 service
*
* This function handles all the FTP protocol requests by calling ftpd6Parse
* function which parses the request string, performs appropriate actions
* and returns the result strings. The main body of this function is a FOREVER
* loop which reads in FTP request commands from the client located on the other
* side of the connection. This function also creates a FTP server timeout task
* which handles client's timeout. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL
* To handle multiple simultaneous connections, this routine and all secondary
* routines which process client commands must be re-entrant. If the server's
* halt routine is started, the shutdown flag is set, causing this routine to
* exit after completing any operation already in progress.
*
*
* \NOMANUAL
*/

#ifndef VIRTUAL_STACK
LOCAL STATUS ftpd6WorkTask
    (
    FTPD6_SESSION_DATA   *pSlot /* pointer to the active slot to be handled */
    )
#else
LOCAL STATUS ftpd6WorkTask
    (
    FTPD6_SESSION_DATA   *pSlot, /* pointer to the active slot to be handled */
    int stackNum        /* stack number of the instance where ftpd6 runs */
    )
#endif /* VIRTUAL_STACK */    
    {
    FAST int    sock;         /* command socket descriptor */
    FAST char * pBuf;         /* pointer to session specific buffer */
    FAST int    numRead;
    char      * upperCommand; /* convert command to uppercase */
    int         addrlen, on = 1, tos;
    int         error;
    char        ftpdTimeoutTaskName [FTPD_WORK_TASK_NAME_LEN];
    int         parseRes;
    int         nrecv = 0;
    int         ftpd6StackNum = 0;
#ifndef _WRS_KERNEL
    timer_t     timerId;
    struct itimerspec  timoValue;
#endif /* !_WRS_KERNEL */
    
#ifdef VIRTUAL_STACK
    ftpd6StackNum = stackNum; 
    
    if (vsMyStackNumSet (stackNum) == ERROR)
        {
        log_err (FTPD_LOG, "vsMyStackNumSet failed");
        return (ERROR);
        }
#endif /* VIRTUAL_STACK */   
    
    pBuf = &pSlot->buf [0];    /* use session specific buffer area */
    sock = pSlot->cmdSock;
 
    if (ftps6ShutdownFlag) 
        {
        /* Server halt in progress - send abort message to client. */

        reply(pSlot, 421,  "Service not available, closing control connection.");
        ftpd6SessionDelete (pSlot);
        return (OK);
        }

    addrlen = sizeof(pSlot->ctrl_addr);
    if (getsockname(sock, (struct sockaddr *)&pSlot->ctrl_addr, &addrlen) < 0) 
        {
        log_err (FTPD_LOG | LOG_ERRNO, "getsockname");
        return (ERROR);
       }

#ifdef IP_TOS
    if (pSlot->ctrl_addr.su_family == AF_INET) 
        {
        tos = IPTOS_LOWDELAY;
        if (setsockopt(sock, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int)) < 0)
            log_warning (FTPD_LOG | LOG_ERRNO, "setsockopt (IP_TOS)");
        }
#endif
    /*
     * Disable Nagle on the control channel so that we don't have to wait
     * for peer's ACK before issuing our next reply.
     */
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) < 0)
        log_warning (FTPD_LOG | LOG_ERRNO, "control setsockopt TCP_NODELAY");

    pSlot->data_source.su_port = htons(ntohs(pSlot->ctrl_addr.su_port) - 1);

    /* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
    if (setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on)) < 0)
        log_err (FTPD_LOG | LOG_ERRNO, "setsockopt");
#endif

    error = getnameinfo((struct sockaddr *)&pSlot->his_addr, 
                        pSlot->his_addr.su_len, pSlot->remotehost,
                        sizeof(pSlot->remotehost) - 1,
                        NULL, 0, NI_NUMERICHOST|NI_WITHSCOPEID);

    log_info (FTPD_LOG, "connection from %s", error == 0 ? pSlot->remotehost : "");

    /*
     * XXX - runtimeName/rumtimeVersion is not yet available in RTP. Once
     *       they are, the following #ifdef will be unnecessary.
     */
#ifdef _WRS_KERNEL    
    reply (pSlot, 220, "%s FTP server (%s %s) ready.", ftpd6hostname, 
    	   runtimeName, runtimeVersion);
#else
    reply (pSlot, 220, "%s FTP server ready.", ftpd6hostname);
#endif /* _WRS_KERNEL */
    
     /* Create data structures for managing timeout task. */

    pSlot->ftpsTimeoutSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pSlot->ftpsTimeoutSem == NULL) 
        {
        reply(pSlot, 421,  "Service not available, closing control connection.");
        ftpd6SessionDelete (pSlot);
        return (ERROR);
 	}
#ifdef _WRS_KERNEL
    pSlot->wdTimeout = wdCreate ();
    if(pSlot->wdTimeout == NULL)
#else
    if (timer_create (CLOCK_REALTIME, NULL, &timerId) == ERROR)
#endif /* _WRS_KERNEL */
        {
        reply(pSlot, 421,  "Service not available, closing control connection.");
        log_err (FTPD_LOG | LOG_ERRNO, "cannot create a new timer.");
        ftpd6SessionDelete (pSlot);
        return (ERROR); 
        }
    
#ifndef _WRS_KERNEL
    pSlot->timoTimer = timerId;

    if (timer_connect (pSlot->timoTimer, ftpd6TimerHandler, 
                       (int) pSlot->ftpsTimeoutSem) == ERROR)
        {
        reply(pSlot, 421,  "Service not available, closing control connection.");
        log_err (FTPD_LOG, "failed to connect timer for timo: %#x", errnoGet());
        ftpd6SessionDelete (pSlot);
        return (ERROR); 
        }
#endif /* !_WRS_KERNEL */    

     sprintf (ftpdTimeoutTaskName, "tFtpd6Tm%d", ftpd6NumTasks);

    /* Create a FTP server timeout task to handle client's timeout. */

    if ((pSlot->timeoutTask =
         taskSpawn(ftpdTimeoutTaskName, ftpd6WorkTaskPriority,
                   ftpd6WorkTaskOptions, ftpd6TimeoutTaskStackSize,
                   (FUNCPTR) toolong, (int) pSlot,
                   ftpd6StackNum, 0, 0, 0, 0, 0, 0, 0, 0)) == ERROR)  
        {
        /* Send transient failure report to client. */
          
        reply(pSlot, 421,  "Service not available, closing control connection.");
        log_err (FTPD_LOG, "cannot create a new Timeout task: %#x", errnoGet());
        ftpd6SessionDelete (pSlot);
        return (ERROR); 
        }

    FOREVER
        {
        taskDelay (1);        /* time share among same priority tasks */
    
        /* start or restart watchdog to activate timeout process */ 

#ifdef _WRS_KERNEL
	wdStart (pSlot->wdTimeout, pSlot->timeout*sysClkRateGet(), 
                    semGive, (int) pSlot->ftpsTimeoutSem);
#else
        /* set timer expiration value */
        timoValue.it_interval.tv_sec = 0;
        timoValue.it_interval.tv_nsec = 0;
        timoValue.it_value.tv_sec = pSlot->timeout;
        timoValue.it_value.tv_nsec = 0;

        if (timer_settime (pSlot->timoTimer, TIMER_RELTIME, &timoValue, NULL)
            == ERROR)
            {
            reply(pSlot, 421,  "Service not available, closing control connection.");
            log_err (FTPD_LOG, "failed to set timer for timo: %#x", errnoGet());
            ftpd6SessionDelete (pSlot);
            return (ERROR); 
            }
#endif /* _WRS_KERNEL */         
        
    /* Check error in writting to the control socket */

        if (pSlot->cmdSockError == ERROR)
            {
            ftpd6SessionDelete (pSlot);
            return (ERROR);
            }

        /*
         * Stop processing client requests if a server halt is in progress.
         * These tests of the shutdown flag are not protected with the
         * mutual exclusion semaphore to prevent unnecessary synchronization
         * between client sessions. Because the secondary tasks execute at
         * a lower priority than the primary task, the worst case delay
         * before ending this session after shutdown has started would only
         * allow a single additional command to be performed.
         *
         * ftps6ShutdownFlag flag can be set in ftpd6Delete() when the whole
         * server is shut down whereas pSlot->sessionShutDown flag can be set 
         * in dologout() when a particular secondary FTP server is shut down. 
         */

        if (ftps6ShutdownFlag || pSlot->sessionShutDown)
            break;

        /* get a request command */

        FOREVER    
            {
            taskDelay (1);    /* time share among same priority tasks */

            if ((numRead = read (sock, pBuf, 1)) <= 0)
                {
                /*
                 * The primary server task will close the control connection
                 * when a halt is in progress, causing an error on the socket.
                 * In this case, ignore the error and exit the command loop
                 * to send a termination message to the connected client.
                 */
                if (ftps6ShutdownFlag || pSlot->sessionShutDown)
                    {
                    *pBuf = EOS;
                    break;
                    }
                /* 
                 * Send a final message if the control socket 
                 * closed unexpectedly.
                 */
                if (numRead == 0)
                    reply(pSlot, 221,  "You could at least say goodbye.");

                ftpd6SessionDelete (pSlot);
                return (ERROR);
                }
        
            /* Skip the CR in the buffer. */
            if ( *pBuf == '\r' )
                continue;

            /* End Of Command delimiter. exit loop and process command */
            if ( *pBuf == '\n' ) 
                {
                *pBuf = EOS;
                nrecv = 0;
                break;
                }    
            /* 
             * Check the overflow of the receive buffer pSlot->buf. If the 
             * received line is too long, exceeds the size of the receive buffer,
             * read the characters further from the socket but do not increase
             * pBuf pointer (last characters are lost). 
             */        
            if ((pBuf - pSlot->buf) >= (sizeof (pSlot->buf) - 1))
                continue;        
            
            if (nrecv  < FTPD6_MAXCMDLEN)
                {
                pBuf++;        /* Advance to next character to read */
                nrecv++;
                }
            else 
                {
                memset(&pSlot->buf [0], 0, BUFSIZ);
                nrecv = 0;
                break;
                }
            }            /*end get a request command FOREVER*/
    
        /*  Reset Buffer Pointer before we use it */
        pBuf = &pSlot->buf [0];
        
        /* convert the command to upper-case */
        
        for (upperCommand = pBuf; (*upperCommand != ' ') &&
                 (*upperCommand != EOS); upperCommand++)
            *upperCommand = toupper ((int)*upperCommand);


        log_info (FTPD_LOG, "read command %s", pBuf);
        
        /*
         * Send an abort message to the client if a server
         * shutdown was started while reading the next command.
         */

        if (ftps6ShutdownFlag || pSlot->sessionShutDown) 
            {
            reply(pSlot, 421,  "Service not available, closing control connection.");
            ftpd6SessionDelete (pSlot);
            return (OK);
            }
        
        /* parse the request string and perform appropriate actions */    

        parseRes = ftpd6Parse(pSlot);
        if (parseRes !=0) 
            {
            ftpd6SessionDelete (pSlot);
            if (parseRes == 1)
                return (OK);
            else
                return (ERROR);
            }
        } /*end of first FOREVER*/
    
    /*
     * Processing halted due to pending server shutdown.
     * Remove all resources and exit.
     */

    ftpd6SessionDelete (pSlot);
    return (OK);
    }     

/*******************************************************************************
*
* ftpd6SessionAdd - add a new entry to the ftpd6 session slot list
*
* Each of the incoming FTP sessions is associated with an entry in the
* FTPv6 server's session list which records session-specific context for each
* control connection established by the FTP clients. This routine creates and
* initializes a new entry in the session list, unless the needed memory is not
* available or the upper limit for simultaneous connections is reached.
*
* RETURNS: A pointer to the session list entry, or NULL of none available.
*
* ERRNO: N/A
*
* INTERNAL
* This routine executes within a critical section of the primary FTP server
* task, so mutual exclusion is already present when adding entries to the
* client list and updating the shared variables indicating the current number
* of connected clients.
*
* \NOMANUAL
*/

LOCAL FTPD6_SESSION_DATA *ftpd6SessionAdd (void)
    {
    FAST FTPD6_SESSION_DATA     *pSlot;

    if (ftps6CurrentClients == ftps6MaxClients)
        return (NULL);

    /* get memory for the new session entry */

    pSlot = (FTPD6_SESSION_DATA *) FTPD6_MALLOC(sizeof (FTPD6_SESSION_DATA));

    if (pSlot == NULL)
        {
        printf("\r\n*******FTPD6_MALLOC error*******\r\n");
        return (NULL);
        }
    bzero ((char *)pSlot, sizeof(FTPD6_SESSION_DATA));

    /* initialize key fields in the newly acquired slot */

    pSlot->cmdSock = -1;
    pSlot->cmdSockError = OK;
    pSlot->byteCount = 0;
    pSlot->epsvall = 0; 
    pSlot->usedefault = 1;
    pSlot->pdata = -1;  
    pSlot->data = -1;
    pSlot->type = TYPE_A; 
    pSlot->form = FORM_N;
    pSlot->stru = STRU_F;
    pSlot->mode = MODE_S;
    pSlot->restart_point =  (off_t) 0;
    pSlot->timeout = timeout;   /* timeout after 15 minutes of inactivity*/ 
    pSlot->login_attempts = 0;    /* number of failed login attempts */
    pSlot->sessionShutDown = FALSE;
    
    /*
     * Assign the default directory for this guy. Maybe overwritten later if
     * it is a guest session.
     */
    
    strcpy (pSlot->curDirName, "/");

    /* Add new entry to the list of active sessions. */

    lstAdd (&ftps6SessionList, &pSlot->node);
    ftpd6NumTasks++;
    ftps6CurrentClients++;

    return (pSlot);
    }

/*******************************************************************************
*
* ftpd6SessionDelete - remove an entry from the FTPv6 session list
*
* This routine removes the session-specific context from the session list
* after the client exits or a fatal error occurs.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL
* Unless an error occurs, this routine is only called in response to a
* pending server shutdown, indicated by the shutdown flag. Even if the
* shutdown flag is not detected and this routine is called because of an
* error, the appropriate signal will still be sent to any pending shutdown
* routine. The shutdown routine will only return after any active client
* sessions have exited.
*
* \NOMANUAL
*/

LOCAL void ftpd6SessionDelete
    (
    FAST FTPD6_SESSION_DATA *pSlot       /* pointer to the slot to be deleted */
    )
    {
    if (pSlot == NULL)            /* null slot? don't do anything */
        return;

    /*
     * The deletion of a session entry must be an atomic operation to support
     * an upper limit on the number of simultaneous connections allowed.
     * This mutual exclusion also prevents a race condition with the server
     * shutdown routine. The last client session will always send an exit
     * signal to the shutdown routine, whether or not the shutdown flag was
     * detected during normal processing.
     */

    semTake (ftps6MutexSem, WAIT_FOREVER);
    --ftpd6NumTasks;
    --ftps6CurrentClients;
    lstDelete (&ftps6SessionList, &pSlot->node);

    ftpd6SockFree (&pSlot->cmdSock);    /* release data and command sockets */
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);

    if (!pSlot->sessionShutDown)
        taskDeleteForce (pSlot->timeoutTask);	/* Quit or ftpd6Delete */
     						/* if timeout, not */
#ifdef _WRS_KERNEL
    wdDelete (pSlot->wdTimeout);
#else
    timer_delete (pSlot->timoTimer);
#endif /* _WRS_KERNEL */
    
    semDelete (pSlot->ftpsTimeoutSem);
  
    FTPD6_FREE((char *)pSlot);
    
    /* Send required signal if all sessions are closed. */

    if (ftps6ShutdownFlag)
        {
        if (ftps6CurrentClients == 0)
            semGive (ftps6SignalSem);
        }
    semGive (ftps6MutexSem);

    return;
    }

/*******************************************************************************
*
* ftpd6SockFree - release a socket
*
* This function is used to examine whether or not the socket pointed
* by pSock is active and release it if it is.
*
* \NOMANUAL
*/

LOCAL void ftpd6SockFree
    (
    int *pSock        /* file descriptor of a socket to be released */
    )
    {
    if (*pSock != -1)
        {
        (void) close (*pSock);
        *pSock = -1;
        }
    }

#ifdef _WRS_KERNEL
/*****************************************************************************
*
* ftpd6RootDevicesList - list root devices
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \INTERNAL
* This routine was ported from ftpdLib.c.
*
* \NOMANUAL
*/

LOCAL STATUS ftpd6RootDevicesList
    (
    FTPD6_SESSION_DATA * pSlot  /* socket descriptor to write on */
    )
    {
    DEV_HDR *pDevHdr;
    DIR     *pDir;            /* ptr to directory descriptor */
    FILE *  dout;
    int     dataSock;
    
    dout = dataconn (pSlot, "file list", (off_t) -1, "w");
    if (dout == NULL)
	return (ERROR);

    dataSock = fileno (dout);

    for (pDevHdr = (DEV_HDR *) DLL_FIRST (&iosDvList); pDevHdr != NULL;
         pDevHdr = (DEV_HDR *) DLL_NEXT (&pDevHdr->node))
        {
	/*
	 * /pty devices should be avoided since opendir will close them.
	 * This can affect open telnet and rlogin connections.
	 */

        if (pDevHdr->name[0] == '/' && strcmp (pDevHdr->name, "/tgtsvr") &&
	    strncmp (pDevHdr->name, "/pty", 4))
            {
            pDir = opendir ((char *)pDevHdr->name);
            if (pDir != NULL)
                {
                closedir (pDir);
                if (fdprintf (dataSock, "%s\r\n", &pDevHdr->name[1]) == ERROR)
                    return (ERROR);
                }
            }
        }

    if (ferror (dout) != 0)
        perror_reply (pSlot, 550, "Data connection");
    else
        reply (pSlot, 226, "Transfer complete.");

    fclose (dout);
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);

    return OK;
    }
#endif /* _WRS_KERNEL */

/*******************************************************************************
*
* retrieve - send FTP data over data connection
*
* When our FTP client does a "RETR" (send me a file) and we find an existing
* file, ftpd6Parse() will call us to perform the actual shipment of the
* file in question over the data connection.
*
* We do the initialization of the new data connection ourselves here
* and make sure that everything is fine and dandy before shipping the
* contents of the file. Also, if restart_point is specified, we set a file
* pointer to this offset.
*
* SEE ALSO:
* store  which is symmetric to this function.
*
* \NOMANUAL
*/
 
void retrieve
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    char *              cmd,    /* FTP command */
    char *              name    /* full path name of a file on the server */
    )
    {
    FILE *fin, *dout;
    struct stat st;
    int (*closefunc) (FILE *);

    fin = fopen(name, "r"), closefunc = fclose;
    st.st_size = 0;

    if (fin == NULL)     
        {
        pSlot->restart_point = (off_t) 0;    
        if (errnoGet () != 0) 
            {        
            perror_reply(pSlot, 550, name);
            if (cmd == 0) 
                {
                LOGCMD(pSlot->curDirName, "get", name);
                }
            }
        return ;
        }

    pSlot->byte_count = -1;
    if (cmd == 0 && (fstat(fileno(fin), &st) < 0 || !S_ISREG(st.st_mode))) 
        {
        pSlot->restart_point = (off_t) 0;    
        reply(pSlot, 550, "%s: not a plain file.", name);
        goto done;
        }
    
    /* set the file pointer to offset */
    
    if (pSlot->restart_point) 
        {
        if (pSlot->type == TYPE_A) 
            {
            off_t i, n;
            int c;

            n = pSlot->restart_point;
            i = 0;
            while (i++ < n) 
                {
                if ((c=getc(fin)) == EOF) 
                    {
                    pSlot->restart_point = (off_t) 0;    
                    perror_reply(pSlot, 550, name);
                    goto done;
                    }
                if (c == '\n')
                    i++;
                }
            } 
        else if (lseek(fileno(fin), pSlot->restart_point, L_SET) < 0) 
            {
            pSlot->restart_point = (off_t) 0;    
            perror_reply(pSlot, 550, name);
            goto done;
            }
        pSlot->restart_point = (off_t) 0;    
        }

    /* get a data connection */    

    dout = dataconn(pSlot, name, st.st_size, "w");
    if (dout == NULL)
        goto done;
    
    /* ship the file over the data connection to client */

    send_data(pSlot, fin, dout, st.st_blksize, st.st_size,
              pSlot->restart_point == 0 && cmd == 0 && S_ISREG(st.st_mode));

    (void) fclose(dout);
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);
done:
    if (cmd == 0) 
        {
        LOGBYTES(pSlot->curDirName, "get", name, pSlot->byte_count);
        }
    (*closefunc)(fin);
    }

/*******************************************************************************
*
* store - receive FTP data over data connection
*
* When our FTP client requests "STOR" command and we were able to
* create a file requested, ftpd6Parse() will call store to actually 
* carry out the request -- receiving the contents of the named file
* and storing it in the new file created.
*
* We do the initialization of the new data connection ourselves here
* and make sure that everything is fine and dandy before receiving the
* contents of the file. Also, if restart_point is specified, we set a file
* pointer to this offset.  
*
* SEE ALSO:
* retrieve which is symmetric to this function.
*
* \NOMANUAL
*/

void store
    (
    FTPD6_SESSION_DATA *pSlot,   /* pointer to the active slot to be handled */
    char *              name,    /* full path of a file */
    char *              mode,    /* can be "w" - write, "a" - append */
    int                 unique   /* 1 - file stored under unique name */
    )
    {
    FILE *fout, *din;
    struct stat st;
    int (*closefunc) (FILE *);
    
    if ((unique || pSlot->guest) && stat(name, &st) == 0 &&
        (name = gunique(pSlot, name)) == NULL) 
        {
        LOGCMD(pSlot->curDirName, *mode == 'w' ? "put" : "append", name);
        pSlot->restart_point = (off_t) 0;    
        return;
        }

    if (pSlot->restart_point)
        mode = "r+";

    fout = fopen(name, mode);
    closefunc = fclose;
    if (fout == NULL) 
        {
        perror_reply(pSlot, 553, name);
        LOGCMD(pSlot->curDirName, *mode == 'w' ? "put" : "append", name);
        pSlot->restart_point = (off_t) 0;    
        return;
        }
    pSlot->byte_count = -1;
    
    /* set the file pointer to offset */
    
    if (pSlot->restart_point) 
        {
        if (pSlot->type == TYPE_A) 
            {
            off_t i, n;
            int c;
            
            n = pSlot->restart_point;
            i = 0;
            while (i++ < n) 
                {
                if ((c=getc(fout)) == EOF) 
                    {
                    pSlot->restart_point = (off_t) 0;    
                    perror_reply(pSlot, 550, name);
                    goto done;
                    }
                if (c == '\n')
                    i++;
                }
            /*
             * We must do this seek to "current" position
             * because we are changing from reading to
             * writing.
             */
            if (fseek(fout, 0L, L_INCR) < 0) 
                {
                pSlot->restart_point = (off_t) 0;    
                perror_reply(pSlot, 550, name);
                goto done;
                }
            } 
        else if (lseek(fileno(fout), pSlot->restart_point, L_SET) < 0) 
            {
            pSlot->restart_point = (off_t) 0;    
            perror_reply(pSlot, 550, name);
            goto done;
            }
        pSlot->restart_point = (off_t) 0;    
        }
    
    /* get a data connection */        
    
    din = dataconn(pSlot, name, (off_t)-1, "r");
    if (din == NULL)
        goto done;
    
    /* receive the contents of the file and store it in the new file created */    
    
    if (receive_data(pSlot, din, fout) == 0) 
        {
        if (unique)
            reply(pSlot, 226, "Transfer complete (unique file name:%s).", name);
        else
            reply(pSlot, 226, "Transfer complete.");
        }
    (void) fclose(din);
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);
done:
    LOGBYTES(pSlot->curDirName, *mode == 'w' ? "put" : "append", name,
             pSlot->byte_count);
    (*closefunc)(fout);
    }

/*******************************************************************************
*
* getdatasock - create data connection socket
*
* RETURNS: A pointer to a stream, or null pointer if an error occurs.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static FILE *getdatasock
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    char *              mode    /* mode to open with */
    )
    {
    int on = 1, s, t, tries;

    if (pSlot->data >= 0)
        return (fdopen(pSlot->data, mode));

    s = socket(pSlot->data_dest.su_family, SOCK_STREAM, 0);
    if (s < 0)
        goto bad;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
        goto bad;
    
    /* anchor socket to avoid multi-homing problems */
    pSlot->data_source = pSlot->ctrl_addr;
    
    pSlot->data_source.su_port = htons(20); /* ftp-data port */
                    
    for (tries = 1; ; tries++) 
        {
        if (bind(s, (struct sockaddr *)&pSlot->data_source,
                 pSlot->data_source.su_len) >= 0)
            break;
        if (errnoGet () != EADDRINUSE || tries > 10)    
            goto bad;
        
        taskDelay(tries*sysClkRateGet( )); 
        }

#ifdef IP_TOS
    if (pSlot->data_source.su_family == AF_INET) 
        {
        on = IPTOS_THROUGHPUT;
        if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
            log_warning (FTPD_LOG | LOG_ERRNO, "setsockopt (IP_TOS)");
        }
#endif
#ifdef TCP_NOPUSH
    /*
     * Turn off push flag to keep sender TCP from sending short packets
     * at the boundaries of each write().  Should probably do a SO_SNDBUF
     * to set the send buffer size as well, but that may not be desirable
     * in heavy-load situations.
     */
    on = 1;
    if (setsockopt(s, IPPROTO_TCP, TCP_NOPUSH, (char *)&on, sizeof on) < 0)
        log_warning (FTPD_LOG | LOG_ERRNO, "setsockopt (TCP_NOPUSH)");
#endif
#ifdef SO_SNDBUF
    on = 65536;
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof on) < 0)
        log_warning (FTPD_LOG | LOG_ERRNO, "setsockopt (SO_SNDBUF)");
#endif

    return (fdopen(s, mode));
bad:
    /* Return the real value of errno (close may change it) */
    t = errnoGet ()    ;        
    (void) close(s);
    errnoSet (t);            
    return (NULL);
    }

/*******************************************************************************
*
* dataconn - get a fresh data connection for FTP data transfer
*
* FTP uses upto two connections per session (as described above) at any
* time.  The command connection is maintained throughout the FTP session 
* to pass the request command strings and replies between the client and
* the server.  For commands that require bulk data transfer such as contents
* of a file or a list of files in a directory, FTP sets up dynamic data
* connections separate from the command connection. This function, dataconn,
* is responsible for creating such connections.
*
* RETURNS: A pointer to a stream, or null pointer if an error occures.
*
* ERRNO: N/A
*
* \NOMANUAL
*/
 
static FILE *dataconn
    (
    FTPD6_SESSION_DATA *pSlot,   /* pointer to the active slot to be handled */
    char *              name,    /* file path */
    off_t               size,    /* size of file, in bytes */
    char *              mode     /* can be "w" - write, "r" - read */
    )
    {
    char sizebuf[32];
    FILE *file;
    int retry = 0, tos;
    off_t    file_size;

    file_size = size;
    pSlot->byte_count = 0;
    
    if (size != (off_t) -1)
	{
	/*
	 * Ugliness: in the VxWorks kernel, an off_t is a long.
	 * In an RTP, an off_t is a long long.
	 */
#ifdef _WRS_KERNEL
        (void) snprintf(sizebuf, sizeof(sizebuf), " (%ld bytes)", size);
#else
        (void) snprintf(sizebuf, sizeof(sizebuf), " (%lld bytes)", size);
#endif
	}
    else
        *sizebuf = EOS;
    
    /* passive mode, server "listen" on the data port */    
    
    if (pSlot->pdata >= 0) 
        {
        union sockunion from;
        int s, fromlen = pSlot->ctrl_addr.su_len;
        struct timeval timeout;
        fd_set set;

        FD_ZERO(&set);
        FD_SET(pSlot->pdata, &set);

        timeout.tv_usec = 0;
        timeout.tv_sec = 120;

        if (select(pSlot->pdata+1, &set, (fd_set *) 0, (fd_set *) 0, &timeout)
            == 0 ||
            (s = accept(pSlot->pdata, (struct sockaddr *) &from, &fromlen)) < 0) 
            {
            reply(pSlot, 425, "Can't open data connection.");
            (void) close(pSlot->pdata);
            pSlot->pdata = -1;
            return (NULL);
            }
        (void) close(pSlot->pdata);
        pSlot->pdata = s;

#ifdef IP_TOS
        if (from.su_family == AF_INET)
            {
            tos = IPTOS_THROUGHPUT;
            (void) setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int));
            }
#endif
        reply(pSlot, 150, "Opening %s mode data connection for '%s'%s.",
              pSlot->type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
        return (fdopen(pSlot->pdata, mode));
        }

    /* we are not in passive mode */
    
    /* 
     * If the data sock is already initialized, we just use the already
     * existing connection.
     */
        
    if (pSlot->data >= 0) 
        {
        reply(pSlot, 125, "Using existing data connection for '%s'%s.",
              name, sizebuf);
        pSlot->usedefault = 1;
        return (fdopen(pSlot->data, mode));
        }
    
    if (pSlot->usedefault)
        pSlot->data_dest = pSlot->his_addr;
    pSlot->usedefault = 1;
    
    /* create data socket */
    
    file = getdatasock(pSlot, mode);
    if (file == NULL) 
        {
        char hostbuf[BUFSIZ], portbuf[BUFSIZ];
        getnameinfo((struct sockaddr *)&pSlot->data_source,
        pSlot->data_source.su_len, hostbuf, sizeof(hostbuf) - 1,
        portbuf, sizeof(portbuf),
        NI_NUMERICHOST|NI_NUMERICSERV|NI_WITHSCOPEID);
        reply(pSlot, 425, "Can't create data socket (%s,%s): %s.",
        hostbuf, portbuf, strerror(errnoGet ())); 
        return (NULL);
        }

    /* connect to the previously specified address */
    
    pSlot->data = fileno(file);
    while (connect(pSlot->data, (struct sockaddr *)&pSlot->data_dest,
                   pSlot->data_dest.su_len) < 0) 
        {
        if (errnoGet () == EADDRINUSE && retry < swaitmax) 
            {    
            taskDelay((unsigned)swaitint*sysClkRateGet( ));
            retry += swaitint;
            continue;
            }
        perror_reply(pSlot, 425, "Can't build data connection");
        (void) fclose(file);
        ftpd6SockFree (&pSlot->data);
        return (NULL);
        }
    
    reply(pSlot, 150, "Opening %s mode data connection for '%s'%s.",
          pSlot->type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
    return (file);
    }
    
/*******************************************************************************
*
* send_data - send FTP data over data connection
*
* Tranfer the contents of "instr" to "outstr" peer using the appropriate
* encapsulation of the data subject to Mode, Structure, and Type.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/
 
static void send_data
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    FILE *        instr,        /* pointer to a local file */
    FILE *        outstr,       /* pointer to a data socket */
    off_t         blksize,      /* max no. of bytes to read into buffer */
    off_t         filesize,     /* size of file, in bytes */
    int         isreg
    )
    {
    int c, filefd, netfd;
    char *buf;
    off_t cnt;

    pSlot->transflag++;

    switch (pSlot->type) 
        {
        case TYPE_A:
            while ((c = getc(instr)) != EOF) 
                {
                pSlot->byte_count++;
                if (c == '\n') 
                    {
                    if (ferror(outstr))
                        goto data_err;
                    fprintf (outstr, "%c", '\r');    
                    }
                fprintf (outstr, "%c", c);    
                }
            fflush(outstr);
            pSlot->transflag = 0;
            if (ferror(instr))
                goto file_err;
            if (ferror(outstr))
                goto data_err;
            reply(pSlot, 226, "Transfer complete.");
            return;

        case TYPE_I:
        case TYPE_L:
            /*
             * isreg is only set if we are not doing restart and we
             * are sending a regular file
             */
            netfd = fileno(outstr);
            filefd = fileno(instr);

	    if (blksize <= 0)
		blksize = (off_t)BUFSIZ;

            if ((buf = (char *)malloc((u_int)blksize)) == NULL) 
                {
                pSlot->transflag = 0;
                perror_reply(pSlot, 451, "Local resource failure: malloc");
                return;
                }

            while ((cnt = read(filefd, buf, (u_int)blksize)) > 0 &&
                   write(netfd, buf, cnt) == cnt)
                pSlot->byte_count += cnt;

            pSlot->transflag = 0;
            (void)free(buf);
            if (cnt != 0) 
                {
                if (cnt < 0)
                    goto file_err;
                goto data_err;
                }
            reply(pSlot, 226, "Transfer complete.");
            return;
            
        default:
            pSlot->transflag = 0;
            reply(pSlot, 550, "Unimplemented TYPE %d in send_data", pSlot->type);
            return;
        }

data_err:
    pSlot->transflag = 0;
    perror_reply(pSlot, 426, "Data connection");
    return;

file_err:
    pSlot->transflag = 0;
    perror_reply(pSlot, 551, "Error on input file");
    }

/*******************************************************************************
*
* receive_data - receive FTP data over data connection
*
* Transfer data from peer to "outstr" using the appropriate encapulation of
* the data subject to Mode, Structure, and Type.
*
* RETURNS: 0 if transfer succeed, otherwise -1
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int receive_data
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    FILE *        instr,        /* pointer to a data socket */
    FILE *        outstr        /* pointer to a local file */
    )
    {
    int  c;
    int  cnt, bare_lfs;
    char buf[BUFSIZ];

    pSlot->transflag++;

    bare_lfs = 0;

    switch (pSlot->type) 
        {
        case TYPE_I:
        case TYPE_L:
            while ((cnt = read(fileno(instr), buf, sizeof(buf))) > 0) 
                {
                if (write(fileno(outstr), buf, cnt) != cnt)
                    {
                    log_err (FTPD_LOG | LOG_ERRNO, 
                             "(%d) Byte written not equal to read: %d\n",
                             __LINE__, cnt);
                    goto file_err;
                    }
                pSlot->byte_count += cnt;
                }
            if (cnt < 0)
                goto data_err;
            pSlot->transflag = 0;
            return (0);

        case TYPE_E:
            reply(pSlot, 553, "TYPE E not implemented.");
            pSlot->transflag = 0;
            return (-1);

        case TYPE_A:
            while ((c = getc(instr)) != EOF) 
                {
                pSlot->byte_count++;
                if (c == '\n')
                    bare_lfs++;
                while (c == '\r') 
                    {
                    if (ferror(outstr))
                        {
                        log_err (FTPD_LOG | LOG_ERRNO, 
                                 "ASCII mode: ferror(outstr) check failed");
                        goto data_err;
                        }
                    if ((c = getc(instr)) != '\n') 
                        {
                        (void) putc ('\r', outstr);
                        if (c == EOS || c == EOF)
                            goto contin2;
                        }
                    }
                (void) putc(c, outstr);
                contin2:    ;
                }
            fflush(outstr);
            if (ferror(instr))
                {
                log_err (FTPD_LOG | LOG_ERRNO, "ferror(instr) check failed");
                goto data_err;
                }
            if (ferror(outstr))
                {
                log_err (FTPD_LOG | LOG_ERRNO, "ferror(outstr) check failed");
                goto file_err;
                }
            pSlot->transflag = 0;
            if (bare_lfs) 
                {
                lreply(pSlot, 226,
                       "WARNING! %d bare linefeeds received in ASCII mode", bare_lfs);
                log_warning (FTPD_LOG, "   File may not have transferred correctly.");
                }
            return (0);
        
        default:
            reply(pSlot, 550, "Unimplemented TYPE %d in receive_data", pSlot->type);
            pSlot->transflag = 0;
            return (-1);
        }

data_err:
    pSlot->transflag = 0;
    perror_reply(pSlot, 426, "Data Connection");
    return (-1);

file_err:
    pSlot->transflag = 0;
    perror_reply(pSlot, 452, "Error writing file");
    return (-1);
    }

/*******************************************************************************
*
* statcmd - send a status response
*
* This function shall cause a status response to be sent over the control 
* connection in the form of a reply.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void statcmd
    (
    FTPD6_SESSION_DATA *pSlot  /* pointer to the active slot to be handled */
    )
    {
    union sockunion *su;
    u_char *a = NULL, *p = NULL;
#ifdef INET6
    char hname[INET6_ADDRSTRLEN];
#else
    char hname[INET_ADDRSTRLEN];
#endif    
    int ispassive;
    int cs;
#if !(NBBY == 8)
    int bytesize = 8;
#endif

    cs = pSlot->cmdSock;

    lreply(pSlot, 211, "%s FTP server status:", ftpd6hostname);
    /*
     * XXX - once runtimeName;/runtimeVersion become available in RTP, the
     *       following #ifdef will be unnecessary.
     */
#ifdef _WRS_KERNEL
    fdprintf(cs, "     %s %s\r\n", runtimeName, runtimeVersion);
#endif /* _WRS_KERNEL */
    fdprintf(cs, "     Connected to %s", pSlot->remotehost);
    
    if (!getnameinfo((struct sockaddr *)&pSlot->his_addr, pSlot->his_addr.su_len,
                     hname, sizeof(hname) - 1, NULL, 0,
                     NI_NUMERICHOST|NI_WITHSCOPEID)) 
        {
        if (strcmp(hname, pSlot->remotehost) != 0)
            fdprintf(cs, " (%s)", hname);
        }
    fdprintf(cs, "\r\n");
    
    if (pSlot->logged_in) 
        {
        if (pSlot->guest)
            fdprintf(cs, "     Logged in anonymously\r\n");
        else
            fdprintf(cs, "     Logged in as %s\r\n", pSlot->user);
        }
    else if (pSlot->askpasswd)
        fdprintf(cs, "     Waiting for password\r\n");
    else
        fdprintf(cs, "     Waiting for user name\r\n");
    
    fdprintf(cs, "     TYPE: %s", typenames[pSlot->type]);
    if (pSlot->type == TYPE_A || pSlot->type == TYPE_E)
        fdprintf(cs, ", FORM: %s", formnames[pSlot->form]);
    if (pSlot->type == TYPE_L)
#if NBBY == 8
        fdprintf(cs, " %d", NBBY);
#else
    fdprintf(cs, " %d", bytesize);    
#endif
    fdprintf(cs, "; STRUcture: %s; transfer MODE: %s\r\n",
             strunames[pSlot->stru], modenames[pSlot->mode]);
    
    if (pSlot->data != -1)
        fdprintf(cs, "     Data connection open\r\n");
    else if (pSlot->pdata != -1) 
        {
        ispassive = 1;
        su = &pSlot->pasv_addr;
        goto printaddr;
        } 
    else if (pSlot->usedefault == 0) 
        {
        ispassive = 0;
        su = &pSlot->data_dest;
printaddr:
#define UC(b) (((int) b) & 0xff)
        if (pSlot->epsvall) 
            {
            fdprintf(cs, "     EPSV only mode (EPSV ALL)\r\n");
            goto epsvonly;
            }

        /* PORT/PASV */
        if (su->su_family == AF_INET) 
            {
            a = (u_char *) &su->su_sin.sin_addr;
            p = (u_char *) &su->su_sin.sin_port;
            fdprintf(cs, "     %s (%d,%d,%d,%d,%d,%d)\r\n",
                     ispassive ? "PASV" : "PORT",
                     UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
                     UC(p[0]), UC(p[1]));
            }

        /* LPRT/LPSV */
        {
        int alen = 0, af, i;

        switch (su->su_family) 
            {
            case AF_INET:
                a = (u_char *) &su->su_sin.sin_addr;
                p = (u_char *) &su->su_sin.sin_port;
                alen = sizeof(su->su_sin.sin_addr);
                af = IPv4_AF;
                break;
#ifdef INET6            
            case AF_INET6:
                a = (u_char *) &su->su_sin6.sin6_addr;
                p = (u_char *) &su->su_sin6.sin6_port;
                alen = sizeof(su->su_sin6.sin6_addr);
                af = IPv6_AF;
                break;
#endif            
            default:
                af = 0;
                break;
            }
        if (af) 
            {
            fdprintf(cs, "     %s (%d,%d,", ispassive ? "LPSV" : "LPRT",
                     af, alen);
            for (i = 0; i < alen; i++)
                fdprintf(cs, "%d,", UC(a[i]));
            fdprintf(cs, "%d,%d,%d)\r\n", 2, UC(p[0]), UC(p[1]));
            }
        }

epsvonly:;
        /* EPRT/EPSV */
        {
        int af;

        switch (su->su_family) 
            {
            case AF_INET:
                af = IPv4_AF_NUMBER;
                break;
            case AF_INET6:
                af = IPv6_AF_NUMBER;
                break;
            default:
                af = 0;
                break;
            }
        if (af) 
            {
            if (!getnameinfo((struct sockaddr *)su, su->su_len,
                             hname, sizeof(hname) - 1, NULL, 0,
                             NI_NUMERICHOST)) 
                {
                fdprintf(cs, "     %s |%d|%s|%d|\r\n",
                         ispassive ? "EPSV" : "EPRT",
                         af, hname, htons(su->su_port));
                }
            }
        }
#undef UC
        } 
    else
        fdprintf(cs, "     No data connection\r\n");
    reply(pSlot, 211, "End of status");
    }

/*****************************************************************************
*
* ftpd6Fatal - send a FTP command, reply 451 - Error in server
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void ftpd6Fatal
    (
    FTPD6_SESSION_DATA *pSlot,    /* pointer to the active slot to be handled */
    char *        s    /* string to be appended to the reply */
    )
    {
    reply(pSlot, 451, "Error in server: %s\n", s);
    reply(pSlot, 221, "Closing connection due to server error.");
    dologout(pSlot);
    /* NOTREACHED */
    }

/*******************************************************************************
*
* reply - send a FTP command reply
*
* In response to a request, we send a reply containing completion
* status, error status, and other information over a command connection.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void reply
    (
    FTPD6_SESSION_DATA *pSlot,   /* pointer to the active slot to be handled */
    int                 n,       /* FTP status code */
    const char *        fmt,     /* printf style format string */
    ...
    ) 
    {
    va_list     ap;
    int         buflen;
    char        buf [BUFSIZ];      /* local buffer */
    FAST char * pBuf = &buf [0];   /* pointer to buffer */

    va_start(ap, fmt);

    if (pSlot->cmdSockError == ERROR)
        return ;

    (void)sprintf(pBuf, "%d ", n);
    buflen = strlen (buf);
    (void) vsnprintf (&pBuf [buflen], sizeof (buf) - buflen - 2, fmt, ap);
    buflen = strlen (buf);      
    (void)sprintf(&pBuf[buflen], "%s", "\r\n");

    /* send it over to our client */

    buflen = strlen (buf);
    
    if ( write (pSlot->cmdSock, buf, buflen) != buflen ) 
        {
        pSlot->cmdSockError = ERROR;
        /*
         * This may happen when a session is timed out. So, use log_info()
         * instead of log_err() here.
         */
        log_info (FTPD_LOG, "sent %s Failed on write", buf);
        va_end (ap);
        return ;    /* Write Error */
        }

    va_end (ap); 
    }

/*******************************************************************************
*
* lreply - send a FTP command long reply
*
* In response to a request, we send a reply containing completion
* status, error status, and other information over a command connection.
* Dash after the FTP status code determines that other reply will follow. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void lreply
    (
    FTPD6_SESSION_DATA *pSlot,   /* pointer to the active slot to be handled */
    int                 n,        /* FTP status code */
    const char *        fmt, ...) /* printf style format string */
    {
    va_list    ap;
    int        buflen;
    char       buf [BUFSIZ];      /* local buffer */
    FAST char *pBuf = &buf [0];   /* pointer to buffer */

    va_start(ap, fmt);

    if (pSlot->cmdSockError == ERROR)
        return ;

    (void)sprintf(pBuf, "%d- ", n);
    buflen = strlen (buf);
    (void) vsnprintf (&pBuf [buflen], sizeof (buf) - buflen - 2, fmt, ap);
    buflen = strlen (buf);
    (void)sprintf(&pBuf[buflen], "%s", "\r\n");

    /* send it over to our client */

    buflen = strlen (buf);

    if ( write (pSlot->cmdSock, buf, buflen) != buflen ) 
        {
        pSlot->cmdSockError = ERROR;
        log_err (FTPD_LOG, "sent %s Failed on write", buf);
        va_end (ap);
        return ;    /* Write Error */
        }

    if (ftpd6Debug) 
        {
        printf ("<--- %d- ", n);
        vprintf (fmt, ap);
        printf ("\n");
        }
    
    va_end (ap);    
    }

/*******************************************************************************
*
* replyNopSlot - send a FTP command reply
*
* In response to a request, we send a reply containing completion
* status, error status, and other information over a command connection.
* FTPD6_SESSION_DATA struct has not been allocated yet, we need to pass
* file descriptor of the control connection. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void replyNopSlot
    (
    int          ctrlSock,    /* file descriptor of the control connection */
    int          n,           /* FTP status code */
    const char * fmt, ...)    /* printf style format string */
    {
    va_list    ap;
    int        buflen;
    char       buf [BUFSIZ];       /* local buffer */
    FAST char *pBuf = &buf [0];    /* pointer to buffer */

    va_start(ap, fmt);
    
    (void)sprintf(pBuf, "%d ", n);
    buflen = strlen (buf);
    (void) vsnprintf (&pBuf [buflen], sizeof (buf) - buflen - 2, fmt, ap);
    buflen = strlen (buf);
    (void)sprintf(&pBuf[buflen], "%s", "\r\n");

    /* send it over to our client */

    buflen = strlen (buf);

    if ( write (ctrlSock, buf, buflen) != buflen ) 
        {
        log_err (FTPD_LOG, "sent %s Failed on write", buf);
        va_end (ap);
        return ;    /* Write Error */
        }

    if (ftpd6Debug) 
        {
        printf ("<--- %d- ", n);
        vprintf (fmt, ap);
        printf ("\n");
        }
    
    va_end (ap);    
    }

/*******************************************************************************
*
* ack - send a FTP command, reply 250 - command successful
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void ack
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    char               *s       /* string to be appended to the reply */
    )
    {
    reply(pSlot, 250, "%s command successful.", s);
    }

/*******************************************************************************
*
* nack - send a FTP command, reply 502 - command not implemented
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void nack
    (
    FTPD6_SESSION_DATA *pSlot,  /* pointer to the active slot to be handled */
    char               *s       /* string to be appended to the reply */
    )
    {
    reply(pSlot, 502, "%s command not implemented.", s);
    }

/*******************************************************************************
*
* delete - delete file
*
* This function causes the file specified in the name to be deleted at the 
* server site.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void deletefile
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * name    /* full path of a file */
    )
    {
    struct stat st;
    
    LOGCMD(pSlot->curDirName, "delete", name);
    if (stat(name, &st) < 0) 
        {
        perror_reply(pSlot, 550, name);
        return;
        }
    if ((st.st_mode&S_IFMT) == S_IFDIR) 
        {
        if (rmdir(name) < 0) 
            {
            perror_reply(pSlot, 550, name);
            return;
            }
        goto done;
        }
    if (unlink(name) < 0) 
        {
        perror_reply(pSlot, 550, name);
        return;
        }
done:
    ack(pSlot, "DELE");
    }

/*******************************************************************************
*
* cwd - set as a Current Working Directory
*
* This function causes the pathname specified in the dirName to be set 
* as a Current Working Directory  at the server site.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void cwd
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * dirName /* full path of a directory */
    )
    {
    char         newPath [MAX_FILENAME_LENGTH];
    struct stat  statBuf;

    if (strcmp (dirName, "/") != 0 &&
	ftpd6CwdStat &&
	((stat (dirName, &statBuf) != OK) ||
	 !S_ISDIR (statBuf.st_mode)))
        {
        reply(pSlot, 501, "'%s': Directory non existent or syntax error.", dirName);
        return;
        }

    (void) strcpy (newPath, dirName);    /* use the whole thing */
    PATH_CONDENSE (newPath);

    /* remember where we are */

    (void) strcpy (pSlot->curDirName, newPath);
        
    ack(pSlot, "CWD");
    }

/*******************************************************************************
*
* makedir - make directory
*
* This function causes the pathname specified in the name to be created
* as a directory (if pathname is absolute) or as a subdirectory of the 
* current directory (if pathname is relative).
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void makedir
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * name    /* full path of a directory */
    )
    {
    LOGCMD(pSlot->curDirName, "mkdir", name);
#ifdef _WRS_KERNEL
    if (mkdir(name) < 0)
#else
    if (mkdir (name, 0000777) < 0)
#endif
        perror_reply(pSlot, 550, name);
    else
        reply (pSlot, 257, "MKD command successful.");
    }

/*******************************************************************************
*
* removedir - remove directory
*
* This function causes the pathname specified in the name to be removed
* as a directory (if pathname is absolute) or as a subdirectory of the 
* current directory (if pathname is relative).
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void removedir
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * name    /* full path of a directory */
    )
    {
    LOGCMD(pSlot->curDirName, "rmdir", name);
    if (rmdir(name) < 0)
        perror_reply(pSlot, 550, name);
    else  
        ack (pSlot, "RMD");
    }

/*******************************************************************************
*
* renamefrom - file to be renamed
*
* This function specifies the old pathname of a file to be renamed.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void renamefrom
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * name    /* full path of a file */
    )
    {
    struct stat st;
       
    if (stat(name, &st) < 0) 
        {
        perror_reply(pSlot, 550, name);
        memset(&pSlot->fromname[0], (char) 0, sizeof(pSlot->fromname));
        return;
        }
    reply(pSlot, 350, "File exists, ready for destination name");
    return;
    }

/*******************************************************************************
*
* renamecmd - rename a file
*
* This function renames the old file from to a new file to.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void renamecmd
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * from,   /* full path of an old file */
    char               * to      /* full path of a new file */
    )
    {
    struct stat st;
        
    LOGCMD2(pSlot->curDirName, "rename", from, to);

    if (pSlot->guest && (stat(to, &st) == 0)) 
        {
        reply(pSlot, 550, "%s: permission denied", to);
        return;
        }

    if (rename(from, to) < 0)
        perror_reply(pSlot, 550, "rename");
    else
        ack(pSlot, "RNTO");
    }

/*******************************************************************************
*
* dologout - do logout for a particular FTP session 
*
* This function causes a particular FTP session to be logged out. It sets
* pSlot->sessionShutDown and closes the socket for control connection. This
* resuls in shutdown of the FTP session.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void dologout
    (
    FTPD6_SESSION_DATA *pSlot  /* pointer to the active slot to be handled */
    )
    {
    pSlot->transflag = 0;    

    semTake (ftps6MutexSem, WAIT_FOREVER);
 
    pSlot->sessionShutDown = TRUE;

    ftpd6SockFree (&pSlot->cmdSock);

    semGive (ftps6MutexSem); 
    }

/*******************************************************************************
*
* passive - Enter Passive Mode (PASV)
*
* This function enters Passive Mode. It creates data connection socket, binds  
* it to a port, begins to listen on this socket and sends the info to client.
*
* NOTE: A response of 425 is not mentioned as a possible response to the PASV
* command in RFC959. However, it has been blessed as a legitimate response by
* Jon Postel in a telephone conversation with Rick Adams on 25 Jan 89.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void passive
    (
    FTPD6_SESSION_DATA *pSlot   /* pointer to the active slot to be handled */
    )
    {
    int len;
    char *p, *a;

    if (pSlot->pdata >= 0)        /* close old port if one set */
        close(pSlot->pdata);

    pSlot->pdata = socket(pSlot->ctrl_addr.su_family, SOCK_STREAM, 0);
    if (pSlot->pdata < 0)     
        {
        perror_reply(pSlot, 425, "Can't open passive connection");
        return ;
        }

#ifdef IP_PORTRANGE
    if (pSlot->ctrl_addr.su_family == AF_INET) 
        {
        int on = restricted_data_ports ? IP_PORTRANGE_HIGH
            : IP_PORTRANGE_DEFAULT;

        if (setsockopt(pSlot->pdata, IPPROTO_IP, IP_PORTRANGE,
                       (char *)&on, sizeof(on)) < 0)
            goto pasv_error;
        }
#endif
#ifdef IPV6_PORTRANGE
    if (pSlot->ctrl_addr.su_family == AF_INET6) 
        {
        int on = restricted_data_ports ? IPV6_PORTRANGE_HIGH
            : IPV6_PORTRANGE_DEFAULT;

        if (setsockopt(pSlot->pdata, IPPROTO_IPV6, IPV6_PORTRANGE,
                       (char *)&on, sizeof(on)) < 0)
            goto pasv_error;
        }
#endif
#ifdef IPV6_PORTRANGE
    if (pSlot->ctrl_addr.su_family == AF_INET6) 
        {
        int on = restricted_data_ports ? IPV6_PORTRANGE_HIGH
            : IPV6_PORTRANGE_DEFAULT;

        if (setsockopt(pSlot->pdata, IPPROTO_IPV6, IPV6_PORTRANGE,
                       (char *)&on, sizeof(on)) < 0)
            goto pasv_error;
        }
#endif

    pSlot->pasv_addr = pSlot->ctrl_addr;
    pSlot->pasv_addr.su_port = 0;
    if (bind(pSlot->pdata, (struct sockaddr *)&pSlot->pasv_addr, pSlot->pasv_addr.su_len) < 0)
        goto pasv_error;

    len = sizeof(pSlot->pasv_addr);
    if (getsockname(pSlot->pdata, (struct sockaddr *) &pSlot->pasv_addr, &len) < 0)
        goto pasv_error;
    if (listen(pSlot->pdata, 1) < 0)
        goto pasv_error;
    
    if (pSlot->pasv_addr.su_family == AF_INET)
        a = (char *) &pSlot->pasv_addr.su_sin.sin_addr;
#ifdef INET6    
    else if (pSlot->pasv_addr.su_family == AF_INET6 &&
             IN6_IS_ADDR_V4MAPPED(&pSlot->pasv_addr.su_sin6.sin6_addr))
        a = (char *) &pSlot->pasv_addr.su_sin6.sin6_addr.s6_addr[12];
#endif
    else
        goto pasv_error;
        
    p = (char *) &pSlot->pasv_addr.su_port;

#define UC(b) (((int) b) & 0xff)

    reply(pSlot, 227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", UC(a[0]),
          UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
    return ;

pasv_error:
    (void) close(pSlot->pdata);
    pSlot->pdata = -1;
    perror_reply(pSlot, 425, "Can't open passive connection");
    return ;
    }

/*******************************************************************************
*
* long_passive - Enter Passive Mode (LPSV, EPSV)
*
* This function enters Passive Mode. It creates data connection socket, binds  
* it to a port, begins to listen on this socket and sends the info to client.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void long_passive
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * cmd,    /* LPSV or EPSV */
    int                  pf      /* protocol family */
    )    
    {
    int len;
    char *p, *a;

    if (pSlot->pdata >= 0)        /* close old port if one set */
    close(pSlot->pdata);

    if (pf != PF_UNSPEC) 
        {
        if (pSlot->ctrl_addr.su_family != pf) 
            {
            switch (pSlot->ctrl_addr.su_family) 
                {
                case AF_INET:
                    pf = IPv4_AF_NUMBER;
                    break;
                case AF_INET6:
                    pf = IPv6_AF_NUMBER;
                    break;
                default:
                    pf = 0;
                    break;
                }
            /*
             * XXX
             * only EPRT/EPSV ready clients will understand this
             */
            if (strcmp(cmd, "EPSV") == 0 && pf) 
                {
                reply(pSlot, 522, "Network protocol mismatch, "
                      "use (%d)", pf);
                } 
            else
                reply (pSlot, 501, "Network protocol mismatch");
            
            return ;
            }
        }
        
    pSlot->pdata = socket(pSlot->ctrl_addr.su_family, SOCK_STREAM, 0);
    if (pSlot->pdata < 0) 
        {
        perror_reply(pSlot, 425, "Can't open passive connection");
        return ;
        }

    pSlot->pasv_addr = pSlot->ctrl_addr;
    pSlot->pasv_addr.su_port = 0;
    len = pSlot->pasv_addr.su_len;

#ifdef IP_PORTRANGE
    if (pSlot->ctrl_addr.su_family == AF_INET) 
        {
        int on = restricted_data_ports ? IP_PORTRANGE_HIGH
            : IP_PORTRANGE_DEFAULT;

        if (setsockopt(pSlot->pdata, IPPROTO_IP, IP_PORTRANGE,
                       (char *)&on, sizeof(on)) < 0)
            goto pasv_error;
        }
#endif
#ifdef IPV6_PORTRANGE
    if (pSlot->ctrl_addr.su_family == AF_INET6) 
        {
        int on = restricted_data_ports ? IPV6_PORTRANGE_HIGH
            : IPV6_PORTRANGE_DEFAULT;
        
        if (setsockopt(pSlot->pdata, IPPROTO_IPV6, IPV6_PORTRANGE,
                       (char *)&on, sizeof(on)) < 0)
            goto pasv_error;
        }
#endif

    if (bind(pSlot->pdata, (struct sockaddr *)&pSlot->pasv_addr, len) < 0)
        goto pasv_error;

    if (getsockname(pSlot->pdata, (struct sockaddr *) &pSlot->pasv_addr, &len) < 0)
        goto pasv_error;
    if (listen(pSlot->pdata, 1) < 0)
        goto pasv_error;

#define UC(b) (((int) b) & 0xff)

    if (strcmp(cmd, "LPSV") == 0) 
        {
        p = (char *)&pSlot->pasv_addr.su_port;
        switch (pSlot->pasv_addr.su_family) 
            {
            case AF_INET:
                a = (char *) &pSlot->pasv_addr.su_sin.sin_addr;
#ifdef INET6
            v4_reply:
#endif              
                reply(pSlot, 228,
                      "Entering Long Passive Mode (%d,%d,%d,%d,%d,%d,%d,%d,%d)",
                      IPv4_AF, sizeof (struct in_addr), 
                      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
                      PORT_ADDR_LENGTH, UC(p[0]), UC(p[1]));
                return ;
#ifdef INET6
            case AF_INET6:
                if (IN6_IS_ADDR_V4MAPPED(&pSlot->pasv_addr.su_sin6.sin6_addr)) 
                    {
                    a = (char *) &pSlot->pasv_addr.su_sin6.sin6_addr.s6_addr[12];
                    goto v4_reply;
                    }
                a = (char *) &pSlot->pasv_addr.su_sin6.sin6_addr;
                reply(pSlot, 228,
"Entering Long Passive Mode "
"(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
                      IPv6_AF, sizeof(struct in6_addr), 
                      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
                      UC(a[4]), UC(a[5]), UC(a[6]), UC(a[7]),
                      UC(a[8]), UC(a[9]), UC(a[10]), UC(a[11]),
                      UC(a[12]), UC(a[13]), UC(a[14]), UC(a[15]),
                      PORT_ADDR_LENGTH, UC(p[0]), UC(p[1]));
                return ;
#endif                
            }
        } 
    else if (strcmp(cmd, "EPSV") == 0) 
        {
        switch (pSlot->pasv_addr.su_family) 
            {
            case AF_INET:
            case AF_INET6:
                reply(pSlot, 229, "Entering Extended Passive Mode (|||%d|)",
                      ntohs(pSlot->pasv_addr.su_port));
                return ;
            }
        } 
    else 
        {
        /* more proper error code? */
        }

pasv_error:
    (void) close(pSlot->pdata);
    pSlot->pdata = -1;
    perror_reply(pSlot, 425, "Can't open passive connection");
    return ;
    }

/*******************************************************************************
*
* gunique - generate unique name
*
* This function generates unique name for file with basename "local".
*
* RETURNS: A pointer to a new file name or NULL if an error occures.
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static char *gunique
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * local   /* full path of a file */
    )
    {
    static char newname[MAX_FILENAME_LENGTH];
    struct stat st;
    int count;
    char *cp;

    cp = strrchr(local, '/');
    if (cp)
        *cp = EOS;
    if (stat(cp ? local : ".", &st) < 0) 
        {
        perror_reply(pSlot, 553, cp ? local : ".");
        return ((char *) 0);
        }
    if (cp)
        *cp = '/';
    /* -4 is for the .nn<null> we put on the end below */
    (void) snprintf(newname, sizeof(newname) - 4, "%s", local);
    cp = newname + strlen(newname);
    *cp++ = '.';
    
    for (count = 1; count < 100; count++) 
        {
        (void)sprintf(cp, "%d", count);
        if (stat(newname, &st) < 0)
            return (newname);
        }
    reply(pSlot, 452, "Unique file name cannot be created.");
    return (NULL);
    }

/*******************************************************************************
*
* perror_reply - format and send reply containing system error number
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void perror_reply
    (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    int                  code,   /* FTP status code */
    char               * string  /* string to be appended to the reply */
    )
    {
    reply(pSlot, code, "%s: %d.", string, errnoGet());    
    }

static char *onefile[] = 
    {
    "",
    0
    };

/*******************************************************************************
*
* send_file_list - list files in a directory for FTP client
*
* This function performs the client's request to list out all the files in a
* given directory.  The VxWorks implementation of stat() does not work on RT-11
* filesystem drivers, it is simply not supported (RT-11 is not supported on
* VxWorks 6.x).
*
* This command is similar to UNIX ls.  It lists the contents of a directory,
* only the names of the files (or subdirectories) in the specified directory 
* are displayed. (ls -l see ftpd6DirListGet) 
*
* The <dirName> parameter specifies the directory to be listed.  If
* <dirName> is omitted or NULL, the current working directory will be
* listed.
*
* Empty directory entries and MS-DOS volume label entries are not
* reported.
*
* INTERNAL
* Borrowed from ls() in usrLib.c.
*
* RETURNS:  OK or ERROR.
*
* SEE ALSO: ls(), stat()
*
* \NOMANUAL
*/

void send_file_list
   (
    FTPD6_SESSION_DATA * pSlot,  /* pointer to the active slot to be handled */
    char               * whichf  /* name of the directory to be listed */
    )
    {
    struct stat st;
    DIR *dirp = NULL;
    struct dirent *dir;
    FILE *dout = NULL;
    char **dirlist, *dirname, *pPattern, *pTempPattern;
    char * searchBuf;
    char * tempwhichf;
    char * nbuf;
    int sd;

    searchBuf = malloc (MAX_FILENAME_LENGTH * 3);
    if (searchBuf == NULL)
	{
	perror_reply(pSlot, 451, "Local resource failure: malloc");
	return;
	}

    tempwhichf = searchBuf + MAX_FILENAME_LENGTH;
    nbuf = tempwhichf + MAX_FILENAME_LENGTH;

    /*
     * If user typed "ls -l", etc, and the client
     * used NLST, do what the user meant.
     */
/*    if (((strncmp(whichf,"-l", 2) == 0) ||
        (strncmp(whichf,"-al", 3) == 0)) &&
        (pSlot->transflag == 0)) 
*/
        {
        ftpd6DirListGet(pSlot, pSlot->curDirName, 1, searchBuf);
        goto send_file_list_done;
        }

    if (ftpd6FullPath (pSlot, whichf, searchBuf, FALSE) != OK)
        {
        reply(pSlot, 553, "Directory path invalid.");
        goto send_file_list_done;
        }

#ifdef _WRS_KERNEL
    if (strcmp (searchBuf, "/") == 0)
        {
        ftpd6RootDevicesList (pSlot);
        goto send_file_list_done;
        }
#endif

    onefile[0] = searchBuf;
    dirlist = onefile;
    
    while ((dirname = *dirlist++) != NULL) 
        {
        pPattern = NULL;
        strcpy (tempwhichf, whichf);
        if (stat(dirname, &st) < 0) 
            {
            /* last component may be a wildcard  pattern */
            if ((pPattern = rindex (dirname, '/')) != NULL)
                *(pPattern++) = EOS;

            if (stat(dirname, &st) < 0)
                {
                perror_reply(pSlot, 550, whichf);
                if (dout != NULL) 
                    {
                    (void) fclose(dout);
                    pSlot->transflag = 0;
                    ftpd6SockFree (&pSlot->data);
                    ftpd6SockFree (&pSlot->pdata);
                    }
		goto send_file_list_done;
                }
            else
                {
                /* 
                 * If we are here, whichf has a pattern. If whichf is
                 * 'dirname/a*.c', tempwhichf should be 'dirname'. If
                 * whichf is 'a*.c', tempwhichf should be null string.
                 */
                if ((pTempPattern = rindex (tempwhichf, '/')) != NULL)
                    *(pTempPattern++) = EOS;
                else
                    tempwhichf[0] = EOS;
                }
            }
        
        if (S_ISREG(st.st_mode)) 
            {
            if (dout == NULL) 
                {
                dout = dataconn(pSlot, "file list", (off_t)-1, "w");
                if (dout == NULL)
		    goto send_file_list_done;
                pSlot->transflag++;
                }
            sd = fileno(dout);
            
            fdprintf(sd, "%s%s\n", whichf,
                     pSlot->type == TYPE_A ? "\r" : "");
            pSlot->byte_count += strlen(whichf) + 1;
            continue;
            } 
        else if (!S_ISDIR(st.st_mode))
            continue;

        if ((dirp = opendir(dirname)) == NULL)
            continue;

        while ((dir = readdir(dirp)) != NULL) 
            {
            if (dir->d_name[0] == '.' && strlen(dir->d_name) == 1)
                continue;
            if (dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
                strlen(dir->d_name) == 2)
                continue;

            if (pPattern != NULL && 
                ftpDirListPattern (pPattern, dir->d_name) == FALSE)
                continue;

            /*
             * We have to do a stat to insure it's
             * not a directory or special file.
             */
            if (dout == NULL) 
                {
                dout = dataconn(pSlot, "file list", (off_t)-1, "w");
                if (dout == NULL)
		    goto send_file_list_done;
                pSlot->transflag++;
                }
            sd = fileno(dout);
        
            /* 
             * Print the whole path of the file if user did not make a list
             * of the current directory.  He has specified the dir to list
             * as a root directory or as a subdirectory of the current dir.
             */

            if (whichf != pSlot->curDirName)
                {
                if (tempwhichf [0] == '\0')
                    {
                    fdprintf(sd, "%s%s\n", dir->d_name,
                             pSlot->type == TYPE_A ? "\r" : "");
                    pSlot->byte_count += strlen(dir->d_name) + 1;
                    }
                else
                    {
                    if (tempwhichf [strlen (tempwhichf) - 1] == '/')
                        snprintf(nbuf, MAX_FILENAME_LENGTH,
                                 "%s%s", tempwhichf, dir->d_name);
                    else
                        snprintf(nbuf, MAX_FILENAME_LENGTH,
                                 "%s/%s", tempwhichf, dir->d_name);
                    fdprintf(sd, "%s%s\n", nbuf,
                             pSlot->type == TYPE_A ? "\r" : "");
                    pSlot->byte_count += strlen(nbuf) + 1;
                    }
                }
            else
                {
                fdprintf(sd, "%s%s\n", dir->d_name,
                         pSlot->type == TYPE_A ? "\r" : "");
                pSlot->byte_count += strlen(dir->d_name) + 1;
                }
            }
        (void) closedir(dirp);
        }

    if (dout == NULL)
        reply(pSlot, 550, "No files found.");
    else if (ferror(dout) != 0)
        perror_reply(pSlot, 550, "Data connection");
    else
        reply(pSlot, 226, "Transfer complete.");

    pSlot->transflag = 0;
    if (dout != NULL)
        (void) fclose(dout);
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);

send_file_list_done:

    free (searchBuf);
    }

/*******************************************************************************
*
* ftpd6DirListGet - list files in a directory for FTP client
*
* This function performs the client's request to list out all the files in a
* given directory.  The VxWorks implementation of stat() does not work on RT-11
* filesystem drivers, it is simply not supported (RT-11 is not supported on 
* VxWorks 6.x).
*
* This command is similar to UNIX ls.  It lists the contents of a directory
* in one of two formats.  If <doLong> is FALSE, only the names of the files
* (or subdirectories) in the specified directory are displayed.  If <doLong>
* is TRUE, then the file name, size, date, and time are displayed.  If
* doing a long listing, any entries that describe subdirectories will also
* be flagged with a "DIR" comment.
*
* The <dirName> parameter specifies the directory to be listed.  If
* <dirName> is omitted or NULL, the current working directory will be
* listed.
*
* The <workBuf> argument provides a required scratch buffer which
* must be at least 2 * MAX_FILENAME_LENGTH in length.
*
* Empty directory entries and MS-DOS volume label entries are not
* reported.
*
* INTERNAL
* Borrowed from ls() in usrLib.c.
*
* RETURNS:  OK or ERROR.
*
* SEE ALSO: ls(), stat()
*
* \NOMANUAL
*/

void ftpd6DirListGet
    (
    FTPD6_SESSION_DATA * pSlot,    /* pointer to the active slot to be handled */
    char               * dirNameIn,/* name of the directory to be listed */
    BOOL                 doLong,   /* if TRUE, do long listing */
    char	       * workBuf   /* work buffer area */
    )
    {
    FAST STATUS        status;      /* return status */
    FAST DIR         * pDir;        /* ptr to directory descriptor */
    FAST struct dirent *pDirEnt;    /* ptr to dirent */
    struct stat        fileStat;    /* file status info    (long listing) */
    struct tm          fileDate;    /* file date/time      (long listing) */
    char             * pDirComment; /* dir comment         (long listing) */
    BOOL               firstFile;   /* first file flag     (long listing) */
    int                sd;
    char	     * fileName;
    char	     * dirName;

                    /* buffer for building file name */
    static char     *monthNames[] = {"???", "Jan", "Feb", "Mar", "Apr",
                                     "May", "Jun", "Jul", "Aug", "Sep",
                                     "Oct", "Nov", "Dec"};
    FILE *        dout = NULL;
    
    fileName = workBuf;
    dirName = workBuf + MAX_FILENAME_LENGTH;

    /* If no directory name specified, use "." */
    
    if (dirNameIn == NULL)
        dirNameIn = ".";

    if (ftpd6FullPath (pSlot, dirNameIn, dirName, FALSE) != OK)
        {
        reply(pSlot, 553, "Directory path invalid.");
        return;
        }
    
#ifdef _WRS_KERNEL
    if (strcmp (dirName, "/") == 0)
        {
	ftpd6RootDevicesList (pSlot);
        return;
	}
#endif

    dout = dataconn(pSlot, "file list",  (off_t)-1, "w");
    if (dout == NULL) 
        return ;
  
    sd = fileno(dout);
 
    /* Open dir */

    if ((pDir = opendir (dirName)) == NULL)
        {
        fdprintf (sd, "Can't open \"%s\".%s\n", dirName, pSlot->type == TYPE_A ? "\r" : "");
        perror_reply(pSlot, 550, "No files found or invalid directory or permission problem");
        (void) fclose(dout);
        ftpd6SockFree (&pSlot->data);
        ftpd6SockFree (&pSlot->pdata);
        return ;
        }

    /* List files */

    status = OK;
    firstFile = TRUE;

    do
        {
        errnoSet(OK);            /* error -> errorSet () */

        pDirEnt = readdir (pDir);

        if (pDirEnt != NULL)
            {
            if (doLong)                /* if doing long listing */
                {
                if (firstFile)
                    {
                    if (fdprintf (sd, 
                            "  size          date       time       name%s\n", 
                           pSlot->type == TYPE_A ? "\r" : "")==ERROR)
                        {
                        closedir (pDir);
                        perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");
                        (void) fclose(dout);
                        ftpd6SockFree (&pSlot->data);
                        ftpd6SockFree (&pSlot->pdata);
                        return ;
                        }    
                    
                    if (fdprintf (sd,
                            "--------       ------     ------    --------%s\n", 
                                  pSlot->type == TYPE_A ? "\r" : "")==ERROR)
                        {
                        closedir (pDir);
                        perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");
                        (void) fclose(dout);
                        ftpd6SockFree (&pSlot->data);
                        ftpd6SockFree (&pSlot->pdata);
                        return;
                        }

                    firstFile = FALSE;
                    }

                /* Construct path/filename for stat */

                (void) PATH_CAT (dirName, pDirEnt->d_name, fileName);
                
                /* Get and print file status info */
                
                if (stat (fileName, &fileStat) != OK)
                    {
                    status = ERROR;
                    break;
                    }

                /* Break down file modified time */

                localtime_r ((time_t *)&fileStat.st_mtime, &fileDate);

                
                if (S_ISDIR (fileStat.st_mode))        /* if a directory */
                    pDirComment = "<DIR>";
                else
                    pDirComment = "";


                if (fdprintf (sd, 
                         "%8lld    %s-%02d-%04d  %02d:%02d:%02d   %-16s  %s%s\n",
                              fileStat.st_size,         /* size in bytes */
                              monthNames [fileDate.tm_mon + 1],/* month */
                              fileDate.tm_mday,        /* day of month */
                              fileDate.tm_year + 1900,    /* year */
                              fileDate.tm_hour,        /* hour */
                              fileDate.tm_min,        /* minute */
                              fileDate.tm_sec,        /* second */
                              pDirEnt->d_name,        /* name */
                              pDirComment,             /* "<DIR>" or "" */
                              pSlot->type == TYPE_A ? "\r" : "") == ERROR)
                    {    
                    closedir (pDir);
                    perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");
                    (void) fclose(dout);
                    ftpd6SockFree (&pSlot->data);
                    ftpd6SockFree (&pSlot->pdata);
                    return;            
                    }
                }
            else                /* just listing file names */
                {
                if (fdprintf (sd, "%s%s\n", pDirEnt->d_name,
                              pSlot->type == TYPE_A ? "\r" : "") == ERROR)
                    {
                    closedir (pDir);
                    perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");
                    (void) fclose(dout);
                    ftpd6SockFree (&pSlot->data);
                    ftpd6SockFree (&pSlot->pdata);
                    return;
                    }
                }
            }
        else                    /* readdir returned NULL */
            {
            if (errnoGet() != OK)        /* if real error, not dir end */                
                {
                if (fdprintf (sd, "error reading entry: %x%s\n", errnoGet(), 
                              pSlot->type == TYPE_A ? "\r" : "") == ERROR) { 
                closedir (pDir);
                perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");
                (void) fclose(dout);
                ftpd6SockFree (&pSlot->data);
                ftpd6SockFree (&pSlot->pdata);
                return;
                }
                status = ERROR;
                }
            }

        } while (pDirEnt != NULL);        /* until end of dir */


    fdprintf (sd, "%s\n", pSlot->type == TYPE_A ? "\r" : "");

    /* Close dir */
    if (closedir(pDir) == ERROR)
        status = ERROR;
   
    if (status == OK) 
        reply(pSlot, 226, "Transfer complete.");    
    else 
        perror_reply(pSlot, 550,
                  "No files found or invalid directory or permission problem");

    (void) fclose(dout);
    ftpd6SockFree (&pSlot->data);
    ftpd6SockFree (&pSlot->pdata);
    return ;
    }

/*******************************************************************************
*
* ftpDirListPattern - match file name pattern with an actual file name
*
* RETURNS: TRUE or FALSE 
*
* ERRNO: N/A
*
* \NOMANUAL
*
* INTERNAL:
* <pat> is a pattern consisting with '?' and '*' wildcard characters,
* which is matched against the <name> filename, and TRUE is returned
* if they match, or FALSE if do not match. Both arguments must be
* null terminated strings.
* The pattern matching is case insensitive.
*
* NOTE: we're using EOS as integral part of the pattern and name.
*/
LOCAL BOOL ftpDirListPattern
    (
    char * pat,
    char * name
    )
    {
    FAST char * pPat, *pNext ;
    const char anyOne = '?' ;
    const char anyMany = '*' ;
    const char *anyManyStr = "*";
    
    pPat = pat ;

    /* if the pattern string is "*" then match everything except
     * anything starting with a '.'
     */
    if ((strcmp (pPat, anyManyStr) == 0) && (name[0] != '.'))
       return TRUE;

    for (pNext = name ; * pNext != EOS ; pNext ++)
        {
        /* DEBUG-  logMsg ("pattern %s, name %s\n", pPat, pNext, 0,0,0,0);*/
        if (*pPat == anyOne)
            {
            pPat ++ ;
            }
        else if (*pPat == anyMany)
            {
            if (pNext[0] == '.')        /* '*' dont match . .. and .xxx */
                return FALSE ;
            if (toupper ((int)pPat[1]) == toupper ((int)pNext[1]))
                pPat ++ ;
            else if (toupper ((int)pPat[1]) == toupper ((int)pNext[0]))
                pPat += 2 ;
            else
                continue ;
            }
        else if (toupper ((int)*pPat) != toupper ((int)*pNext))
            {
            return FALSE ;
            }
        else
            {
            pPat ++ ;
            }
        }
 
    /* loop is done, let's see if there is anything left */
 
    if ((*pPat == EOS) || (pPat[0] == anyMany && pPat[1] == EOS))
        return TRUE ;
    else
        return FALSE ;
    }
/*******************************************************************************
*
* ftpd6EnableSecurity - Enable security so user logins are authenticated
*
* This function instructs the FTP server to perform user authentication using
* the system password database. Once the security is enabled, only authenticated
* users will be allowed to login into the FTP server.
*
* To add users to the password database, refer to loginLib documentation.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* SEE ALSO: loginLib
*
*/
void ftpd6EnableSecurity (void)
    {
    ftpd6SecurityEnabled = TRUE;
    }

/*******************************************************************************
*
* ftpd6DisableSecurity - Disable security so user logins are not authenticated
*
* This function instructs the FTP server to not perform user authentication 
* using the system password database. Once the security is disabled, all users
* will be allowed to login into the FTP server without userid or password
* validation.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/
void ftpd6DisableSecurity (void)
    {
    ftpd6SecurityEnabled = FALSE;
    }

/*******************************************************************************
*
* ftpdInit - initialize the FTP server task
*
* This routine is provided for backward-compatibility for users of the now
* obsolete ftpdLib. It is a wrapper to ftpd6Init().
*
* \NOMANUAL
*/

STATUS ftpdInit
    (
    FUNCPTR     pLoginFunc,     /* Pointer to a login function or NULL */
    int         stackSize       /* task stack size, or 0 for default */
    )
    {
    return (ftpd6Init ("-4", pLoginFunc, stackSize));
    }

/*******************************************************************************
*
* ftpdDelete - terminate the FTP server task
*
* This routine is provided for backward-compatibility for users of the now
* obsolete ftpdLib. It is a wrapper for ftpd6Delete().
*
* \NOMANUAL
*/

STATUS ftpdDelete (void)
    {
    return (ftpd6Delete ());
    }

/*******************************************************************************
*
* ftpdAnonymousAllow - allow guest login to FTP server
*
* This routine is provided for backward-compatibility for users of the now
* obsolete ftpdLib. It is a wrapper for ftpd6GuestAllow().
*
* \NOMANUAL
*/

STATUS ftpdAnonymousAllow
    (
    const char * rootDir,  /* path for guest root directory */
    const char * uploadDir /* path for guest uploads */
    )
    { 
    return (ftpd6GuestAllow (rootDir, uploadDir, FALSE));
    }

/*******************************************************************************
*
* ftpdEnableSecurity - enable security so that all user logins are authenticated
*
* This routine is provided for backward-compatibility for users of the now
* obsolete ftpdLib. It is a wrapper for ftpd6EnableSecurity().
*
* \NOMANUAL
*/

void ftpdEnableSecurity (void)
    {
    ftpd6EnableSecurity ();
    return;
    }

/*******************************************************************************
*
* ftpdDisableSecurity - Disable security so user logins are not authenticated
*
* This routine is provided for backward-compatibility for users of the now
* obsolete ftpdLib. It is a wrapper for ftpd6DisableSecurity().
*
* \NOMANUAL
*/

void ftpdDisableSecurity (void)
    {
    ftpd6DisableSecurity ();
    return;
    }

