#include <exec/types.h>
#include <rexx/storage.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos.h>
#include <dos/dos.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "NiKomStr.h"

#include "NiKomLib.h"
/*
#include "/nikomlib/nikom_pragmas.h"
#include "/nikomlib/nikom_protos.h"
*/
void ClearFiles(void);

struct Library *NiKomBase;

struct SysInfo info;

void init(void)
{
	FILE *fp;
	int ret, x;
	short foo;

	memset((void *)&info, 0, sizeof(struct SysInfo));

	ClearFiles();

	if(NiKomBase = OpenLibrary("nikom.library",19))
	{
		ret = CreateUserTags(-1,
								US_Name, "sysop",
								US_Street, "",
								US_Address, "",
								US_Country, "",
								US_OtherInfo, "",
								US_Phonenumber, "",
								US_Password, "sysop",
								US_Prompt, "-->",
								US_Charset, (byte)0,
								US_Status, (byte)100,
								US_Rader, (byte)24,
								US_Flags, (byte)2047,
								TAG_END);

		printf("Returvärde = %d\n", ret);
		CloseLibrary(NiKomBase);
		if(ret < 0) return;
	}
	else
	{
		printf("Kunde inte öppna NiKom.library!\n");
		return;
	}

	if(fp=fopen("NiKom:DatoCfg/Sysinfo.dat","w"))
	{
		fwrite((void *)&info,sizeof(struct SysInfo),1,fp);
		fclose(fp);
	}
	else
		printf("Kunde inte öppna NiKom:DatoCfg/Sysinfo.dat\n");

	if(fp=fopen("NiKom:DatoCfg/Möten.dat","w"))
		fclose(fp);
	else
		printf("Kunde inte öppna NiKom:DatoCfg/Möten.dat\n");

/*	for(x=0;x<MAXTEXTS;x++)
		foo[x]=~0; */

	if(fp=fopen("NiKom:DatoCfg/Textmot.dat","w"))
	{
		foo = ~0;
		for(x=0;x<MAXTEXTS;x++)
		{
			if(fwrite((void *)&foo,2,1,fp)!=1)
			{
				printf("Kunde inte skriva till TextMot.dat\n");
				break;
			}
		}
		fclose(fp);
	}
	else
		printf("Kunde inte öppna NiKom:DatoCfg/Textmot.dat\n");

	if(fp=fopen("NiKom:DatoCfg/Areor.dat","w"))
		fclose(fp);
	else
		printf("Kunde inte öppna NiKom:DatoCfg/Areor.dat\n");

	if(fp=fopen("NiKom:DatoCfg/Grupper.dat","w"))
		fclose(fp);
	else
		printf("Kunde inte öppna NiKom:DatoCfg/Grupper.dat\n");
}

int main(int argc, char **argv)
{
	char input[20];
	int count=0;

	if(argc != 2)
	{
		printf("Detta program kommer att rensa följande filer\n");
		printf("Users/0/0\n");
		printf("Sysinfo.dat\n");
		printf("Möten.dat\n");
		printf("Textmot.dat\n");
		printf("Areor.dat\n");
		printf("Grupper.dat\n\n");
		printf("Är detta OK? (j/N) ");
		while((input[count++]=getchar())!='\n');
		if(input[0]=='j' || input[0]=='J') {
			init();
		} else printf("Nehepp!\n");
	}
	else
	{
		printf("Skapar lite filer för att NiKom ska gå att starta upp, var god vänta...   ");
		fflush(stdout);
		init();
		printf("klart!\n");
	}

	return 0;
}

void ClearFiles(void)
{
	DeleteFile("NiKom:DatoCfg/Sysinfo.dat");
	DeleteFile("NiKom:Users/0/0/Data");
	DeleteFile("NiKom:Users/0/0/Bitmap0");
	DeleteFile("NiKom:DatoCfg/Grupper.dat");
	DeleteFile("NiKom:DatoCfg/Areor.dat");
	DeleteFile("NiKom:DatoCfg/Textmot.dat");
}
