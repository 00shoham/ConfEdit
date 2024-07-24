#include "base.h"

#define SUCCESS "success"
#define ERROR "error"

#define DEBUG 1

extern _FN_RECORD methods[];

_TAG_VALUE* SendShortResponse( int code, char* result, _TAG_VALUE* response )
  {
  char responseBuf[BIGBUF];
  response = NewTagValueInt( "code", code, response, 1 );
  response = NewTagValue( "result", result, response, 1 );
  ListToJSON( response, responseBuf, sizeof(responseBuf)-1 );
  if( printedContentType==0 )
    {
    fputs( "Content-Type: application/json\r\n\r\n", stdout );
    printedContentType = 1;
    }
  fputs( responseBuf, stdout );
  fputs( "\r\n", stdout );
  return response;
  }

int LooksLikeJSON( char* str )
  {
  if( EMPTY( str ) )
    return -1;

  while( isspace(*str) )
    ++str;
  
  if( (*str)=='{' )
    return 0;

  return -2;
  }

int LooksEmpty( char* str )
  {
  if( EMPTY( str ) )
    return 0;

  while( isspace(*str) )
    ++str;
  if( (*str)==0 )
    return 0;

  return -1;
  }

int LooksLikeHTMLForm( char* str )
  { /* COMMAND=Simulate&FILENAME=idan-003-012.txt&DOWNLOAD=myfile.txt */
  if( EMPTY( str ) )
    return -1;

  char* copy = strdup( str );

  char* outsidePtr = NULL;
  for( char* chunk = strtok_r( copy, "&", &outsidePtr );
       chunk!=NULL;
       chunk=strtok_r( NULL, "&", &outsidePtr ) )
    {
    if( strchr( chunk, '=' )==NULL )
      {
      free( copy );
      return -2;
      }
    }

  free( copy );
  return 0;
  }

_TAG_VALUE* ParseHTMLForm( char* str )
  {
  if( EMPTY( str ) )
    return NULL;

  _TAG_VALUE* list = NULL;
  char* copy = strdup( str );

  char* outsidePtr = NULL;
  for( char* chunk = strtok_r( copy, "&", &outsidePtr );
       chunk!=NULL;
       chunk=strtok_r( NULL, "&", &outsidePtr ) )
    {
    char* eq = strchr( chunk, '=' );
    if( eq==NULL )
      {
      free( copy );
      return list;
      }

    *eq = 0;
    list = NewTagValueGuessType( chunk, eq+1, list, 1 );
    }

  free( copy );

  return list;
  }

void CallAPIFunction( _CONFIG* conf, char* userID, char* method, char* testingInput )
  {
  char* ptr = NULL;
  char* topic = strtok_r( method, "&/", &ptr );
  char* action = strtok_r( NULL, "&/", &ptr );

  if( EMPTY( topic ) || EMPTY( action ) )
    APIError( method, -4, "API calls must specify a topic and action " );

  /* is this a known user?  If not, API error */
  if( EMPTY( userID ) )
    APIError( method, -5, "API call without an authenticated user ID" );

  if( conf->users==NULL
      || FindUser( conf->users, userID )==NULL )
    {
    if( NOTEMPTY( topic )
        && strcasecmp( topic, "logout" )==0
        && NOTEMPTY( action )
        && strcasecmp( action, "get-url" )==0 )
      { /* api method to get logout URL does not require authentication or authorization */
      }
    else
      { /* sorry, not permitted! */
      APIError( method, -6, "API call using non-authorized user ID (%s)", NULLPROTECT( userID ) );
      }
    }

  char methodNameBuf[BUFLEN];
  snprintf( methodNameBuf, sizeof(methodNameBuf)-1, "%s/%s", topic, action );

  _TAG_VALUE* args = NULL;

  if( testingInput==NULL )
    {
    char inputBuf[BUFLEN];
    if( fgets( inputBuf, sizeof(inputBuf)-1, stdin ) != inputBuf )
      inputBuf[0] = 0;
    Notice( "Parsing API parameters: [%s]", inputBuf );
    if( LooksEmpty( inputBuf )==0 )
      {}
    else if( LooksLikeJSON( inputBuf )==0 )
      {
      args = ParseJSON( inputBuf );
      }
    else if( LooksLikeHTMLForm( inputBuf )==0 )
      {
      args = ParseHTMLForm( inputBuf );
      }
    else
      APIError( method, -7, "Input parameters look like neither JSON nor an HTML form" );
    }
  else
    args = ParseJSON( testingInput );

  int gotOne = 0;
  for( _FN_RECORD* fn=methods; fn->topic!=NULL; ++fn )
    {
    if( NOTEMPTY( fn->topic )
        && NOTEMPTY( fn->action )
        && fn->function!=NULL
        && strcasecmp( fn->topic, topic )==0
        && strcasecmp( fn->action, action )==0 )
      {
      (fn->function)( conf, userID, methodNameBuf, args );
      gotOne = 1;
      break;
      }
    }

  if( ! gotOne )
    APIError( method, -7, "Invalid function and/or topic. (%s/%s)", topic, action );

  FreeTagValue( args );
  }
