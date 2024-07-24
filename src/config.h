#ifndef _INCLUDE_CONFIG
#define _INCLUDE_CONFIG

#define CONFIGDIR "/usr/local/etc/"
#define CONFIGFILE "confedit.ini"

typedef struct _config
  {
  char* configFolder;

  int currentlyParsing;
  _TAG_VALUE* parserLocation;

  /* to avoid duplicate includes */
  _TAG_VALUE *includes;

  /* macro expansion */
  _TAG_VALUE *list;

  /* used in installers and diagnostics */
  int listIncludes;
  int includeCounter;

  /* local content*/
  _VARIABLE* variables;
  _VARIABLE* lastVar;

  _RUN_COMMAND* commands;

  char* validateCommand;
  char* validateOk;
  char* validateErrorKeyword;

  char* userEnvVar;
  uint8_t key[AES_KEYLEN];
  char* sessionCookieName;
  char* authServiceUrl;
  char* urlEnvVar;
  char* remoteAddrEnvVar;
  char* userAgentEnvVar;

  char* workDir;
  char* myCSS;
  _USER* users;
  } _CONFIG;

void SetDefaults( _CONFIG* config );
void ReadConfig( _CONFIG* config, char* filePath );
void PrintConfig( FILE* f, _CONFIG* config );
void FreeConfig( _CONFIG* config );
void ValidateConfig( _CONFIG* config );

void UpdateGlobalParsingLocation( _CONFIG* config );

#endif
