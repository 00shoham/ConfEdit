#include "base.h"

void DocumentVariable( FILE* f, _VARIABLE* var )
  {
  char varBuf[BUFLEN];
  char extraBuf[BUFLEN];

  if( var->parent==NULL && var->children!=NULL )
    fprintf( f, "\n\n\\section{%s}\n\n", TexEscape( varBuf, sizeof(varBuf), var->id ) );
  else
    fprintf( f, "\n\n\\textbf{%s:}\n\n", TexEscape( varBuf, sizeof(varBuf), var->id ) );


  fprintf( f, "\n%s\n", TexEscape( varBuf, sizeof(varBuf), var->helpText ) );

  fprintf( f, "\nSet this variable to a value of type '%s.'\n", NameOfType( var->type ) );
  if( var->minInt != INT_MAX )
    fprintf( f, "\nThe smallest permissible value is %d.\n", var->minInt );
  if( var->maxInt != INT_MIN )
    fprintf( f, "\nThe largest permissible value is %d.\n", var->maxInt );
  if( var->minFloat != DBL_MAX )
    fprintf( f, "\nThe smallest permissible value is %.1lf.\n", var->minFloat );
  if( var->maxFloat != DBL_MIN )
    fprintf( f, "\nThe largest permissible value is %.1lf.\n", var->maxFloat );

  if( var->xref!=NULL && NOTEMPTY( var->xref->id ) )
    fprintf( f, "\nSet the value to the ID of a %s variable.\n",
             TexEscape( varBuf, sizeof( varBuf ), var->xref->id ) );

  if( var->leftType!=NULL && NOTEMPTY( var->leftType->id )
      && var->rightType!=NULL && NOTEMPTY( var->rightType->id ) )
    fprintf( f, "\nSet this to the IDs of two other variables, separated by a space:  "
                "The left one is of type %s and the right one is of type %s.\n",
             TexEscape( varBuf, sizeof(varBuf), var->leftType->id ),
             TexEscape( extraBuf, sizeof(extraBuf), var->rightType->id ) );

  if( var->singleton )
    fprintf( f, "\nThis variable can only appear once in the configuration file.\n" );

  if( var->mandatory )
    fprintf( f, "\nYou must include %s this variable in any valid configuration file.\n",
             var->singleton? "" : "at least one instance of" );

  if( var->minValues>0 )
    {
    if( var->minValues==1 )
      {
      fprintf( f, "\nYou can set this variable repeatedly -- once per line in the"
                  "configuration file.  You have to set it at least once.\n" );
      }
    else if( var->minValues==2 )
      {
      fprintf( f, "\nYou can set this variable repeatedly -- once per line in the"
                  "configuration file.  You have to set it at least twice.\n" );
      }
    else
      {
      fprintf( f, "\nYou can set this variable repeatedly -- once per line in the"
                  "configuration file.  You have to set it at least %d times.\n", var->minValues );
      }
    }

  if( var->gotDefault )
    {
    switch( var->type )
      {
      case vt_int:
        fprintf( f, "\nIf you don't set this variable, its default value will be %d.\n", var->defaultInt );
        break;
      case vt_float:
        fprintf( f, "\nIf you don't set this variable, its default value will be %lg\n", var->defaultFloat );
        break;
      case vt_bool:
        fprintf( f, "\nIf you don't set this variable, its default value will be %s\n", var->defaultBool ? "true" : "false" );
        break;
      case vt_string:
        fprintf( f, "\nIf you don't set this variable, its default value will be %s\n", TexEscape( varBuf, sizeof( varBuf ), var->defaultString ) );
        break;
      default:
        Error( "Default for variable %s of type %s", var->id, NameOfType( var->type ) );
      }
    }

  if( var->children != NULL )
    fprintf( f, "\nThis variable is typically followed by the following \"subsidiary\" variables:\n" );

  for( _VARPTR * vp = var->children; vp!=NULL; vp=vp->next )
    {
    DocumentVariable( f, vp->var );
    }
  }

void DocumentSchema( FILE* f, _CONFIG* config )
  {
  if( f==NULL )
    {
    Error("Cannot document configuration to NULL file");
    }

  if( FileExists( TEX_HEADER ) == 0 )
    {
    FILE* src = fopen( TEX_HEADER, "r" );
    if( src != NULL )
      {
      FileCopyHandles( src, f );
      fclose( src );
      }
    }

  /* Do the ones that have children first: */
  for( _VARIABLE* var = config->variables; var!=NULL; var=var->next )
    {
    if( var->parent==NULL && var->children!=NULL )
      DocumentVariable( f, var );
    }

  char* preAmbleForSingletons = 
    "\n\n\\section{Singleton variables}\n\n"
    "\nThese variables appear at most once in the configuration.\n";
  int printedPreamble = 0;

  /* Next, do the ones that don't have children: */
  for( _VARIABLE* var = config->variables; var!=NULL; var=var->next )
    {
    if( var->parent==NULL && var->children==NULL && var->singleton )
      {
      if( printedPreamble == 0 )
        {
        fputs( preAmbleForSingletons, f );
        printedPreamble = 1;
        }
      DocumentVariable( f, var );
      }
    }

  printedPreamble = 0;
  char* preAmbleForRemainder = 
     "\n\n\\section{Everything else}\n\n"
     "\nThese variables can appear more than once, but have no 'child' variables.\n";

  /* Next, do the ones that don't have children: */
  for( _VARIABLE* var = config->variables; var!=NULL; var=var->next )
    {
    if( var->parent==NULL && var->children==NULL && var->singleton==0 )
      {
      if( printedPreamble == 0 )
        {
        fputs( preAmbleForRemainder, f );
        printedPreamble = 1;
        }
      DocumentVariable( f, var );
      }
    }

  if( FileExists( TEX_FOOTER ) == 0 )
    {
    FILE* src = fopen( TEX_FOOTER, "r" );
    if( src != NULL )
      {
      FileCopyHandles( src, f );
      fclose( src );
      }
    }
  }
