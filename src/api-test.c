#include "base.h"

int main( int argc, char** argv )
  {
  char* confDir = ".";
  char* confFile = "config.ini";
  int printConf = 0;
  char* apiTopic = NULL;
  char* apiAction = NULL;
  char* tag = NULL;
  char* value = NULL;
  char* apiUser = NULL;

  _TAG_VALUE* args = NULL;

  logFileHandle = fopen( "api-test.log", "w" );

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-conf" )==0 && i+1<argc )
      {
      confFile = argv[++i];
      }
    else if( strcmp( argv[i], "-dir" )==0 && i+1<argc )
      {
      confDir = argv[++i];
      }
    else if( strcmp( argv[i], "-printconf" )==0 )
      {
      printConf = 1;
      }
    else if( strcmp( argv[i], "-apitest" )==0 && i+2<argc )
      {
      apiTopic = argv[++i];
      apiAction = argv[++i];
      }
    else if( strcmp( argv[i], "-arg" )==0 && i+2<argc )
      {
      tag = argv[++i];
      value = argv[++i];
      args = NewTagValueGuessType( tag, value, args, 0 );
      }
    else if( strcmp( argv[i], "-array" )==0 && i+3<argc && argv[i+2][0]==OPENSQ )
      {
      char* arrayName = argv[++i];
      printf( "Array name %s\n", arrayName );

      while( argv[i][0]!=OPENSQ && i<argc )
        ++i;

      _TAG_VALUE* array = NULL;
      ++i;
      while( argv[i][0]!=CLOSESQ && i<argc )
        {
        _TAG_VALUE* newItem = NewTagValue( NULL, argv[i], NULL, 0 );
        array = AppendTagValue( array, newItem );
        ++i;
        }

      args = NewTagValueList( arrayName, array, args, 0 );
      }
    else if( strcmp( argv[i], "-user" )==0 && i+1<argc )
      {
      apiUser = argv[++i];
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      printf("USAGE: %s [-conf configFile] [-dir configDir] ...\n", argv[0] );
      printf("       ... [-printconf]\n" );
      printf("       ... [-apitest TOPIC ACTION [-user UU] [-arg TAG VALUE] [-arg TAG VALUE] ...]\n" );
      exit(0);
      }
    else
      {
      printf("ERROR: unknown argument [%s]\n", argv[i] );
      exit(1);
      }
    }

  char* confPath = MakeFullPath( confDir, confFile );
  _CONFIG* conf = (_CONFIG*)calloc( 1, sizeof( _CONFIG ) );
  if( conf==NULL ) Error( "Cannot allocate CONFIG object" );

  SetDefaults( conf );
  ReadConfig( conf, confPath );
  ValidateConfig( conf );
  free( confPath );
  confPath = NULL;

  if( printConf )
    PrintConfig( stdout, conf );

  if( NOTEMPTY( apiTopic ) && NOTEMPTY( apiAction ) )
    {
    inCGI = 1;
    if( EMPTY( apiUser ) )
      apiUser = getenv( "LOGNAME" );
    if( EMPTY( apiUser ) )
      Error( "Cannot call API function without user name" );

    char method[BUFLEN];
    snprintf( method, sizeof(method)-1, "%s/%s", apiTopic, apiAction );

    char json[BUFLEN];
    if( ListToJSON( args, json, sizeof(json)-1 )!=0 )
      Error( "Failed to convert args to JSON" );
    else
      Notice( "JSON args: %s\n", json );
    CallAPIFunction( conf, apiUser, method, json );
    }

  FreeConfig( conf );

  if( args!=NULL )
    FreeTagValue( args );

  fclose( logFileHandle );

  return 0;
  }
