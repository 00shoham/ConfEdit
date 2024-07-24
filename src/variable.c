#include "base.h"

enum varType TypeFromString( char* value )
  {
  enum varType retVal = vt_invalid;

  if( EMPTY( value ) )
    {}
  else if( strcasecmp( value, "int" )==0 )
    retVal = vt_int;
  else if( strcasecmp( value, "date" )==0 )
    retVal = vt_date;
  else if( strcasecmp( value, "string" )==0 )
    retVal = vt_string;
  else if( strcasecmp( value, "bool" )==0 )
    retVal = vt_bool;
  else if( strcasecmp( value, "float" )==0 )
    retVal = vt_float;
  else if( strcasecmp( value, "xref" )==0 )
    retVal = vt_xref;
  else if( strcasecmp( value, "dateval" )==0 )
    retVal = vt_dateval;
  else if( strcasecmp( value, "intlist" )==0 )
    retVal = vt_intlist;
  else
    Error( "Type must be int|date|string|bool|float|xref|dateval" );

  return retVal;
  }

char* NameOfType( enum varType type )
  {
  switch( type )
    {
    case vt_invalid:
      return "invalid";
    case vt_int:
      return "int";
    case vt_date:
      return "date";
    case vt_string:
      return "string";
    case vt_bool:
      return "bool";
    case vt_float:
      return "float";
    case vt_xref:
      return "xref";
    case vt_dateval:
      return "dateval";
    case vt_intlist:
      return "intlist";
    default:
      Error( "Impossible type in NameOfType" );
    }

  /* not actually happening - see above */
  return NULL;
  }

_VARIABLE* FindVariable( _VARIABLE* list, char* id )
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

_VARIABLE* AppendVariable( _CONFIG* conf, char* id )
  {
  if( conf==NULL || EMPTY( id ) )
    return NULL;

  _VARIABLE* newV = (_VARIABLE*)SafeCalloc( 1, sizeof( _VARIABLE ), "New variable" );
  newV->id = strdup( id );
  UpperCase( newV->id, strlen( newV->id ), newV->id );

  newV->minInt = INT_MAX;
  newV->maxInt = INT_MIN;
  newV->minFloat = DBL_MAX;
  newV->maxFloat = DBL_MIN;

  _VARIABLE** vPtr = &( conf->variables );
  for( ; vPtr!=NULL && (*vPtr)!=NULL; )
    {
    _VARIABLE* v = *vPtr;
    vPtr = &(v->next);
    }
  if( vPtr==NULL )
    Error( "Failed to append to variable list" );

  *vPtr = newV;
  conf->lastVar = newV;
  return newV;
  }

void PrintVariable( FILE* f, _VARIABLE* var )
  {
  fprintf( f, "VARIABLE=%s\n", var->id );

  if( var->parent!=NULL && NOTEMPTY( var->parent->id ) )
    fprintf( f, "PARENT=%s\n", var->parent->id );

  fprintf( f, "TYPE=%s\n", NameOfType( var->type ) );

  if( var->listOfValues )
    fprintf( f, "ISLIST=true\n" );

  if( var->xref!=NULL && NOTEMPTY( var->xref->id ) )
    fprintf( f, "XREF=%s\n", var->xref->id );

  if( var->leftType!=NULL && NOTEMPTY( var->leftType->id ) )
    fprintf( f, "REFLEFT=%s\n", var->leftType->id );

  if( var->rightType!=NULL && NOTEMPTY( var->rightType->id ) )
    fprintf( f, "REFRIGHT=%s\n", var->rightType->id );

  if( var->minInt != INT_MAX )
    fprintf( f, "MIN=%d\n", var->minInt );
  if( var->maxInt != INT_MIN )
    fprintf( f, "MAX=%d\n", var->maxInt );

  if( var->minFloat != DBL_MAX )
    fprintf( f, "MIN=%lg\n", var->minFloat );
  if( var->maxFloat != DBL_MIN )
    fprintf( f, "MAX=%lg\n", var->maxFloat );

  if( var->minValues > 0 )
    fprintf( f, "MINVALUES=%d\n", var->minValues );

  if( NOTEMPTY( var->childrenValidateExpression ) )
    fprintf( f, "VALIDATE=%s\n", var->childrenValidateExpression );

  if( var->gotDefault )
    {
    switch( var->type )
      {
      case vt_int:
        fprintf( f, "DEFAULT=%d\n", var->defaultInt );
        break;
      case vt_float:
        fprintf( f, "DEFAULT=%lg\n", var->defaultFloat );
        break;
      case vt_bool:
        fprintf( f, "DEFAULT=%s\n", var->defaultBool ? "true" : "false" );
        break;
      case vt_string:
        fprintf( f, "DEFAULT=%s\n", var->defaultString );
        break;
      default:
        Error( "Default for variable %s of type %s", var->id, NameOfType( var->type ) );
      }
    }

  if( var->singleton )
    fprintf( f, "SINGLETON=true\n" );

  if( NOTEMPTY( var->helpText ) )
    fprintf( f, "HELP=%s\n", var->helpText );

  if( var->follows!=NULL && NOTEMPTY( var->follows->id ) )
    fprintf( f, "FOLLOWS=%s\n", var->follows->id );

  if( var->next!=NULL )
    fprintf( f, "\n" );
  }

int ValidateVariable( _VARIABLE* v )
  {
  if( EMPTY( v->id ) )
    Error( "Variable has NULL ID" );

  if( v->type==vt_xref )
    {
    if( v->leftType==NULL )
      Error( "Var type %s is xref but no left var type", v->id );
    if( v->rightType==NULL )
      Error( "Var type %s is xref but no right var type", v->id );
    }

  if( v->listOfValues )
    {
    if( v->minValues <=0 )
      Error( "Var type %s is list of values but no minimum quanitty", v->id );
    }

  for( _VARIABLE* vTest = v->next; v!=NULL; v=v->next )
    {
    if( NOTEMPTY( vTest->id ) && strcasecmp( vTest->id, v->id )==0 )
      Error( "Two variables called [%s}", v->id );
    }

  return 0;
  }

void FreeVariable( _VARIABLE* v )
  {
  FREEIFNOTNULL( v->id );

  for( _VARPTR* vp=v->children; vp!=NULL; )
    {
    _VARPTR* vpNext = vp->next;
    free( vp );
    vp = vpNext;
    }

  FREEIFNOTNULL( v->childrenValidateExpression );
  FREEIFNOTNULL( v->defaultString );
  FREEIFNOTNULL( v->helpText );

  free( v );
  }
