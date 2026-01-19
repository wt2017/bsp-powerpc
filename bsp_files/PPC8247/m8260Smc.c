/* m8260Smc.c - Motorola MPC8260 Serial Management Controller c file */

#include "vxWorks.h"
#include "intLib.h"
#include "errno.h"
#include "sioLib.h"
#include "m8260Smc.h"
#include "drv/sio/m8260Cp.h"
#include "drv/intrCtl/m8260IntrCtl.h"

/* defines */

static STATUS m8260SmcIoctl (M8260_SMC_CHAN *pSmcChan,int request,int arg);
static int    m8260SmcPollOutput (SIO_CHAN *pSmcChan,char);
static int    m8260SmcPollInput (SIO_CHAN *pSmcChan,char *);
static int   m8260SmcStartup (M8260_SMC_CHAN *pSmcChan);
static int    m8260SmcCallbackInstall (SIO_CHAN *pSmcChan, int, STATUS (*)(), void *);

void   m8260SmcResetChannel (M8260_SMC_CHAN *pSmcChan);

static SIO_DRV_FUNCS m8260SmcDrvFuncs =
    {
    (int (*)())			m8260SmcIoctl,
    (int (*)())			m8260SmcStartup,
    (int (*)())			m8260SmcCallbackInstall,
    (int (*)())			m8260SmcPollInput,
    (int (*)(SIO_CHAN *,char))	m8260SmcPollOutput
    };

/*******************************************************************************
*
* m8260SmcDevInit - initialize the SMC
*
* This routine is called to initialize the chip to a quiescent state.

*/

void m8260SmcDevInit
    (
    M8260_SMC_CHAN *pSmcChan
    )
    {
    UINT32  immrVal = pSmcChan->immrVal;    /* holder for the immr value */
    UINT8   smcNum  = pSmcChan->sccNum;     /* holder for the smc number */
    UINT8   smc     = smcNum - 1;          	/* a convenience */
    VINT8   *pSMCM  = (VINT8 *) (immrVal + M8260_SMCM + 
                                 (smc * M8260_SMC_OFFSET_NEXT_SMC));

    if (smcNum > N_SIO_CHANNELS)	
	    return;
    /*
     * Disable receive and transmit interrupts.
     * Always flush cache pipe before first read
     */

    CACHE_PIPE_FLUSH ();
	
    *pSMCM &= ~(M8260_SMCM_UART_RX | M8260_SMCM_UART_TX);

    pSmcChan->baudRate  = CONSOLE_BAUD_RATE ;
    pSmcChan->pDrvFuncs = &m8260SmcDrvFuncs;
	
    }


/*******************************************************************************
*
* m8260SmcResetChannel - initialize an SMC channel
*
* This routines initializes an SMC channel. Initialized are:
* 
* .CS
*        CPCR
*        BRGC   Baud rate
*        Buffer Descriptors
*        RBASE field in SMC Parameter RAM
*        TBASE field in SMC Parameter RAM
*        RFCR  field in SMC Parameter RAM
*        TFCR  field in SMC Parameter RAM
*        MRBLR field in SMC Parameter RAM
*
* RETURNS: NA
*
* .CE
*/

void m8260SmcResetChannel 
    (
    M8260_SMC_CHAN *pSmcChan
    )
    {
    int     ix = 0;
    int     oldlevel; 
    UINT32  immrVal = pSmcChan->immrVal;    /* holder for the immr value */
    UINT8   smcNum = pSmcChan->sccNum;      /* holder for the fcc number */
    UINT8   smc = smcNum - 1;            	/* a convenience */
    VINT32  cpcrVal = 0;            		/* a convenience */
 
    VINT16  *pRBASE,*pTBASE,*pMRBLR,*pMAX_IDL,*pBRKLN,*pBRKCR,*pBRKEC,*pSMCBASE;
    VINT8   *pRFCR,*pTFCR;
    
    VINT32  *pBRGC = (VINT32 *) (immrVal + M8260_BRGC1_ADRS + (smc * M8260_BRGC_OFFSET_NEXT_BRGC));
    VINT16 *pSMCMR = (VINT16 *) (immrVal + M8260_SMCMR + smc * M8260_SMC_OFFSET_NEXT_SMC );
    VINT8  *pSMCM  = (VINT8 *) (immrVal + M8260_SMCM  + (smc * M8260_SMC_OFFSET_NEXT_SMC));
        
    pSMCBASE = (VINT16 *)(M8260_SMC_BASE_REG + (smc * 0x100));   
    *pSMCBASE = SMC_PARAM_OFF + (smc * 0x80);   
	
    pRBASE   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_RBASE);    
    pTBASE   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_TBASE);  
    pRFCR    = (VINT8 *) (immrVal +  *pSMCBASE + M8260_SMC_PRAM_RFCR);    
    pTFCR    = (VINT8 *)  (immrVal + *pSMCBASE + M8260_SMC_PRAM_TFCR);
    pMRBLR   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_MRBLR);    
    pMAX_IDL = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_MAXIDL);    
    pBRKLN   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_BRKLN);
    pBRKCR   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_PRAM_BRKCR);
    pBRKEC   = (VINT16 *) (immrVal + *pSMCBASE + M8260_SMC_UART_PRAM_BRKEC); 

    oldlevel = intLock ();	/* lock interrupts */ 

    CACHE_PIPE_FLUSH ();

    /* wait until the CP is clear */

    do
    {
	M8260_SCC_32_RD((M8260_CPCR (immrVal)), cpcrVal);

    if (ix++ == M8260_CPCR_LATENCY)
            break;
    } while (cpcrVal & M8260_CPCR_FLG);

    if (ix >= M8260_CPCR_LATENCY)
    {
        /* what to do, other than log an error? */
           logMsg("m8260Smc.c CPCR command exec. time out.\n",0,0,0,0,0,0);
    }

    /* Stop transmitting on SMC, if doing so */

    cpcrVal = (M8260_CPCR_OP (M8260_CPCR_TX_STOP)
               | M8260_CPCR_SBC (M8260_CPCR_SBC_SMC1   + smc * 0x1)
               | M8260_CPCR_PAGE (M8260_CPCR_PAGE_SMC1 + smc * 0x1)
               | M8260_CPCR_FLG);

    M8260_SCC_32_WR (M8260_CPCR (immrVal), cpcrVal);

    /* flush cache pipe when switching from write to read */

    CACHE_PIPE_FLUSH ();

   /* wait until the CP is clear */

    ix = 0;
    do
        {
        M8260_SCC_32_RD((M8260_CPCR (immrVal)), cpcrVal);

        if (ix++ == M8260_CPCR_LATENCY)
            break;
        } while (cpcrVal & M8260_CPCR_FLG);

    if (ix >= M8260_CPCR_LATENCY)
        {

        /* what to do, other than log an error? */
           logMsg("m8260Smc.c CPCR command exec. time out.\n",0,0,0,0,0,0);

        }

    /* set up SMC as NMSI, select Baud Rate Generator */
 
    /* reset baud rate generator, wait for reset to clear... */
 
     *pBRGC |= M8260_BRGC_RST;

    /* flush cache pipe when switching from write to read */

    CACHE_PIPE_FLUSH ();

    while ((*pBRGC) & M8260_BRGC_RST);

    m8260SmcIoctl (pSmcChan, SIO_BAUD_SET, pSmcChan->baudRate);

    /* set up the RECEIVE buffer descriptors */

    /* buffer status/control */
    M8260_SCC_16_WR((pSmcChan->pBdBase + 
                     M8260_SMC_RCV_BD_OFF + 
                     M8260_SMC_BD_STAT_OFF), 0xB000);  /*UM 26-14*/
    CACHE_PIPE_FLUSH ();

    /* buffer length */
    M8260_SCC_16_WR((pSmcChan->pBdBase + 
                     M8260_SMC_RCV_BD_OFF + 
                     M8260_SMC_BD_LEN_OFF), 0x0001);
                     
    CACHE_PIPE_FLUSH ();

    /* buffer address */

    M8260_SCC_32_WR((pSmcChan->pBdBase + 
                     M8260_SMC_RCV_BD_OFF + 
                     M8260_SMC_BD_ADDR_OFF), pSmcChan->rcvBufferAddr);

    /* set the SMC parameter ram field RBASE */

    * pRBASE = (SMC1_BD_OFF + M8260_SMC_RCV_BD_OFF) + (0x80 * smc); 

    /* set up the TRANSMIT buffer descriptors */

    /* 
     * buffer status/control;
     * not ready, Wrap, Bit Clear-to-send_report, Interrupt 
     */

    M8260_SCC_16_WR((pSmcChan->pBdBase + 
                     M8260_SMC_TX_BD_OFF + 
                     M8260_SMC_BD_STAT_OFF), 0xB000);

    /* buffer length */

    M8260_SCC_16_WR((pSmcChan->pBdBase + 
                     M8260_SMC_TX_BD_OFF + 
                     M8260_SMC_BD_LEN_OFF), 0x0001);

    /* buffer address */

    M8260_SCC_32_WR((pSmcChan->pBdBase + 
                     M8260_SMC_TX_BD_OFF + 
                     M8260_SMC_BD_ADDR_OFF), pSmcChan->txBufferAddr);

    /* set the SMC parameter ram field TBASE */

    * pTBASE =  (SMC1_BD_OFF + M8260_SMC_TX_BD_OFF) + (0x80 * smc) ;

    /* disable transmit and receive interrupts */

    * pSMCM &= ~(M8260_SMCM_UART_RX | M8260_SMCM_UART_TX);

    /* set SMC attributes to standard UART mode */

    *pSMCMR = (M8260_SMCMR_CLEN_STD  | \
               M8260_SMCMR_STOPLEN_1 | \
               M8260_SMCMR_NO_PARITY | \
               M8260_SMCMR_UART      | \
               M8260_SMCMR_NORM) & ~(M8260_SMCMR_ENT | M8260_SMCMR_ENR);

    /* initialize parameter RAM area for this SMC */

    * pRFCR = 0x18;	/* function code.supervisor data access */

    * pTFCR = 0x18;/* function code.supervisor data access */

    * pMRBLR = 0x0001;

    /* initialize some unused parameters in SCC parameter RAM */

    * pMAX_IDL = 0x0;
    * pBRKCR   = 0x0001;
    * pBRKLN   = 0x0;
    * pBRKEC   = 0x0;

    CACHE_PIPE_FLUSH ();

   /* wait until the CP is clear */
    ix = 0;
    do
        {
        M8260_SCC_32_RD((M8260_CPCR (immrVal)), cpcrVal);

        if (ix++ == M8260_CPCR_LATENCY)
            break;
        } while (cpcrVal & M8260_CPCR_FLG);

    if (ix >= M8260_CPCR_LATENCY)
        {

        /* what to do, other than log an error? */

        }

    /* Tell CP to initialize tx and rx parameters for SMC */

    cpcrVal = (M8260_CPCR_OP (M8260_CPCR_RT_INIT)
               | M8260_CPCR_SBC (M8260_CPCR_SBC_SMC1   + smc * 0x1)
               | M8260_CPCR_PAGE (M8260_CPCR_PAGE_SMC1 + smc * 0x1)
               | M8260_CPCR_FLG);

    M8260_SCC_32_WR (M8260_CPCR (immrVal), cpcrVal);

    CACHE_PIPE_FLUSH ();

    /* wait until the CP is clear */

    ix = 0;
    do
        {
        M8260_SCC_32_RD((M8260_CPCR (immrVal)), cpcrVal);

        if (ix++ == M8260_CPCR_LATENCY)
            break;
        } while (cpcrVal & M8260_CPCR_FLG) ;

    if (ix >= M8260_CPCR_LATENCY)
        {

        /* what to do, other than log an error? */
           logMsg("m8260Sio.c CPCR command exec time out.\n",0,0,0,0,0,0);
        }

    CACHE_PIPE_FLUSH ();

    /* lastly, enable the transmitter and receiver  */

    
    *pSMCMR |= (M8260_SMCMR_ENT | M8260_SMCMR_ENR);
        
    CACHE_PIPE_FLUSH ();

    intUnlock (oldlevel);			/* UNLOCK INTERRUPTS */
    }

/*******************************************************************************
*
* m8260SmcIoctl - special device control
*
* Allows the caller to get and set the buad rate; to get and set the mode;
* and to get the allowable modes.
*
* RETURNS: OK on success, EIO on device error, ENOSYS on unsupported
*          request.
*
*/

LOCAL STATUS m8260SmcIoctl
    (
    M8260_SMC_CHAN *	pSmcChan,	/* device to control */
    int			request,	        /* request code */
    int			arg		            /* some argument */
    )
    {
    int 	oldlevel;
    STATUS 	status = OK;
    UINT8   smcNum = pSmcChan->sccNum;      /* holder for the fcc number */
    UINT8   smc    = smcNum - 1;            /* a convenience */
    UINT32  immrVal = pSmcChan->immrVal;    /* holder for the immr value */	
    	
    int		prescale;
    VINT32 *pBRGC = (VINT32 *) (immrVal + M8260_BRGC1_ADRS + 
                               (smc * M8260_BRGC_OFFSET_NEXT_BRGC));

    VINT8  *pSMCM = (VINT8 *) (immrVal + M8260_SMCM + smc * M8260_SMC_OFFSET_NEXT_SMC );

    switch (request)
	{
	case SIO_BAUD_SET:
	    if (arg >=  300 && arg <= 38400)     /* could go higher... */
        {
	/*((CPM FRQ*2)/16)=BAUD_RATE * (BRGCx[DIV16])*(BRGCx[CD] + 1)*(GSMRx_L[xDCR])*/
	/*BRGC[CD] value X = (CPM_FRQ*2)/16/16/19200-1) */
		prescale = (CPM_FRQ*2)/16/16/ arg;     /* calculate the prescaler value */
		prescale--;		                /* add 1 as it counts to zero */
		
		* pBRGC = M8260_BRGC_EN | (prescale << 1);
		pSmcChan->baudRate = arg;
        }
        else
	        status = EIO;
	    break;
    
	case SIO_BAUD_GET:
	    * (int *) arg = pSmcChan->baudRate;
	    break;

	case SIO_MODE_SET:
            if (!((int) arg == SIO_MODE_POLL || (int) arg == SIO_MODE_INT))
            {
                status = EIO;
                break;
            }

            /* lock interrupt  */

            oldlevel = intLock();

            /* initialize channel on first MODE_SET */

            if (!pSmcChan->channelMode)
                m8260SmcResetChannel(pSmcChan);

            
            if (arg == SIO_MODE_INT)
            	{

		 /* enable SMC1 or SMC2 interrupts at the SIU Interrupt Controller */
		 if (smc == 0) 
	            m8260IntEnable(INUM_SMC1);
         
                 /* enable receive and transmit interrupts at the smc */
         
            	 * pSMCM |= (M8260_SMCM_UART_RX | M8260_SMCM_UART_TX);
		         CACHE_PIPE_FLUSH ();
		       }
            else
		    {
		     /* disable transmit and receive interrupts */

		     * pSMCM &= ~(M8260_SMCM_UART_RX | M8260_SMCM_UART_TX);

		     CACHE_PIPE_FLUSH ();

		     /* mask off this SMC's interrupt */ 

             if (smc == 0)
                m8260IntDisable(INUM_SMC1);

 
             CACHE_PIPE_FLUSH ();
            }
            pSmcChan->channelMode = arg;

            intUnlock(oldlevel);

            break;

        case SIO_MODE_GET:
            * (int *) arg = pSmcChan->channelMode;
	    break;

        case SIO_AVAIL_MODES_GET:
            *(int *)arg = SIO_MODE_INT | SIO_MODE_POLL;
	    break;

	default:
	    status = ENOSYS;
	}

    return (status);
    }

/*******************************************************************************
*
* m8260SmcInt - handle an SMC interrupt
*
* This routine is called to handle SMC interrupts.
*
* RETURNS: NA
*/

void m8260SmcInt
    (
    M8260_SMC_CHAN *pSmcChan
    )
    {
    char		outChar;
    VINT16      bdStatus;                       /* holder for the BD status */
    UINT8       smcNum = pSmcChan->sccNum;      /* holder for the smc number */
    UINT8       smc = smcNum - 1;            	/* a convenience */
    UINT32      immrVal = pSmcChan->immrVal;    /* holder for the immr value */

    VINT8 *pSMCE = (VINT8 *) (immrVal + M8260_SMCE + (smc * M8260_SMC_OFFSET_NEXT_SMC));

    CACHE_PIPE_FLUSH ();

    /* check for a receive event */

    if (* pSMCE & M8260_SMCM_UART_RX )
	{

	/*
	 * clear receive event bit by setting the bit in the event register.
	 * This also clears the bit in SIPNR
	 */

       * pSMCE = M8260_SMCE_UART_RX; 
	CACHE_PIPE_FLUSH ();

	/* 
	 * as long as there is a character: 
	 * Inspect bit 0 of the status word which is the empty flag:
	 * 0 if buffer is full or if there was an error
	 * 1 if buffer is not full
	 */

	M8260_SCC_16_RD((pSmcChan->pBdBase +
			 M8260_SMC_RCV_BD_OFF +
			 M8260_SMC_BD_STAT_OFF), bdStatus);

        /* if bit is set there is nothing */

	while (!(bdStatus & M8260_SMC_UART_RX_BD_EMPTY)) 
	    {
	    M8260_SCC_8_RD(pSmcChan->rcvBufferAddr, outChar);

            /*
             * indicate that we've processed this buffer; set empty
             * flag to 1 indicating that the buffer is empty
             */

	    M8260_SCC_16_WR((pSmcChan->pBdBase +
                     M8260_SMC_RCV_BD_OFF +
                     M8260_SMC_BD_STAT_OFF), 
                     bdStatus |= M8260_SMC_UART_RX_BD_EMPTY);

            /* necessary when switching from write to read */

	    CACHE_PIPE_FLUSH ();

            /* pass the received character up to the tty driver */

	    (*pSmcChan->putRcvChar) (pSmcChan->putRcvArg,outChar);

            /* if driver is in polled mode, we're done */

	    if (pSmcChan->channelMode == SIO_MODE_POLL)
	    	break;

	        /*
             * If interrupt mode, read the status again;
             * it's possible a new char has arrived
             */

	    M8260_SCC_16_RD((pSmcChan->pBdBase +
			     M8260_SMC_RCV_BD_OFF +
			     M8260_SMC_BD_STAT_OFF), bdStatus);
	    }
	}

    /* check for a transmit event and if a character needs to be output */

    /* transmit event */

    CACHE_PIPE_FLUSH ();

    if ((* pSMCE & M8260_SMCM_UART_TX ) && 
        (pSmcChan->channelMode != SIO_MODE_POLL)) /* ...and we're not polling */
	{

	/*
     * clear transmit event bit by setting the bit in the event register.
     * This also clears the bit in SIPNR
     */

        * pSMCE = M8260_SMCE_UART_TX; 

	CACHE_PIPE_FLUSH ();
    
        if ((*pSmcChan->getTxChar) (pSmcChan->getTxArg, &outChar) == OK)
            {
	    CACHE_PIPE_FLUSH ();

	    /* wait for the ready bit to be 0 meaning the buffer is free */

	    do
		{
		M8260_SCC_16_RD((pSmcChan->pBdBase +
				 M8260_SMC_TX_BD_OFF +
				 M8260_SMC_BD_STAT_OFF), bdStatus);
		} while (bdStatus & M8260_SMC_UART_TX_BD_READY);

	    M8260_SCC_8_WR(pSmcChan->txBufferAddr, outChar); /* write char */

	    /* set buffer length */

	    M8260_SCC_16_WR((pSmcChan->pBdBase +
			     M8260_SMC_TX_BD_OFF +
			     M8260_SMC_BD_LEN_OFF), 0x0001);

	    /* set ready bit so CPM will process buffer */

	    M8260_SCC_16_WR((pSmcChan->pBdBase +
			     M8260_SMC_TX_BD_OFF +
			     M8260_SMC_BD_STAT_OFF), 
                 bdStatus |= M8260_SMC_UART_TX_BD_READY);

	    CACHE_PIPE_FLUSH ();
	    }
        }

    /*
     * acknowledge all other interrupts - ignore events. This also clears
     * the bit in SIPNR
     */

    * pSMCE &= ~(M8260_SMCE_UART_TX | M8260_SMCE_UART_RX);

    CACHE_PIPE_FLUSH ();

   }

/*******************************************************************************
*
* m8260SmcStartup - transmitter startup routine
* 
* Starts transmission on the indicated channel
*
* RETURNS: NA
*/
LOCAL int m8260SmcStartup
    (
    M8260_SMC_CHAN *pSmcChan		/* ty device to start up */
    )
    {
    char    outChar;
    UINT16  bdStatus;      /* holder for the BD status */

    if (pSmcChan->channelMode == SIO_MODE_POLL)
	return (ENOSYS);

    /* 
     * check if a transmit buffer is ready and if a character needs to be 
     * output 
     */

    CACHE_PIPE_FLUSH ();                /* before first read */

    M8260_SCC_16_RD((pSmcChan->pBdBase +
		     M8260_SMC_TX_BD_OFF +
		     M8260_SMC_BD_STAT_OFF), bdStatus);

    if (!(bdStatus & M8260_SMC_UART_TX_BD_READY))
	if ((*pSmcChan->getTxChar) (pSmcChan->getTxArg, &outChar) == OK)
	    {

        /* write char; set length; flag buffer as not empty */

	    M8260_SCC_8_WR(pSmcChan->txBufferAddr, outChar); /* write char */

	    /* set buffer length */
	    M8260_SCC_16_WR((pSmcChan->pBdBase +
			     M8260_SMC_TX_BD_OFF +
			     M8260_SMC_BD_LEN_OFF), 0x0001);

	    /* flag buffer as not empty */

	    M8260_SCC_16_WR((pSmcChan->pBdBase +
			     M8260_SMC_TX_BD_OFF +
			     M8260_SMC_BD_STAT_OFF), bdStatus |= 0x8000);

	    }
    CACHE_PIPE_FLUSH ();

    return (OK);
    }

/******************************************************************************
*
* m8260SmcPollInput - poll the device for input.
*
* Poll the indicated device for input characters
*
* RETURNS: OK if a character arrived, ERROR on device error, EAGAIN
*          if the input buffer is empty.
*/

LOCAL int m8260SmcPollInput
    (
    SIO_CHAN *	pSmccChan,
    char *	thisChar
    )
    {
    M8260_SMC_CHAN * pSmcChan = (M8260_SMC_CHAN *) pSmccChan;  /*pSmccChan,to avoid duplicate pointer name*/
    UINT8       smcNum = pSmcChan->sccNum;      /* holder for the fcc number */
    UINT8       smc    = smcNum - 1;            	/* a convenience */
    UINT16      bdStatus;                       /* holder for the BD status */
    UINT32      immrVal = pSmcChan->immrVal;    /* holder for the immr value */

    VINT8 *pSMCE = (VINT8 *) (immrVal + M8260_SMCE + (smc * M8260_SMC_OFFSET_NEXT_SMC));

    CACHE_PIPE_FLUSH ();

    /* is there a receive event? */

    if (!(* pSMCE & M8260_SMCM_UART_RX ))
        return (EAGAIN);    /* No more processes */

    /* is the buffer empty? */

    M8260_SCC_16_RD((pSmcChan->pBdBase +
		     M8260_SMC_RCV_BD_OFF +
		     M8260_SMC_BD_STAT_OFF), bdStatus);

    /* if bit is high there is nothing */

    if (bdStatus & M8260_SMC_UART_RX_BD_EMPTY) 
	return (EAGAIN);

    /* get a character */

    M8260_SCC_8_RD(pSmcChan->rcvBufferAddr, * thisChar);

    /* set the empty bit */

    M8260_SCC_16_WR((pSmcChan->pBdBase +
	     M8260_SMC_RCV_BD_OFF +
	     M8260_SMC_BD_STAT_OFF), bdStatus |= M8260_SMC_UART_RX_BD_EMPTY);

    CACHE_PIPE_FLUSH ();

    /* only clear RX event if no more characters are ready */

    M8260_SCC_16_RD((pSmcChan->pBdBase +
		     M8260_SMC_RCV_BD_OFF +
		     M8260_SMC_BD_STAT_OFF), bdStatus);

    CACHE_PIPE_FLUSH ();

    /* if bit is high there is nothing */

    if (bdStatus & M8260_SMC_UART_RX_BD_EMPTY) 
	*pSMCE = M8260_SMCE_UART_RX;  /* clear rx event bit */

    CACHE_PIPE_FLUSH ();

    return (OK);
    }

/******************************************************************************
*
* m8260SmcPollOutput - output a character in polled mode.
*
* Transmit a character in polled mode
*
* RETURNS: OK if a character is sent, ERROR on device error, EAGAIN
*          if the output buffer if full.
*/

static int m8260SmcPollOutput
    (
    SIO_CHAN *	pSmccChan,
    char	outChar
    )
    {
    M8260_SMC_CHAN * pSmcChan = (M8260_SMC_CHAN *) pSmccChan;  /*pSmccChan,to avoid duplicate pointer name*/
    int			i;
    UINT16      bdStatus;                       /* holder for the BD status */
    UINT8       smcNum = pSmcChan->sccNum;      /* holder for the fcc number */
    UINT8       smc = smcNum - 1;            	/* a convenience */
    UINT32      immrVal = pSmcChan->immrVal;    /* holder for the immr value */

    VINT8 *pSMCE = (VINT8 *) (immrVal + M8260_SMCE + 
                             (smc * M8260_SMC_OFFSET_NEXT_SMC));

    CACHE_PIPE_FLUSH ();

    /*
     * wait a bit for the last character to get out
     * because the PPC603 is a very fast processor
     */

    for (i=0; i<M8260_SMC_POLL_OUT_DELAY; i++)
        ;
	
    /*
     * is the transmitter ready yet to accept a character?
     * if still not, we have a problem
     */

    M8260_SCC_16_RD((pSmcChan->pBdBase +
		     M8260_SMC_TX_BD_OFF +
		     M8260_SMC_BD_STAT_OFF), bdStatus);

    if (!(bdStatus & 0x8000))
	     return(EAGAIN);

    /* reset the transmitter status bit */

    /*
     * clear transmit event bit by setting the bit in the event register.
     * This also clears the bit in SIPNR
     */

    * pSMCE = M8260_SMCE_UART_TX; 

    /* write char; set length; flag buffer as not empty */

    M8260_SCC_8_WR(pSmcChan->txBufferAddr, outChar); /* write char */

    /* set buffer length */

    M8260_SCC_16_WR((pSmcChan->pBdBase +
		     M8260_SMC_TX_BD_OFF +
		     M8260_SMC_BD_LEN_OFF), 0x0001);

    /* flag buffer as not empty */

    M8260_SCC_16_WR((pSmcChan->pBdBase +
		     M8260_SMC_TX_BD_OFF +
		     M8260_SMC_BD_STAT_OFF), 
                     bdStatus |= M8260_SMC_UART_TX_BD_READY);
    CACHE_PIPE_FLUSH ();

    return (OK);
    }

/******************************************************************************
*
* m8260SmcCallbackInstall - install ISR callbacks to get and put chars.
*
* Install the indicated ISR callback functions that are used to get and
* put characters
*
* RETURNS: NA
*
*/

static int m8260SmcCallbackInstall
    (
    SIO_CHAN *	pSmccChan,
    int		callbackType,
    STATUS	(* callback)(),
    void *	callbackArg
    )
    {
    M8260_SMC_CHAN * pSmcChan = (M8260_SMC_CHAN *) pSmccChan; /*pSmccChan,to avoid duplicate pointer name*/
    CACHE_PIPE_FLUSH ();

    switch (callbackType)
        {
        case SIO_CALLBACK_GET_TX_CHAR:
            pSmcChan->getTxChar    = callback;
            pSmcChan->getTxArg     = callbackArg;
            return (OK);
	    break;

        case SIO_CALLBACK_PUT_RCV_CHAR:
            pSmcChan->putRcvChar   = callback;
            pSmcChan->putRcvArg    = callbackArg;
            return (OK);
	    break;

        default:
            return (ENOSYS);
        }
    }

LOCAL M8260_SMC_CHAN m8260SmcChan1;
LOCAL M8260_SMC_CHAN m8260SmcChan2;

/******************************************************************************
*
* sysSerialHwInit - initialize the BSP serial devices to a quiesent state
*
* This routine initializes the BSP serial device descriptors and puts the
* devices in a quiesent state.	It is called from sysHwInit() with
* interrupts locked.
*
* Buffers and Buffer Descriptors for the two channels:
* 
* .CS
*													Address per SCC
*										-----------------------------------
* field 								size	SCC1			SCC2
* ------								------- ----------- 	-----------
* Receive Buffer Descriptor 			8 bytes 0xFFF0_0000 	0xFFF0_0100
* Receive Buffer Status 				2 bytes 0xFFF0_0000 	0xFFF0_0100
* Receive Buffer Length 				2 bytes 0xFFF0_0002 	0xFFF0_0102
* Pointer to Receive Buffer 			4 bytes 0xFFF0_0004 	0xFFF0_0104
* Receive Buffer						1 bytes 0xFFF0_0040 	0xFFF0_0140
*
* Transmit Buffer Descriptor			8 bytes 0xFFF0_0008 	0xFFF0_0108
* Transmit Buffer Status				2 bytes 0xFFF0_0008 	0xFFF0_0108
* Transmit Buffer Length				2 bytes 0xFFF0_000A 	0xFFF0_010A
* Transmit to Receive Buffer			4 bytes 0xFFF0_000C 	0xFFF0_010C
* Transmit Buffer						1 bytes 0xFFF0_0060 	0xFFF0_0160
* 
* field 								size	SMC1			SMC2
* ------								------- ----------- 	-----------
* Receive Buffer Descriptor 			8 bytes 0xFFF0_0200 	0xFFF0_0300
* Receive Buffer Status 				2 bytes 0xFFF0_0200 	0xFFF0_0300
* Receive Buffer Length 				2 bytes 0xFFF0_0202 	0xFFF0_0302
* Pointer to Receive Buffer 			4 bytes 0xFFF0_0204 	0xFFF0_0304
* Receive Buffer						1 bytes 0xFFF0_0240 	0xFFF0_0340
*
* Transmit Buffer Descriptor			8 bytes 0xFFF0_0208 	0xFFF0_0308
* Transmit Buffer Status				2 bytes 0xFFF0_0208 	0xFFF0_0308
* Transmit Buffer Length				2 bytes 0xFFF0_020A 	0xFFF0_030A
* Transmit to Receive Buffer			4 bytes 0xFFF0_020C 	0xFFF0_030C
* Transmit Buffer						1 bytes 0xFFF0_0260 	0xFFF0_0360

* CE
*
*
*
* RETURNS: N/A
*/ 

void sysSerialHwInit (void)
{
	 int immrVal = vxImmrGet();

	/* this is in order of the structure contents */
	m8260SmcChan1.channelMode = 0;
	m8260SmcChan1.baudRate = CONSOLE_BAUD_RATE;
	m8260SmcChan1.sccNum = 1;
	m8260SmcChan1.immrVal = immrVal;
	m8260SmcChan1.pBdBase		= (char *)immrVal + SMC1_BD_OFF;
	m8260SmcChan1.rcvBufferAddr = (char *)immrVal + SMC1_RX_BUF_OFF;
	m8260SmcChan1.txBufferAddr	= (char *)immrVal +  SMC1_TX_BUF_OFF;

	m8260IntDisable(INUM_SMC1);
	/* reset the channel */
	m8260SmcDevInit(&m8260SmcChan1);

#ifdef M8260_SMC2
	if (NUM_TTY >= 2)
	{
		m8260SmcChan2.channelMode = 0;
		m8260SmcChan2.baudRate = CONSOLE_BAUD_RATE;
		m8260SmcChan2.sccNum = 1;
		m8260SmcChan2.immrVal = immrVal;
		m8260SmcChan2.pBdBase		= (char *)immrVal + SMC2_BD_OFF ;
		m8260SmcChan2.rcvBufferAddr = (char *)immrVal + SMC2_RX_BUF_OFF ;
		m8260SmcChan2.txBufferAddr	= (char *)immrVal +  SMC2_TX_BUF_OFF ;

		m8260IntDisable(INUM_SMC2);
			/* reset the channel */
		m8260SmcDevInit(&m8260SmcChan2);
	}
#endif	/*M8260_SMC2*/

}

/******************************************************************************
*
* sysSerialHwInit2 - connect BSP serial device interrupts
*
* This routine connects the BSP serial device interrupts.  It is called from
* sysHwInit2().  Serial device interrupts could not be connected in
* sysSerialHwInit() because the kernel memory allocator was not initialized
* at that point, and intConnect() calls malloc().
*
* RETURNS: N/A
*/ 

void sysSerialHwInit2 (void)
    {

    /* connect serial interrupts */

	(void) intConnect (INUM_TO_IVEC(INUM_SMC1), 
		      (VOIDFUNCPTR) m8260SmcInt, (int) &m8260SmcChan1);
#ifdef M8260_SMC2
	(void) intConnect (INUM_TO_IVEC(INUM_SMC2), 
	 	      (VOIDFUNCPTR) m8260SmcInt, (int) &m8260SmcChan2);
#endif

    }

/******************************************************************************
*
* sysSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* This routine gets the SIO_CHAN device associated with a specified serial
* channel.
*
* RETURNS: A pointer to the SIO_CHAN structure for the channel, or ERROR
* if the channel is invalid.
*/

SIO_CHAN * sysSerialChanGet
    (
    int channel		/* serial channel */
    )
    {
    if (channel == 0)
		return ((SIO_CHAN *) &m8260SmcChan1); 
    else if (channel == 1)
		return ((SIO_CHAN *) &m8260SmcChan2); 
    else 
		return ((SIO_CHAN *) ERROR);
    }

/*******************************************************************************
*
* sysSerialReset - reset the serail device 
*
* This function calls sysSerialHwInit() to reset the serail device
*
* RETURNS: N/A
*
*/

void sysSerialReset (void)
    {
    sysSerialHwInit ();
    }


