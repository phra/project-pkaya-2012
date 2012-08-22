#ifndef __INTERRUPT__
#define __INTERRUPT__

U32 devBaseAddrCalc(U8 line, U8 devNum);
unsigned int deviceHandler(unsigned int intline);

#endif
