#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include "NiKomStr.h"
#include "NiKomFuncs.h"
#include "Logging.h"

#include "ConfHeaderExtensions.h"

struct MemHeaderExtension *CreateMemHeaderExtension(long textId) {
  struct MemHeaderExtension *res;

  if(!(res = AllocMem(sizeof(struct MemHeaderExtension), MEMF_CLEAR | MEMF_PUBLIC))) {
    return NULL;
  }
  res->textId = textId;
  NewList((struct List *)&res->nodes);
  res->notInHeaderYet = 1;
}

void DeleteMemHeaderExtension(struct MemHeaderExtension *ext) {
  struct MemHeaderExtensionNode *node;
  while(node = (struct MemHeaderExtensionNode *)RemHead((struct List *)&ext->nodes)) {
    FreeMem(node, sizeof(struct MemHeaderExtensionNode));
  }
  FreeMem(ext, sizeof(struct MemHeaderExtension));
}

struct MemHeaderExtensionNode *AddMemHeaderExtensionNode(struct MemHeaderExtension *ext) {
  struct MemHeaderExtensionNode *node;

  if(!(node = AllocMem(sizeof(struct MemHeaderExtensionNode), MEMF_CLEAR | MEMF_PUBLIC))) {
    return NULL;
  }
  AddTail((struct List *)&ext->nodes, (struct Node *)node);
  node->dirty = 1;
  return node;
}

int SaveHeaderExtension(struct MemHeaderExtension *ext) {
  struct Header header;
  char fileName[40];
  BPTR fh;
  struct MemHeaderExtensionNode *node, *nextNode;
  int pos = 0;

  if(IsListEmpty((struct List *)&ext->nodes)) {
    return 0;
  }

  NiKForbid();
  sprintf(fileName, "NiKom:Moten/Extensions%d.dat", ext->textId / 512);
  if(!(fh = Open(fileName, MODE_READWRITE))) {
    NiKPermit();
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s for writing.", fileName);
    return 1;
  }

  ITER_EL_R(node, ext->nodes, node, struct MemHeaderExtensionNode *) {
    nextNode = (struct MemHeaderExtensionNode *) node->node.mln_Succ;
    if(node->ext.nextIndex == 0 && nextNode->node.mln_Succ != NULL) {
      // Only do this for nodes which were previosly the last node
      // or that are newly added but are not the last node.
      node->ext.nextIndex = nextNode->index;
      node->dirty = 1;
    }
    if(!node->dirty) {
      continue;
    }
    if(node->index > 0) {
      pos = (node->index - 1) * sizeof(struct HeaderExtension);
      if(Seek(fh, pos, OFFSET_BEGINNING) == -1) {
        Close(fh);
        NiKPermit();
        LogEvent(SYSTEM_LOG, ERROR, "Can't Seek() to pos %d in %s.", pos, fileName);
        return 1;
      }
    } else {
      if(Seek(fh, 0, OFFSET_END) == -1) {
        Close(fh);
        NiKPermit();
        LogEvent(SYSTEM_LOG, ERROR, "Can't Seek() to the end in %s.", fileName);
        return 1;
      }
      pos = Seek(fh, 0, OFFSET_CURRENT);
      node->index = (pos / sizeof(struct HeaderExtension)) + 1;
    }
    if(Write(fh, &node->ext ,sizeof(struct HeaderExtension)) == -1) {
      Close(fh);
      NiKPermit();
      LogEvent(SYSTEM_LOG, ERROR, "Error writing to %s.", fileName);
      return 1;
    }
  }
  Close(fh);
  NiKPermit();
  
  if(ext->notInHeaderYet) {
    if(readtexthead(ext->textId, &header)) {
      return 1;
    }
    header.extensionIndex = ((struct MemHeaderExtensionNode *)ext->nodes.mlh_Head)->index;
    if(writetexthead(ext->textId, &header)) {
      return 1;
    }
  }
  return 0;
}

struct MemHeaderExtension *ReadHeaderExtension(int textId, int index) {
  struct MemHeaderExtension *ext;
  struct MemHeaderExtensionNode *node;
  BPTR fh;
  int pos;
  char fileName[40];
  
  NiKForbid();
  sprintf(fileName, "NiKom:Moten/Extensions%d.dat", textId / 512);
  if(!(fh = Open(fileName, MODE_OLDFILE))) {
    NiKPermit();
    LogEvent(SYSTEM_LOG, ERROR, "Can't open %s for reading.", fileName);
    return NULL;
  }
  
  if(!(ext = CreateMemHeaderExtension(textId))) {
    Close(fh);
    NiKPermit();
    LogEvent(SYSTEM_LOG, ERROR, "Can't allocate MemHeaderExtension when reading text %d.", textId);
    return NULL;
  }
  ext->notInHeaderYet = 0;
  
  while(index > 0) {
    if(!(node = AddMemHeaderExtensionNode(ext))) {
      Close(fh);
      DeleteMemHeaderExtension(ext);
      NiKPermit();
      LogEvent(SYSTEM_LOG, ERROR,
               "Can't allocate MemHeaderExtensionNode when reading index %d for text %d.",
               index, textId);
      return NULL;
    }
    node->dirty = 0;
    node->index = index;

    pos = (index - 1) * sizeof(struct HeaderExtension);
    if(Seek(fh, pos, OFFSET_BEGINNING) == -1
       || Read(fh, &node->ext, sizeof(struct HeaderExtension)) != sizeof(struct HeaderExtension)) {
      Close(fh);
      DeleteMemHeaderExtension(ext);
      NiKPermit();
      LogEvent(SYSTEM_LOG, ERROR, "Can't read HeaderExtension at index %d for text %d.",
               index, textId);
      return NULL;
    }
    index = node->ext.nextIndex;
  }
  Close(fh);
  NiKPermit();
  return ext;
}
