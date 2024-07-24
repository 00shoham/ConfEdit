#ifndef _INCLUDE_API
#define _INCLUDE_API

#define SUCCESS "success"
#define ERROR "error"

#define DEBUG 1

typedef struct fnRecord
  {
  char* topic;
  char* action;
  void (*function)( _CONFIG* conf, char* userID, char* methodName, _TAG_VALUE* args );
  char* description;
  } _FN_RECORD;

_TAG_VALUE* SendShortResponse( int code, char* result, _TAG_VALUE* response );
void CallAPIFunction( _CONFIG* conf, char* userID, char* method, char* testingInput );

#endif
