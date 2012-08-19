
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


/*Funzione di gestione degli interrupt, settata nel boot*/
void intHandler(){
    unsigned int devSts;
    /* si assegna inizialmente la linea di INT_TIMER perchè le linee 0 e 1 non sono utilizzabili*/
    int intline=INT_TIMER;

    /*Su quale linea c'è stato l'interrupt:*/
    while(intline<=INT_TERMINAL){
        if(CAUSE_IP_GET(((state_t *) INT_OLDAREA)->cause , intline)){break;}
        intline++;          
    }

    if(intline>INT_TERMINAL){/*in Questo caso nessun tipo di interrupt e' stato sollevato*/}
    else{
        if (intline == INT_TIMER){/* in questo caso è scaduto il time slice*/
            if(currentThread!=NULL){
                /*stop e inserimento in ReadyQueue*/
                stopThread(1,(state_t *)INT_OLDAREA);
            }
            /*risetta da capo il time slice */      
            else{SET_IT(SCHED_TIME_SLICE);}
        }       
        else{/*chiamo il gestore dei device, passandogli la linea su cui c'è stato l'interrupt*/
            devSts=deviceHandler(intline);
            /*invio al thread in attesa di IO il risultato della deviceHandler(uno state)*/
            MsgSend(SEND,firstWaitio(&frozenList) ,devSts );
            /*LDST del INT_OLDAREA per far ripartire il codice*/
            LDST((state_t *)INT_OLDAREA);
            /*nextThread(); */
        }
    }
}

void tlb_handler(){
	/* verifico la presenza di un gestore dell'eccezione  */
//    if ( currentProcess->specified_handler[TLBTRAP] != NULL ){
  //      updateProcessStateFromOldArea(HMAX-TLBHAND);
    //    currentProcess->proc_state.s_pc += 4;
    	//copyProcState(&(currentProcess->proc_state), currentProcess->specified_handler[TLBTRAP+3]);
  //  	LDST( currentProcess->specified_handler[TLBTRAP] );
    //}
   // else{	/* gestore non presente, processo terminato */
    //	terminateThread();
    	//schedule();
  //  }
}

void prg_trap_handler(){
	/* verifico la presenza di un gestore dell'eccezione  */
  /*  if ( currentProcess->specified_handler[PROGTRAP] != NULL ){
    	updateProcessStateFromOldArea(HMAX-PRGHAND);
    	currentProcess->proc_state.s_pc += 4;
    	copyProcState(&(currentProcess->proc_state), currentProcess->specified_handler[PROGTRAP+3]);
    	LDST( currentProcess->specified_handler[PROGTRAP] );
    }
    else{*/	/* gestore non presente, processo terminato */
    //	terminateThread();
	//	schedule();
    //}
}

void breakpoint_handler(){
	/**/
}

void syscall_bp_handler(){
	//U32 sysValue;

	/* carica sul processo appena deschedulato il suo stato salvato al momento dell'interruzione */
//	updateProcessStateFromOldArea(HMAX-SYSBPHAND);
	/* incremento il programCounter del processo */
//	currentProcess->proc_state.s_pc += 4;
	/* prelevo il numero della SYSCALL */
//	sysValue = (U32)(((state_t*)SYSBK_OLDAREA)->s_a0);

	/* se il chiamante non è in kernel mode */
//	if ( (currentProcess->proc_state.s_status & STATUS_KUp) != 0 ){
		/* gestisci no kernel mode*/
//		if ( currentProcess->specified_handler[PROGTRAP] != NULL ){	/* gestisco eccezione in usermode */
//			copyProcState(&(currentProcess->proc_state), currentProcess->specified_handler[PROGTRAP+3]);
//			currentProcess->specified_handler[PROGTRAP+3]->s_cause = CAUSERESVINSTR;
//			LDST( currentProcess->specified_handler[PROGTRAP] );
//		}else
//			terminateThread();
//	}
//	else{	/* SYSCALL IN KERNELMODE chiamo il gestore adeguato */

		state_t* before = (state_t*)new_old_areas[getPRID()][SYSBK_OLD];
		switch (CAUSE_EXCCODE_GET(getCAUSE())){
			case 8: /*SYSCALL*/
				if (before->status & STATUS_KUp){ /*#FIXME KUp or KUc?*/
					/*SYSCALL invoked in user mode*/
					//#FIXME
					PANIC();
					break;
				} else switch (before->reg_a0){ /*SYSCALL invoked in kernel mode*/
					case CREATE_PROCESS:
						create_process();
						break;
					case CREATE_BROTHER:
						create_brother();
						break;
					case TERMINATE_PROCESS:
						terminate_process();
						break;
					case VERHOGEN:
						verhogen();
						break;
					case PASSEREN:
						passeren();
						break;
					case GET_CPU_TIME:
						get_cpu_time();
						break;
					case WAIT_FOR_CLOCK:
						wait_for_clock();
						break;
					case WAIT_FOR_IO_DEVICE:
						wait_for_io_device();
						break;
					case SPECIFY_PRG_STATE_VECTOR:
						specify_prg_state_vector();
						break;
					case SPECIFY_TLB_STATE_VECTOR:
						specify_tlb_state_vector();
						break;
					case SPECIFY_SYS_STATE_VECTOR:
						specify_sys_state_vector();
						break;
					default:
						PANIC();
				} break;
			case 9:
				/*BREAKPOINT*/
				breakpoint_handler();
				break;
			default:
				PANIC();
	}
	//scheduler(); #FIXME
}
