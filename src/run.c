#include "base.h"

_RUN_COMMAND* NewRunCommand( char* label, _RUN_COMMAND* list )
  {
  if( EMPTY( label ) )
    Error( "Cannot create a new but blank _RUN_COMMAND" );

  _RUN_COMMAND* rc = (_RUN_COMMAND*)SafeCalloc( 1, sizeof( _RUN_COMMAND ), "_RUN_COMMAND" );
  rc->label = strdup( label );
  rc->next = list;
  return rc;
  }

_RUN_COMMAND* FindRunCommand( _RUN_COMMAND* list, char* label )
  {
  if( EMPTY( label ) )
    return NULL;

  while( list!=NULL )
    {
    if( NOTEMPTY( list->label ) && strcasecmp( list->label, label )==0 )
      return list;

    list = list->next;
    }

  return NULL;
  }

void FreeRunCommand( _RUN_COMMAND* rc )
  {
  if( rc->next )
    {
    FreeRunCommand( rc->next );
    rc->next = NULL;
    }

  FreeIfAllocated( &(rc->label) );
  FreeIfAllocated( &(rc->path) );
  FreeIfAllocated( &(rc->folderArg) );
  FreeIfAllocated( &(rc->configArg) );
  FreeIfAllocated( &(rc->success) );

  FreeTagValue( rc->outputFiles );
  free( rc );
  }

void ValidateRunCommand( _RUN_COMMAND* rc )
  {
  if( rc==NULL )
    return;

  if( EMPTY( rc->label ) )
    Error( "COMMAND defined but has no label" );

  if( EMPTY( rc->path ) )
    Error( "COMMAND %s defined but has no path", rc->label );

  if( EMPTY( rc->configArg ) )
    Error( "COMMAND %s does not specify a CONFIG file argument", rc->label );

  if( EMPTY( rc->success ) )
    Error( "COMMAND %s does not specify a COMMAND_SUCCESS file argument", rc->label );

  if( rc->outputFiles!=NULL && EMPTY( rc->workDir ) )
    Error( "COMMAND %s has output files but no COMMAND_WORKDIR", rc->label );

  if( NOTEMPTY( rc->workDir ) )
    {
    if( StringIsSimpleFolder( rc->folderArg )!=0 )
      Error( "COMMAND %s specifies a folder with spaces or other nontrivial chars.", rc->label );

    if( DirExists( rc->workDir )!=0 )
      {
      int err = mkdir( rc->workDir, 0755 );
      if( err )
        Error( "COMMAND %s specifies a folder (%s) that does not exist and count not be created (%d:%d:%s)",
               rc->label, rc->workDir, err, errno, strerror( errno ) );
      if( DirExists( rc->workDir )!=0 )
        Error( "COMMAND %s specifies a folder (%s) that does not exist and was not created",
               rc->label, rc->workDir );
      }
    }

  for( _TAG_VALUE* of=rc->outputFiles; of!=NULL; of=of->next )
    {
    if( EMPTY( of->tag ) )
      Error( "COMMAND %s has an output file with no argument label" );
    if( EMPTY( of->value ) )
      Error( "COMMAND %s has an output file with no argument cmdline" );
    }

  for( _RUN_COMMAND* rcTest = rc->next; rcTest!=NULL; rcTest=rcTest->next )
    {
    if( NOTEMPTY( rcTest->label ) && strcasecmp( rcTest->label, rc->label )==0 )
      Error( "Two run commands called [%s}", rc->label );
    }
  }

void PrintRunCommand( FILE* f, _RUN_COMMAND* rc )
  {
  if( f==NULL )
    return;
  if( rc==NULL )
    return;
  if( EMPTY( rc->label ) )
    Error( "Command with no 'command'" );

  fprintf( f, "COMMAND=%s\n", rc->label );
  if( NOTEMPTY( rc->path ) )
    fprintf( f, "COMMAND_PATH=%s\n", rc->path );
  if( NOTEMPTY( rc->folderArg ) )
    fprintf( f, "FOLDER_ARG=%s\n", rc->folderArg );
  if( NOTEMPTY( rc->configArg ) )
    fprintf( f, "CONFIG_ARG=%s\n", rc->configArg );
  if( NOTEMPTY( rc->success ) )
    fprintf( f, "COMMAND_SUCCESS=%s\n", rc->success );
  if( NOTEMPTY( rc->workDir ) )
    fprintf( f, "COMMAND_WORKDIR=%s\n", rc->workDir );
  for( _TAG_VALUE* of=rc->outputFiles; of!=NULL; of=of->next )
    if( NOTEMPTY( of->value ) )
      fprintf( f, "COMMAND_OUTPUT=%s:%s\n", of->tag, of->value );
  }


