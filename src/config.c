#include "base.h"

char defaultKey[] =
  {
  0xdb, 0xd2, 0x5a, 0x74, 0xc7, 0x44, 0x3b, 0x43,
  0xd2, 0x24, 0xcb, 0xce, 0x63, 0x4a, 0xe5, 0x4d,
  0x5d, 0x64, 0xd6, 0xcc, 0xcb, 0x97, 0x75, 0x2b,
  0x95, 0xa8, 0x52, 0x85, 0xe6, 0xed, 0x32, 0xc1
  };

void UpdateGlobalParsingLocation( _CONFIG* config )
  {
  FreeIfAllocated( &parsingLocation );
  if( config!=NULL
      && config->parserLocation!=NULL
      && NOTEMPTY( config->parserLocation->tag ) )
    {
    char whereAmI[BUFLEN];
    snprintf( whereAmI, sizeof(whereAmI)-1, "%s::%d ",
              config->parserLocation->tag,
              config->parserLocation->iValue );
    parsingLocation = strdup( whereAmI );
    }
  }

void SetDefaults( _CONFIG* config )
  {
  memset( config, 0, sizeof(_CONFIG) );
  memcpy( config->key, defaultKey, AES_KEYLEN );

  config->authServiceUrl = strdup( DEFAULT_AUTH_URL );
  config->myCSS = strdup( DEFAULT_MY_CSS );
  config->remoteAddrEnvVar = strdup( DEFAULT_REMOTE_ADDR );
  config->sessionCookieName = strdup( DEFAULT_ID_OF_AUTH_COOKIE );
  config->urlEnvVar = strdup( DEFAULT_REQUEST_URI_ENV_VAR );
  config->userAgentEnvVar = strdup( DEFAULT_USER_AGENT_VAR );
  config->userEnvVar = strdup( DEFAULT_USER_ENV_VAR );

  config->workDir = strdup( DEFAULT_WORK_DIR );
  config->users = NULL;
  }

void FreeConfig( _CONFIG* config )
  {
  if( config==NULL )
    return;

  FreeIfAllocated( &(config->configFolder) );

  if( config->list )
    FreeTagValue( config->list );

  if( config->includes )
    FreeTagValue( config->includes );

  if( config->parserLocation )
    FreeTagValue( config->parserLocation );

  for( _VARIABLE* v = config->variables; v!=NULL; )
    {
    _VARIABLE* vNext = v->next;
    FreeVariable( v );
    v = vNext;
    }

  FreeIfAllocated( &(config->authServiceUrl) );
  FreeIfAllocated( &(config->myCSS) );

  FreeIfAllocated( &(config->remoteAddrEnvVar) );
  FreeIfAllocated( &(config->sessionCookieName) );
  FreeIfAllocated( &(config->urlEnvVar) );
  FreeIfAllocated( &(config->userAgentEnvVar) );
  FreeIfAllocated( &(config->userEnvVar) );
  FreeIfAllocated( &(config->workDir) );

  FreeIfAllocated( &(config->validateCommand) );
  FreeIfAllocated( &(config->validateOk) );
  FreeIfAllocated( &(config->validateErrorKeyword) );

  FreeUser( config->users ); /* it's recursive */
  config->users = NULL;

  free( config );
  }

int IsValidHelpText( char* str )
  {
  if( EMPTY( str ) )
    return -1;

  if( strchr( str, QUOTE )!=NULL )
    return -2;

  for( char* ptr=str; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( !isprint( c ) )
      return -3;
    }

  return 0;
  }

void ProcessConfigLine( char* ptr, char* equalsChar, _CONFIG* config )
  {
  *equalsChar = 0;

  char* variable = TrimHead( ptr );
  TrimTail( variable );
  char* value = TrimHead( equalsChar+1 );
  TrimTail( value );

  /* indicates that we used strdup() to recompute the value ptr */
  int allocatedValue = 0;

  if( NOTEMPTY( variable ) && NOTEMPTY( value ) )
    {
    char valueBuf[BUFLEN];

    /* expand any macros in the value */
    if( strchr( value, '$' )!=NULL )
      {
      int loopMax = 10;
      while( loopMax>0 )
        {
        int n = ExpandMacros( value, valueBuf, sizeof( valueBuf ), config->list );
        if( n>0 )
          {
          if( allocatedValue )
            FREE( value );
          value = strdup( valueBuf );
          allocatedValue = 1;
          }
        else
          {
          break;
          }
        --loopMax;
        }
      }

    config->list = NewTagValue( variable, value, config->list, 1 );

    if( strcasecmp( variable, "work_dir" )==0 )
      {
      if( DirExists( value )!=0 )
        Error( "Cannot access WORK_DIR [%s]", value );

      FreeIfAllocated( &( config->workDir ) );
      config->workDir = strdup( value );
      }
    else if( strcasecmp( variable, "validate_command" )==0 )
      {
      FreeIfAllocated( &( config->validateCommand ) );
      config->validateCommand = strdup( value );
      }
    else if( strcasecmp( variable, "validate_ok" )==0 )
      {
      FreeIfAllocated( &( config->validateOk ) );
      config->validateOk = strdup( value );
      }
    else if( strcasecmp( variable, "validate_error_keyword" )==0 )
      {
      FreeIfAllocated( &( config->validateErrorKeyword ) );
      config->validateErrorKeyword = strdup( value );
      }
    else if( strcasecmp( variable, "variable" )==0 )
      {
      _VARIABLE* v = AppendVariable( config, value );
      if( v==NULL )
        Error( "Failed to create new variable %s", value );
      }
    else if( strcasecmp( variable, "parent" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      _VARIABLE* parent = FindVariable( config->variables, value );
      if( parent==NULL ) Error( "Cannot find earlier variable %s (parent of %s)", value, config->lastVar->id );
      if( config->lastVar->parent!=NULL ) Error( "%s already has a parent", config->lastVar->id );
      config->lastVar->parent = parent;

      /* append lastvar to parent's children */
      _VARPTR* vptr = (_VARPTR*)SafeCalloc( 1, sizeof( _VARPTR ), "Var ptr" );
      vptr->next = parent->children;
      vptr->var = config->lastVar;
      parent->children = vptr;
      }
    else if( strcasecmp( variable, "type" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      config->lastVar->type = TypeFromString( value );
      if( config->lastVar->type == vt_float )
        {
        config->lastVar->minFloat = 0;
        config->lastVar->maxFloat = 100;
        config->lastVar->step = 0.1;
        }
      else if( config->lastVar->type == vt_int )
        {
        config->lastVar->minInt = 0;
        config->lastVar->maxInt = 100;
        }
      if( config->lastVar->type==vt_invalid ) Error( "Invalid variable type %s", value );
      }
    else if( strcasecmp( variable, "islist" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( strcasecmp( value, "true" ) )
        config->lastVar->listOfValues = 1;
      else
        config->lastVar->listOfValues = 0;
      }
    else if( strcasecmp( variable, "refleft" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type != vt_xref ) Error( "%s must follow type=xref", variable );
      if( config->lastVar->leftType != NULL ) Error( "%s must be unique", variable );
      _VARIABLE* left = FindVariable( config->variables, value );
      if( left==NULL )
        Error( "%s must reference an already defined variable", variable );
      config->lastVar->leftType = left;
      }
    else if( strcasecmp( variable, "refright" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type != vt_xref ) Error( "%s must follow type=xref", variable );
      if( config->lastVar->rightType != NULL ) Error( "%s must be unique", variable );
      _VARIABLE* right = FindVariable( config->variables, value );
      if( right==NULL )
        Error( "%s must reference an already defined variable", variable );
      config->lastVar->rightType = right;
      }
    else if( strcasecmp( variable, "min" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type == vt_int
          || config->lastVar->type == vt_intlist
          || config->lastVar->type == vt_dateval )
        {
        int iVal = atoi( value );
        config->lastVar->minInt = iVal;
        }
      else if( config->lastVar->type == vt_float )
        {
        double dVal = atof( value );
        config->lastVar->minFloat = dVal;
        }
      else
        Error( "Variable %s does not qualify for %s", config->lastVar->id, variable );
      /* handle date min/max? */
      }
    else if( strcasecmp( variable, "legal" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->legalValues ) Error( "%s cannot have multiple lists of legal values", config->lastVar->id );
      if( config->lastVar->type != vt_string ) Error( "%s only applies to variables of type string", variable );
      if( strchr( value, '|' )==NULL ) Error( "%s legal values (%s) must have at least two values, separated by at least one comma.", variable, value );
      config->lastVar->legalValues = strdup( value );
      }
    else if( strcasecmp( variable, "default" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->gotDefault ) Error( "%s cannot have multiple defaults", config->lastVar->id );
      config->lastVar->gotDefault = 1;
      if( config->lastVar->type == vt_int )
        {
        int iVal = atoi( value );
        config->lastVar->defaultInt = iVal;
        }
      else if( config->lastVar->type == vt_float )
        {
        double dVal = atof( value );
        config->lastVar->defaultFloat = dVal;
        }
      else if( config->lastVar->type == vt_string )
        {
        config->lastVar->defaultString = strdup( value );
        }
      else if( config->lastVar->type == vt_bool )
        {
        config->lastVar->defaultBool = (strcasecmp( value, "true" )==0) ? 1 : 0;
        }
      else
        Error( "Don't know how to handle default values for variables of type %s",
               NameOfType( config->lastVar->type ) );
      }
    else if( strcasecmp( variable, "singleton" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( strcasecmp( value, "true" )!=0 ) Error( "%s must be set to 'true' if set at all", variable );
      config->lastVar->singleton = 1;
      }
    else if( strcasecmp( variable, "mandatory" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( strcasecmp( value, "true" )!=0 ) Error( "%s must be set to 'true' if set at all", variable );
      config->lastVar->mandatory = 1;
      }
    else if( strcasecmp( variable, "help" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( IsValidHelpText( value ) != 0 )
        Error( "Help text for %s invalid - cannot contain quotes or control chars", config->lastVar->id );
      if( NOTEMPTY( config->lastVar->helpText ) )
        Error( "More than one help text for %s", config->lastVar->id );
      config->lastVar->helpText = strdup( value );
      }
    else if( strcasecmp( variable, "follows" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->follows!=NULL ) Error( "Cannot set FOLLOWS twice on %s", config->lastVar->id );
      _VARIABLE* f = FindVariable( config->variables, value );
      if( f==NULL ) Error( "%s FOLLOWS=%s - not found", config->lastVar->id, value );
      if( f->nextVar!=NULL ) Error( "Cannot set the same FOLLOWS attribute on two different variables (%s)", value );
      if( f == config->lastVar ) Error( "A variable (%s) cannot follow itself!", value );
      config->lastVar->follows = f;
      config->lastVar->follows->nextVar = config->lastVar;
      }
    else if( strcasecmp( variable, "max" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type == vt_int
          || config->lastVar->type == vt_intlist
          || config->lastVar->type == vt_dateval )
        {
        int iVal = atoi( value );
        config->lastVar->maxInt = iVal;
        }
      else if( config->lastVar->type == vt_float )
        {
        double dVal = atof( value );
        config->lastVar->maxFloat = dVal;
        }
      else
        Error( "Variable %s does not qualify for %s", config->lastVar->id, variable );
      /* handle date min/max? */
      }
    else if( strcasecmp( variable, "step" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type != vt_float ) Error( "%s must follow a variable of type FLOAT", variable );
      double step = atof( value );
      if( step<0.001 || step>1000000 ) Error( "Step of [%s] is too small or too large", value );
      config->lastVar->step = step;
      }
    else if( strcasecmp( variable, "xref" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( config->lastVar->type == vt_invalid ) config->lastVar->type = vt_string; /* implicitly so */
      if( config->lastVar->type != vt_string ) Error( "XREF's must be for variables of type %s", NameOfType( vt_string ) );
      _VARIABLE* xref = FindVariable( config->variables, value );
      if( xref==NULL ) Error( "Variable %s not yet defined", value );
      config->lastVar->xref = xref;
      }
    else if( strcasecmp( variable, "minvalues" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      int n = atoi( value );
      if( n<1 ) Error( "%s must be 1 or more", value );
      config->lastVar->minValues = n;
      }
    else if( strcasecmp( variable, "validate" )==0 )
      {
      if( config->lastVar==NULL ) Error( "%s must follow VARIABLE", variable );
      if( NOTEMPTY( config->lastVar->childrenValidateExpression ) )
        Error( "You can only specify one %s for a variable", variable );
      config->lastVar->childrenValidateExpression = strdup( value );
      }
    else if( strcasecmp( variable, "USER" )==0 )
      {
      if( StringIsAnIdentifier( value )!=0 )
        Error( "User IDs should be formatted as identifiers (not %s)", value );
      config->users = NewUser( value, config->users );
      }
    else if( strcasecmp( variable, "FOLDER" )==0 )
      {
      if( config->users==NULL )
        Error( "%s must follow USER", variable );
      if( DirExists( value )!=0 )
        Error( "Invalid folder" );
      FreeIfAllocated( &( config->users->folder ) );
      config->users->folder = strdup( value );
      }
    else if( strcasecmp( variable, "SESSION_COOKIE_ENCRYPTION_KEY" )==0 )
      {
      uint8_t binaryKey[100];
      memset( binaryKey, 0, sizeof(binaryKey) );
      UnescapeString( value, binaryKey, sizeof(binaryKey) );
      memset( config->key, 0, AES_KEYLEN );
      memcpy( config->key, binaryKey, AES_KEYLEN );
      }
    else if( strcasecmp( variable, "COMMAND" )==0 )
      {
      config->commands = NewRunCommand( value, config->commands );
      }
    else if( strcasecmp( variable, "COMMAND_PATH" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( NOTEMPTY( config->commands->path ) ) Error( "Duplicate %s in command %s", variable, config->commands->label );
      config->commands->path = strdup( value );
      }
    else if( strcasecmp( variable, "FOLDER_ARG" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( NOTEMPTY( config->commands->folderArg ) ) Error( "Duplicate %s in command %s", variable, config->commands->label );
      config->commands->folderArg = strdup( value );
      }
    else if( strcasecmp( variable, "CONFIG_ARG" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( NOTEMPTY( config->commands->configArg ) ) Error( "Duplicate %s in command %s", variable, config->commands->label );
      config->commands->configArg = strdup( value );
      }
    else if( strcasecmp( variable, "COMMAND_SUCCESS" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( NOTEMPTY( config->commands->success ) ) Error( "Duplicate %s in command %s", variable, config->commands->label );
      config->commands->success = strdup( value );
      }
    else if( strcasecmp( variable, "COMMAND_WORKDIR" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( NOTEMPTY( config->commands->workDir ) ) Error( "Duplicate %s in command %s", variable, config->commands->label );
      config->commands->workDir = strdup( value );
      }
    else if( strcasecmp( variable, "COMMAND_OUTPUT" )==0 )
      {
      if( config->commands==NULL ) Error( "%s must follow COMMAND", variable );
      if( strchr( value, ':' )==NULL )
        Error( "%s must be formatted 'label:-arg' (no :)", value );
      char* tmp = strdup( value );
      char* ptr = NULL;
      char* label = strtok_r( tmp, ":", &ptr );
      char* cmdLine = strtok_r( NULL, ":", &ptr );
      if( EMPTY( label ) || EMPTY( cmdLine ) )
        Error( "%s must be formatted 'label:-arg' (at least one is empty)", value );
      config->commands->outputFiles = NewTagValue( label, cmdLine, config->commands->outputFiles, 0 );
      free( tmp );
      }
    else if( strcasecmp( variable, "USER_ENV_VARIABLE" )==0 )
      {
      FreeIfAllocated( &(config->userEnvVar) );
      config->userEnvVar = strdup( value );
      }
    else if( strcasecmp( variable, "SESSION_COOKIE_NAME" )==0 )
      {
      FreeIfAllocated( &(config->sessionCookieName) );
      config->sessionCookieName = strdup( value );
      }
    else if( strcasecmp( variable, "AUTHENTICATION_SERVICE_URL" )==0 )
      {
      FreeIfAllocated( &(config->authServiceUrl) );
      config->authServiceUrl = strdup( value );
      }
    else if( strcasecmp( variable, "URL_ENV_VARIABLE" )==0 )
      {
      FreeIfAllocated( &(config->urlEnvVar) );
      config->urlEnvVar = strdup( value );
      }
    else if( strcasecmp( variable, "REMOTE_ADDR_ENV_VARIABLE" )==0 )
      {
      FreeIfAllocated( &(config->remoteAddrEnvVar ) );
      config->remoteAddrEnvVar = strdup( value );
      }
    else if( strcasecmp( variable, "USER_AGENT_ENV_VARIABLE" )==0 )
      {
      FreeIfAllocated( &(config->userAgentEnvVar ) );
      config->userAgentEnvVar = strdup( value );
      }
    else if( strcasecmp( variable, "SESSION_COOKIE_ENCRYPTION_KEY" )==0 )
      {
      uint8_t binaryKey[100];
      memset( binaryKey, 0, sizeof(binaryKey) );
      UnescapeString( value, binaryKey, sizeof(binaryKey) );
      memset( config->key, 0, AES_KEYLEN );
      memcpy( config->key, binaryKey, AES_KEYLEN );
      }
    else
      {
      /* append this variable to our linked list, for future expansion */
      /* do this always, so not here for just
         invalid commands:
         config->list = NewTagValue( variable, value, config->list, 1 );
      */
      }
    }

  if( allocatedValue )
    FREE( value );
  }

void PrintConfig( FILE* f, _CONFIG* config )
  {
  if( f==NULL )
    {
    Error("Cannot print configuration to NULL file");
    }

  if( NOTEMPTY( config->myCSS )
      && strcmp( config->myCSS, DEFAULT_MY_CSS )!=0 )
    fprintf( f, "MY_CSS=%s\n", config->myCSS );

  if( NOTEMPTY( config->authServiceUrl )
      && strcmp( config->authServiceUrl, DEFAULT_AUTH_URL )!=0 )
    {
    fprintf( f, "AUTHENTICATION_SERVICE_URL=%s\n", config->authServiceUrl );
    }

  if( memcmp( config->key, defaultKey, AES_KEYLEN ) !=0 )
    {
    char key_ascii[100];
    fprintf( f, "SESSION_COOKIE_ENCRYPTION_KEY=%s\n", EscapeString( config->key, AES_KEYLEN, key_ascii, sizeof( key_ascii ) ) );
    }

  if( NOTEMPTY( config->userEnvVar )
      && strcmp( config->userEnvVar, DEFAULT_USER_ENV_VAR )!=0 )
    {
    fprintf( f, "USER_ENV_VARIABLE=%s\n", config->userEnvVar );
    }

  if( NOTEMPTY( config->sessionCookieName )
      && strcmp( config->sessionCookieName, DEFAULT_ID_OF_AUTH_COOKIE )!=0 )
    {
    fprintf( f, "SESSION_COOKIE_NAME=%s\n", config->sessionCookieName );
    }

  if( NOTEMPTY( config->urlEnvVar )
      && strcmp( config->urlEnvVar, DEFAULT_REQUEST_URI_ENV_VAR )!=0 )
    {
    fprintf( f, "URL_ENV_VARIABLE=%s\n", config->urlEnvVar );
    }

  if( NOTEMPTY( config->remoteAddrEnvVar )
      && strcmp( config->remoteAddrEnvVar, DEFAULT_REMOTE_ADDR )!=0 )
    {
    fprintf( f, "REMOTE_ADDR_ENV_VARIABLE=%s\n", config->remoteAddrEnvVar );
    }

  if( NOTEMPTY( config->userAgentEnvVar )
      && strcmp( config->userAgentEnvVar, DEFAULT_USER_AGENT_VAR )!=0 )
    {
    fprintf( f, "USER_AGENT_ENV_VARIABLE=%s\n", config->userAgentEnvVar );
    }

  if( NOTEMPTY( config->workDir )
      && strcmp( config->workDir, DEFAULT_WORK_DIR )!=0 )
    fprintf( f, "WORK_DIR=%s\n", config->workDir );

  if( NOTEMPTY( config->validateCommand ) )
    fprintf( f, "VALIDATE_COMMAND=%s\n", config->validateCommand );
  if( NOTEMPTY( config->validateOk ) )
    fprintf( f, "VALIDATE_OK=%s\n", config->validateOk );
  if( NOTEMPTY( config->validateErrorKeyword ) )
    fprintf( f, "VALIDATE_ERROR_KEYWORD=%s\n", config->validateErrorKeyword );

  for( _RUN_COMMAND* rc = config->commands; rc!=NULL; rc=rc->next )
    PrintRunCommand( f, rc );

  if( memcmp( config->key, defaultKey, AES_KEYLEN ) !=0 )
    {
    char key_ascii[100];
    fprintf( f, "SESSION_COOKIE_ENCRYPTION_KEY=%s\n", EscapeString( config->key, AES_KEYLEN, key_ascii, sizeof( key_ascii ) ) );
    }

  for( _USER* u=config->users; u!=NULL; u=u->next )
    PrintUser( f, u );

  for( _VARIABLE* var = config->variables; var!=NULL; var=var->next )
    PrintVariable( f, var );
  }

void ReadConfig( _CONFIG* config, char* filePath )
  {
  /* Notice( "ReadConfig( %s )", filePath ); */
  if( EMPTY( filePath ) )
    {
    Error( "Cannot read configuration file with empty/NULL name");
    }

  char folder[BUFLEN];
  folder[0] = 0;
  (void)GetFolderFromPath( filePath, folder, sizeof( folder )-1 );

  if( EMPTY( folder ) )
    config->configFolder = NULL;
  else
    config->configFolder = strdup( folder );

  FILE* f = fopen( filePath, "r" );
  if( f==NULL )
    Error( "Failed to open configuration file %s", filePath );

  config->parserLocation = NewTagValue( filePath, "", config->parserLocation, 0 );
  config->parserLocation->iValue = 0;
  UpdateGlobalParsingLocation( config );
  ++ ( config->currentlyParsing );

  char buf[BUFLEN];
  char* endOfBuf = buf + sizeof(buf)-1;
  while( fgets(buf, sizeof(buf)-1, f )==buf )
    {
    ++(config->parserLocation->iValue);
    UpdateGlobalParsingLocation( config );

    char* ptr = TrimHead( buf );
    TrimTail( ptr );

    while( *(ptr + strlen(ptr) - 1)=='\\' )
      {
      char* startingPoint = ptr + strlen(ptr) - 1;
      if( fgets(startingPoint, endOfBuf-startingPoint-1, f )!=startingPoint )
        {
        ++(config->parserLocation->iValue);
        UpdateGlobalParsingLocation( config );
        break;
        }
      ++config->parserLocation->iValue;
      UpdateGlobalParsingLocation( config );
      TrimTail( startingPoint );
      }

    if( *ptr==0 )
      {
      continue;
      }

    if( *ptr=='#' )
      {
      ++ptr;
      if( strncmp( ptr, "include", 7 )==0 )
        { /* #include */
        ptr += 7;
        while( *ptr!=0 && ( *ptr==' ' || *ptr=='\t' ) )
          {
          ++ptr;
          }
        if( *ptr!='"' )
          {
          Error("#include must be followed by a filename in \" marks.");
          }
        ++ptr;
        char* includeFileName = ptr;
        while( *ptr!=0 && *ptr!='"' )
          {
          ++ptr;
          }
        if( *ptr=='"' )
          {
          *ptr = 0;
          }
        else
          {
          Error("#include must be followed by a filename in \" marks.");
          }

        int redundantInclude = 0;
        for( _TAG_VALUE* i=config->includes; i!=NULL; i=i->next )
          {
          if( NOTEMPTY( i->tag ) && strcmp( i->tag, includeFileName )==0 )
            {
            redundantInclude = 1;
            break;
            }
          }

        if( redundantInclude==0 )
          {
          config->includes = NewTagValue( includeFileName, "included", config->includes, 1 );

          if( config->listIncludes )
            {
            if( config->includeCounter )
              {
              fputs( " ", stdout );
              }
            fputs( includeFileName, stdout );
            ++config->includeCounter;
            }

          char* confPath = SanitizeFilename( CONFIGDIR, NULL, includeFileName, 0 );
          if( FileExists( confPath )==0 )
            {
            ReadConfig( config, confPath );
            }
          else
            {
            confPath = SanitizeFilename( folder, NULL, includeFileName, 0 );
            if( FileExists( confPath )==0 )
              {
              ReadConfig( config, confPath );
              }
            else
              {
              Warning( "Cannot open #include \"%s\" -- skipping.",
                       confPath );
              }
            FreeIfAllocated( &confPath );
            }
          FreeIfAllocated( &confPath );
          }
        }
      else if( strncmp( ptr, "print", 5 )==0 )
        { /* #print */
        ptr += 5;
        while( *ptr!=0 && ( *ptr==' ' || *ptr=='\t' ) )
          {
          ++ptr;
          }
        if( *ptr!='"' )
          {
          Error("#include must be followed by a filename in \" marks.");
          }
        ++ptr;
        char* printFileName = ptr;
        while( *ptr!=0 && *ptr!='"' )
          {
          ++ptr;
          }
        if( *ptr=='"' )
          {
          *ptr = 0;
          }
        else
          {
          Error("#print must be followed by a filename in \" marks.");
          }

        FILE* printFile = fopen( printFileName, "w" );
        if( printFile==NULL )
          {
          Error( "Could not open/create %s to print configuration.",
                 printFileName );
          }
        PrintConfig( printFile, config );
        fclose( printFile );
        Notice( "Printed configuration to %s.", printFileName );
        }
      else if( strncmp( ptr, "exit", 4 )==0 )
        { /* #exit */
        ptr += 4;
        ValidateConfig( config );
        Notice( "Exit program due to command in config file." );
        exit(0);
        }

      /* not #include or #include completely read by now */
      continue;
      }

    /* printf("Processing [%s]\n", ptr ); */
    char* equalsChar = NULL;
    for( char* eolc = ptr; *eolc!=0; ++eolc )
      {
      if( equalsChar==NULL && *eolc == '=' )
        {
        equalsChar = eolc;
        }

      if( *eolc == '\r' || *eolc == '\n' )
        {
        *eolc = 0;
        break;
        }
      }

    if( *ptr!=0 && equalsChar!=NULL && equalsChar>ptr )
      {
      ProcessConfigLine( ptr, equalsChar, config );
      }
    }

  /* unroll the stack of config filenames after ReadConfig ended */
  _TAG_VALUE* tmp = config->parserLocation->next;
  if( config->parserLocation->tag!=NULL ) { FREE( config->parserLocation->tag ); }
  if( config->parserLocation->value!=NULL ) { FREE( config->parserLocation->value ); }
  FREE( config->parserLocation );
  config->parserLocation = tmp;
  UpdateGlobalParsingLocation( config );
  -- ( config->currentlyParsing );

  fclose( f );
  }

int VarSequenceCanReach( _VARIABLE* start, _VARIABLE* testFor )
  {
  if( start == testFor )
    return 0;
  if( start->nextVar == testFor )
    return 0;
  if( start->nextVar != NULL )
    return VarSequenceCanReach( start->nextVar, testFor );
  return -1;
  }

void CheckForVariableLoops( _CONFIG* config )
  {
  for( _VARIABLE* v = config->variables; v!=NULL; v=v->next )
    {
    if( v->nextVar==NULL )
      continue;
    if( VarSequenceCanReach( v->nextVar, v )==0 )
      Error( "Variable loop involving %s", NULLPROTECT( v->id ) );
    }
  }

void ValidateConfig( _CONFIG* config )
  {
  if( config==NULL )
    Error( "Cannot validate a NULL configuration" );

  if( EMPTY( config->workDir ) )
    Error( "You must specify WORK_DIR" );

  if( DirExists( config->workDir )!=0 )
    Error( "Cannot access WORK_DIR [%s]", config->workDir );

  if( NOTEMPTY( config->validateCommand ) && EMPTY( config->validateOk ) )
    Error( "If you specify VALIDATE_COMMAND, you must also specify VALIDATE_OK" );
  if( EMPTY( config->validateCommand ) && NOTEMPTY( config->validateOk ) )
    Error( "If you specify VALIDATE_OK, you must also specify VALIDATE_COMMAND" );
  if( EMPTY( config->validateCommand ) && NOTEMPTY( config->validateErrorKeyword ) )
    Error( "If you specify VALIDATE_ERROR_KEYWORD, you must also specify VALIDATE_COMMAND" );

  for( _RUN_COMMAND* rc = config->commands; rc!=NULL; rc=rc->next )
    ValidateRunCommand( rc );

  if( config->variables==NULL )
    Error( "If you specify at least one variable" );

  if( config->users==NULL )
    Error( "You must specify at least one USER" );
  for( _USER* u=config->users; u!=NULL; u=u->next )
    ValidateUser( u );

  CheckForVariableLoops( config );
  }

