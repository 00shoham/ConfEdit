#include "base.h"

_USER* NewUser( char* id, _USER* list )
  {
  if( EMPTY( id ) )
    Error( "NewUser without an ID" );
  _USER* u = (_USER*)SafeCalloc( 1, sizeof(_USER), "New USER" );
  u->id = strdup( id );
  u->next = list;
  return u;
  }

_USER* FindUser( _USER* list, char* id )
  {
  if( EMPTY( id ) )
    return NULL;

  while( list!=NULL )
    {
    if( NOTEMPTY( list->id ) && strcasecmp( list->id, id )==0 )
      return list;
    list = list->next;
    }

  return NULL;
  }

void PrintUser( FILE* f, _USER* u )
  {
  if( f==NULL || u==NULL || EMPTY( u->id ) )
    return;

  fprintf( f, "USER=%s\n", u->id );
  if( NOTEMPTY( u->folder ) )
    fprintf( f, "FOLDER=%s\n", u->folder );
  }

int ValidateUser( _USER* u )
  {
  if( u==NULL )
    Error( "Invalid - NULL - user" );
  if( EMPTY( u->id ) )
    Error( "Invalid - empty - user ID" );
  if( NOTEMPTY( u->folder )
      && DirExists( u->folder ) !=0 )
    Error( "User %s supposed to use folder %s but it does not exist", u->id, u->folder );

  return 0;
  }

void FreeUser( _USER* u )
  {
  if( u==NULL )
    return;

  if( u->next!=NULL )
    FreeUser( u->next );

  FREEIFNOTNULL( u->id );
  FREEIFNOTNULL( u->folder );

  free( u );
  }
