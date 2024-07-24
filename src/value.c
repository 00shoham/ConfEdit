#include "base.h"

_VALUE* AppendValueToList( _CONFIG* conf, _VALUE** listPtr, char* id )
  {
  if( listPtr==NULL )
    {
    Warning( "AppendValue to NULL list" );
    return NULL;
    }

  if( EMPTY( id ) )
    {
    Warning( "AppendValue with no ID" );
    return NULL;
    }

  _VARIABLE* var = FindVariable( conf->variables, id );
  if( var==NULL )
    {
    Warning( "Illegal variable name %s", id );
    return NULL;
    }

  _VALUE* newV = (_VALUE*)SafeCalloc( 1, sizeof( _VALUE ), "New value" );
  newV->var = var;

  for( ; listPtr!=NULL && (*listPtr)!=NULL; )
    {
    _VALUE* v = *listPtr;
    listPtr = &(v->next);
    }
  if( listPtr==NULL )
    Error( "Failed to append to value list" );

  *listPtr = newV;

  return newV;
  }

void FreeValue( _VALUE* v )
  {
  if( NOTEMPTY( v->strVal ) )
    {
    free( v->strVal );
    v->strVal = NULL;
    }
  free( v );
  }

void FreeValueList( _VALUE* valueList )
  {
  if( valueList==NULL )
    return;
  
  FreeValueList( valueList->next );
  FreeValue( valueList );
  }

void ProcessWorkingConfigLine( _CONFIG* config, _CONFIG* workingConf, _VALUE* valuesList, char* ptr, char* equalsChar )
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
        int n = ExpandMacros( value, valueBuf, sizeof( valueBuf ), workingConf->list );
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

    workingConf->list = NewTagValue( variable, value, workingConf->list, 1 );

    /* QQQ parse the line */
    /* printf( "Adding [%s] = [%s]\n", variable, value ); */
    }
  }

void ReadWorkingConfig( _CONFIG* config, _CONFIG* workingConf,
                        _VALUE* valuesList, char* filePath )
  {
  if( EMPTY( filePath ) )
    Error( "Cannot read NULL configuration file" );

  char folder[BUFLEN];
  folder[0] = 0;
  (void)GetFolderFromPath( filePath, folder, sizeof( folder )-1 );

  if( EMPTY( folder ) )
    workingConf->configFolder = NULL;
  else
    workingConf->configFolder = strdup( folder );

  FILE* f = fopen( filePath, "r" );
  if( f==NULL )
    {
    Error( "Failed to open working configuration file %s", filePath );
    }

  workingConf->parserLocation = NewTagValue( filePath, "", workingConf->parserLocation, 0 );
  workingConf->parserLocation->iValue = 0;
  UpdateGlobalParsingLocation( workingConf );
  ++ ( workingConf->currentlyParsing );

  char buf[BUFLEN];
  char* endOfBuf = buf + sizeof(buf)-1;
  while( fgets(buf, sizeof(buf)-1, f )==buf )
    {
    ++(workingConf->parserLocation->iValue);
    UpdateGlobalParsingLocation( workingConf );

    char* ptr = TrimHead( buf );
    TrimTail( ptr );

    while( *(ptr + strlen(ptr) - 1)=='\\' )
      {
      char* startingPoint = ptr + strlen(ptr) - 1;
      if( fgets(startingPoint, endOfBuf-startingPoint-1, f )!=startingPoint )
        {
        ++(workingConf->parserLocation->iValue);
        UpdateGlobalParsingLocation( workingConf );
        break;
        }
      ++workingConf->parserLocation->iValue;
      UpdateGlobalParsingLocation( workingConf );
      TrimTail( startingPoint );
      }

    int c = *ptr;
    if( c=='#' )
      {
      /* printf( "# directive - %s\n", ptr ); */
      ++ptr;
      if( strncmp( ptr, "include", 7 )==0 )
        { /* #include */
        ptr += 7;

        /* printf( "#include %s\n", ptr ); */

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
        for( _TAG_VALUE* i=workingConf->includes; i!=NULL; i=i->next )
          {
          if( NOTEMPTY( i->tag ) && strcmp( i->tag, includeFileName )==0 )
            {
            redundantInclude = 1;
            break;
            }
          }

        if( redundantInclude==0 )
          {
          workingConf->includes = NewTagValue( includeFileName, "included", workingConf->includes, 1 );

          if( workingConf->listIncludes )
            {
            if( workingConf->includeCounter )
              {
              fputs( " ", stdout );
              }
            fputs( includeFileName, stdout );
            ++workingConf->includeCounter;
            }

          char* confPath = SanitizeFilename( WORKINGDIR, NULL, includeFileName, 0 );
          if( FileExists( confPath )==0 )
            {
            ReadWorkingConfig( config, workingConf, valuesList, confPath );
            }
          else
            {
            confPath = SanitizeFilename( folder, NULL, includeFileName, 0 );
            if( FileExists( confPath )==0 )
              {
              ReadWorkingConfig( config, workingConf, valuesList, confPath );
              }
            else
              {
              Warning( "Cannot open #include \"%s\" -- skipping (B).",
                       confPath );
              }
            FreeIfAllocated( &confPath );
            }
          FreeIfAllocated( &confPath );
          }
        }
      else if( strncmp( ptr, "print", 5 )==0 )
        { /* #print - no need for this in working config */
        }
      else if( strncmp( ptr, "exit", 4 )==0 )
        { /* #exit - no need for this in working config */
        }

      /* not a known # directive */
      continue;
      }

    /* remove comments, working backwards from EOL */
    for( char* endP = buf + strlen(buf)-1; endP>=buf; --endP )
      {
      if( *endP=='#'
          && ( endP==buf || *(endP-1)!='\\' ) )
        *endP = 0;
      }
    ptr = buf;
    TrimTail( ptr );

    if( *ptr==0 )
      {
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
      ProcessWorkingConfigLine( config, workingConf, valuesList, ptr, equalsChar );
      }
    }

  /* unroll the stack of config filenames after ReadConfig ended */
  _TAG_VALUE* tmp = workingConf->parserLocation->next;
  if( workingConf->parserLocation->tag!=NULL ) { FREE( workingConf->parserLocation->tag ); }
  if( workingConf->parserLocation->value!=NULL ) { FREE( workingConf->parserLocation->value ); }
  FREE( workingConf->parserLocation );
  workingConf->parserLocation = tmp;
  UpdateGlobalParsingLocation( workingConf );
  -- ( workingConf->currentlyParsing );

  fclose( f );
  }
