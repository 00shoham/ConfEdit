#ifndef _INCLUDE_USER
#define _INCLUDE_USER

typedef struct _user
  {
  char* id;
  char* folder;

  struct _user* next;
  } _USER;

_USER* NewUser( char* id, _USER* list );
_USER* FindUser( _USER* list, char* id );
void PrintUser( FILE* f, _USER* u );
int ValidateUser( _USER* u );
void FreeUser( _USER* u );

#endif
