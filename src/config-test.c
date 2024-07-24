#include "base.h"


int main( int argc, char** argv )
  {
  char* confDir = ".";
  char* confFile = "config.ini";
  char* workDir = NULL;
  char* workFile = NULL;
  char* outFile = NULL;
  int generateDoc = 0;

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-c" )==0 && i+1<argc )
      {
      confFile = argv[++i];
      }
    else if( strcmp( argv[i], "-d" )==0 && i+1<argc )
      {
      confDir = argv[++i];
      }
    else if( strcmp( argv[i], "-w" )==0 && i+1<argc )
      {
      workFile = argv[++i];
      }
    else if( strcmp( argv[i], "-D" )==0 && i+1<argc )
      {
      workDir = argv[++i];
      }
    else if( strcmp( argv[i], "-o" )==0 && i+1<argc )
      {
      outFile = argv[++i];
      }
    else if( strcmp( argv[i], "-doc" )==0 )
      {
      generateDoc = 1;
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      printf("USAGE: %s [-c configFile] [-d configDir] [-o outFile] [-D workDir] [-w workFile] [-doc]\n", argv[0] );
      exit(0);
      }
    else
      {
      printf("ERROR: unknown argument [%s] (%d/%d)\n", argv[i], i, argc );
      exit(1);
      }
    }

  char* confPath = MakeFullPath( confDir, confFile );
  _CONFIG* conf = (_CONFIG*)SafeCalloc( 1, sizeof( _CONFIG ), "CONFIG" );
  if( conf==NULL ) Error( "Cannot allocate CONFIG object" );

  SetDefaults( conf );
  ReadConfig( conf, confPath );
  ValidateConfig( conf );

  free( confPath );
  confPath = NULL;

  if( NOTEMPTY( workFile ) )
    {
    char* workPath = MakeFullPath( workDir, workFile );
    _VALUE* valueList = NULL;
    _CONFIG* workingConf = (_CONFIG*)SafeCalloc( 1, sizeof( _CONFIG ), "Working conf" );

    ReadWorkingConfig( conf, workingConf, valueList, workPath );
    free( workPath );
    workPath = NULL;
    FreeConfig( workingConf );
    FreeValueList( valueList );
    }

  if( NOTEMPTY( outFile ) )
    {
    FILE* out = stdout;
    if( NOTEMPTY( outFile ) && strcmp( outFile, "-" )!=0 )
      {
      out = fopen( outFile, "w" );
      if( out==NULL )
        Error( "Cannot open %s", outFile );
      }

    if( generateDoc )
      DocumentSchema( out, conf );
    else
      PrintConfig( out, conf );

    if( out != stdout )
      fclose( out );
    }

  FreeConfig( conf );

  return 0;
  }
