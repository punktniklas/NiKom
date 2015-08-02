/* Håll reda på NiKom versionerna... */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
	char *filnamn = "NiKomsrc:nikversion";
	FILE *fil;
	char nikversion[8], nikrevision[8], nikrelease[64];
	char niklibversion[8], niklibrevision[8];
	char nikmenyversion[8], nikmenyrevision[8];
	char nikserverversion[8], nikserverrevision[8];
	int revision, librevision, menyrevision, typ;
	char buffer[1024], datum[22];
	int serverrevision;
	time_t t;
	struct tm *tid;
	
	char argument[512];

	if(argc < 2)
	{
		printf("Skriv: nikversion <typ> <argument>\n");
		return 100;
	}

	if(!(fil = fopen(filnamn,"r")))
	{
		printf("Kunde inte öppna filen %s för läsning\n",filnamn);
		return 100;
	}
	
	typ = atoi(argv[1]);
	strcpy(argument,argv[2]);

	/* Typ kan vara:
	1	NiKom version
	2	Nikom LIBRARY version
	3	Menynod version
	4	Server version
	5	Länkningen av library versionen.
	*/
	
	fgets(nikrelease,63,fil);
	fgets(nikversion,7,fil);
	fgets(nikrevision,7,fil);
	fgets(niklibversion,7,fil);
	fgets(niklibrevision,7,fil);
	fgets(nikmenyversion,7,fil);
	fgets(nikmenyrevision,7,fil);
	fgets(nikserverversion,7,fil);
	fgets(nikserverrevision,7,fil);
	fclose(fil);

	nikrelease[strlen(nikrelease)-1] = NULL;
	nikversion[strlen(nikversion)-1] = NULL;
	nikrevision[strlen(nikrevision)-1] = NULL;
	niklibversion[strlen(niklibversion)-1] = NULL;
	niklibrevision[strlen(niklibrevision)-1] = NULL;
	nikmenyversion[strlen(nikmenyversion)-1] = NULL;
	nikmenyrevision[strlen(nikmenyrevision)-1] = NULL;
	nikserverversion[strlen(nikserverversion)-1] = NULL;
	nikserverrevision[strlen(nikserverrevision)-1] = NULL;
	
	if(typ == 1)
	{
		revision = atoi(nikrevision) + 1 ;
		sprintf(nikrevision,"%d",revision);
	}
	else if(typ == 2)
	{
		printf("%s\n", niklibrevision);
		librevision = atoi(niklibrevision) + 1;
		sprintf(niklibrevision,"%d",librevision);
		printf("%s\n", niklibrevision);
	}
	else if(typ == 3)
	{
		menyrevision = atoi(nikmenyrevision) + 1;
		sprintf(nikmenyrevision,"%d",menyrevision);
	}
	else if(typ == 4)
	{
		serverrevision = atoi(nikserverrevision) + 1;
		sprintf(nikserverrevision,"%d",serverrevision);
	}
	
	printf("NRel: %s\n", nikrelease);
	printf("Version %s.%s\n", nikversion, nikrevision);
	printf("Lib version: %s.%s\n", niklibversion, niklibrevision);
	printf("Meny version: %s.%s\n", nikmenyversion, nikmenyrevision);
	printf("Server version: %s.%s\n", nikserverversion, nikserverrevision);

	if(typ == 5)
	{
		time(&t);
		tid = localtime(&t);
		strftime(datum, 21 , "%d.%m.%y", tid);
		sprintf(buffer, "%s LIBVERSION %s LIBREVISION %s LIBID \"nikom.library %s.%s %s\"", argument, niklibversion, niklibrevision, niklibversion, niklibrevision, datum);
		system(buffer);
		return 0;
	}

	if(typ != 0)
	{
		if(!(fil = fopen(filnamn,"w")))
		{
			printf("Kunde inte öppna filen %s för skrivning\n",filnamn);
			return 100;
		}

		fprintf(fil,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", nikrelease, nikversion, nikrevision, niklibversion, niklibrevision, nikmenyversion, nikmenyrevision, nikserverversion, nikserverrevision);
		fclose(fil);
	}
	
	if(typ != 3)
	{
		if(typ == 4)
			sprintf(argument,"%s DEFINE=NIKSERVERVERSION=\"%s\" DEFINE=NIKSERVERREVISION=\"%s\"", argument, nikserverversion, nikserverrevision);
		else
			sprintf(argument,"%s DEFINE=NIKRELEASE=\"%s\" DEFINE=NIKVERSION=\"%s\" DEFINE=NIKREVISION=\"%s\" DEFINE=NIKLIBVERSION=\"%s\" DEFINE=NIKLIBREVISION=\"%s\"", argument, nikrelease, nikversion, nikrevision, niklibversion, niklibrevision);
	}
	else
	{
		sprintf(argument,"%s DEFINE=NIKRELEASE=\"%s\" DEFINE=NIKVERSION=\"%s\" DEFINE=NIKREVISION=\"%s\" DEFINE=NIKLIBVERSION=\"%s\" DEFINE=NIKLIBREVISION=\"%s\"", argument, nikrelease, nikmenyversion, nikmenyrevision, niklibversion, niklibrevision);
	}
	
	if(argc > 2)
		system(argument);
}
