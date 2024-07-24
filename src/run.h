#ifndef _INCLUDE_RUN
#define _INCLUDE_RUN

typedef struct runCommand
  {
  char* label;
  char* path;
  char* folderArg;
  char* configArg;
  char* workDir;
  char* success;
  _TAG_VALUE* outputFiles;
  struct runCommand* next;
  } _RUN_COMMAND;

_RUN_COMMAND* NewRunCommand( char* label, _RUN_COMMAND* list );
_RUN_COMMAND* FindRunCommand( _RUN_COMMAND* list, char* label );
void FreeRunCommand( _RUN_COMMAND* rc );
void ValidateRunCommand( _RUN_COMMAND* rc );
void PrintRunCommand( FILE* f, _RUN_COMMAND* rc );

#endif
