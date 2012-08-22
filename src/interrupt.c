#include "lib/const11.h"
#include "lib/const.h"
#include "lib/base.h"
#include "lib/uMPStypes.h"
#include "lib/libumps.h"
#include "lib/listx.h"
#include "lib/types11.h"

#include "phase1/pcb.h"
#include "phase1/asl.h"
#include "lib/utils.h"

#include "init.h"
#include "scheduler.h"
#include "exception.h"
#include "syscall.h"


U32 devBaseAddrCalc(U8 line, U8 devNum)
{
	return DEV_REGS_START + (line * 0x80) + (devNum * 0x10);
}

/*Funzione per la gestione dei specifici device*/
unsigned int deviceHandler(unsigned int intline){
    unsigned int devicesAdd,device_requested,terminalRcv,terminalTra ;
    unsigned int returnValue=0;
    unsigned int i=0;

    devicesAdd= *(   (int*)(PENDING_BITMAP_START + (intline - 3)* WORD_SIZE)  );
    /*cerco il numero del device che ha effetuato lÍnterrupt*/
    while (i<= DEV_PER_INT && (devicesAdd%2 == 0)) {
            devicesAdd >>= 1;
            i++;
    }
    /*ricavo il registro del device richiesto*/
    device_requested=DEV_REGS_START + ((intline - 3) * 0x80) + (i * 0x10);
    
    /*ACK*/
    if(intline!=INT_TERMINAL){ /*non è sulla linea del terminale*/
        ((dtpreg_t*)device_requested)->command = DEV_C_ACK;

        returnValue= ( ((dtpreg_t*)device_requested)->status)& 0xFF;
    }else{ 

        terminalRcv = ( ((termreg_t *)device_requested)->recv_status)& 0xFF  ;

        terminalTra = ( ((termreg_t *)device_requested)->transm_status)& 0xFF ;

        if (terminalTra == DEV_TTRS_S_CHARTRSM) {
            ((termreg_t*)device_requested)->transm_command = DEV_C_ACK;
            returnValue=terminalTra;
        }else{

            if (terminalRcv == DEV_TRCV_S_CHARRECV) {
                ((termreg_t*)device_requested)->recv_command = DEV_C_ACK;
                returnValue=terminalRcv;
            }

        }
    }
    /*ritorno il valore dello status a seconda che ci si trovi sul terminal o su altri device*/
    return returnValue;

}
