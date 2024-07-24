#ifndef _INCLUDE_VALUE
#define _INCLUDE_VALUE

#define WORKINGDIR "/var/www/confedit"

typedef struct _value
  {
  _VARIABLE* var;
  int intVal;
  double floatVal;
  char* strVal;
  _MMDD dateVal;
  struct _value* leftValue;
  struct _value* rightValue;

  struct _value* next;
  } _VALUE;
  
_VALUE* AppendValueToList( _CONFIG* conf, _VALUE** listPtr, char* id );
void FreeValue( _VALUE* v );
void FreeValueList( _VALUE* valueList );
void ProcessWorkingConfigLine( _CONFIG* config, _CONFIG* workingConf,
                               _VALUE* valuesList, char* ptr, char* equalsChar );
void ReadWorkingConfig( _CONFIG* config, _CONFIG* workingConf,
                        _VALUE* valuesList, char* filePath );

#endif
