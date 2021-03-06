/**
memset(puntatore a void, valore da assegnare, quantità dei byte da scrivere): setta il valore val da *ptr a *ptr + len.
* \param ptr indirizzo dell'area di memoria da scrivere
* \param val valore da scrivere
* \param len dimensione
*/
void inline memset(void *ptr, int val, unsigned int len){
	register int i;
	register int value = val;
	/*puntatore a carattere perché ha 1 byte, quindi scrive val in ogni byte fino a p+len-1*/
	register char* p = (char*)ptr;
	for (i = 0; i<len; i++) {
		p[i]=(char)value;
	}
}

/**
 * memcmp(puntatore a void, puntatore a void, lunghezza da confrontare): confronta le due aree di memoria
 * ritorna l'n-esimo byte da cui differiscono oppure in caso di uguaglianza ritorna 0
 * \param ptr1 primo indirizzo
 * \param ptr2 secondo indirizzo
 * \param lenght dimensione
 * \return 1 se sono diversi, 0 altrimenti
 */
int memcmp(void* ptr1, void* ptr2, int lenght){
	char* p1 = (char*)ptr1;
	char* p2 = (char*)ptr2;
	int i = 0;
	for(;i<lenght;i++)
		if (p1[i] != p2[i])
			return ++i;
	return 0;
}

/**
 * memcpy(puntatore a void, puntatore a void, lunghezza da copiare): copia il contenuto binario di src in dest
 * \param dest indirizzo destinazione
 * \param src indirizzo sorgente
 * \param count dimensione
 */

void inline *memcpy(void* dest, const void* src, unsigned int count) {
	register char* dst8 = (char*)dest;
	register char* src8 = (char*)src;
        
	if (count & 1) {
		dst8[0] = src8[0];
		dst8 += 1;
		src8 += 1;
	}
	count /= 2;
	while (count--) {
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8 += 2;
		src8 += 2;
	}
	return dest;
}

/**
 * strlen(puntatore a char): ritorna il numero di char in una stringa ASCII
 * senza contare lo zero terminatore
 * \param str puntatore alla stringa zeroterminata
 * \return lunghezza della stringa senza '\0'
 */
int strlen(char* str){
	register char* p = str;
	while(*p++ != '\0');
	return p - str;
}

/**
 * strcmp(puntatore a char, puntatore a char): confronta le due stringhe
 * ritorna 0 in caso di uguaglianza, -1 altrimenti
 * \param str1 puntatore alla prima stringa
 * \param str2 puntatore alla seconda stringa
 * \return 0 se uguali, 1 altrimenti
 */
int strcmp(char* str1, char* str2){
	register char* s1 = str1;
	register char* s2 = str2;
	while(*s1 == *s2++){
		if (*s1++ == '\0') return 0;
	}
	return -1;
}

/**
 * strcpy(puntatore a char, puntatore a char): trascrive una stringa
 * \param dest destinazione
 * \param src sorgente
 */
void strcpy(char* dest, char* src){
	char* s1 = dest;
	char* s2 = src;
	register char c;
	do {
        c = *s2++;
        *s1++ = c;
    } while ('\0' != c);
}

/**
 * strreverse(puntatore a char, puntatore a char): rovescia una stringa
 * \param begin inizio stringa
 * \param end fine stringa
 */
void strreverse(char* begin, char* end) {
	
	char aux;
	while(end>begin)
		aux=*end, *end--=*begin, *begin++=aux;
	
}

/**
 * itoa(valore, puntatore della stringa, base in cui convertire) : converte un intero in una stringa nella base specificata
 * \param value intero da convertire
 * \param str indirizzo dove scrivere la stringa
 * \param base base in cui convertire l'intero
 */
void itoa(int value, char* str, int base) {
	
	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char* wstr=str;
	int sign;
	
	// Validate base
	if (base<2 || base>35){ *wstr='\0'; return; }
	
	// Take care of sign
	if ((sign=value) < 0) value = -value;

	// Conversion. Number is reversed.
	do *wstr++ = num[value%base]; while(value/=base);
	if(sign<0) *wstr++='-';
	*wstr='\0';
	
	// Reverse string
	strreverse(str,wstr-1);
}
