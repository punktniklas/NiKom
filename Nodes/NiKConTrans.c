#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <stdio.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"

struct MinList tf_list;
char xprfilnamn[] = "<Ingen fil>";

extern char inmat[];

int download(void) {
	puttekn("\r\n\nDu kan ju knappast ladda ner lokalt...\r\n",-1);
	return(0);
}

int upload(void) {
	puttekn("\r\n\nDu kan ju knappast ladda upp lokalt...\r\n",-1);
	return(0);
}

int sendbinfile(void) {
	puttekn("\r\n\nDu kan ju knappast skicka någon fil lokalt...\r\n",-1);
	return(0);
}

int recbinfile(char *nisse) {
	puttekn("\r\n\nDu kan ju knappast ta emot någon fil lokalt...\r\n",-1);
	return(0);
}
