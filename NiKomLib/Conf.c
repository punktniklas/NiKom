#include <exec/types.h>
#include <dos/dos.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

struct Mote * __saveds __asm LIBGetConfPoint(register __d0 int motnr, register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *letpek;

	if(!NiKomBase->Servermem) return(NULL);

	letpek=(struct Mote *)NiKomBase->Servermem->mot_list.mlh_Head;
	for(;letpek->mot_node.mln_Succ;letpek=(struct Mote *)letpek->mot_node.mln_Succ)
		if(letpek->nummer==motnr) return(letpek);
	return(NULL);
}

int __saveds __asm LIBMaySeeConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(usr->status >= NiKomBase->Servermem->cfg.st.medmoten) return(TRUE);
	if(motpek->status & SUPERHEMLIGT) return(FALSE);
	if(motpek->status & AUTOMEDLEM) return(TRUE);
	if((motpek->status & HEMLIGT) && !LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase)) return(FALSE);
	return(TRUE);
}

int __saveds __asm LIBMayBeMemberConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(usr->status >= NiKomBase->Servermem->cfg.st.medmoten) return(TRUE);
	if(motpek->status & SUPERHEMLIGT) return(TRUE);
	if(motpek->status & AUTOMEDLEM) return(TRUE);
	if(motpek->status & SKRIVSTYRT) return(TRUE);
	if(BAMTEST(usr->motratt,mote)) return(TRUE);
	if(motpek->grupper & usr->grupper) return(TRUE);
	return(FALSE);
}

int __saveds __asm LIBMayReadConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {

	return(LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase));
}

int __saveds __asm LIBMayWriteConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(!LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase)) return(FALSE);
	if(LIBMayAdminConf(mote,usrnr,usr,NiKomBase)) return(TRUE);
	if(usr->status >= NiKomBase->Servermem->cfg.st.skriv) return(TRUE);
	if(motpek->status & SKRIVSKYDD) return(FALSE);
	if(motpek->status & SKRIVSTYRT) return(BAMTEST(usr->motratt,mote) || (motpek->grupper & usr->grupper));
	return(TRUE);
}

int __saveds __asm LIBMayReplyConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(!LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase)) return(FALSE);
	if(LIBMayAdminConf(mote,usrnr,usr,NiKomBase)) return(TRUE);
	if(usr->status >= NiKomBase->Servermem->cfg.st.skriv) return(TRUE);
	if(motpek->status & KOMSKYDD) return(FALSE);
	if(motpek->status & SKRIVSTYRT) return(BAMTEST(usr->motratt,mote) || (motpek->grupper & usr->grupper));
	return(TRUE);
}

int __saveds __asm LIBMayAdminConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(!LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase)) return(FALSE);
	if(usr->status >= NiKomBase->Servermem->cfg.st.texter) return(TRUE);
	if(motpek->skapat_av ==  usrnr) return(TRUE);
	if(motpek->mad == usrnr) return(TRUE);
	return(FALSE);
}

int __saveds __asm LIBIsMemberConf(register __d0 int mote, register __d1 int usrnr, register __a0 struct User *usr,register __a6 struct NiKomBase *NiKomBase) {
	struct Mote *motpek;

	if(!NiKomBase->Servermem) return(FALSE);

	motpek = LIBGetConfPoint(mote,NiKomBase);
	if(!motpek) return(FALSE);

	if(motpek->status & SUPERHEMLIGT) return(TRUE);
	if(motpek->status & AUTOMEDLEM) return(TRUE);
	if(!LIBMayBeMemberConf(mote,usrnr,usr,NiKomBase)) return(FALSE);
	return(BAMTEST(usr->motmed,mote));
}
