#ifndef _INCLUDE_VARIABLE
#define _INCLUDE_VARIABLE

enum varType
  {
  vt_invalid,
  vt_int,
  vt_date,
  vt_string,
  vt_bool,
  vt_float,
  vt_xref,
  vt_dateval,
  vt_intlist
  };

struct _varPtr;

typedef struct _variable
  {
  char* id;
  enum varType type;
  int listOfValues;

  struct _variable* parent;
  struct _varPtr* children;

  int minInt;
  int maxInt;
  double minFloat;
  double maxFloat;
  double step;
  struct _variable* xref;

  struct _variable* leftType;
  struct _variable* rightType;

  int singleton;
  int mandatory;
  int minValues;

  int gotDefault;
  int defaultInt;
  double defaultFloat;
  double defaultBool;
  char* defaultString;
  char* legalValues;

  char* helpText;
  struct _variable* follows;
  struct _variable* nextVar;

  char* childrenValidateExpression;

  struct _variable* next;
  } _VARIABLE;

typedef struct _varPtr
  {
  _VARIABLE* var;
  struct _varPtr* next;
  } _VARPTR;

enum varType TypeFromString( char* value );
char* NameOfType( enum varType type );
_VARIABLE* FindVariable( _VARIABLE* list, char* id );
_VARIABLE* AppendVariable( _CONFIG* conf, char* id );
void PrintVariable( FILE* f, _VARIABLE* v );
int ValidateVariable( _VARIABLE* v );
void FreeVariable( _VARIABLE* v );

#endif
