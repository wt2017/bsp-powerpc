/* epic.c - */

/* Copyright 1998-2000 Wind River Systems, Inc. */
/* NOMANUAL */

#include "vxWorks.h"
#include "intLib.h"
#include "excLib.h"
#include "epic.h"
#include "stdio.h"
#include "string.h"
#include "arch\ppc\ivPpc.h"
#include "bspcommon.h"

extern ULONG 	sysEUMBBARRead(ULONG regNum);
extern void 	sysEUMBBARWrite(ULONG regNum, ULONG regVal);

IMPORT STATUS	(*_func_intConnectRtn) (VOIDFUNCPTR *, VOIDFUNCPTR, int);
IMPORT int	(*_func_intEnableRtn) (int);
IMPORT int	(*_func_intDisableRtn)  (int);

/*
 * Typedef: epic_table_t
 * Purpose: Defines an entry in the EPIC table indexed by vector.
 *	    Each entry represents a vector that is permanently
 *	    assigned to a specific function as follows (see definitions
 *	    for EPIC_VECTOR_xxx):
 *
 *	Vector(s)	Assignment
 *	---------	-------------------
 *	0-15		External (and unused serial interrupts)
 *	16-19		Global timers
 *	20		I2C
 *	21-22		DMA
 *	23		I2O message
 *
 * NOTE: Higher priority numbers represent higher priority.
 */

typedef struct epic_table_s {
    int		et_prio;	/* Interrupt priority, fixed */
    VOIDFUNCPTR	et_func;	/* Function to call, set when connected */
    int		et_par;		/* Param to func, set when connected */
    UINT32	et_vecadr;	/* Address of EPIC vector register */
    UINT32	et_desadr;	/* Address of EPIC destination register */
} epic_table_t;

static epic_table_t epic_table[EPIC_VECTOR_USED] = {
  /* 0  */  {  5, 0, 0, EPIC_EX_INTx_VEC_REG(0),  EPIC_EX_INTx_DES_REG(0)  },
  /* 1  */  {  5, 0, 0, EPIC_EX_INTx_VEC_REG(1),  EPIC_EX_INTx_DES_REG(1)  },
  /* 2  */  {  5, 0, 0, EPIC_EX_INTx_VEC_REG(2),  EPIC_EX_INTx_DES_REG(2)  },
  /* 3  */  {  5, 0, 0, EPIC_EX_INTx_VEC_REG(3),  EPIC_EX_INTx_DES_REG(3)  },
  /* 4  */  { 10, 0, 0, EPIC_EX_INTx_VEC_REG(4),  EPIC_EX_INTx_DES_REG(4)  },
  /* 5  */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(5),  EPIC_SR_INTx_DES_REG(5)  },
  /* 6  */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(6),  EPIC_SR_INTx_DES_REG(6)  },
  /* 7  */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(7),  EPIC_SR_INTx_DES_REG(7)  },
  /* 8  */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(8),  EPIC_SR_INTx_DES_REG(8)  },
  /* 9  */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(9),  EPIC_SR_INTx_DES_REG(9)  },
  /* 10 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(10), EPIC_SR_INTx_DES_REG(10) },
  /* 11 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(11), EPIC_SR_INTx_DES_REG(11) },
  /* 12 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(12), EPIC_SR_INTx_DES_REG(12) },
  /* 13 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(13), EPIC_SR_INTx_DES_REG(13) },
  /* 14 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(14), EPIC_SR_INTx_DES_REG(14) },
  /* 15 */  {  5, 0, 0, EPIC_SR_INTx_VEC_REG(15), EPIC_SR_INTx_DES_REG(15) },
  /* 16 */  {  8, 0, 0, EPIC_TMx_VEC_REG(0),      EPIC_TMx_DES_REG(0)      },
  /* 17 */  {  5, 0, 0, EPIC_TMx_VEC_REG(1),      EPIC_TMx_DES_REG(1)      },
  /* 18 */  {  5, 0, 0, EPIC_TMx_VEC_REG(2),      EPIC_TMx_DES_REG(2)      },
  /* 19 */  {  5, 0, 0, EPIC_TMx_VEC_REG(3),      EPIC_TMx_DES_REG(3)      },
  /* 20 */  {  5, 0, 0, EPIC_I2C_INT_VEC_REG,     EPIC_I2C_INT_DES_REG     },
  /* 21 */  {  5, 0, 0, EPIC_DMA0_INT_VEC_REG,    EPIC_DMA0_INT_DES_REG    },
  /* 22 */  {  5, 0, 0, EPIC_DMA1_INT_VEC_REG,    EPIC_DMA1_INT_DES_REG    },
  /* 23 */  {  5, 0, 0, EPIC_MSG_INT_VEC_REG,     EPIC_MSG_INT_DES_REG     },
  /* 24 */  {  5, 0, 0, EPIC_DUART1_INT_VEC_REG,  EPIC_DUART1_INT_DES_REG   },
  /* 25 */  {  5, 0, 0, EPIC_DUART2_INT_VEC_REG,  EPIC_DUART2_INT_DES_REG   },
};

STATUS epic_int_configure(int vec, int prio, VOIDFUNCPTR func, int par)
/*
 * Function: 	epic_int_configure
 * Purpose:	Configure an EPIC interrupt with a given vector and prio.
 * Parameters:	vec - interrupt vector number/level
 *		prio - PPC interrupt priority
 *		func - function to call
 *		par - parameter passed to func on interrupt.
 * Returns:	OK/ERROR
 */
{
    epic_table_t *et = &epic_table[vec];

    if (NULL != et->et_func || vec < 0 || vec >= EPIC_VECTOR_USED) {
	return(ERROR);
    }
    et->et_func	 = func;
    et->et_par	 = par;
    et->et_prio	 = prio;
    return(OK);
}

STATUS epic_int_connect(VOIDFUNCPTR *v_ptr, VOIDFUNCPTR rtn, int par)
/*
 * Function: 	epic_int_connect
 * Purpose:	Connect an epic interrupt.
 * Parameters:	v_ptr - vector #.
 *		rtn - function to call.
 *		par - parameter passed to function.
 * Returns:	OK/ERROR
 * Notes:	This routine uses the same priority as found in the table.
 */
{
    int		vec	= (int) v_ptr;

    if (vec < 0 || vec >= EPIC_VECTOR_USED)
	return(ERROR);

    return epic_int_configure(vec, epic_table[vec].et_prio, rtn, par);
}

void  epic_int_handler(void)
/*
 * Function: 	epic_int_handler
 * Purpose:	Handle an interrupt from the epic.
 * Parameters:	None.
 * Returns:	Nothing
 */
{
    int	vec;
    ULONG	o_pri;			/* Old priority */
    epic_table_t *et;

    vec = epic_read(EPIC_PROC_INT_ACK_REG) ;
    et  = epic_table + vec;
    if ((vec >= 0) && (vec < EPIC_VECTOR_USED) && (NULL != et->et_func)) {
	o_pri = epic_read(EPIC_PROC_CTASK_PRI_REG); 	/* Save prev level */
	epic_write(EPIC_PROC_CTASK_PRI_REG, et->et_prio);/* Set task level */ 
	intUnlock (_PPC_MSR_EE);			/* Allow nesting */
	et->et_func(et->et_par);			/* Call handler */
	//intLock();
	epic_write(EPIC_PROC_CTASK_PRI_REG, o_pri);	/* Restore  level */
    }
    epic_write(EPIC_PROC_EOI_REG, 0x0);			/* Signal EOI */
}

STATUS epic_int_enable(int vec)
/*
 * Function: 	epic_int_enable
 * Purpose:	Enable interrupts for the specified vector.
 * Parameters:	vec - vector # to enable interrupts for.
 * Returns:	OK/ERROR
 */
{
    epic_table_t	*et = epic_table + vec;
    UINT32		vecreg;

    if (NULL == et->et_func) {
	return(ERROR);
    }


    /*
     * Look up the interrupt level for the vector and enable that level
     * setting the correct vector #, priority, 
     */

    vecreg = (et->et_prio << EPIC_VECREG_PRIO_SHFT |
	      vec << EPIC_VECREG_VEC_SHFT);

    if (vec >= EPIC_VECTOR_EXT0 && vec <= EPIC_VECTOR_EXT4)
	vecreg |= EPIC_VECREG_S;	/* Set sense bit */

    epic_write(et->et_vecadr, vecreg);

    /* Direct the interrupt to this processor */
    epic_write(et->et_desadr, 1);

    return(OK);
}

STATUS epic_int_disable(int vec)
/*
 * Function: 	epic_int_disable
 * Purpose:	Disable interrupts for the specified vector.
 * Parameters:	vec - vector number to disable.
 * Returns:	OK/ERROR
 */
{
    epic_table_t	*et = epic_table + vec;
    UINT32		vecreg;

    if (NULL == et->et_func) {
	return(ERROR);
    }

    /* Set mask bit */

    vecreg = epic_read(et->et_vecadr);
    vecreg |= EPIC_VECREG_M;
    epic_write(et->et_vecadr, vecreg);

    return(OK);
}

/*
STATUS epic_set_tmBase_counter(UINT32 timer_id ,UINT32 val)
timer_id的取值范围:[0,3]
val : bit31位为TIMER x的控制位, == 1 :禁止timer, == 0 ,表示使能timer
*/

STATUS epic_set_tmBase_counter(UINT32 timer_id ,UINT32 val)
{
    if(timer_id <= 3)
		epic_write(EPIC_TMx_BASE_COUNT_REG(timer_id) , val ) ;
	return OK ;
}

STATUS epic_get_tmBase_counter(UINT32 timer_id ,UINT32 *val)
{
    if(timer_id <= 3 && val != NULL)
		 *val =  epic_read(EPIC_TMx_BASE_COUNT_REG(timer_id)) ;
	return OK ;
}

STATUS epic_get_current_counter(UINT32 timer_id ,UINT32 *val)
{
    if(timer_id <= 3 && val != NULL)
		 *val =  epic_read(EPIC_TMx_CUR_COUNT_REG(timer_id)) ;
	return OK ;
}


/*
STATUS epic_timer_set_cascade(UINT32 enable , UINT32 cascade_id)
param: 
mpc 8245 共有0~3 4个EPIC timer
cascade_id = 0 :表示级联合timer 0，1
cascade_id = 1 :表示级联合timer 1，2
cascade_id = 2 :表示级联合timer 2，3
cascade_id =为其它值则非法
enable = 0 ,禁止级联，为非0值则表示使能级联；
返回值:OK表示成功，ERROR表示失败
*/
STATUS epic_timer_set_cascade(UINT32 enable , UINT32 cascade_id)
{
    UINT32	regval ;

	if(cascade_id > 3)	return ERROR;

	regval = epic_read(EPIC_TM_CONTROL_REG) ;
	if(enable) //enable
		regval |= (1 << cascade_id)|(1 <<(cascade_id + 24));
	else 		  //disable
		regval &= ~((1 << cascade_id)|(1 <<(cascade_id + 24)));		
	epic_write(EPIC_TM_CONTROL_REG , regval) ;
	
    return(OK);
}

void epic_init(void)
/*
 * Function: 	epic_init
 * Purpose:	Initialize the EPIC for interrupts.
 * Parameters:	None.
 * Returns:	Nothing.
 */
{
    ULONG	tmp;

    /* Set reset bit and wait for reset to complete */
    epic_write(EPIC_GLOBAL_REG, EPIC_GCR_RESET);
    while (epic_read(EPIC_GLOBAL_REG) & EPIC_GCR_RESET)
	;

    /* mixed interrupt mode,not pass through */
    epic_write(EPIC_GLOBAL_REG, EPIC_GCR_M);/*GCR[M]=1,with ICR[SIE]=0,decide we use IRQ0~IRQ4*/

    /* Set direct interrupts (disable serial interrupts) */
    tmp = epic_read(EPIC_INT_CONF_REG);
    epic_write(EPIC_INT_CONF_REG, tmp & ~EPIC_ICR_SIE);/*ICR[SIE]=0*/

    /* check if all pending interrupt */
    while (epic_read(EPIC_PROC_INT_ACK_REG) != 0xff);

    /* Broadcom uses EPIC only, no winpic stuff. */
    _func_intEnableRtn	= epic_int_enable;
    _func_intDisableRtn = epic_int_disable;
    _func_intConnectRtn = epic_int_connect;

    /* Connect the real PPC vector to our handler. */
    excIntConnect ((VOIDFUNCPTR *) _EXC_OFF_INTR, epic_int_handler);

    /* Set current task priority to 0 */
    epic_write(EPIC_PROC_CTASK_PRI_REG, 0);
}

#if 1 

#define COUNTER_100us 833

int aaaa  = 0 ;
void timer_isr_handle(int handle_param)
{
	aaaa++ ;
}
 
/* 
bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR *timer_handle , int handle_param)
设置周期性的中断处理接口
action = 0 ,关闭中断(其它参数可以不填)，else 使能中断
timer_value : 执行的中断的时间间隔= COUNTER_100us*timer_value [1,1000],100微妙到100豪秒之间
timer_handle :中断处理函数
handle_param :中断处理函数的参数
*/

STATUS bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR timer_handle , int handle_param)
{
	if(action && ((timer_value <= 0) || (timer_value > 1000) ||  (timer_handle == NULL)))
		{
		printf("bsp_timer1_set param error , timer_value  = %d( 0 < timer_value <1000) ,timer_handle = 0x%x\n" 
			, timer_value,(int)timer_handle );
		return ERROR ;
		}
		
	if (action)
	{
		epic_write(EPIC_TMx_BASE_COUNT_REG(1), COUNTER_100us*timer_value); 
		intConnect(INUM_TO_IVEC(EPIC_VECTOR_TM1), (VOIDFUNCPTR) timer_handle , (int) handle_param);  
		intEnable(EPIC_VECTOR_TM1);/*Timer1 int enable*/
	}
	else
	{
		intDisable(EPIC_VECTOR_TM1);
	}
	return OK ;
}

STATUS bsp_timer0_set(int action  , int timer_value , VOIDFUNCPTR timer_handle , int handle_param)
{
	if(action && ((timer_value <= 0) || (timer_value > 1000) ||  (timer_handle == NULL)))
		{
		printf("bsp_timer1_set param error , timer_value  = %d( 0 < timer_value <1000) ,timer_handle = 0x%x\n" 
			, timer_value,(int)timer_handle );
		return ERROR ;
		}
		
	if (action)
	{
		epic_write(EPIC_TMx_BASE_COUNT_REG(2), COUNTER_100us*timer_value); 
		intConnect(INUM_TO_IVEC(EPIC_VECTOR_TM2), (VOIDFUNCPTR) timer_handle , (int) handle_param);  
		intEnable(EPIC_VECTOR_TM2);/*Timer1 int enable*/
	}
	else
	{
		intDisable(EPIC_VECTOR_TM2);
	}
	return OK ;
}

#endif

	
