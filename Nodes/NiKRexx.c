#include "NiKomCompat.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomFuncs.h"
#include "Terminal.h"
#include "RexxUtils.h"
#include "ExecUtils.h"
#include "Logging.h"
#include "BasicIO.h"
#include "KOM.h"
#include "Fifo.h"
#include "CommandParser.h"

#define EKO             1
#define EJEKO   0

extern struct System *Servermem;
extern long logintime,extratime;
extern int nodnr,inloggad,senast_text_typ,senast_text_nr,senast_text_mote,
  nu_skrivs,rad,mote2,area2,senast_brev_nr,senast_brev_anv,rxlinecount,radcnt,
  nodestate;
extern char outbuffer[],inmat[],*argument,brevtxt[][81],typeaheadbuf[],xprfilnamn[],vilkabuf[];
extern struct MsgPort *rexxport;
extern struct Header readhead;
extern struct MinList edit_list,tf_list;
extern struct Inloggning Statstr;

void handleRexxFinished(struct RexxMsg *nikrexxmess);
void handleRexxCommand(char *cmdName, struct RexxMsg *mess);
void rexxgetstring(struct RexxMsg *mess);
void rexxsendtextfile(struct RexxMsg *mess);
void senastread(struct RexxMsg *mess);
void kommando(struct RexxMsg *mess);
void niknrcommand(struct RexxMsg *mess);
void rxedit(struct RexxMsg *mess);
void rexxsendstring(struct RexxMsg *);
void rexxsendserstring(struct RexxMsg *);
void rexxgetchar(struct RexxMsg *mess);
void rexxchkbuffer(struct RexxMsg *mess);
void rexxyesno(struct RexxMsg *mess);
void whicharea(struct RexxMsg *mess);
void whichmeet(struct RexxMsg *mess);
void rexxsendbinfile(struct RexxMsg *mess);
void rexxrecbinfile(struct RexxMsg *mess);
void rxlogout(struct RexxMsg *mess);
void rexxvisabrev(struct RexxMsg *mess);
void rxrunfifo(struct RexxMsg *mess);
void rxrunrawfifo(struct RexxMsg *mess);
void rxvisatext(struct RexxMsg *mess);
void rxentermeet(struct RexxMsg *mess);
void rxsetlinecount(struct RexxMsg *mess);
void rxextratime(struct RexxMsg *mess);
void rxgettime(struct RexxMsg *mess);
void rxsendchar(struct RexxMsg *mess);
void rxsendserchar(struct RexxMsg *mess);
void rxsetnodeaction(struct RexxMsg *mess);
void rxsendrawfile(struct RexxMsg *);
void rxchglatestinfo(struct RexxMsg *);
void rxgetnumber(struct RexxMsg *mess);

static const char *rexxErrors[] = {
  "the script explicitly exited with an exit code > 0",
  "Program not found",
  "execution halted",
  "no memory available",
  "invalid character in program",
  "unmatched quote",
  "unterminated comment",
  "clause too long",
  "unrecognized token",
  "symbol or string too long",
  "invalid message packet",
  "command string error",
  "error return from function",
  "host environment not found",
  "required library not found",
  "function not found",
  "no return value",
  "wrong number of arguments",
  "invalid argument to function",
  "invalid PROCEDURE",
  "unexpected THEN/ELSE",
  "unexpected WHEN/OTHERWISE",
  "unexpected LEAVE or ITERATE",
  "invalid statement in SELECT",
  "missing THEN clauses",
  "missing OTHERWISE",
  "missing or unexpected END",
  "symbol mismatch on END",
  "invalid DO syntax",
  "incomplete DO/IF/SELECT",
  "label not found",
  "symbol expected",
  "string or symbol expected",
  "invalid sub-keyword",
  "required keyword missing",
  "extraneous characters",
  "sub-keyword conflict",
  "invalid template",
  "invalid TRACE request",
  "uninitialized variable",
  "invalid variable name",
  "invalid expression",
  "unbalanced parentheses",
  "nesting level exceeded",
  "invalid expression result",
  "expression required",
  "boolean value not 0 or 1",
  "arithmetic conversion error",
  "invalid operand"
};

void commonsendrexx(int komnr,int hasarg) {
  char rexxCmd[1081];
  struct RexxMsg *nikrexxmess, *mess;

  if(!hasarg) {
    sprintf(rexxCmd, "NiKom:rexx/ExtKom%d %d %d", komnr, nodnr, inloggad);
  } else {
    sprintf(rexxCmd, "NiKom:rexx/ExtKom%d %d %d %s", komnr, nodnr, inloggad, argument);
  }

  if(!(nikrexxmess = (struct RexxMsg *) CreateRexxMsg(
      rexxport, "nik", rexxport->mp_Node.ln_Name))) {
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't allocate a RexxMsg.");
    return;
  }
  if(!(nikrexxmess->rm_Args[0] =
       (STRPTR)CreateArgstring(rexxCmd,strlen(rexxCmd)))) {
    DeleteRexxMsg(nikrexxmess);
    LogEvent(SYSTEM_LOG, ERROR, "Couldn't allocate a rexx ArgString.");
    return;
  }

  nikrexxmess->rm_Action = RXCOMM;
  if(!SafePutToPort((struct Message *)nikrexxmess, "REXX")) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't launch ARexx script, REXX port not found.");
    return;
  }

  for(;;) {
    mess = (struct RexxMsg *)WaitPort(rexxport);
    while((mess = (struct RexxMsg *)GetMsg(rexxport))) {
      if(mess->rm_Node.mn_Node.ln_Type == NT_REPLYMSG) {
        handleRexxFinished(nikrexxmess);
        return;
      }
      handleRexxCommand(mess->rm_Args[0], mess);
      ReplyMsg((struct Message *)mess);
    }
  }
}

void handleRexxFinished(struct RexxMsg *nikrexxmess) {
  if(nikrexxmess->rm_Result1 != 0) {
    LogEvent(SYSTEM_LOG, WARN,
             "Error running ARexx script: error level %d, code %d: %s "
             "(command = \"%s\")",
             nikrexxmess->rm_Result1, nikrexxmess->rm_Result2,
             rexxErrors[nikrexxmess->rm_Result2], nikrexxmess->rm_Args[0]);
    SendString("\r\n\nError executing ARexx script.\r\n\n");
  }
  DeleteArgstring(nikrexxmess->rm_Args[0]);
  DeleteRexxMsg(nikrexxmess);
  if(!rxlinecount) {
    rxlinecount=TRUE;
    radcnt = 0;
  }
}

void handleRexxCommand(char *cmdName, struct RexxMsg *mess) {
      if(!strnicmp(cmdName,"sendstring",10)) rexxsendstring(mess);
      else if(!strnicmp(cmdName,"sendserstring",13)) rexxsendserstring(mess);
      else if(!strnicmp(cmdName,"getstring",9)) rexxgetstring(mess);
      else if(!strnicmp(cmdName,"sendtextfile",12)) rexxsendtextfile(mess);
      // sendfile(rexxargs1=hittaefter(cmdName));
      else if(!strnicmp(cmdName,"showtext",8)) rxvisatext(mess);
      else if(!strnicmp(cmdName,"showletter",10)) rexxvisabrev(mess);
      else if(!strnicmp(cmdName,"lasttext",8)) senastread(mess);
      else if(!strnicmp(cmdName,"nikcommand",10)) kommando(mess);
      else if(!strnicmp(cmdName,"niknrcommand",12)) niknrcommand(mess);
      else if(!strnicmp(cmdName,"edit",4)) rxedit(mess);
      else if(!strnicmp(cmdName,"sendbinfile",11)) rexxsendbinfile(mess);
      else if(!strnicmp(cmdName,"recbinfile",10)) rexxrecbinfile(mess);
      else if(!strnicmp(cmdName,"getchar",7)) rexxgetchar(mess);
      else if(!strnicmp(cmdName,"chkbuffer",9)) rexxchkbuffer(mess);
      else if(!strnicmp(cmdName,"yesno",5)) rexxyesno(mess);
      else if(!strnicmp(cmdName,"whicharea",9)) whicharea(mess);
      else if(!strnicmp(cmdName,"whichmeet",9)) whichmeet(mess);
      else if(!strnicmp(cmdName,"logout",6)) rxlogout(mess);
      else if(!strnicmp(cmdName,"runfifo",7)) rxrunfifo(mess);
      else if(!strnicmp(cmdName,"runrawfifo",10)) rxrunrawfifo(mess);
      else if(!strnicmp(cmdName,"entermeet",9)) rxentermeet(mess);
      else if(!strnicmp(cmdName,"setlinecount",12)) rxsetlinecount(mess);
      else if(!strnicmp(cmdName,"extratime",9)) rxextratime(mess);
      else if(!strnicmp(cmdName,"gettime",7)) rxgettime(mess);
      else if(!strnicmp(cmdName,"sendchar",8)) rxsendchar(mess);
      else if(!strnicmp(cmdName,"sendserchar",11)) rxsendserchar(mess);
      else if(!strnicmp(cmdName,"setnodeaction",13)) rxsetnodeaction(mess);
      else if(!strnicmp(cmdName,"sendrawfile",11)) rxsendrawfile(mess);
      else if(!strnicmp(cmdName,"changelatestinfo",16)) rxchglatestinfo(mess);
      else if(!strnicmp(cmdName,"getnumber",9)) rxgetnumber(mess);
      else {
        LogEvent(SYSTEM_LOG, WARN,
                 "Unknown command from ARexx script: '%s'", cmdName);
        SetRexxErrorResult(mess, 10);
      }
}

void sendrexx(int komnr) {
  commonsendrexx(komnr,1);
}

void sendautorexx(int komnr) {
  commonsendrexx(komnr,0);
}

void rexxsendstring(struct RexxMsg *mess) {
  char *str;
  int res;

  str = hittaefter(mess->rm_Args[0]);
  res = puttekn(str, -1);
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {    
    SetRexxResultString(mess, res ? "1" : "0");
  }
}

void rexxsendserstring(struct RexxMsg *mess) {
  char *str, res;
  str = hittaefter(mess->rm_Args[0]);
  if(Servermem->nodtyp[nodnr] != NODCON) {
    res = serputtekn(str, -1);
  } else {
    res = conputtekn(str, -1);
  }
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {    
    SetRexxResultString(mess, res ? "1" : "0");
  }
}

void rexxgetstring(struct RexxMsg *mess) {
  char *rexxargs1, *rexxargs2;
  char defaultstring[257] = "";
  int echo = EKO, maxchrs = 50;

  rexxargs1 = hittaefter(mess->rm_Args[0]);
  rexxargs2 = hittaefter(rexxargs1);

  if(rexxargs1[0]) {
    maxchrs = atoi(rexxargs1);
    if(maxchrs == 0) {
      maxchrs = 50;
    }
  }

  if(rexxargs2[0]) {
    if(strncmp(rexxargs2,"NOECHO",6) == 0) {
      echo = EJEKO;
    } else if(strncmp(rexxargs2,"STARECHO",8) == 0) {
      echo = STAREKO;
    } else {
      strcpy(defaultstring,rexxargs2);
    }
  }

  if(getstring(echo, maxchrs, defaultstring)) {
    SetRexxErrorResult(mess, 100);
  } else {
    SetRexxResultString(mess, inmat);
  }
}

void rexxsendtextfile(struct RexxMsg *mess) {
  char *filename;

  filename = hittaefter(mess->rm_Args[0]);
  if(filename[0] == '\0') {
    SetRexxErrorResult(mess, 10);
    return;
  }
  sendfile(filename);
  SetRexxErrorResult(mess, ImmediateLogout() ? 100 : 0);
}

void senastread(struct RexxMsg *mess) {
  char tempstr[20];
  if(senast_text_typ != BREV && senast_text_typ!=TEXT) {
    SetRexxResultString(mess, NULL);
    return;
  }
  if(senast_text_typ==BREV) {
    sprintf(tempstr,"B %d %d",senast_brev_anv,senast_brev_nr);
  } else {
    sprintf(tempstr,"T %d %d",senast_text_nr,senast_text_mote);
  }
  SetRexxResultString(mess, tempstr);
}

void kommando(struct RexxMsg *mess) {
  int parseRes, cmdId = 0;
  char *cmdStr;
  struct Kommando *parseResult[50], *cmd = NULL;

  static char argbuf[1081];

  cmdStr = hittaefter(mess->rm_Args[0]);

  parseRes = ParseCommand(cmdStr, Servermem->inne[nodnr].language, &Servermem->inne[nodnr], parseResult, argbuf);
  switch(parseRes) {
  case -2:
    cmdId = 212;
    argument = cmdStr;
    break;

  case -1:
    LogEvent(SYSTEM_LOG, WARN, "Empty command sent to ARexx 'NikCommand'");   
    SetRexxResultString(mess, "2");
    return;

  case 0:
    LogEvent(SYSTEM_LOG, WARN,
             "Invalid NiKom command in ARexx 'NikCommand': '%s'", cmdStr);
    SetRexxResultString(mess, "1");
    return;

  case 1:
    if(!HasUserCmdPermission(parseResult[0], &Servermem->inne[nodnr])
       || parseResult[0]->losen[0]) {
      LogEvent(SYSTEM_LOG, WARN,
               "User %d has no permission executing NiKom command in ARexx 'NikCommand': '%s'",
               inloggad, cmdStr);   
      SetRexxResultString(mess, "3");
      return;
    }
    cmd = parseResult[0];
    argument = argbuf;
    break;

  default:
    LogEvent(SYSTEM_LOG, WARN,
             "Ambigous NiKom command in ARexx 'NikCommand': '%s'", cmdStr);
    SetRexxResultString(mess, "4");
    return;
  }

  if(cmd == NULL) {
    if((cmd = getkmdpek(cmdId)) == NULL) {
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't find command definition for command %d when trying to "
               "execute %s from ARexx.", cmdId, cmdStr);
      SetRexxErrorResult(mess, 20);
      return;
    }
  }
  ExecuteCommand(cmd);
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {
    SetRexxResultString(mess, "0");
  }
}

void niknrcommand(struct RexxMsg *mess) {
  int cmdId;
  char *commandstr = hittaefter(mess->rm_Args[0]);

  cmdId = atoi(commandstr);
  argument=hittaefter(commandstr);

  ExecuteCommandById(cmdId);
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {
    SetRexxErrorResult(mess, 0);
  }
}

void rxedit(struct RexxMsg *mess) {
  int editret, cnt, len;
  char *filename;
  BPTR fh;
  struct EditLine *el;
  nu_skrivs = ANNAT;
  filename = hittaefter(mess->rm_Args[0]);
  if(filename[0] == '\0') {
    SetRexxErrorResult(mess, 5);
    return;
  }

  if((editret = edittext(filename)) == 1) {
    SetRexxErrorResult(mess, 100);
    return;
  } else if(editret == 0) {
    if(!(fh = Open(filename, MODE_NEWFILE))) {
      LogEvent(SYSTEM_LOG, WARN,
               "Could not open '%s' for writing in ARexx 'edit'.", filename);
      SetRexxErrorResult(mess, 20);
      return;
    }
    cnt = 0;
    ITER_EL(el, edit_list, line_node, struct EditLine *) {
      len = strlen(el->text);
      el->text[len] = '\n';
      if(Write(fh, el->text, len + 1) == -1) {
        LogEvent(SYSTEM_LOG, WARN,
                 "Error writing '%s' in ARexx 'edit'.", filename);
        Close(fh);
        SetRexxErrorResult(mess, 20);
        return;
      }
      cnt++;
    }
    Close(fh);
    freeeditlist();
  }
  SetRexxResultString(mess, editret ? "0" : "1");
}

void rexxgetchar(struct RexxMsg *mess) {
  char singleCharStr[2] = " ", *res;
  int ch;

  ch = GetChar();
  if(ch > 0) {
    singleCharStr[0] = ch;
    res = singleCharStr;
  } else {
    switch(ch) {
    case GETCHAR_LOGOUT:
      SetRexxErrorResult(mess, 100);
      return;
    case GETCHAR_RETURN:
      res = "\r";
      break;
    case GETCHAR_SOL:
      res = "SOL";
      break;
    case GETCHAR_EOL:
      res = "EOL";
      break;
    case GETCHAR_BACKSPACE:
      res = "\b";
      break;
    case GETCHAR_DELETE:
      res = "DELETE";
      break;
    case GETCHAR_DELETELINE:
      res = "DELETELINE";
      break;
    case GETCHAR_UP:
      res = "UP";
      break;
    case GETCHAR_DOWN:
      res = "DOWN";
      break;
    case GETCHAR_RIGHT:
      res = "RIGHT";
      break;
    case GETCHAR_LEFT:
      res = "LEFT";
      break;
    default:
      /* Should not happen. */
      res = NULL;
      break;
    }
  }
  SetRexxResultString(mess, res);
}

void rexxchkbuffer(struct RexxMsg *mess) {
  char res[5];

  sprintf(res, "%d", checkcharbuffer());
    SetRexxResultString(mess, res);
  mess->rm_Result1=0;
}

void rexxyesno(struct RexxMsg *mess) {
  int isYes;
  char yes, no, def,*argstr;
  argstr = hittaefter(mess->rm_Args[0]);
  if(argstr[0] == '\0') {
    yes = 'j'; no = 'n'; def = 1;
  } else {
    yes = argstr[0];
    argstr = hittaefter(argstr);
    if(argstr[0] == '\0') {
      no = 'n'; def = 1;
    } else {
      no = argstr[0];
      argstr = hittaefter(argstr);
      if(argstr[0] == '\0') {
        def=1;
      } else {
        def = argstr[0] - '0';
        if(def != 1 && def != 2) {
          SetRexxErrorResult(mess, 5);
        }
      }
    }
  }

  if(GetYesOrNo(NULL, NULL, &yes, &no, NULL, NULL, NULL, def == 1, &isYes)) {
    SetRexxErrorResult(mess, 100);
    return;
  }
  SetRexxResultString(mess, isYes ? "1" : "0");
}

void whicharea(struct RexxMsg *mess) {
  char buf[5];
  sprintf(buf,"%d",area2);
  SetRexxResultString(mess, buf);
}

void whichmeet(struct RexxMsg *mess) {
  char buf[5];
  sprintf(buf,"%d",mote2);
  SetRexxResultString(mess, buf);
}

void rexxsendbinfile(struct RexxMsg *mess) {
  char buf[257], *argstr, filtemp[257], tmp[3];
  int i, quote;
  struct TransferFiles *tf;
  
  argstr = hittaefter(mess->rm_Args[0]);
  NewList((struct List *)&tf_list);
  // TODO: Extract into some generic tokenization function
  while(argstr[0] != '\0') {
    filtemp[0] = '\0';
    i = 0; quote = 0;
    while(argstr[0] == ' ' && argstr[0] != '\0') {
      argstr++;
    }
    if(argstr[0]=='"') {
      quote=1; argstr++;
    }
    if(argstr[0] == '\0') {
      break;
    }
    while(((argstr[0] != '"' && quote)
           || (argstr[0] != ' ' && !quote))
          && argstr[0] != '\0') {
      filtemp[i++] = argstr[0];
      argstr++;
    }
    if(argstr[0] == '"') {
      argstr++;
    }
    filtemp[i] = '\0';
    if(!(tf=(struct TransferFiles *)AllocMem(sizeof(struct TransferFiles),MEMF_CLEAR))) {
      // TODO: Just breaking here is pretty lame..
      break;
    } else {
      strcpy(tf->path, filtemp);
      tf->filpek = NULL;
      AddTail((struct List *)&tf_list,(struct Node *)tf);
    }
  }
  sendbinfile();
  buf[0] = '\0';
  ITER_EL(tf, tf_list, node, struct TransferFiles *) {
    sprintf(tmp,"%d ",tf->sucess);
    strcat(buf,tmp);
  }
  buf[strlen(buf) - 1] = '\0';
  
  while((tf=(struct TransferFiles *)RemHead((struct List *)&tf_list)))
    FreeMem(tf,sizeof(struct TransferFiles));

  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  }
  SetRexxResultString(mess, buf);
}

void rexxrecbinfile(struct RexxMsg *mess) {
  char buf[1024];
  struct TransferFiles *tf;

  Servermem->action[nodnr] = UPLOAD;
  Servermem->varmote[nodnr] = 0;
  Servermem->vilkastr[nodnr] = NULL;

  if(recbinfile(hittaefter(mess->rm_Args[0]))) {
    SetRexxErrorResult(mess, ImmediateLogout() ? 100 : 10);
    return;
  }
  buf[0] = '\0';
  ITER_EL(tf, tf_list, node, struct TransferFiles *) {
    strcat(buf,FilePart(tf->Filnamn));
    strcat(buf,(char *)" ");
  }
  buf[strlen(buf)-1] = '\0';

  while((tf=(struct TransferFiles *)RemHead((struct List *)&tf_list)))
    FreeMem(tf,sizeof(struct TransferFiles));
  SetRexxResultString(mess, buf);
}

void rxlogout(struct RexxMsg *mess) {
  nodestate |= NIKSTATE_USERLOGOUT;
  SetRexxErrorResult(mess, 0);
}

void rexxvisabrev(struct RexxMsg *mess) {
  char *argstr1,*argstr2;
  argstr1=hittaefter(mess->rm_Args[0]);
  argstr2=hittaefter(argstr1);
  if(!argstr1[0] || !argstr2[0]) {
    SetRexxErrorResult(mess, 5);
    return;
  }
  visabrev(atoi(argstr2),atoi(argstr1));
  SetRexxErrorResult(mess, 0);
}

void rxrunfifo(struct RexxMsg *mess) {
  ExecFifo(hittaefter(mess->rm_Args[0]),TRUE);
  SetRexxErrorResult(mess, 0);
}

void rxrunrawfifo(struct RexxMsg *mess) {
  ExecFifo(hittaefter(mess->rm_Args[0]),FALSE);
  SetRexxErrorResult(mess, 0);
}

void rxvisatext(struct RexxMsg *mess) {
  struct Mote *conf;
  int text, confId, type;
  char *argstr;

  argstr=hittaefter(mess->rm_Args[0]);
  if(!argstr[0]) {
    SetRexxErrorResult(mess, 5);
    return;
  }
  text = atoi(argstr);
  argstr = hittaefter(argstr);
  if(!argstr[0]) {
    type=MOTE_ORGINAL;
  } else {
    confId = atoi(argstr);
    conf = getmotpek(confId);
    if(!conf) {
      SetRexxErrorResult(mess, 5);
      return;
    }
    type = conf->type;
  }
  if(type == MOTE_ORGINAL) {
    org_visatext(text, FALSE);
  }
  // TODO: Other conference types
  SetRexxErrorResult(mess, 0);
}

void rxentermeet(struct RexxMsg *mess) {
  int confId;
  confId = atoi(hittaefter(mess->rm_Args[0]));
  if(!getmotpek(confId)) {
    SetRexxErrorResult(mess, 5);
    return;
  }
  GoConf(confId);
  SetRexxErrorResult(mess, 0);
}

void rxsetlinecount(struct RexxMsg *mess) {
  char *arg = hittaefter(mess->rm_Args[0]);
  if(stricmp(arg,"ON") == 0) {
    rxlinecount=TRUE;
    radcnt = 0;
  } else if(stricmp(arg,"OFF") == 0) {
    rxlinecount=FALSE;
    radcnt = 0;
  } else {
    SetRexxErrorResult(mess, 5);
    return;
  }
  SetRexxErrorResult(mess, 0);
}

void rxextratime(struct RexxMsg *mess) {
  char buf[10], *arg=hittaefter(mess->rm_Args[0]);

  if(strcmp(arg,"GET") == 0) {
    sprintf(buf,"%ld",extratime);
    SetRexxResultString(mess, buf);
  } else {
    extratime=atoi(arg);
    SetRexxErrorResult(mess, 0);
  }
}

void rxgettime(struct RexxMsg *mess) {
  long limit, timeLeft, now;
  char buf[10];
  if(Servermem->cfg->maxtid[Servermem->inne[nodnr].status] > 0) {
    time(&now);
    limit = 60 * Servermem->cfg->maxtid[Servermem->inne[nodnr].status] + extratime;
    timeLeft = limit - (now - logintime);
  } else {
    timeLeft=0;
  }
  sprintf(buf,"%ld",timeLeft);
  SetRexxResultString(mess, buf);
}

void rxsendchar(struct RexxMsg *mess) {
  char *arg;
  arg = hittaefter(mess->rm_Args[0]);
  if(arg[0] != '/' || arg[1] == '\0') {
    SetRexxErrorResult(mess, 5);
    return;
  }
  eka(arg[1]);
  SetRexxErrorResult(mess, ImmediateLogout() ? 100 : 0);
}

void rxsendserchar(struct RexxMsg *mess) {
  char *arg;
  arg=hittaefter(mess->rm_Args[0]);
  if(arg[0] != '/' || arg[1] == '\0') {
    SetRexxErrorResult(mess, 5);
    return;
  }
  sereka(arg[1]);
  SetRexxErrorResult(mess, ImmediateLogout() ? 100 : 0);
}

void rxsetnodeaction(struct RexxMsg *mess) {
  strncpy(vilkabuf, hittaefter(mess->rm_Args[0]), 49);
  vilkabuf[49] = '\0';
  Servermem->action[nodnr] = GORNGTANNAT;
  Servermem->vilkastr[nodnr] = vilkabuf;
  SetRexxErrorResult(mess, 0);
}

void rxsendrawfile(struct RexxMsg *mess) {
  BPTR fh;
  int bytesRead;
  char *retstr, *filename = hittaefter(mess->rm_Args[0]);
  if(!(fh = Open(filename,MODE_OLDFILE))) {
    retstr = "0";
  } else {
    while((bytesRead = Read(fh,outbuffer,99))) {
      if(bytesRead == -1) {
        break;
      }
      outbuffer[bytesRead] = 0;
      putstring(outbuffer, -1, 0);
    }
    Close(fh);
    retstr = "1";
  }
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {
    SetRexxResultString(mess, retstr);
  }
}

void rxchglatestinfo(struct RexxMsg *mess) {
  char *what, retstr[15];
  int amount, retval;
  what = hittaefter(mess->rm_Args[0]);
  amount = atoi(hittaefter(what));
  switch(what[0]) {
  case 'r' : case 'R' :
    retval = Statstr.read += amount;
    break;
  case 'w' : case 'W' :
    retval = Statstr.write += amount;
    break;
  case 'd' : case 'D' :
    retval = Statstr.dl += amount;
    break;
  case 'u' : case 'U' :
    retval = Statstr.ul += amount;
    break;
  default:
    SetRexxErrorResult(mess, 5);
    return;
  }
  sprintf(retstr, "%d", retval);
  SetRexxResultString(mess, retstr);
}

void rxgetnumber(struct RexxMsg *mess) {
  int minvalue = INT_MIN, maxvalue = INT_MAX, defaultvalue;
  char retstr[12], defaultvaluestr[12] = "";
  char *arg1 = NULL, *arg2 = NULL, *arg3 = NULL;
  
  arg1 = hittaefter(mess->rm_Args[0]);
  arg2 = hittaefter(arg1);
  arg3 = hittaefter(arg2);

  if(arg1[0] && !arg2[0]) {
    defaultvalue = atoi(arg1);
    sprintf(defaultvaluestr, "%d", defaultvalue);
  } else if(arg1[0] && arg2[0]) {
    minvalue = atoi(arg1);
    maxvalue = atoi(arg2);
    if(arg3[0]) {
      defaultvalue = atoi(arg3);
      sprintf(defaultvaluestr, "%d", defaultvalue);
    }
  }

  sprintf(retstr, "%d", GetNumber(minvalue, maxvalue, defaultvaluestr));
  if(ImmediateLogout()) {
    SetRexxErrorResult(mess, 100);
  } else {
    SetRexxResultString(mess, retstr);
  }
}
