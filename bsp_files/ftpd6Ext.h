/* ftpd6Ext.h - File Transfer Protocol (FTP) Server library */

/* Copyright 2001 - 2005 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,02jan05,dlk  Decrease stack usage in directory listing code path
		 (SPR #116334).
01m,16feb05,vvv  removed inetLib.h include
01l,05jan05,syy  Port to RTP
01k,28dec04,syy  Moved over macros for path lib
01j,07dec04,syy  Fixed SPR#104035: add support for guest login
01i,23aug04,vvv  merged from COMP_WN_IPV6_BASE6_ITER5_TO_UNIFIED_PRE_MERGE
01h,16apr04,nee  fix header path strnicmp.h
01g,17mar04,spm  merged from Orion for MSP 3.0 baseline
01f,02feb04,pas  fixed header paths for base6
01e,06nov03,rlm  Ran batch header update for header re-org.
01d,06jun02,ant  function renamefrom changed, field fromname in the struct
                 FTPD6_SESSION_DATA changed.
01c,08may02,ppp  Merging teamf1 T202 changes into clarinet
01b,15jan02,ant  file clean up, sockunion moved to apps/common/sockunion.h
01a,14sep01,ant  written
*/
  
#ifndef __INCftpd6Exth
#define __INCftpd6Exth
 
#ifdef __cplusplus
extern "C" {
#endif

/* includes */

#include "taskLib.h"
#include <errnoLib.h>
#include "string.h"
#include "dirent.h"
#include "sys/stat.h"
#include <sockLib.h>
#include <ioLib.h>    
#include "sysLib.h"
#include "time.h"
#include "loginLib.h"
#include <netinet/ip.h>
#include <netinet/tcp.h>
#ifdef _WRS_KERNEL    
#include "version.h"
#include "wdLib.h"
#else   /* RTP */
#include <stdio.h>
#endif /* _WRS_KERNEL */
#include <hostLib.h>
#include "signal.h"
#include <applUtilLib.h>
#include <netinet/sockunion.h>

    
/* defines */

#define TYPE_A	1
#define TYPE_E	2
#define TYPE_I	3
#define TYPE_L	4

#define FORM_N	1
#define FORM_T	2
#define FORM_C	3

#define STRU_F	1
#define STRU_R	2
#define STRU_P	3

#define MODE_S	1
#define MODE_B	2
#define MODE_C	3

#define IPv4_AF 4		/* IPv4 address family */
#define IPv6_AF 6		/* IPv6 address family */
#define IPv4_AF_NUMBER 1	/* IPv4 address family number, RFC 1700 */
#define IPv6_AF_NUMBER 2	/* IPv6 address family number, RFC 1700 */
#define PORT_ADDR_LENGTH 2	/* port address length */

/* typedefs */

typedef struct
    {
    NODE	node;			/* for link-listing */
    int		status;			/* see various status bits above */
    int		byteCount;		/* bytes transferred */
    int		cmdSock;		/* command socket */
    STATUS		cmdSockError;   /* Set to ERROR on write error */
    union sockunion 	ctrl_addr;      /* address of control connection */
    union sockunion 	data_source;	/* local address of data connection */
    union sockunion 	data_dest;	/* peer address of data connection */
    union sockunion 	his_addr;	/* peer address */
    union sockunion 	pasv_addr;	/* pasive address of data connection (ctrl_addr)*/
    int		type;			/* TYPE*/
    int		form;			/**/
    int		stru;			/* STRU avoid C keyword */
    int		mode;			/* MODE*/
    int		data;			/* when PORT command received */
    int		pdata;			/* for passive mode */
    int		logged_in;		/* flag user has logged in*/
    int		epsvall;		/* set after EPSV ALL*/
    int		usedefault;		/* use his_addr for data_dest*/
    off_t	restart_point;		/* restart point for data transfer*/
    off_t	byte_count;		/* count of transfered bytes*/
    sig_atomic_t  	transflag;		/* the state of transfet 0/1*/
    char	remotehost[MAXHOSTNAMELEN]; /* remote host name*/
    int 	askpasswd;		/* had user command, ask for passwd */
    int		guest;			/* user has logged as with "ftp" or "anonymous"*/
    char	fromname[MAX_FILENAME_LENGTH];	/* used with RNFR and RNTO*/
    int		timeout;    		/* timeout after 15 minutes of inactivity*/ 
    int		login_attempts;		/* number of failed login attempts */
#ifdef _WRS_KERNEL
    WDOG_ID	wdTimeout;		/* Client inactivity measurement*/
#else /* RTP */
    timer_t     timoTimer;              /* Client session timo timer */
#endif  /* _WRS_KERNEL */
    SEM_ID	ftpsTimeoutSem;		/* Client inactivity measurement*/
    int		timeoutTask;		/* Client inactivity measurement*/
    char	buf [BUFSIZ]; 		/* multi-purpose buffer per session */
    char 	curDirName [MAX_FILENAME_LENGTH]; /* active directory */
    char        user [MAX_LOGIN_NAME_LEN+1];  /* current user */
    BOOL	sessionShutDown;	/* idle, timeout of ftpd6WorkTask */
    } FTPD6_SESSION_DATA;

/* function declarations */

void	cwd (FTPD6_SESSION_DATA *, char *);
void	deletefile (FTPD6_SESSION_DATA *, char *);
void	dologout (FTPD6_SESSION_DATA *);
void	ftpd6Fatal (FTPD6_SESSION_DATA *, char *);
void	lreply (FTPD6_SESSION_DATA *, int, const char *, ...);
void 	replyNopSlot (int, int, const char *, ...);
void	makedir (FTPD6_SESSION_DATA *, char *);
void	ack (FTPD6_SESSION_DATA *, char *);
void	nack (FTPD6_SESSION_DATA *, char *);
void	passive (FTPD6_SESSION_DATA *);
void	long_passive (FTPD6_SESSION_DATA *, char *, int);
void	perror_reply (FTPD6_SESSION_DATA *, int, char *);
void	removedir (FTPD6_SESSION_DATA *, char *);
void	renamecmd (FTPD6_SESSION_DATA *, char *, char *);
void  	renamefrom (FTPD6_SESSION_DATA *, char *);
void	reply (FTPD6_SESSION_DATA *, int, const char *, ...);
void	retrieve (FTPD6_SESSION_DATA *, char *, char *);
void	send_file_list (FTPD6_SESSION_DATA *, char *);
void	statcmd (FTPD6_SESSION_DATA *);
void	store (FTPD6_SESSION_DATA *, char *, char *, int);
int	ftpd6Parse (FTPD6_SESSION_DATA *);
void    guestPathReset (void);

#ifndef VIRTUAL_STACK
void toolong (FTPD6_SESSION_DATA *);
#else
void toolong (FTPD6_SESSION_DATA *pSlot,int stackNum);
#endif /* VIRTUAL_STACK */

void	ftpd6DirListGet (FTPD6_SESSION_DATA *, char *, BOOL, char *);
STATUS  ftpd6FullPath (FTPD6_SESSION_DATA *, char *, char *, BOOL);
    
#define FTPD6_MALLOC         malloc
#define FTPD6_FREE           free    

#ifdef _WRS_KERNEL
#define PATH_CAT             pathCat
#define PATH_CONDENSE        pathCondense
#define PATH_IS_SUB_DIR      pathIsSubDir    
#else
#define PATH_CAT             pathUtilCat
#define PATH_CONDENSE        pathUtilCondense
#define PATH_IS_SUB_DIR      pathUtilIsSubDir    
extern  void  ftpd6TimerHandler (timer_t, int);
#endif /* _WRS_KERNEL */
    
#ifdef __cplusplus
}
#endif

#endif /* __INCftpd6Exth */
