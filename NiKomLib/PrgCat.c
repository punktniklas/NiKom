#include "/Include/NiKomStr.h"
#include "/Include/NiKomLib.h"
#include "NiKomBase.h"

#include "funcs.h"

#include <ctype.h>
/* #include <stdlib.h> */
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

/****** nikom.library/AddProgramData ******************************************

    NAME
        AddProgramData  -- <todo>
    SYNOPSIS
        result = AddProgramData(anvnummer, filnamn, kategori, typ, data)
        D0                      D0         D1       A1        A2   A3

    FUNCTION
    INPUTS
    RESULT
    EXAMPLE
    NOTES
    BUGS
    SEE ALSO

******************************************************************************/

int __saveds __asm LIBAddProgramData( register __d0 int usernumber, register __a1 char *kategorinamn, register __a2 char *typ, register __a3 char *data, register __a6 struct NiKomBase *NiKomBase)
{
	struct ProgramCategory *start = NULL;
	struct ProgramDataCache *prgdata = NULL;

/*	debug_req("Obtain semaphore", NULL);
	ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]);
	debug_req("semaphore Obtained", NULL); */

	prgdata = CheckifUserisCached(usernumber, NiKomBase);
	if(prgdata != NULL)
		start = prgdata->PrgCat; 
	else
	{
		start = CacheProgramDataforUser(usernumber, NiKomBase);
	}

	if(start && !GetCategorypek(kategorinamn, start))
	{
		InsertProgramCategory(kategorinamn, &start);
	}

	if(start)
		InsertProgramData(kategorinamn, typ, data, start);
	
/*	SaveAll(usernumber, start); */

/*	debug_req("release semaphore", NULL);
	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]);
	debug_req("semaphore released", NULL); */
	return(1);
}

void DisplayProgramDataCache(char *string, struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *start;
	char buffer[257];

	start = NiKomBase->Servermem->PrgDataCache;
	sprintf(buffer,"%s\n", string);
	Debuglog(buffer);
	if(start != NULL)
	{
		while(start != NULL)
		{
			sprintf(buffer,"User = #%d\n",start->usernumber);
			Debuglog(buffer);
			if(start->PrgCat)
				DisplayAll("ProgramData:", start->PrgCat);
			start = start->next;
		}
	}
	else
	{
		sprintf(buffer,"Listan är tom!\n");
		Debuglog(buffer);
	}
}

struct ProgramCategory *CacheProgramDataforUser(int usernumber, struct NiKomBase *NiKomBase)
{
	struct ProgramCategory *start = NULL;
	char buffer[257];
	
	sprintf(buffer, "Före LoadAll #%d\n", usernumber);
	Debuglog(buffer);

	sprintf(buffer, "NiKom:Users/%d/%d/ProgramData", usernumber/100, usernumber);
	LoadAll(buffer, &start);

	sprintf(buffer, "Efter LoadAll #%d\n", usernumber);
	Debuglog(buffer);

	InsertProgramDataCachedUser(usernumber, start, NiKomBase);
	return start;
}

void InsertProgramDataCachedUser(int usernumber, struct ProgramCategory *start, struct NiKomBase *NiKomBase)
{
	int antal = 0;
	char buffer[65];

	struct ProgramDataCache *prgdata = NiKomBase->Servermem->PrgDataCache, *pek, *pek2;

	if(prgdata == NULL)
	{
		sprintf(buffer, "Ny användare %d\n", usernumber);
		Debuglog(buffer);
		prgdata = (struct ProgramDataCache *) AllocMem(sizeof(struct ProgramDataCache), MEMF_CLEAR | MEMF_PUBLIC);
		prgdata->prev = NULL;
		prgdata->next = NULL;
		prgdata->usernumber = usernumber;
		prgdata->PrgCat = start;
		NiKomBase->Servermem->PrgDataCache = prgdata;
	}
	else
	{
		pek = prgdata;
		while(pek->next != NULL)
		{
			pek = pek->next;
			antal++;
		}

		if(antal >= MAXPRGCATCACHE)
		{
			RemoveFirstCachedUser(NiKomBase);

		}
		pek2 = (struct ProgramDataCache *) AllocMem(sizeof(struct ProgramDataCache), MEMF_CLEAR | MEMF_PUBLIC);
		pek->next = pek2;
		pek2->prev = pek;
		pek2->next = NULL;
		pek2->usernumber = usernumber;
		pek2->PrgCat = start;
	}
}

void RemoveFirstCachedUser(struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *prgdata = NiKomBase->Servermem->PrgDataCache, *pek;
	char buffer[81];

	if(LIBSaveProgramCategory(prgdata->usernumber, NiKomBase))
	{
		sprintf(buffer, "Tar bort första cachade användaren... #%d\n", prgdata->usernumber);
		Debuglog(buffer);
		pek = prgdata;
		pek->next->prev = NULL;
		NiKomBase->Servermem->PrgDataCache = pek->next;
		DeleteAll(&prgdata->PrgCat);
		FreeMem(pek, sizeof(struct ProgramDataCache));
	}
}

struct ProgramDataCache *CheckifUserisCached(int usernumber, struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *prgdata = NiKomBase->Servermem->PrgDataCache;
	
	if(prgdata != NULL)
	{
		while(prgdata && prgdata->usernumber != usernumber)
			prgdata = prgdata->next;
	}
	
	if(prgdata != NULL)
		Debuglog("Användaren är cachad.\n");

	return prgdata;
}

char __saveds __asm *LIBGetProgramData( register __d0 int usernumber, register __a0 char *filnamn, register __a1 char *kategorinamn, register __a2 char *typ, register __a6 struct NiKomBase *NiKomBase)
{
	struct ProgramCategory *start;
	struct ProgramDataCache *prgdata;
	char buffer[257];
	
	start = NULL;
	
/*	ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]); */

	prgdata = CheckifUserisCached(usernumber, NiKomBase);
	if(prgdata != NULL)
		start = prgdata->PrgCat;
	else
	{
		start = CacheProgramDataforUser(usernumber, NiKomBase);
	}

	if(InternGetProgramData(kategorinamn, typ, start))
		strcpy(buffer, InternGetProgramData(kategorinamn, typ, start));
	else
		buffer[0] = NULL;
		
/*	if(filnamn != NULL || nodnummer == -1)
		DeleteAll(&start); */

/*	ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]); */

	return(&buffer[0]);
}

int __saveds __asm LIBFreeProgramCategory( register __d0 int usernumber, register __a6 struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *prgdata;

	prgdata = CheckifUserisCached(usernumber, NiKomBase);
	if(prgdata != NULL)
		/* start = prgdata->PrgCat; */

	return 0;


/*		ObtainSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]); */
/*		DeleteAll(&NiKomBase->Servermem->PrgCat[nodnummer]); */
/*		ReleaseSemaphore(&NiKomBase->Servermem->semaphores[NIKSEM_PRGCAT]); */
		return(1);
}

int __saveds __asm LIBLoadProgramCategory( register __d0 int usernumber, register __a6 struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *prgdata;
	char buffer[81];

	sprintf(buffer,"Display vid LoadProgramCategory för användare #%d\n", usernumber);
	DisplayProgramDataCache(buffer, NiKomBase);

	prgdata = CheckifUserisCached(usernumber, NiKomBase);
	if(prgdata == NULL)
		CacheProgramDataforUser(usernumber, NiKomBase);
	
	Debuglog("Slut av LoadProgramCategory\n");
	return(1);
}

int __saveds __asm LIBSaveProgramCategory( register __d0 int usernumber, register __a6 struct NiKomBase *NiKomBase)
{
	struct ProgramDataCache *prgdata;

	prgdata = CheckifUserisCached(usernumber, NiKomBase);
	if(prgdata == NULL)
		return(0);

	SaveAll(usernumber, prgdata->PrgCat);
	return(1);
}

 
/* Funktioner som används av ovanstående funktioner... */

int InsertProgramCategory(char *namn, struct ProgramCategory **start)
{
	struct ProgramCategory *pek, *pek2;
	
	pek = *start;
	
	while(pek != NULL)
	{
		if(!stricmp(namn, pek->namn))
			return(-1);
		pek = pek->next;
	}
	
	if(*start == NULL)
	{
		*start = (struct ProgramCategory *) AllocMem(sizeof(struct ProgramCategory), MEMF_CLEAR | MEMF_PUBLIC);
		pek = *start;
		pek->prev = NULL;
		pek->next = NULL;
		strncpy(pek->namn, namn, 80);
		pek->datastart = NULL;
	}
	else
	{
		pek = *start;
		while(pek->next != NULL)
			pek = pek->next;

		pek2 = (struct ProgramCategory *) AllocMem(sizeof(struct ProgramCategory), MEMF_CLEAR | MEMF_PUBLIC);
		pek->next = pek2;
		pek2->prev = pek;
		pek2->next = NULL;
		pek2->datastart = NULL;
		strncpy(pek2->namn, namn, 80);
	}
}

int InsertProgramData(char *categoryname, char *typ, char *data, struct ProgramCategory *start)
{
	struct ProgramCategory *pek;
	struct ProgramData *datapek, *datapek2;

	pek = GetCategorypek(categoryname, start);
	if(pek == NULL) { return(0); }

	datapek = pek->datastart;
	
	if(datapek == NULL)
	{
		datapek = (struct ProgramData *) AllocMem(sizeof(struct ProgramData), MEMF_CLEAR | MEMF_PUBLIC);
		datapek->prev = NULL;
		datapek->next = NULL;
		strncpy(datapek->typ, typ, 40);
		strncpy(datapek->data, data, 80);
		pek->datastart = datapek;
	}
	else
	{
		datapek2 = datapek;
		while(datapek2 != NULL && stricmp(datapek2->typ,typ))
			datapek2 = datapek2->next;

		if(datapek2 == NULL)
		{
			while(datapek->next != NULL)
				datapek = datapek->next;

			datapek2 = (struct ProgramData *) AllocMem(sizeof(struct ProgramData), MEMF_CLEAR | MEMF_PUBLIC);
			datapek->next = datapek2;
			datapek2->prev = datapek;
			datapek2->next = NULL;
		}

		strncpy(datapek2->typ,typ,40);
		strncpy(datapek2->data,data,80);
	}
	
	return(1);
}

char *InternGetProgramData(char *categoryname, char *typ, struct ProgramCategory *start)
{
	struct ProgramCategory *pek;
	struct ProgramData *datapek;

	if(start != NULL)
	{
		pek = GetCategorypek(categoryname, start);
		if(pek != NULL)
		{
			datapek = pek->datastart;

			while(datapek != NULL)
			{
				if(!stricmp(datapek->typ, typ))
					return(&datapek->data[0]);
				datapek = datapek->next;
			}
		}
	}
	
	return(NULL);
}

struct ProgramCategory *GetCategorypek(char *namn, struct ProgramCategory *start)
{
	while(start != NULL)
	{
		if(!stricmp(namn, start->namn))
			return(start);
		start = start->next;
	}
	
	return(NULL);
}

void DisplayAll(char *string, struct ProgramCategory *start)
{
	struct ProgramData *pek;
	char buffer[257];

	sprintf(buffer,"%s\n", string);
	Debuglog(buffer);
	if(start != NULL)
	{
		while(start != NULL)
		{
			sprintf(buffer,"[%s]\n",start->namn);
			Debuglog(buffer);
			if(start->datastart != NULL)
			{
				pek = start->datastart;
				while(pek != NULL)
				{
					sprintf(buffer," %s=%s\n",pek->typ,pek->data);
					Debuglog(buffer);
					pek = pek->next;
				}
			}
			start = start->next;
		}
	}
	else
	{
		sprintf(buffer,"ProgramKategori listan är tom!\n");
		Debuglog(buffer);
	}
}

int DeleteCategory(char *namn, struct ProgramCategory **start)
{
	struct ProgramCategory *pek;
	
	pek = GetCategorypek(namn, *start);
	if(pek != NULL)
	{
		DeleteProgramData(pek->datastart);
		pek->datastart = NULL;
		if(pek->prev != NULL)
		{
			pek->prev->next = pek->next;
			pek->next->prev = pek->prev;
		}
		else
		{
			if(pek->next != NULL)
				*start = pek->next;
			else
				*start = NULL;
		}
		FreeMem(pek, sizeof(struct ProgramCategory));
		return(1);
	}
	
	return(0);
}

void DeleteProgramData(struct ProgramData *pek)
{
	if(pek != NULL)
	{
		DeleteProgramData(pek->next);
		FreeMem(pek, sizeof(struct ProgramData));
	}
}

void DeleteAll(struct ProgramCategory **start)
{
	struct ProgramCategory *pek, *pek2;

	pek = *start;

	while(pek != NULL)
	{
		pek2 = pek->next;

		DeleteProgramData(pek->datastart);
		pek->datastart = NULL;
		FreeMem(pek, sizeof(struct ProgramCategory));

		pek = pek2;
	}
	*start = NULL;
}

int LoadAll(char *filnamn, struct ProgramCategory **start)
{
	BPTR fh;
	char buffer[256], namn[50], *pek, typ[41], data[81], tempbuffer[257];
	int i;
	if(fh = Open(filnamn, MODE_OLDFILE))
	{
		while(FGets(fh,buffer,255))
		{
			if(strchr(buffer, '['))
			{
				pek = &buffer[1];
				buffer[strlen(buffer)-2] = NULL;
				strncpy(namn, pek, 80);
				sprintf(tempbuffer, "Prgcat Namn: %s\n", namn);
				Debuglog(tempbuffer);
				InsertProgramCategory(namn, start);
			}
			else
			{
				pek = &buffer[0];
				i = 0;

				while(pek[0] != '=')
				{
					typ[i] = pek[0];
					pek++;
					i++;
				}
				
				typ[i] = NULL;
				i = 0;
				pek++;
				
				while(pek[0] != '\n')
				{
					data[i] = pek[0];
					pek++;
					i++;
				}
				
				data[i] = NULL;
				sprintf(tempbuffer, "Prgcat Data: %s, %s\n", typ, data);
				Debuglog(tempbuffer);
				InsertProgramData(namn, typ, data, *start);
			}
		}
		Close(fh);
		return(1);
	}
	
	return(0);
}

int SaveAll(int usernumber, struct ProgramCategory *start)
{
	BPTR fh;
	char buffer[257], filnamn[257];
	struct ProgramData *pek;

	sprintf(filnamn, "NiKom:Users/%d/%d/ProgramData", usernumber/100, usernumber);

	if(start != NULL)
	{
		if(fh = Open(filnamn, MODE_NEWFILE))
		{
			while(start != NULL)
			{
				sprintf(buffer,"[%s]\n",start->namn);
				FPuts(fh, buffer);
				if(start->datastart != NULL)
				{
					pek = start->datastart;
					while(pek != NULL)
					{
						sprintf(buffer,"%s=%s\n",pek->typ,pek->data);
						FPuts(fh, buffer);
						pek = pek->next;
					}
				}
				start = start->next;
			}
		
			Close(fh);
			return(1);
		}
	}
	
	return(0);
}

void Debuglog(char *rad)
{

/*	BPTR fh;

	if(fh = Open("ram:NiKomdebuglog.txt", MODE_READWRITE))
	{
		Seek(fh,0,OFFSET_END);
		FPuts(fh, rad);
		Close(fh);
	} */
	
	return;
}
