#include "base.h"

void ProcessPage()
  {
  printf("Content-Type: text/html\r\n\r\n");
  printedContentType = 1;

  printf("<html><body><p>Please use the API - CGI style operation is not supported.</p></body></html>\n");
  }

int main( int argc, char** argv )
  {
  logFileHandle = fopen( "/data/confedit/confedit-api.log", "a" );

  char* confPath = MakeFullPath( CONFIGDIR, CONFIGFILE );
  _CONFIG* conf = (_CONFIG*)calloc( 1, sizeof( _CONFIG ) );
  if( conf==NULL ) Error( "Cannot allocate CONFIG object" );

  SetDefaults( conf );
  ReadConfig( conf, confPath );
  ValidateConfig( conf );

  char* q = getenv( "QUERY_STRING" );

  if( NOTEMPTY( q ) )
    { /* URL received - CGI action */
    inCGI = 2;

    char* whoAmI = ExtractUserIDOrDieEx( cm_api,
                                         conf->userEnvVar,
                                         conf->remoteAddrEnvVar,
                                         conf->userAgentEnvVar,
                                         conf->sessionCookieName,
                                         conf->urlEnvVar,
                                         conf->authServiceUrl,
                                         conf->key,
                                         conf->myCSS );
#if 0
    char* whoAmI = "idan";
#endif

    CallAPIFunction( conf, whoAmI, q, NULL );
    FreeConfig( conf );
    exit(0);
    }

  if( argc==1 || (argc>1 && argv[1][0]!='-') ) /* q!=NULL && *q!=0 */
    { /* CGI */
    ProcessPage();
    }
  else
    Error( "This is intended to be run as a CGI..." );

  return 0;
  }
