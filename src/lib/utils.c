/*
memset(puntatore a void, valore da assegnare, quantità dei byte da scrivere): setta il valore val da *ptr a *ptr + len.
*/
void memset(void *ptr, int val, unsigned int len){
	int i;
	/*puntatore a carattere perché ha 1 byte, quindi scrive val in ogni byte fino a p+len-1*/
	char* p = (char*)ptr;
	for (i = 0; i<len; i++) {
		p[i]=(char)val;
	}
}

/*
 * memcmp(puntatore a void, puntatore a void, lunghezza da confrontare): confronta le due aree di memoria
 * ritorna l'n-esimo byte da cui differiscono oppure in caso di uguaglianza ritorna 0
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

/*
 * memcpy(puntatore a void, puntatore a void, lunghezza da copiare): copia il contenuto binario di src in dest
 */
void memcpy(void* dest, void* src, int lenght){
	char* p1 = (char*)dest;
	char* p2 = (char*)src;
	int i = 0;
	for(;i<lenght;i++)
		p1[i] = p2[i];
}

/*
 * strlen(puntatore a char): ritorna il numero di char in una stringa ASCII
 * senza contare lo zero terminatore
 */
int strlen(char* str){
	int i = 0;
	char* p = str;
	while(*str != '\0') i++;
	return i;
}

/*
 * strcmp(puntatore a char, puntatore a char): confronta le due stringhe
 * ritorna 0 in caso di uguaglianza, -1 altrimenti
 */
int strcmp(char* str1, char* str2){
	int i = 0;
	char* s1 = str1;
	char* s2 = str2;
	while(s1[i] == s2[i]){
		if ((s1[i] == '\0') && (s2[i] == '\0')) return 0;
		i++;
	}
	return -1;
}

/*
 * strcpy(puntatore a char, puntatore a char): trascrive una stringa
 */
void strcpy(char* dest, char* src){
	int i = 0;
	char* s1 = dest;
	char* s2 = src;
	while(s2[i] != '\0'){
		s1[i] = s2[i];
		i++;
	}
	s2[i] = '\0';
}

void strreverse(char* begin, char* end) {
	
	char aux;
	
	while(end>begin)
	
		aux=*end, *end--=*begin, *begin++=aux;
	
}
	
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
