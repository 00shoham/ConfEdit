#include "base.h"

extern _FN_RECORD methods[];

#define N_CMD_RESULTS 100
#define CMD_RESULT_LINE_LEN 500

/***************************************************************************
 * Utility and access control related functions
 ***************************************************************************/
char* GetWorkingDir( _CONFIG* conf, char* userID )
  {
  if( conf==NULL || EMPTY( userID ) )
    Error( "GetWorkingDir() - no config or no user specified" );

  _USER* u = FindUser( conf->users, userID );
  if( u==NULL )
    Error( "GetWorkingDir() - User %s is not known", userID );

  if( NOTEMPTY( u->folder ) )
    return u->folder;

  return conf->workDir;
  }

char* MakeTemporaryFileName( char* userId, time_t tNow, int n, char* suffix )
  {
  char buf[BUFLEN];
  char* ptr = buf;
  char* end = buf + sizeof(buf) - 1;
  strncpy( ptr, userId, end-ptr-1 );
  ptr += strlen( ptr );
  strncpy( ptr, "-", end-ptr-1 );
  ptr += strlen( ptr );
  snprintf( ptr, end-ptr-1, "%03d", n );
  ptr += strlen( ptr );
  strncpy( ptr, "-", end-ptr-1 );
  ptr += strlen( ptr );
  int x = (int)((long)tNow % 1000);
  snprintf( ptr, end-ptr-1, "%03d", x );
  ptr += strlen( ptr );
  strncpy( ptr, suffix, end-ptr-1 );
  ptr += strlen( ptr );
  *ptr = 0;

  return strdup( buf );
  }

/***************************************************************************
 * API introspection
 ***************************************************************************/

void ApiList( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* api = NULL;
  for( _FN_RECORD* fn=methods; fn->function!=NULL; ++fn )
    {
    if( NOTEMPTY( fn->topic ) )
      {
      if( FindTagValueNoCase( api, fn->topic )==NULL )
        {
        _TAG_VALUE* actions = NULL;
        for( _FN_RECORD* action=fn; action->function!=NULL; ++action )
          {
          if( NOTEMPTY( action->topic )
              && strcasecmp( action->topic, fn->topic )==0
              && NOTEMPTY( action->action ) )
            {
            actions = NewTagValue( NULL, action->action, actions, 1 );
            }
          }
        api = NewTagValueList( fn->topic, actions, api, 0 );
        }
      }
    }
  _TAG_VALUE* response = NewTagValueList( "methods", api, NULL, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

void ApiFunctionDescribe( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  char* desc = NULL;
  _TAG_VALUE* topic = FindTagValueNoCase( args, "topic" );
  if( topic==NULL || EMPTY( topic->value ) )
    APIError( methodName, -1, "No topic specified" );
  _TAG_VALUE* action = FindTagValueNoCase( args, "action" );
  if( action==NULL || EMPTY( action->value ) )
    APIError( methodName, -2, "No action specified" );
  for( _FN_RECORD* fn=methods; fn->topic!=NULL; ++fn )
    {
    if( NOTEMPTY( fn->topic ) && NOTEMPTY( fn->action ) && fn->function!=NULL
        && strcasecmp( fn->topic, topic->value )==0
        && strcasecmp( fn->action, action->value )==0 )
      {
      desc = fn->description;
      }
    }
  if( EMPTY( desc ) )
    APIError( methodName, -3, "No description specified for this topic/action" );

  _TAG_VALUE* response = NewTagValue( "description", desc, NULL, 0 );

  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

/***************************************************************************
 * Variable types
 ***************************************************************************/

void VarTypeList( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* parent = FindTagValueNoCase( args, "parenttype" );
  char* parentName = NULL;
  if( parent!=NULL && NOTEMPTY( parent->value ) )
    parentName = parent->value;

  _TAG_VALUE* list = NULL;

  for( _VARIABLE* v=conf->variables; v!=NULL; v=v->next )
    {
    if( EMPTY( v->id ) )
      { /* no id - weird */
      Warning( "Variable with no ID" );
      continue;
      }

    if( EMPTY( parentName ) && v->parent!=NULL )
      { /* this vartype has a parent, but we didn't specify one in args */
      continue;
      }

    if( NOTEMPTY( parentName ) )
      { /* caller specified a parent.  does thie variable have that parent? */
      if( v->parent==NULL )
        continue;
      if( EMPTY( v->parent->id ) )
        continue;
      if( strcasecmp( v->parent->id, parentName )!=0 )
        continue;
      }
    Notice( "Adding variable [%s] to list", v->id );

    list = NewTagValue( NULL, v->id, list, 1 );
    }

  _TAG_VALUE* response = NewTagValueList( "vartypes", list, NULL, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

void VarTypeDescribe( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* type = FindTagValueNoCase( args, "type" );
  if( type==NULL )
    APIError( methodName, -1, "No 'type' specified for the variable to describe" );
  _VARIABLE* var = FindVariable( conf->variables, type->value );

  if( var==NULL )
    APIError( methodName, -2, "Variable not found" );

  _TAG_VALUE* definition = NULL;
  definition = NewTagValue( "ID", var->id, definition, 0 );
  definition = NewTagValue( "TYPE", NameOfType( var->type ), definition, 0 );
  if( var->listOfValues )
    definition = NewTagValue( "IS-LIST", "true", definition, 0 );
  if( var->parent != NULL )
    definition = NewTagValue( "PARENT", var->parent->id, definition, 0 );
  definition = NewTagValue( "HAS-CHILDREN", var->children==NULL ? "false" : "true", definition, 0 );
  if( ( var->type==vt_int || var->type==vt_intlist ) && var->minInt != INT_MIN )
    definition = NewTagValueInt( "MINIMUM", var->minInt, definition, 0 );
  if( ( var->type==vt_int || var->type==vt_intlist ) && var->maxInt != INT_MAX )
    definition = NewTagValueInt( "MAXIMUM", var->maxInt, definition, 0 );
  if( var->type==vt_float && var->minFloat != DBL_MIN )
    definition = NewTagValueDouble( "MINIMUM", var->minFloat, definition, 0 );
  if( var->type==vt_float && var->maxFloat != DBL_MAX )
    definition = NewTagValueDouble( "MAXIMUM", var->maxFloat, definition, 0 );
  if( var->type==vt_float && var->step > 0 )
    definition = NewTagValueDouble( "STEP", var->step, definition, 0 );
  if( var->type==vt_xref && var->leftType != NULL )
    definition = NewTagValue( "LEFT", var->leftType->id, definition, 0 );
  if( var->type==vt_xref && var->rightType != NULL )
    definition = NewTagValue( "RIGHT", var->rightType->id, definition, 0 );
  if( var->minValues )
    definition = NewTagValueInt( "MIN-VALUES", var->minValues, definition, 0 );
  if( var->xref!=NULL )
    definition = NewTagValue( "XREF", var->xref->id, definition, 0 );
  if( var->singleton )
    definition = NewTagValue( "SINGLETON", "true", definition, 0 );
  if( NOTEMPTY( var->legalValues ) )
    definition = NewTagValue( "LEGAL-VALUES", var->legalValues, definition, 0 );
  if( var->gotDefault )
    {
    switch( var->type )
      {
      case vt_int:
        definition = NewTagValueInt( "DEFAULT", var->defaultInt, definition, 0 );
        break;
      case vt_string:
        definition = NewTagValue( "DEFAULT", var->defaultString, definition, 0 );
        break;
      case vt_bool:
        definition = NewTagValue( "DEFAULT", var->defaultBool ? "true" : "false", definition, 0 );
        break;
      case vt_float:
        definition = NewTagValueDouble( "DEFAULT", var->defaultFloat, definition, 0 );
      default:
        APIError( methodName, -3, "Default set for unsupported type (var %s)", var->id );
      }
    }

  if( NOTEMPTY( var->helpText ) )
    definition = NewTagValue( "HELP", var->helpText, definition, 0 );

  if( var->follows!=NULL && NOTEMPTY( var->follows->id ) )
    definition = NewTagValue( "FOLLOWS", var->follows->id, definition, 0 );

  if( var->nextVar!=NULL && NOTEMPTY( var->nextVar->id ) )
    definition = NewTagValue( "NEXT", var->nextVar->id, definition, 0 );

  if( var->parent!=NULL && NOTEMPTY( var->parent->id ) )
    definition = NewTagValue( "PARENT", var->parent->id, definition, 0 );
  else
    Warning( "Variable %s has no parent", var->id );

  _TAG_VALUE* response = NewTagValueList( "DEFINITION", definition, NULL, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

/***************************************************************************
 * Commands that we can run
 ***************************************************************************/

void CommandList( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* list = NULL;

  for( _RUN_COMMAND* rc=conf->commands; rc!=NULL; rc=rc->next )
    {
    if( EMPTY( rc->label ) )
      { /* no id - weird */
      Warning( "Command with no label" );
      continue;
      }

    list = NewTagValue( NULL, rc->label, list, 1 );
    }

  if( list==NULL )
    {
    char errmsg[BUFLEN];
    snprintf( errmsg, sizeof(errmsg)-1, "User [%s] has no authorized commands.", userID );
    _TAG_VALUE* response = SendShortResponse( -100, errmsg, NULL );
    FreeTagValue( response );
    }
  else
    {
    _TAG_VALUE* response = NewTagValueList( "commands", list, NULL, 0 );
    response = SendShortResponse( 0, SUCCESS, response );
    FreeTagValue( response );
    }
  }

void CommandRun( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* cmd = FindTagValueNoCase( args, "command" );
  if( cmd==NULL )
    APIError( methodName, -1, "No 'command' specified for what to run" );
  if( EMPTY( cmd->value ) )
    APIError( methodName, -1, "Blank 'command' specified for what to run" );
  _RUN_COMMAND* rc = FindRunCommand( conf->commands, cmd->value );
  if( rc==NULL )
    APIError( methodName, -2, "No such command" );

  _TAG_VALUE* ini = FindTagValueNoCase( args, "inifile" );
  if( ini==NULL || EMPTY( ini->value ) )
    APIError( methodName, -3, "No (or blank) 'inifile' specified for what to run" );
  if( StringIsAnIdentifier( ini->value )!=0 )
    APIError( methodName, -4, "Configuration filenames must be simple (no spaces, etc.)." );
  char* fullPath = MakeFullPath( GetWorkingDir( conf, userID ), ini->value );
  if( EMPTY( fullPath ) )
    APIError( methodName, -5, "Cannot locate file path for configuration to run" );
  int appendIni = 0;
  if( FileExists( fullPath )!=0 )
    {
    appendIni = 1;
    char* buf = (char*)SafeCalloc( strlen(fullPath)+10, sizeof(char), "new file path for conf" );
    strcpy( buf, fullPath );
    strcat( buf, ".ini" );
    free( fullPath );
    fullPath = buf;
    if( FileExists( fullPath )!=0 )
      APIError( methodName, -6, "Configuration file [%s] not found", fullPath );
    }
  free( fullPath );
  fullPath = NULL;

  char cli[BIGBUF];
  char* ptr = cli;
  char* end = cli + sizeof(cli) - 1;

  strncpy( ptr, rc->path, end-ptr-1 );
  ptr += strlen( ptr );

  if( NOTEMPTY( rc->folderArg ) )
    {
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, rc->folderArg, end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, GetWorkingDir( conf, userID ), end-ptr-1 );
    ptr += strlen( ptr );
    }

  if( NOTEMPTY( rc->configArg ) )
    {
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, rc->configArg, end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, ini->value, end-ptr-1 );
    ptr += strlen( ptr );
    if( appendIni )
      {
      strncpy( ptr, ".ini", end-ptr-1 );
      ptr += strlen( ptr );
      }
    }

  if( rc->outputFiles!=NULL )
    {
    if( EMPTY( rc->workDir ) )
      APIError( methodName, -7, "You cannot have output files with no workdir" );
    if( DirExists( rc->workDir )!=0 )
      APIError( methodName, -8, "Work directory [%s] does not exist", rc->workDir );
    }

  _TAG_VALUE* files = NULL;
  int n = 0;
  for( _TAG_VALUE* of=rc->outputFiles; of!=NULL; of=of->next )
    {
    ++n;
    if( EMPTY( of->tag ) || EMPTY( of->value ) )
      continue;
    char* fileName = MakeTemporaryFileName( userID, time(NULL), n, ".txt" );
    files = NewTagValue( of->tag, fileName, files, 0 );
    char* argPath = MakeFullPath( rc->workDir, fileName );
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, of->value, end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, " ", end-ptr-1 );
    ptr += strlen( ptr );
    strncpy( ptr, argPath, end-ptr-1 );
    ptr += strlen( ptr );
    free( argPath );
    free( fileName );
    }
  *ptr = 0;

  /* for debug purposes only...
  _TAG_VALUE* response = NewTagValue( "command", cli, NULL, 0 );
  */
  _TAG_VALUE* response = NULL;

  char* result = NULL;
  int err = POpenAndSearchRegEx( cli, rc->success, &result );
  if( err )
    APIError( methodName, -9, "Failed to run command - error %d", err );

  if( EMPTY( result ) )
    APIError( methodName, -10, "Command ran but output did not include success code" );

  response = NewTagValueList( "files", files, response, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  }

void CommandFetchOutput( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  { /* QQQ handle DOWNLOADNAME later */
  _TAG_VALUE* cmd = FindTagValueNoCase( args, "command" );
  if( cmd==NULL )
    APIError( methodName, -1, "No 'command' specified for what to run" );
  if( EMPTY( cmd->value ) )
    APIError( methodName, -1, "Blank 'command' specified for what to run" );
  _RUN_COMMAND* rc = FindRunCommand( conf->commands, cmd->value );
  if( rc==NULL )
    APIError( methodName, -2, "No such command" );

  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );
  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -3, "No (or empty) FILENAME specified." );

  _TAG_VALUE* download = FindTagValueNoCase( args, "DOWNLOAD" );
  if( download==NULL || EMPTY( download->value ) )
    APIError( methodName, -4, "No (or empty) DOWNLOAD specified." );

  if( EMPTY( rc->workDir ) )
    APIError( methodName, -5, "This command does not specify a workdir" );
  if( StringIsAnIdentifier( fileName->value ) != 0 )
    APIError( methodName, -6, "Invalid filename - %s", fileName->value );

  char* path = MakeFullPath( rc->workDir, fileName->value );
  if( FileExists( path ) !=0 )
    APIError( methodName, -7, "File [%s] not found.", fileName->value );

  long fileLen = FileSize( path );
  if( fileLen <= 0 )
    APIError( methodName, -8, "Empty file" );

  DownloadFile( fileLen, path, download->value );
  }

/***************************************************************************
 * Operate on configuration files
 ***************************************************************************/

enum lineType { lt_invalid, lt_blank, lt_include, lt_var, lt_comment };

enum lineType StringToLineType( char* str )
  {
  if( EMPTY( str ) )
    return lt_invalid;
  if( strcasecmp( str, "BLANK" )==0 )
    return lt_blank;
  if( strcasecmp( str, "INCLUDE" )==0 )
    return lt_include;
  if( strcasecmp( str, "VAR" )==0 )
    return lt_var;
  if( strcasecmp( str, "COMMENT" )==0 )
    return lt_comment;
  return lt_invalid;
  }

char* LineTypeToString( enum lineType lt )
  {
  switch( lt )
    {
    case lt_invalid:
      return "INVALID";
    case lt_blank:
      return "BLANK";
    case lt_include:
      return "INCLUDE";
    case lt_var:
      return "VAR";
    case lt_comment:
      return "COMMENT";
    default:
      return "INVALID";
    }
  }

enum lineType ClassifyLineType( char* line )
  {
  if( EMPTY( line ) )
    return lt_blank;

  while( isspace(*line) )
    ++line;

  if( EMPTY( line ) )
    return lt_blank;

  if( strncasecmp( line, "#include", 8 )==0 )
    {
    char* ptr = line+8;
    while( *ptr!=0 && *ptr!=QUOTE )
      ++ptr;
    if( *ptr!=QUOTE )
      return lt_invalid;
    ++ptr;
    while( *ptr!=0 && *ptr!=QUOTE )
      ++ptr;
    if( *ptr!=QUOTE )
      return lt_invalid;
    return lt_include;
    }

  if( *line=='#' )
    return lt_comment;

  char* ptr = strchr( line, '=' );
  if( ptr==NULL )
    return lt_invalid;

  /* left of equals sign must be identifier chars */
  for( char* p=line; p<ptr; ++p )
    if( strchr( validIdentifierChars, (int)(*p) )==NULL )
      return lt_invalid;

  return lt_var;
  }

FILE* OpenConfigFileForReading( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args, char** pathPtr )
  {
  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );

  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -1, "No FILENAME specified" );

  if( StringIsAnIdentifier( fileName->value )!=0 )
    APIError( methodName, -2, "FILENAME must be an identifier" );

  int l = strlen( fileName->value );

  char* iniName = (char*)SafeCalloc( l+10, sizeof(char), "ini filename" );
  strncpy( iniName, fileName->value, l );
  strncat( iniName, ".ini", l+5 );

  char* path = MakeFullPath( GetWorkingDir( conf, userID ), iniName );

  if( EMPTY( path ) )
    APIError( methodName, -3, "Failed to create file path" );

  if( FileExists( path )!=0 )
    { /* but can we create it? */
    if( Touch( path )!=0 )
      {
      APIError( methodName, -4, "File [%s] does not exist and could not be created.", iniName );
      }
    }

  free( iniName );

  FILE* f = fopen( path, "r" );
  if( f==NULL )
    APIError( methodName, -5, "Failed to open file for reading" );

  *pathPtr = path;

  return f;
  }

FILE* OpenTemporaryConfigFileForWriting( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args, char** pathPtr )
  {
  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );

  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -1, "No FILENAME specified" );

  if( StringIsAnIdentifier( fileName->value )!=0 )
    APIError( methodName, -2, "FILENAME must be an identifier" );

  int l = strlen( fileName->value );

  char* iniName = (char*)SafeCalloc( l+10, sizeof(char), "ini filename" );
  strncpy( iniName, fileName->value, l );
  strncat( iniName, ".temp", l+6 );

  char* path = MakeFullPath( GetWorkingDir( conf, userID ), iniName );
  free( iniName );

  if( EMPTY( path ) )
    APIError( methodName, -3, "Failed to create file path" );

  if( FileExists( path )==0 )
    APIError( methodName, -4, "Failed to open file path - temp file already exists" );

  FILE* f = fopen( path, "w" );
  if( f==NULL )
    APIError( methodName, -5, "Failed to open file path (temp file for writing)" );

  *pathPtr = path;

  return f;
  }

FILE* OpenConfigFileForAppending( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args, char** pathPtr )
  {
  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );

  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -1, "No FILENAME specified" );

  if( StringIsAnIdentifier( fileName->value )!=0 )
    APIError( methodName, -2, "FILENAME must be an identifier" );

  int l = strlen( fileName->value );

  char* iniName = (char*)SafeCalloc( l+10, sizeof(char), "ini filename" );
  strncpy( iniName, fileName->value, l );
  strncat( iniName, ".ini", l+5 );

  char* path = MakeFullPath( GetWorkingDir( conf, userID ), iniName );
  free( iniName );

  if( EMPTY( path ) )
    APIError( methodName, -3, "Failed to create file path" );

  if( FileExists( path )!=0 )
    APIError( methodName, -4, "File does not exist" );

  FILE* f = fopen( path, "a" );
  if( f==NULL )
    APIError( methodName, -5, "Failed to open file path for appending" );

  *pathPtr = path;

  return f;
  }

void ConfigFileReadRaw( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  char* path = NULL;
  FILE* f = OpenConfigFileForReading( conf, userID, methodName, args, &path );
  free( path );

  int nLines = 0;

  char buf[BUFLEN];
  _TAG_VALUE* file = NULL;
  _TAG_VALUE** last = NULL;
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    ++nLines;

    char* cleanLine = StripEOL( buf );
    _TAG_VALUE* thisLine = NewTagValue( NULL, cleanLine, NULL, 0 );
    if( last==NULL )
      {
      file = thisLine;
      last = &( thisLine->next );
      }
    else
      {
      *last = thisLine;
      last = &( thisLine->next );
      }
    }

  fclose( f );

  _TAG_VALUE* response = NULL;
  response = NewTagValueList( "CONFIG", file, response, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

_TAG_VALUE* CreateLineInclude( int lineNo, char* methodName, char* line )
  {
  if( strncasecmp( line, "#include", 8 )!=0 )
    APIError( methodName, -100, "Line [%s] does not start with #include", line );
  line += 8;
  while( *line!=0 && *line!=QUOTE )
    ++line;
  if( *line!=QUOTE )
    APIError( methodName, -101, "Filename in [%s] must be in quotes", line );
  ++line;
  char* fileName = line;
  if( EMPTY( fileName ) )
    APIError( methodName, -102, "Filename in [%s] is blank", line );
  while( *line!=0 && *line!=QUOTE )
    ++line;
  if( *line!=QUOTE )
    APIError( methodName, -103, "Filename in [%s] missing closing quote", line );
  *line = 0;

  /* Notice( "CreateLineInclude( %s )", fileName ); */

  char* safeFileName = StripQuotes( fileName );
  _TAG_VALUE* inner = NewTagValue( "FILENAME", fileName, NULL, 0 );
  if( fileName != safeFileName )
    free( safeFileName );

  inner = NewTagValue( "TYPE", LineTypeToString( lt_include ), inner, 0 );

  char label[BUFLEN];
  snprintf( label, sizeof(label)-1, "%05d", lineNo );
  return NewTagValueList( label, inner, NULL, 0 );
  }

_TAG_VALUE* CreateLineVar( int lineNo, char* methodName, char* line )
  {
  if( EMPTY( line ) )
    APIError( methodName, -100, "Empty line when expecting a variable assignment" );
  char* equals = strchr( line, '=' );
  if( equals==NULL )
    APIError( methodName, -101, "Line [%s] has no equals sign", line );
  char varName[BUFLEN];
  char varValue[BUFLEN];
  int varLen = MIN( (equals-line), sizeof(varName)-1 );
  int valLen = MIN( (line + strlen(line) - 1 - equals), sizeof(varValue)-1 );
  strncpy( varName, line, varLen );
  varName[varLen] = 0;
  strncpy( varValue, equals+1, valLen );
  varValue[valLen] = 0;

  /* Notice( "CreateLineVar( %s=%s )", varName, varValue ); */

  char* safeName = StripQuotes( varName );
  _TAG_VALUE* inner = NewTagValue( "VARIABLE", safeName, NULL, 0 );
  if( safeName != varName )
    free( safeName );

  char* safeValue = StripQuotes( varValue );
  inner = NewTagValue( "VALUE", safeValue, inner, 0 );
  if( safeValue != varValue )
    free( safeValue );

  inner = NewTagValue( "TYPE", LineTypeToString( lt_var ), inner, 0 );

  char label[BUFLEN];
  snprintf( label, sizeof(label)-1, "%05d", lineNo );
  return NewTagValueList( label, inner, NULL, 0 );
  }

_TAG_VALUE* CreateLineComment( int lineNo, char* methodName, char* line )
  {
  if( EMPTY( line ) )
    APIError( methodName, -100, "Empty line when expecting a variable assignment" );

  /* Notice( "CreateLineComment( %s )", line ); */

  char* safeLine = StripQuotes( line );
  _TAG_VALUE* inner = NewTagValue( "COMMENT", safeLine, NULL, 0 );
  if( safeLine != line )
    free( safeLine );

  inner = NewTagValue( "TYPE", LineTypeToString( lt_comment ), inner, 0 );

  char label[BUFLEN];
  snprintf( label, sizeof(label)-1, "%05d", lineNo );
  return NewTagValueList( label, inner, NULL, 0 );
  }

_TAG_VALUE* CreateLineBlank( int lineNo, char* methodName )
  {
  /* Notice( "CreateLineBlank( )" ); */

  char label[BUFLEN];
  snprintf( label, sizeof(label)-1, "%05d", lineNo );
  _TAG_VALUE* inner = NewTagValue( "TYPE", LineTypeToString( lt_blank ), NULL, 0 );
  return NewTagValueList( label, inner, NULL, 0 );
  }

/* if line has "... #something ..." then remove the "#something ..." */
void RemoveTrailingCommentsFromLine( char* line )
  {
  /* nothing to do with blank lines */
  if( EMPTY( line ) )
    return;

  /* starts with a comment?  leave that alone. */
  char* ptr = NULL;
  for( ptr=line; *ptr!=0 && isspace(*ptr); ++ptr )
    ;
  if( *ptr=='#' )
    return;

  /* no '#' mark anywhere */
  if( strchr( line, '#' )==NULL )
    return;

  int inQuote = 0;
  for( ptr=line; *ptr!=0; ++ptr )
    {
    if( *ptr==QUOTE )
      {
      inQuote = inQuote ? 0 : 1;
      continue;
      }
    if( *ptr=='#' && inQuote==0 )
      {
      *ptr = 0;
      (void)StripEOL( line );
      break;
      }
    }
  }

void ConfigFileReadStructured( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  char* path = NULL;
  FILE* f = OpenConfigFileForReading( conf, userID, methodName, args, &path );
  free( path );

  int nLines = 0;

  char buf[BUFLEN];
  _TAG_VALUE* file = NULL;
  _TAG_VALUE** last = &file;
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    ++nLines;

    char* cleanLine = StripEOL( buf );
    RemoveTrailingCommentsFromLine( cleanLine );
    enum lineType lt = ClassifyLineType( cleanLine );
    switch( lt )
      {
      case lt_include:
        *last = CreateLineInclude( nLines, methodName, cleanLine );
        break;
      case lt_var:
        *last = CreateLineVar( nLines, methodName, cleanLine );
        break;
      case lt_comment:
        *last = CreateLineComment( nLines, methodName, cleanLine );
        break;
      default: /* includes blank and invalid */
        *last = CreateLineBlank( nLines, methodName );
        break;
      }

    last = &( (*last)->next );
    }

  fclose( f );

  _TAG_VALUE* response = NULL;
  response = NewTagValueList( "CONFIG", file, response, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

void ConfigFileInsertLine( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* lineText = FindTagValueNoCase( args, "TEXT" );
  if( lineText==NULL )
    APIError( methodName, -100, "You must specify TEXT to insert" );
  if( lineText->value==NULL ) /* note, it can be empty, just not NULL */
    APIError( methodName, -101, "TEXT arguments must have a string value" );

  _TAG_VALUE* lineNumber = FindTagValueNoCase( args, "LINENUMBER" );
  if( lineNumber==NULL )
    APIError( methodName, -102, "You must specify LINENUMBER" );
  if( lineNumber->iValue<=0 )
    APIError( methodName, -103, "LINENUMBER must be at least 1" );

  char* readPath = NULL;
  FILE* read = OpenConfigFileForReading( conf, userID, methodName, args, &readPath );
  char* writePath = NULL;
  FILE* write = OpenTemporaryConfigFileForWriting( conf, userID, methodName, args, &writePath );

  char line[BUFLEN];
  int lineNo = 0;
  int written = 0;
  while( fgets( line, sizeof(line)-1, read )==line )
    {
    ++lineNo;
    if( lineNo==lineNumber->iValue )
      {
      fputs( lineText->value, write );
      fputs( "\n", write );
      written = 1;
      }
    fputs( line, write );
    }

  /* end of file line number */
  ++lineNo;

  /* append? */
  if( lineNo==lineNumber->iValue )
    {
    fputs( lineText->value, write );
    fputs( "\n", write );
    written = 1;
    }

  fclose( read );
  fclose( write );

  if( unlink( readPath )!=0 )
    APIError( methodName, -104, "Failed to unlink old configuration file (%d:%s)", errno, strerror( errno ) );

  if( rename( writePath, readPath )!=0 )
    APIError( methodName, -105, "Failed to rename new configuration file (%d:%s)", errno, strerror( errno ) );

  free( readPath );
  free( writePath );

  if( written==0 )
    APIError( methodName, -106, "Failed to find line number %d (out of %d) for inserting text", lineNumber->iValue, lineNo );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

void ConfigFileAppendLine( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* lineText = FindTagValueNoCase( args, "TEXT" );
  if( lineText==NULL )
    APIError( methodName, -100, "You must specify TEXT to append" );
  if( lineText->value==NULL ) /* note, it can be empty, just not NULL */
    APIError( methodName, -101, "TEXT arguments must have a string value" );

  char* writePath = NULL;
  FILE* write = OpenConfigFileForAppending( conf, userID, methodName, args, &writePath );

  fputs( lineText->value, write );
  fputs( "\n", write );
  fclose( write );
  free( writePath );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

void ConfigFileDeleteLine( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* lineNumber = FindTagValueNoCase( args, "LINENUMBER" );
  if( lineNumber==NULL )
    APIError( methodName, -102, "You must specify LINENUMBER" );
  if( lineNumber->iValue<=0 )
    APIError( methodName, -103, "LINENUMBER must be at least 1" );

  char* readPath = NULL;
  FILE* read = OpenConfigFileForReading( conf, userID, methodName, args, &readPath );
  char* writePath = NULL;
  FILE* write = OpenTemporaryConfigFileForWriting( conf, userID, methodName, args, &writePath );

  char line[BUFLEN];
  int lineNo = 0;
  int found = 0;
  while( fgets( line, sizeof(line)-1, read )==line )
    {
    ++lineNo;
    if( lineNo==lineNumber->iValue )
      found = 1;
    else
      fputs( line, write );
    }

  fclose( read );
  fclose( write );

  if( unlink( readPath )!=0 )
    APIError( methodName, -104, "Failed to unlink old configuration file (%d:%s)", errno, strerror( errno ) );

  if( rename( writePath, readPath )!=0 )
    APIError( methodName, -105, "Failed to rename new configuration file (%d:%s)", errno, strerror( errno ) );

  free( readPath );
  free( writePath );

  if( found==0 )
    APIError( methodName, -106, "Failed to find line number %d for deleting", lineNumber->iValue );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

void ConfigFileReplaceLine( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* lineText = FindTagValueNoCase( args, "TEXT" );
  if( lineText==NULL )
    {
    PrintTagValue( 2, args );
    APIError( methodName, -100, "You must specify TEXT to replace" );
    }
  if( lineText->value==NULL ) /* note, it can be empty, just not NULL */
    APIError( methodName, -101, "TEXT arguments must have a string value" );

  _TAG_VALUE* lineNumber = FindTagValueNoCase( args, "LINENUMBER" );
  if( lineNumber==NULL )
    APIError( methodName, -102, "You must specify LINENUMBER" );
  if( lineNumber->iValue<=0 )
    APIError( methodName, -103, "LINENUMBER must be at least 1" );

  char* readPath = NULL;
  FILE* read = OpenConfigFileForReading( conf, userID, methodName, args, &readPath );
  char* writePath = NULL;
  FILE* write = OpenTemporaryConfigFileForWriting( conf, userID, methodName, args, &writePath );

  char line[BUFLEN];
  int lineNo = 0;
  int written = 0;
  while( fgets( line, sizeof(line)-1, read )==line )
    {
    ++lineNo;
    if( lineNo==lineNumber->iValue )
      {
      fputs( lineText->value, write );
      fputs( "\n", write );
      written = 1;
      }
    else
      fputs( line, write );
    }

  fclose( read );
  fclose( write );

  if( unlink( readPath )!=0 )
    APIError( methodName, -104, "Failed to unlink old configuration file (%d:%s)", errno, strerror( errno ) );

  if( rename( writePath, readPath )!=0 )
    APIError( methodName, -105, "Failed to rename new configuration file (%d:%s)", errno, strerror( errno ) );

  free( readPath );
  free( writePath );

  if( written==0 )
    APIError( methodName, -106, "Failed to find line number %d/%d for replacing text", lineNumber->iValue, lineNo );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

void ConfigFileReplaceFile( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* contents = FindTagValueNoCase( args, "CONTENTS" );
  if( contents==NULL )
    APIError( methodName, -100, "You must specify CONTENTS to replace" );
  if( contents->type!=VT_LIST )
    APIError( methodName, -101, "CONTENTS must be a list" );

  char* readPath = NULL;
  FILE* read = OpenConfigFileForReading( conf, userID, methodName, args, &readPath );
  fclose( read );

  char* writePath = NULL;
  FILE* write = OpenTemporaryConfigFileForWriting( conf, userID, methodName, args, &writePath );

  for( _TAG_VALUE* line = contents->subHeaders; line!=NULL; line=line->next )
    {
    if( NOTEMPTY( line->value ) )
      {
      uint8_t buf[BUFLEN];
      UnescapeString( line->value, buf, sizeof(buf)-1 );
      fputs( (char*)buf, write );
      }
    fputs( "\n", write );
    }

  fclose( write );

  if( unlink( readPath )!=0 )
    APIError( methodName, -104, "Failed to unlink old configuration file (%d:%s)", errno, strerror( errno ) );

  if( rename( writePath, readPath )!=0 )
    APIError( methodName, -105, "Failed to rename new configuration file (%d:%s)", errno, strerror( errno ) );

  free( readPath );
  free( writePath );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

void ConfigFileValidate( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );
  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -1, "No FILENAME specified" );
  if( StringIsAnIdentifier( fileName->value )!=0 )
    APIError( methodName, -2, "FILENAME must be an identifier" );

  int l = strlen( fileName->value );
  char* iniName = (char*)SafeCalloc( l+10, sizeof(char), "ini filename" );
  strncpy( iniName, fileName->value, l );
  strncat( iniName, ".ini", l+5 );

  char* path = MakeFullPath( GetWorkingDir( conf, userID ), iniName );
  if( EMPTY( path ) )
    APIError( methodName, -3, "Failed to create file path" );
  if( FileExists( path )!=0 )
    APIError( methodName, -4, "File [%s] does not exist.", iniName );
  free( path );

  if( EMPTY( conf->validateCommand ) || EMPTY( conf->validateOk ) )
    APIError( methodName, -5, "A configuration file validation command or result are not specified" );
  if( strstr( conf->validateCommand, "WORKDIR" )==NULL )
    APIError( methodName, -6, "Validation command does not specify WORKDIR" );
  if( strstr( conf->validateCommand, "INIFILE" )==NULL )
    APIError( methodName, -7, "Validation command does not specify INIFILE" );

  char* cmd1 = SearchAndReplace( conf->validateCommand, "INIFILE", iniName );
  char* cmd2 = SearchAndReplace( cmd1, "WORKDIR", GetWorkingDir( conf, userID ) );
  free( cmd1 );

  char** results = NULL;
  results = (char**)SafeCalloc( N_CMD_RESULTS, sizeof(char*), "results buffer set" );
  for( int i=0; i<N_CMD_RESULTS; ++i )
    results[i] = (char*)SafeCalloc( CMD_RESULT_LINE_LEN, sizeof(char), "single result buffer" );

  fflush( stdout );
  int err = ReadLinesFromCommand( cmd2,
                                  results,
                                  N_CMD_RESULTS /*lines*/,
                                  CMD_RESULT_LINE_LEN /*buflen*/,
                                  1 /*seconds/line*/,
                                  3 /*total time*/ );

  free( cmd2 );
  free( iniName );

  if( err<0 )
    {
    for( int i=0; i<N_CMD_RESULTS; ++i )
      free( results[i] );
    free( results );
    APIError( methodName, -8, "Failed to run validation command - %d", err );
    }

  int gotOkay = 0;
  for( int i=0; i<N_CMD_RESULTS && results[i][0]!=0; ++i )
    {
    if( strstr( results[i], conf->validateOk )!=NULL )
      {
      gotOkay = 1;
      break;
      }
    }

  if( gotOkay )
    {
    for( int i=0; i<N_CMD_RESULTS; ++i )
      free( results[i] );
    free( results );
    _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
    FreeTagValue( response );
    return;
    }

  _TAG_VALUE* response = NULL;
  for( int i=0; i<N_CMD_RESULTS && results[i][0]!=0; ++i )
    {
    if( EMPTY( conf->validateErrorKeyword )
        || strstr( results[i], conf->validateErrorKeyword )!=NULL )
      {
      response = NewTagValue( "RESULT", results[i], response, 0 );
      }
    free( results[i] );
    }
  free( results );

  response = SendShortResponse( -100, "Configuration file error", response );
  FreeTagValue( response );
  }

void ConfigFileDelete( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  _TAG_VALUE* fileName = FindTagValueNoCase( args, "FILENAME" );
  if( fileName==NULL || EMPTY( fileName->value ) )
    APIError( methodName, -1, "No FILENAME specified" );
  if( StringIsAnIdentifier( fileName->value )!=0 )
    APIError( methodName, -2, "FILENAME must be an identifier" );

  int l = strlen( fileName->value );
  char* iniName = (char*)SafeCalloc( l+10, sizeof(char), "ini filename" );
  strncpy( iniName, fileName->value, l );
  strncat( iniName, ".ini", l+5 );

  char* path = MakeFullPath( GetWorkingDir( conf, userID ), iniName );
  if( EMPTY( path ) )
    APIError( methodName, -3, "Failed to create file path" );
  if( FileExists( path )!=0 )
    APIError( methodName, -4, "File [%s] does not exist.", iniName );

  unlink( path );
  if( FileExists( path )==0 )
    APIError( methodName, -5, "Unable to delete file [%s].", iniName );

  free( path );

  _TAG_VALUE* response = SendShortResponse( 0, SUCCESS, NULL );
  FreeTagValue( response );
  }

/***************************************************************************
 * FUNCTIONS RELATED TO SESSION AND LOGIN:
 ***************************************************************************/
void GetLogoutURL( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  if( conf==NULL )
    APIError( methodName, -1, "No configuration" );
  if( EMPTY( conf->authServiceUrl ) )
    APIError( methodName, -2, "No AUTH_SERVICE_URL not set in configuration" );

  char url[BUFLEN];
  snprintf( url, sizeof(url)-1, "%s?LOGOUT", conf->authServiceUrl );

  _TAG_VALUE* response = NewTagValue( "url", url, NULL, 0 );
  response = SendShortResponse( 0, SUCCESS, response );
  FreeTagValue( response );
  }

void GetMyIdentity( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args )
  {
  if( conf==NULL )
    APIError( methodName, -1, "No configuration" );
  if( userID==NULL )
    APIError( methodName, -2, "No ID provided" );

  int gotKnownUser = 0;
  for( _USER* u=conf->users; NOTEMPTY( userID ) && u!=NULL; u=u->next )
    {
    if( NOTEMPTY( u->id ) && strcasecmp( u->id, userID )==0 )
      {
      gotKnownUser = 1;
      break;
      }
    }

  if( gotKnownUser==0 )
    {
    char errmsg[BUFLEN];
    snprintf( errmsg, sizeof(errmsg)-1, "User [%s] is not recognized.", userID );
    _TAG_VALUE* response = SendShortResponse( -100, errmsg, NULL );
    FreeTagValue( response );
    }
  else
    {
    _TAG_VALUE* response = NewTagValue( "id", userID, NULL, 0 );
    response = SendShortResponse( 0, SUCCESS, response );
    FreeTagValue( response );
    }
  }

/***************************************************************************
 * API FUNCTIONS HAVE TO BE REGISTERED HERE:
 ***************************************************************************/
_FN_RECORD methods[] =
  {
    { "api", "list",                ApiList,
      "List all available API functions." },
    { "api", "describe",            ApiFunctionDescribe,
      "Describe a single API function (Provide TOPIC,ACTION)." },

    { "vartype", "list",            VarTypeList,
      "List variable types.  args=[parenttype]; if none specified, then only list top-level types." },
    { "vartype", "describe",        VarTypeDescribe,
      "Describe a single variable type.  args=[type] (mandatory)." },

    { "command", "list",            CommandList,
      "List all available commands that we can run" },
    { "command", "run",             CommandRun,
      "Run a command.  Single argument: COMMAND" },
    { "command", "fetchoutput",     CommandFetchOutput,
      "Fetch an output file previously generated by a command.  Three arguments: COMMAND, FILENAME, DOWNLOADNAME." },

    { "config", "readraw",          ConfigFileReadRaw,
      "Read a configuration file named in FILENAME.  Output is just raw text lines (in JSON array)." },
    { "config", "readstr",          ConfigFileReadStructured,
      "Read a configuration file named in FILENAME.  Output is structured - each line has a type." },
    { "config", "insertline",       ConfigFileInsertLine,
      "Insert a line of text into the configuration file.  Three arguments: FILENAME, LINENUMBER, TEXT." },
    { "config", "appendline",       ConfigFileAppendLine,
      "Append a line of text to the configuration file.  Two arguments: FILENAME, TEXT." },
    { "config", "deleteline",       ConfigFileDeleteLine,
      "Delete a line of text from the configuration file.  Two arguments: FILENAME, LINENUMBER." },
    { "config", "replaceline",      ConfigFileReplaceLine,
      "Replace a line of text in the configuration file.  Three arguments: FILENAME, LINENUMBER, TEXT." },
    { "config", "replacefile",      ConfigFileReplaceFile,
      "Replace entire configuration file.  Two arguments: FILENAME, CONTENTS (array of strings)." },
    { "config", "validate",         ConfigFileValidate,
      "Check whether the configuration file is valid - return error messages if not.  One argument: FILENAME." },
    { "config", "deletefile",       ConfigFileDelete,
      "Delete a file.  Dangerous.  One argument: FILENAME" },

    { "logout", "get-url", GetLogoutURL,
      "Get the URL to sign out of the current login session.  No arguments." },
    { "user", "whoami", GetMyIdentity,
      "Return the login ID from the (encrypted, so opaque to client) session cookie." },

    { NULL, NULL, NULL, NULL }
  };

