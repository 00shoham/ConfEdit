var baseURL = "/cgi-bin/confedit-api?";

var messageTimeout = 5*1000;

var onLineNow = window.navigator.onLine;
var tabIsVisible = false;

var iniFile = "";
var iniArray = [];
var schema = [];
var gotCommands = 0;
var commands = [];

var textMode = false;

var sleepSetTimeout_ctrl;

var BG_EDITED = "#ffcccc";
var BG_SAVED  = "#ffffff";

function ProtectString( str )
  {
  if( typeof str == "string" )
    return str;
  else
    return "";
  }

function MarkEdited( element )
  {
  if( element )
    {
    element.style.background = BG_EDITED;
    }
  }

function MarkSaved( element )
  {
  if( element )
    {
    element.style.background = BG_SAVED;
    }
  }

function Sleep( ms )
  {
  clearInterval( sleepSetTimeout_ctrl );
  return new Promise( resolve => sleepSetTimeout_ctrl = setTimeout( resolve, ms ) );
  }

function PadDigits( number, digits )
  {
  return Array( Math.max( digits - String(number).length + 1, 0) ).join(0) + number;
  }

function ReportResults( msg )
  {
  // Ensure that refresh works again.

  var errDiv = document.getElementById("error");
  if( typeof errDiv != "undefined" )
    {
    errDiv.innerHTML = msg;
    errDiv.style.display = "block";
    setTimeout(function(){ errDiv.style.display = "none"; }, messageTimeout);
    }
  }

function Online()
  {
  onLineNow = true;
  if( tabIsVisible )
    {
    ReportResults( "Restored network connection" );
    }
  }

function Offline()
  {
  onLineNow = false;
  if( tabIsVisible )
    {
    ReportResults( "Lost network connection" );
    }
  }

function SetCookie( cname, cvalue, exdays )
  {
  const d = new Date();
  d.setTime(d.getTime() + (exdays*24*60*60*1000));
  let expires = "expires="+ d.toUTCString();
  document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
  }

function GetCookie( cname )
  {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent( document.cookie );
  var ca = decodedCookie.split( ';' );
  for( let i = 0; i<ca.length; i++ )
    {
    let c = ca[i];
    while( c.charAt(0) == ' ')
      {
      c = c.substring(1);
      }
    if( c.indexOf(name) == 0 )
      {
      return c.substring( name.length, c.length );
      }
    }
  return "";
  }

function ToggleObjectDisplay( id )
  {
  var obj = document.getElementById( id );
  if( ! obj || typeof obj == "undefined" )
    return;
  if( obj.style.display == "none")
    obj.style.display = "block";
  else
    obj.style.display = "none";
  }

function DisplayError( msg )
  {
  if( typeof msg == "string" )
    {
    console.log( msg );
    }
  else
    {
    console.log( "Unknown message" );
    }

  var errDiv = document.getElementById("error");
  if( typeof errDiv != "undefined" )
    {
    errDiv.innerHTML = msg;
    errDiv.style.display = "block";
    setTimeout(function(){ errDiv.style.display = "none"; }, messageTimeout);
    }
  else
    alert( msg );
  }

function CallAPIFunction( topic, action, args )
  {
  if( ! onLineNow )
    {
    return "API calls not supported while offline";
    }

  var method = "/" + topic + "/" + action;
  var xhttp = new XMLHttpRequest();
  xhttp.open( "POST", baseURL + method, false );
  xhttp.send( JSON.stringify(args) );
  var responseText = xhttp.responseText;
  var responseObj;
  try
    {
    responseObj = JSON.parse( responseText );
    }
  catch( err )
    {
    DisplayError( "API returned error (A) - " + err.message );
    }

  var errmsg = "An error has occurred.  No details available.";

  if( typeof( responseObj ) == "undefined"
      || typeof responseObj.code == "undefined"
      || typeof responseObj.result == "undefined" )
    {
    DisplayError( errmsg );
    }
  else if( responseObj.code !=0 )
    {
    errmsg = responseObj.result;
    if( errmsg != "Variable not found"
        && errmsg != "Configuration file error" )
      { /* special case that happens often and we don't want to alert anyone */
      DisplayError( "API returned error (B) - " + errmsg );
      }
    }

  return responseObj;
  }

function SetFileName( id )
  {
  var inputId = "include-" + id;

  var inputField = document.getElementById( inputId );
  if( typeof inputField == "undefined" )
    {
    DisplayError( "Cannot locate ID include-"+id );
    return;
    }

  var selectedFile = inputField.value;

  var iniField = document.getElementById( "ini" );
  if( typeof iniField == "undefined" )
    {
    DisplayError( "Missing ini input field" );
    return;
    }

  iniField.value = selectedFile;
  LoadFile( "" );
  }

function ValidateIntList( value, definition )
  {
  if( typeof( value ) != "string" )
    {
    DisplayError( "No discernable value in field that should be a list of integers" );
    return false;
    }

  if( typeof( definition ) != "object" )
    {
    DisplayError( "ValidateIntList() - no definition passed in as argument" );
    return false;
    }

  var minValue = definition["MINIMUM"];
  var maxValue = definition["MAXIMUM"];
  if( typeof minValue != "number" || typeof maxValue != "number" )
    {
    DisplayError( "ValidateIntList() - min/max not properly defined - " + minValue + " / " + maxValue );
    return false;
    }

  var noSpace = value.replace( " ", "" );
  var listOfValues = noSpace.split( "," );
  var l = listOfValues.length;
  for( let i=0; i<l; i++ )
    {
    var item = listOfValues[i];
    if( typeof item == "string" )
      {
      if( ! item.match( /^[0-9]*$/ ) )
        {
        DisplayError( "Item " + item + " is not a (positive) integer!" );
        return false;
        }
      item = Number( item );
      }

    if( typeof item != "number" )
      {
      DisplayError( "Item " + item + " is not a number!" );
      return false;
      }
    if( item < minValue )
      {
      DisplayError( "Item " + item + " is too small (minimum = " + minValue + ")" );
      return false;
      }
    if( item > maxValue )
      {
      DisplayError( "Item " + item + " is too large (maximum = " + maxValue + ")" );
      return false;
      }
    }

  return true;
  }

function SaveVariable( id )
  {
  if( typeof id == "undefined" || "" == id )
    {
    DisplayError( "Cannot save without specifying an ID" );
    return;
    }

  if( typeof iniFile == "undefined" || "" == iniFile )
    {
    DisplayError( "Cannot save to empty filename" );
    return;
    }

  var varId = "variable-" + id;
  var varField = document.getElementById( varId );
  if( typeof varField == "undefined" )
    {
    DisplayError( "Cannot locate ID variable-"+id );
    return;
    }

  var leftId = "left-" + id;
  var leftField = document.getElementById( leftId );
  var rightId = "right-" + id;
  var rightField = document.getElementById( rightId );
  var value = "";
  if( leftField
      && rightField
      && typeof leftField.value == "string"
      && typeof rightField.value == "string" )
    {
    value = leftField.value + " " + rightField.value;
    }

  if( "" == value )
    {
    var valId = "value-" + id;
    var valField = document.getElementById( valId );
    if( typeof valField == "undefined" )
      {
      DisplayError( "Cannot locate ID value-"+id );
      return;
      }
    }

  var n = Number( id );
  if( ! iniArray[n] )
    {
    DisplayError( "Cannot find line " + n + " in in-memory array" );
    return;
    }

  var obj = iniArray[n];
  if( obj.type != "VAR" )
    {
    DisplayError( "Line " + n + " is not a variable" );
    return;
    }

  var definition = schema[ obj.variable ];
  if( typeof definition != "object" )
    {
    console.log( "Cannot locate definition for variable type " + obj.variable );
    }
  else
    {
    var type = definition["TYPE"];

    if( type == "int" || type == "float" )
      {
      if( valField.value < definition["MINIMUM"] )
        {
        DisplayError( "Too small - must be at least " + definition["MINIMUM"] );
        return;
        }
      if( valField.value > definition["MAXIMUM"] )
        {
        DisplayError( "Too large - must be at most " + definition["MAXIMUM"] );
        return;
        }
      }
    else if( type == "intlist" )
      {
      if( ! ValidateIntList( valField.value, definition ) )
        {
        return;
        }
      }
    else if( type == "bool" )
      {
      if( valField.checked )
        {
        valField.value = true;
        }
      else
        {
        valField.value = false;
        }
      }
    }

  if( "" == value )
    {
    value = valField.value;
    }

  var secondId = "second-value-" + id;
  var secondField = document.getElementById( secondId );
  if( secondField )
    {
    value += " " + secondField.value;
    }

  if( obj.value == value )
    { // no change.  that's not an error.
    return;
    }

  var lineString = varField.value + "=" + value;

  var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ), "TEXT": lineString };
  var dontCare = CallAPIFunction( "config", "replaceline", args );

  if( valField )
    {
    MarkSaved( valField );
    }
  if( secondField )
    {
    MarkSaved( secondField );
    }
  if( leftField )
    {
    MarkSaved( leftField );
    }
  if( rightField )
    {
    MarkSaved( rightField );
    }
  obj.value = value;
  }

function GetListOfChildren( id )
  {
  var n = Number( id );
  if( n < 1 )
    {
    return;
    }

  var obj = iniArray[n];
  if( ! obj || obj.type != "VAR" )
    {
    return;
    }

  var varName = obj["variable"];
  if( typeof varName != "string" )
    {
    return;
    }

  var args = {};
  args["PARENTTYPE"] = varName;
  var response = CallAPIFunction( "vartype", "list", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get a list of child variables of " + varName );
    return;
    }

  var varTypes = response["vartypes"];
  if( typeof varTypes == "undefined" || varTypes == null )
    {
    DisplayError( "No vartypes in API response" );
    return;
    }

  var resultSet = new Array();
  var l = iniArray.length;
  for( let i = n+1; i<l; i++ )
    {
    var obj = iniArray[i];
    if( ! obj || obj["type"]!="VAR" )
      { /* no object here, or it's not a variable? */
      continue;
      }
    var candidateVar = obj["variable"];
    if( typeof candidateVar != "string" )
      { /* no varname?  weird.  move on.*/
      continue;
      }
    if( candidateVar == varName )
      { /* we've reached the next parent, so we're done. */
      break;
      }
    if( ! varTypes.includes( candidateVar ) )
      { /* we're on to the next thing - presumably done. */
      break;
      }
    if( varTypes.includes( candidateVar ) )
      {
      var miniArray = {};
      miniArray["id"] = PadDigits( i, 5 );
      miniArray["varname"] = candidateVar;
      resultSet.push( miniArray );
      }
    }

  return resultSet;
  }

function DeleteLine( id )
  {
  if( typeof id == "undefined" || "" == id )
    {
    DisplayError( "Cannot save without specifying an ID" );
    return;
    }

  if( typeof iniFile == "undefined" || "" == iniFile )
    {
    DisplayError( "Cannot save to empty filename" );
    return;
    }

  var n = Number( id );
  if( n < 1 )
    {
    DisplayError( "DeleteLine() - invalid ID" );
    return;
    }

  var arrayLen = iniArray.length;
  console.log( "Deleting line " + n + " (" + id + ") out of array " + arrayLen );
  if( n >= arrayLen )
    {
    console.log( "End of array..  nothing to do" );
    return;
    }

  var obj = iniArray[n];
  if( obj && obj.type == "VAR" )
    {
    var varName = obj["variable"];
    var varValue = obj["value"];
    if( typeof varName == "string" && typeof varValue == "string" )
      {
      LookupDefinition( varName );
      var definition = schema[varName];
      if( typeof definition != "object" )
        {
        console.log( "Trying to get definition for " + varName + " but failed." );
        }
      }
    if( definition && definition["HAS-CHILDREN"] )
      {
      var childrenToMaybeDelete = GetListOfChildren( id );
      var l = childrenToMaybeDelete.length;
      if( l>0 )
        { /* there are child variables in the file */
        var listText = "";
        for( let i=0; i<l; i++ )
          {
          var obj = childrenToMaybeDelete[i];
          if( "" == listText )
            {
            listText = obj["varname"];
            }
          else
            {
            if( i==l-1 )
              {
              listText += " and ";
              }
            else
              {
              listText += ", ";
              }
            listText += obj["varname"];
            if( i==l-1 )
              {
              listText += ".";
              }
            }
          }

        var html = "";
        html += "<legend>Also delete sub-variables of (" + varName + "=" + varValue + ") ?</legend>\n"
              + "\n"
              + "<p>The following variables relate to the above:<br/>\n"
              + listText + "</p>\n"
              + "<form tabindex=0 class='dialog-form' id='action-form' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
              + "  <div><input id='firstItem' class='dialog-save' type='submit' id='delete' value='Delete children too'></div>\n"
              + "  <div><input class='dialog-save' type='submit' id='parent' value='Only delete parent'></div>\n"
              + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel - delete nothing'></div>\n"
              + "</form>"
              + "\n"
              ;
        var dialogDiv = document.getElementById( "dialog" );
        var contentDiv = document.getElementById( "content" );
        var deleteItem = document.getElementById( "delete" );
        if( typeof dialogDiv == "object"
            && typeof contentDiv == "object"
            && typeof deleteItem == "object" )
          {
          dialogDiv.innerHTML = html;
          dialogDiv.style.display = "inline-block";

          var firstItem = document.getElementById( "firstItem" );
          if( typeof firstItem != "unknown" )
            {
            firstItem.focus( { focusVisible: true } );
            }

          const form = document.getElementById( "action-form" );
          form.addEventListener(
            'submit', event =>
              {
              var deletedAnything = false;

              dialogDiv.innerHTML = "";
              contentDiv.inert = false;

              event.preventDefault();
              dialogDiv.style.display = "none";

              if( event.submitter.value=="Delete children too" )
                { /* delete the child lines */
                var l = childrenToMaybeDelete.length;
                for( let i=l-1; i>=0; i-- )
                  {
                  var obj = childrenToMaybeDelete[i];
                  if( typeof obj == "object" )
                    {
                    var lineId = obj["id"];
                    if( typeof lineId == "string" )
                      {
                      var args = { "FILENAME": iniFile, "LINENUMBER": Number( lineId ) };
                      var dontCare = CallAPIFunction( "config", "deleteline", args );
                      deletedAnything = true;
                      }
                    else
                      {
                      // console.log( "Wrong type: " + typeof lineID );
                      }
                    }
                  }
                }

              /* don't forget to delete the parent as well */
              if( event.submitter.value=="Delete children too"
                  || event.submitter.value=="Only delete parent" )
                {
                var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ) };
                var dontCare = CallAPIFunction( "config", "deleteline", args );
                deletedAnything = true;
                }

              if( deletedAnything )
                {
                LoadFile( id );
                }
              }
            );
          }
        }
      else
        { /* child variables in the schema, but not in the file.  just delete. */
        var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ) };
        var dontCare = CallAPIFunction( "config", "deleteline", args );
        LoadFile( id );
        }
      }
    else
      { /* no child variables; delete now. */
      var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ) };
      var dontCare = CallAPIFunction( "config", "deleteline", args );
      LoadFile( id );
      }
    }
  else
    { /* just delete the single line, synchronously */
    var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ) };
    var dontCare = CallAPIFunction( "config", "deleteline", args );
    LoadFile( id );
    }

  }

function MaybeSaveVariable( myEvent, id )
  {
  if( myEvent.code=="Enter" )
    {
    SaveVariable( id );
    }
  }

function MaybeInsertDeleteLine( myEvent, id )
  {
  if( myEvent.code=="Delete" || myEvent.code=="Backspace" || myEvent.code=="Minus" )
    {
    var inFocus = document.activeElement.id;
    var pattern = /^[0-9][0-9][0-9][0-9][0-9]$/;
    if( pattern.test( inFocus ) )
      { /* only delete if we've got the div tag selected */
      DeleteLine( id );
      }
    }
  else if( myEvent.code=="Insert" || myEvent.code=="Equal" ) // + on equal key
    {
    var inFocus = document.activeElement.id;
    var pattern = /^[0-9][0-9][0-9][0-9][0-9]$/;
    if( pattern.test( inFocus ) )
      { /* only delete if we've got the div tag selected */
      InsertLine( id );
      }
    }
  }

function MaybeCloseForm( myEvent, divID )
  {
  if( myEvent && myEvent.code && myEvent.code=="Escape" )
    {
    var dialogDiv = document.getElementById("dialog");
    if( dialogDiv )
      {
      dialogDiv.style.display = "none";
      dialogDiv.innerHTML = "";
      }
    var contentDiv = document.getElementById("content");
    if( typeof contentDiv != "undefined" )
      {
      contentDiv.inert = false;
      }
    }
  }

function ParentVariableOfLine( id )
  {
  if( typeof id == "undefined" )
    {
    DisplayError( "InsertIncludeLine() - no ID" );
    return "";
    }

  var n = Number( id );
  if( n < 1 )
    {
    DisplayError( "InsertIncludeLine() - invalid ID" );
    return "";
    }

  --n;

  if( ! iniArray[n] )
    {
    DisplayError( "Cannot find line " + n + " in in-memory array" );
    return "";
    }

  while( n>0 && iniArray[n].type != "VAR" )
    --n;

  if( n<0 )
    {
    console.log( "No preceding variable before line " + id + " (A)");
    return "";
    }

  if( n<1 || iniArray[n].type != "VAR" )
    {
    console.log( "No preceding variable before line " + id + " (B)");
    return "";
    }

  var parentVarName = iniArray[n].variable;
  if( typeof parentVarName != "string" )
    {
    DisplayError( "Cannot determine name of variable at position " + n );
    return "";
    }

  // ensure we have the definition for this variable.
  var nAsString = PadDigits( n, 5 );
  GetVarDefinition( nAsString, parentVarName );

  var definition = schema[ parentVarName ];
  if( typeof definition != "object" )
    {
    console.log( "No definition found for " + parentVarName );
    return "";
    }

  var hasChildren = definition["HAS-CHILDREN"];
  if( hasChildren != "true" )
    {
    var parent = definition["PARENT"];
    if( typeof parent == "string" )
      {
      return definition["PARENT"];
      }
    return "";
    }

  return iniArray[n].variable;
  }

function InsertIncludeLine( id )
  {
  if( typeof id == "undefined" )
    {
    DisplayError( "InsertIncludeLine() - no ID" );
    return;
    }

  var i = Number( id );
  if( i < 1 )
    {
    DisplayError( "InsertIncludeLine() - invalid ID" );
    return;
    }

  var args = { "FILENAME": iniFile, "LINENUMBER": i, "TEXT": "#include \"filename.ini\"" };
  var dontCare = CallAPIFunction( "config", "insertline", args );

  LoadFile( id );
  }

function InsertBlankLine( id )
  {
  if( typeof id == "undefined" )
    {
    DisplayError( "InsertBlankLine() - no ID" );
    return;
    }

  var i = Number( id );
  if( i < 1 )
    {
    DisplayError( "InsertBlankLine() - invalid ID" );
    return;
    }

  var args = { "FILENAME": iniFile, "LINENUMBER": i, "TEXT": "" };
  var dontCare = CallAPIFunction( "config", "insertline", args );

  LoadFile( id );
  }

function VariableAlreadyExists( varName )
  {
  var l = iniArray.length;
  for( let i=0; i<l; i++ )
    {
    let obj = iniArray[i];
    if( typeof obj == "object" )
      {
      if( obj["variable"] == varName )
        {
        return true;
        }
      }
    }
  return false;
  }

function InsertVariableLine( id, varName )
  {
  if( typeof id == "undefined" || "" == id )
    {
    DisplayError( "InsertVariableLine() - no ID" );
    return;
    }

  var lineNo = Number( id );
  if( lineNo < 1 )
    {
    DisplayError( "InsertVariableLine() - invalid ID" );
    return;
    }

  var newLine = varName + "=";
  var definition = {}

  LookupDefinition( varName );
  if( typeof schema[ varName ] == "object" )
    {
    definition = schema[ varName ];
    if( typeof( definition["DEFAULT"] ) == "number"
        || typeof( definition["DEFAULT"] ) == "string" )
      {
      newLine += definition["DEFAULT"];
      }

    if( definition["SINGLETON"] ) // implicitly true
      {
      if( VariableAlreadyExists( varName ) )
        {
        DisplayError( "You can only have one " + varName );
        return;
        }
      }
    }

  var args = { "FILENAME": iniFile, "LINENUMBER": lineNo, "TEXT": newLine };
  var dontCare = CallAPIFunction( "config", "insertline", args );

  if( typeof definition["NEXT"] == "string" )
    { // we should add the usual 'follows-next' variable after this one.
    ++ lineNo;
    var nextId = PadDigits( lineNo, 5 );
    InsertVariableLine( nextId, definition["NEXT"] );
    }

  LoadFile( id );
  }

function InsertFreeFormLine( id )
  {
  console.log( "InsertFreeFormLine()" );

  if( typeof id == "undefined" )
    {
    DisplayError( "InsertFreeFormLine() - no ID" );
    return;
    }

  var i = Number( id );
  if( i < 1 )
    {
    DisplayError( "InsertFreeFormLine() - invalid ID" );
    return;
    }

  clickType = "";
  var html = "";
  html += "<legend>Variable name:</legend>"
        + "\n"
        + "<form tabindex=0 class='dialog-form' id='varname' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
        + "  <div><input id='firstItem' type='text' value='' name='varname'></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='next' value='Next' onclick='clickType=\"Save\";'></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel' onclick='clickType=\"Cancel\";'></div>\n"
        + "</form>"
        + "\n"
        ;

  var dialogDiv = document.getElementById("dialog");
  if( typeof dialogDiv == "undefined" )
    {
    DisplayError( "Cannot find div tag for dialog" );
    return;
    }

  var contentDiv = document.getElementById("content");
  if( typeof content == "undefined" )
    {
    DisplayError( "Cannot find div tag for content" );
    return;
    }

  dialogDiv.innerHTML = html;
  dialogDiv.style.display = "inline-block";

  console.log( "InsertFreeFormLine() - set html on div" );
  var firstItem = document.getElementById( "firstItem" );
  if( typeof firstItem != "unknown" )
    {
    firstItem.focus( { focusVisible: true } );
    // console.log( "InsertFreeFormLine() - set focus" );
    }

  const form = document.getElementById( "varname" );
  console.log( "InsertFreeFormLine() - located varName (maybe)" );
  form.addEventListener(
    'submit', event =>
      {
      // console.log( "InsertFreeFormLine() - callback" );
      event.preventDefault();
      dialogDiv.style.display = "none";
      if( clickType=="Save" )
        {
        var varName = "";
        var inputObj = document.getElementById( "firstItem" );
        if( typeof inputObj != "undefined" && inputObj != null )
          {
          varName = inputObj.value;
          // console.log( "InsertFreeFormLine() - varName = " + varName );
          }
        }
      if( "" != varName )
        {
        InsertVariableLine( id, varName );
        }

      dialogDiv.innerHTML = "";
      contentDiv.inert = false;
      }
    );

  /*
  var args = { "FILENAME": iniFile, "LINENUMBER": i, "TEXT": "" };
  var dontCare = CallAPIFunction( "config", "insertline", args );
  */

  LoadFile( id );
  }

function InsertFixedLine( id, parentVarName )
  {
  if( typeof id == "undefined" )
    {
    DisplayError( "InsertFixedLine() - no ID" );
    return;
    }

  var i = Number( id );
  if( i < 1 )
    {
    DisplayError( "InsertFixedLine() - invalid ID" );
    return;
    }

  var args = {};
  if( parentVarName != "" )
    {
    args["PARENTTYPE"] = parentVarName;
    }
  var response = CallAPIFunction( "vartype", "list", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get a list of known variable types" );
    return;
    }

  var varTypes = response["vartypes"];
  if( typeof varTypes == "undefined" || varTypes == null )
    {
    DisplayError( "No vartypes in API response" );
    return;
    }

  clickType = "";
  var html = "";
  html += "<legend>Which variable would you like to add?</legend>"
        + "\n"
        + "<form tabindex=0 class='dialog-form' id='varname' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
        ;

  varTypes.sort();
  var isFirst = 1;
  for( var i=0; i<varTypes.length; ++i )
    {
    var item = varTypes[i];

    // remove singletons that we've already got in the config file.
    if( VariableAlreadyExists( item ) )
      {
      /* make sure we have the definition for this var type */
      LookupDefinition( item );
      var definition = schema[ item ];
      if( typeof definition != "object" )
        {
        console.log( item + " appears to be undefined" );
        }
      else
        {
        if( definition["SINGLETON"] )
          { /* don't add it to the set of options, as it's already defined
               and a singleton. */
          continue;
          }
        }
      }

    var firstBlob = "";
    if( isFirst )
      {
      firstBlob = "id='firstItem'";
      isFirst = 0;
      }
    html += "  <div><input " + firstBlob + " type='radio' id='" + item + "' value='" + item + "' name='vartype'><label for='" + item + "'>" + item + "</label></div>\n"
    }

  html += ""
        + "  <div><input class='dialog-save' type='submit' id='next' value='Next' onclick='clickType=\"Save\";'></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel' onclick='clickType=\"Cancel\";'></div>\n"
        + "</form>"
        + "\n"
        ;

  var dialogDiv = document.getElementById("dialog");
  if( typeof dialogDiv == "undefined" )
    {
    DisplayError( "Cannot find div tag for dialog" );
    return;
    }

  var contentDiv = document.getElementById("content");
  if( typeof content == "undefined" )
    {
    DisplayError( "Cannot find div tag for content" );
    return;
    }

  dialogDiv.innerHTML = html;
  dialogDiv.style.display = "inline-block";

  var firstItem = document.getElementById( "firstItem" );
  if( typeof firstItem != "unknown" )
    {
    firstItem.focus( { focusVisible: true } );
    }

  const form = document.getElementById( "varname" );
  form.addEventListener(
    'submit', event =>
      {
      event.preventDefault();
      dialogDiv.style.display = "none";
      if( clickType=="Save" )
        {
        var varTypeQuery = document.querySelector('input[name="vartype"]:checked');
        if( typeof varTypeQuery != "undefined" && varTypeQuery != null )
          {
          var varType = varTypeQuery.value;
          }
        if( typeof varType == "string" )
          {
          InsertVariableLine( id, varType );
          }
        }

      dialogDiv.innerHTML = "";
      contentDiv.inert = false;
      }
    );

  LoadFile( id );
  }

function InsertFixedLineChooseNewOrParent( id, parentVarName )
  {
  clickChoice = "";
  var html = "";
  html += "<legend>Start a new block or add a variable under " + parentVarName + "?</legend>"
        + "\n"
        + "<form tabindex=0 class='dialog-form' id='varname' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
        ;

  html += "  <div><input id='new-block' type='radio' value='New block' name='vartype'><label for='new-block'>Start a new block</label></div>\n"
  html += "  <div><input id='child-var' type='radio' value='Child of "+parentVarName+"' name='vartype'><label for='child-var'>Child of " + parentVarName + "</label></div>\n"

  html += ""
        + "  <div><input class='dialog-save' type='submit' id='next' value='Next' onclick='clickType=\"Continue\";'></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel' onclick='clickType=\"Cancel\";'></div>\n"
        + "</form>"
        + "\n"
        ;

  var dialogDiv = document.getElementById("dialog");
  if( typeof dialogDiv == "undefined" )
    {
    DisplayError( "Cannot find div tag for dialog" );
    return;
    }

  var contentDiv = document.getElementById("content");
  if( typeof content == "undefined" )
    {
    DisplayError( "Cannot find div tag for content" );
    return;
    }

  dialogDiv.innerHTML = html;
  dialogDiv.style.display = "inline-block";

  var firstItem = document.getElementById( "new-block" );
  if( typeof firstItem != "unknown" )
    {
    firstItem.focus( { focusVisible: true } );
    }

  const form = document.getElementById( "varname" );
  form.addEventListener(
    'submit', event =>
      {
      event.preventDefault();
      dialogDiv.style.display = "none";
      if( clickType=="Continue" )
        {
        var varTypeQuery = document.querySelector('input[name="vartype"]:checked');
        if( typeof varTypeQuery != "undefined" && varTypeQuery != null )
          {
          var varType = varTypeQuery.value;
          }
        if( typeof varType == "string" )
          {
          if( varType.includes( "Child of " ) )
            {
            dialogDiv.innerHTML = "";
            contentDiv.inert = false;
            InsertFixedLine( id, varType.replace( "Child of ", "" ) );
            }
          else
            {
            dialogDiv.innerHTML = "";
            contentDiv.inert = false;
            InsertFixedLine( id, "" );
            }
          }
        }
      else
        {
        dialogDiv.innerHTML = "";
        contentDiv.inert = false;
        }
      }
    );
  }

async function InsertCommentLine( id )
  {
  if( typeof id == "undefined" )
    {
    DisplayError( "InsertCommentLine() - no ID" );
    return;
    }

  var i = Number( id );
  if( i < 1 )
    {
    DisplayError( "InsertCommentLine() - invalid ID" );
    return;
    }

  var args = { "FILENAME": iniFile, "LINENUMBER": i, "TEXT": "# " };
  var dontCare = CallAPIFunction( "config", "insertline", args );

  LoadFile( "comment-" + id );
  }

var clickType = "";
function InsertLine( id )
  {
  var dialogDiv = document.getElementById("dialog");
  if( typeof dialogDiv == "undefined" )
    {
    DisplayError( "Cannot find div tag for dialog" );
    return;
    }

  var contentDiv = document.getElementById("content");
  if( typeof content == "undefined" )
    {
    DisplayError( "Cannot find div tag for content" );
    return;
    }

  contentDiv.inert = true;
  dialogDiv.style.display = "inline-block";

  clickType = "";
  var html = "";
  html += "<legend>Select a type of entry to add:</legend>"
        + "\n"
        + "<form tabindex=0 class='dialog-form' id='linetype' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
        + "  <div><input id='firstItem' type='radio' id='blank' value='blank' name='vartype'><label for='blank'>Blank line</label></div>\n"
        + "  <div><input type='radio' id='comment' value='comment' name='vartype'><label for='comment'>Comment line</label></div>\n"
        + "  <div><input type='radio' id='include' value='include' name='vartype'><label for='include'>Include another file</label></div>\n"
        + "  <div><input type='radio' id='freeform' value='freeform' name='vartype'><label for='freeform'>Variable for subsequent expansion</label></div>\n"
        + "  <div><input type='radio' id='fixed' value='fixed' name='vartype'><label for='fixed'>Keyword variable</label></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='next' value='Next' onclick='clickType=\"Save\";'></div>\n"
        + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel' onclick='clickType=\"Cancel\";'></div>\n"
        + "</form>"
        + "\n"
        ;

  dialogDiv.innerHTML = html;

  var firstItem = document.getElementById( "firstItem" );
  if( typeof firstItem != "unknown" )
    {
    firstItem.focus( { focusVisible: true } );
    }

  const lineType = document.getElementById( "linetype" );
  lineType.addEventListener(
    'submit', event =>
      {
      event.preventDefault();
      dialogDiv.style.display = "none";
      if( clickType=="Save" )
        {
        var varType = "none";
        var varTypeQuery = document.querySelector('input[name="vartype"]:checked');
        if( typeof varTypeQuery != "undefined" && varTypeQuery != null )
          {
          varType = varTypeQuery.value;
          }
        if( typeof varType != "undefined" )
          {
          if( varType == "include" )
            {
            InsertIncludeLine( id );
            dialogDiv.innerHTML = "";
            contentDiv.inert = false;
            }
          else if( varType == "comment" )
            {
            InsertCommentLine( id );
            dialogDiv.innerHTML = "";
            contentDiv.inert = false;
            }
          else if( varType == "blank" )
            {
            InsertBlankLine( id );
            dialogDiv.innerHTML = "";
            contentDiv.inert = false;
            }
          else if( varType == "freeform" )
            {
            InsertFreeFormLine( id );
            }
          else if( varType == "fixed" )
            {
            var parentVar = ParentVariableOfLine( id );
            if( parentVar == "" )
              {
              InsertFixedLine( id, "" );
              }
            else
              {
              InsertFixedLineChooseNewOrParent( id, parentVar );
              }
            }
          }
        }
      else
        {
        dialogDiv.innerHTML = "";
        contentDiv.inert = false;
        }
      }
    );
  }

function SaveComment( id )
  {
  if( typeof id == "undefined" || "" == id )
    {
    DisplayError( "Cannot save without specifying an ID" );
    return;
    }

  if( typeof iniFile == "undefined" || "" == iniFile )
    {
    DisplayError( "Cannot save to empty filename" );
    return;
    }

  var commentId = "comment-" + id;

  var commentField = document.getElementById( commentId );
  if( typeof commentField == "undefined" )
    {
    DisplayError( "Cannot locate ID comment-"+id );
    return;
    }

  var commentString = commentField.value;

  var n = Number( id );
  if( ! iniArray[n] )
    {
    DisplayError( "Cannot find line " + n + " in in-memory array" );
    return;
    }

  var obj = iniArray[n];
  if( obj.type != "COMMENT" )
    {
    DisplayError( "Line " + n + " is not a comment" );
    return;
    }

  if( obj.comment == commentString )
    { // no change.  that's not an error.
    return;
    }

  var pattern = /^[ ]*\#/;
  if( ! pattern.test( commentString ) )
    {
    DisplayError( "Comments must start with a '#' sign!" );
    return;
    }

  var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ), "TEXT": commentString };
  var dontCare = CallAPIFunction( "config", "replaceline", args );

  iniArray[n].comment = commentString;
  MarkSaved( commentField );
  }

function MaybeSaveComment( myEvent, id )
  {
  if( myEvent.code=="Enter" )
    {
    SaveComment( id );
    }
  }

function CommentLine( id, type, vals )
  {
  var c = vals["COMMENT"];
  if( typeof c == "undefined" )
    {
    c = "# API error";
    }

  return "<div class='focusable' onkeydown='MaybeInsertDeleteLine( event, \"" + id + "\")' tabindex=0 id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "<input type='text' id='comment-" + id + "' class='value' oninput='MarkEdited(this)' onkeydown='MaybeSaveComment( event, \"" + id + "\")' size=80 maxlength=80 value='" + c + "'/>"
//         + "<input type='button' class='save' value='Delete' onclick='DeleteLine(\"" + id + "\")' />"
//         + "<input type='button' class='save' value='Save' onclick='SaveComment(\"" + id + "\")' />"
         + "\n"
         + "</div>"
         + "\n";
  }

function BlankLine( id, type, vals )
  {
  return "<div class='focusable' onkeydown='MaybeInsertDeleteLine( event, \"" + id + "\")' tabindex=0 id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "<span class='blankline'>(blank)</span>"
//         + "<input type='button' class='save' value='Delete' onclick='DeleteLine(\"" + id + "\")' />"
         + "\n"
         + "</div>"
         + "\n";
  }

function EmptyLine( id, type, vals )
  {
  return "<div class='focusable' onkeydown='MaybeInsertDeleteLine( event, \"" + id + "\")' tabindex=0 id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "</div>"
         + "\n";
  }

function GetAllValuesOfVariableX( varName )
  {
  var list = [];
  var l = iniArray.length;
  for( let i=0; i<l; i++ )
    {
    let obj = iniArray[i];
    if( typeof obj != "object" )
      {
      continue;
      }
    if( typeof obj["variable"] != "string"
        || typeof obj["value"] != "string" )
      {
      continue;
      }
    if( obj["variable"] == varName )
      {
      list.push( obj["value"] );
      }
    }

  return list;
  }

function LookupDefinition( varname )
  {
  if( typeof schema[ varname ] == "object" )
    { // already got it.
    return;
    }

  var args = { "TYPE": varname };
  definition = CallAPIFunction( "vartype", "describe", args );

  if( typeof definition == "object"
      && typeof definition["code"] == "number"
      && definition["code"] == 0 )
    {
    schema[ varname ] = definition["DEFINITION"];
    }
  else
    console.log( "Failed to locate definition for " + varname  );
  }

function DisplayHelpText( helpText )
  {
  DisplayError( helpText );
  }

function SetHelpText( id, varname )
  {
  var n = Number( id );
  if( ! iniArray[n] )
    {
    return;
    }
  var obj = iniArray[n];
  if( obj.type != "VAR" )
    {
    return;
    }
  var definition = schema[varname];
  if( typeof definition != "object" )
    {
    console.log( "Trying to set help text on label for " + varname + " but it's not defined." );
    return;
    }
  var helpText = definition["HELP"];
  if( typeof helpText != "string" )
    {
    // console.log( "No help text available for " + varname );
    return;
    }

  var varNameId = "variable-" + id;
  var varNameField = document.getElementById( varNameId );
  if( typeof varNameField != "object" )
    {
    console.log( "Trying to set help text on " + varNameId + " but could not find DOM element." );
    return;
    }
  if( "" != varNameField.title )
    {
    // console.log( "Help text already set for var-" + varname );
    return;
    }
  varNameField.title = helpText;

  var equalsId = "equals-" + id;
  var equalsField = document.getElementById( equalsId );
  if( typeof equalsField != "object" )
    {
    console.log( "Trying to set help text on " + equalsId + " but could not find DOM element." );
    return;
    }
  if( "" != equalsField.title )
    {
    console.log( "Help text already set for equals-" + varname );
    return;
    }
  equalsField.title = helpText;
  equalsField.innerHTML = " <a class='help-link' onclick='DisplayHelpText(\"" + helpText + "\")'>=</a> ";
  }


/* This function also changes the HTML markup for a variable to match
 * its type ... hence the optional ID arg.  pass id=="" if you don't
 * want this to happen
 */
function GetVarDefinition( id, varname )
  {
  if( typeof varname != "string" || "" == varname )
    {
    console.log( "GetVarDefinition() - must specify a type" );
    return;
    }

  var gotDefinition = 0;
  var definition = {};
  if( typeof schema[ varname ] == "object" )
    {
    if( "" != id )
      {
      SetHelpText( id, varname );
      }
    gotDefinition = 1;

    definition = schema[varname];

    // if it's got an xref, then re-read it as the drop-down may have changed
    // otherwise, we've already got it, so we can quit now.
    if( typeof definition["XREF"] != "string" )
      {
      // console.log( "Already have definition for " + varname + " and it is not xref." );
      return;
      }
    }

  if( "" == id )
    { /* the rest of this stuff assumes we have a particular line number in mind: */
    return;
    }

  var n = Number( id );
  if( ! iniArray[n] )
    {
    DisplayError( "Cannot find line " + n + " in in-memory array" );
    return;
    }

  var obj = iniArray[n];
  if( obj.type != "VAR" )
    {
    DisplayError( "Line " + n + " is not a variable" );
    return;
    }

  // if we haven't got a definition for this var type, load it now.
  if( gotDefinition == 0 )
    {
    var args = { "TYPE": varname };
    definition = CallAPIFunction( "vartype", "describe", args );
    schema[ varname ] = definition["DEFINITION"];
    SetHelpText( id, varname );
    }

  var valId = "value-" + id;
  var valField = document.getElementById( valId );

  if( typeof valField != "object" )
    {
    console.log( "Cannot locate DOM object " + valId );
    return;
    }

  var divRow = document.getElementById( id );
  if( ! divRow )
    {
    console.log( "Cannot locate DIV DOM object " + id );
    return;
    }

  var definition = schema[varname];
  if( typeof definition != "object" ) // problem..  or not!
    {
    console.log( "Definition of " + varname + " not found" );
    return;
    }

  var type = definition["TYPE"];

  var xref = definition["XREF"];
  if( typeof xref != "string" )
    {
    xref = "";
    }

  if( type == "date" )
    {
    if( typeof valField == "undefined" )
      {
      DisplayError( "Cannot locate ID value-"+id );
      return;
      }

    valField.type = "date";
    }
  else if( type == "int" )
    {
    valField.type = "number";
    valField.min = definition["MINIMUM"];
    valField.max = definition["MAXIMUM"];
    valField.step = 1;
    }
  else if( type == "float" )
    {
    valField.type = "number";
    valField.min = definition["MINIMUM"];
    valField.max = definition["MAXIMUM"];
    valField.step = definition["STEP"];
    }
  else if( type == "bool" )
    {
    valField.type = "checkbox";
    if( valField.value == "true" )
      {
      valField.checked = true;
      }
    else
      {
      valField.checked = false;
      }
    }
  else if( type == "string" && "" != xref )
    {
    var xrefValues = GetAllValuesOfVariableX( xref );
    if( ! Array.isArray( xrefValues ) )
      {
      console.log( "GetAllValuesOfVariableX() did not return an array" );
      }
    else
      {
      var divContents = divRow.innerHTML;
      var divAlreadyIncludes = divContents.includes( "<select" );

      var html = "";
      html += "<select id='value-" + id + "' onfocus='GetVarDefinition(\"" + id + "\",\"" + obj.variable + "\")' class='select-value' oninput='MarkEdited(this)' onkeydown='MaybeSaveVariable( event, \"" + id + "\")'/>";
      var l = xrefValues.length;
      for( var i=0; i<l; ++i )
        {
        if( divAlreadyIncludes && ! divContents.includes( xrefValues[i] ) )
          {
          divAlreadyIncludes = false;
          }
        var selected = "";
        if( obj.value == xrefValues[i] )
          {
          selected = " selected";
          }
        html += "  <option value='" + xrefValues[i] + "'" + selected + ">" + xrefValues[i] + "</option>";
        }
      html += "</select>";

      if( divAlreadyIncludes )
        { // already got it
        // console.log( "HTML already includes our select" );
        }
      else
        { // the html does not already include what we want to push into it.
        var oldInput = "<input type=.text. id=.value-"+id+"[^/]*>";
        var oldSelect = "<select.*<\/select>";
        var oldInputRe = new RegExp( oldInput, "i" );
        var oldSelectRe = new RegExp( oldSelect, "i" );
        divContents = divContents.replace( oldInputRe, "" );
        divContents = divContents.replace( oldSelectRe, "" );
        divContents += html;
        divRow.innerHTML = divContents;
        delete oldInputRe;
        delete oldSelectRe;
        }
      }
    }
  else if( type == "xref" )
    {
    var leftType = definition["LEFT"];
    if( typeof leftType != "string" )
      {
      console.log( "GetAllValuesOfVariableX() - leftType not a string" );
      return;
      }
    var leftValues = GetAllValuesOfVariableX( leftType );
    if( ! Array.isArray( leftValues ) )
      {
      console.log( "GetAllValuesOfVariableX() did not return an array for left values" );
      return;
      }

    var rightType = definition["RIGHT"];
    if( typeof rightType != "string" )
      {
      console.log( "GetAllValuesOfVariableX() - rightType not a string" );
      return;
      }
    var rightValues = GetAllValuesOfVariableX( rightType );
    if( ! Array.isArray( rightValues ) )
      {
      console.log( "GetAllValuesOfVariableX() did not return an array for right values" );
      return;
      }

    var divContents = divRow.innerHTML;
    var divAlreadyIncludes = divContents.includes( "<select" );

    var leftHtml = "";
    leftHtml += "<select id='left-" + id + "' onfocus='GetVarDefinition(\"" + id + "\",\"" + obj.variable + "\")' class='select-value' oninput='MarkEdited(this)' onkeydown='MaybeSaveVariable( event, \"" + id + "\")'/>";
    var l = leftValues.length;
    for( var i=0; i<l; ++i )
      {
      if( divAlreadyIncludes && ! divContents.includes( leftValues[i] ) )
        {
        divAlreadyIncludes = false;
        }
      var selected = "";
      if( obj.value.includes( leftValues[i] ) )
        {
        selected = " selected";
        }
      leftHtml += "  <option value='" + leftValues[i] + "'" + selected + ">" + leftValues[i] + "</option>";
      }
    leftHtml += "</select>";

    var rightHtml = "";
    rightHtml += "<select id='right-" + id + "' onfocus='GetVarDefinition(\"" + id + "\",\"" + obj.variable + "\")' class='select-value' oninput='MarkEdited(this)' onkeydown='MaybeSaveVariable( event, \"" + id + "\")'/>";
    var l = rightValues.length;
    for( var i=0; i<l; ++i )
      {
      if( divAlreadyIncludes && ! divContents.includes( rightValues[i] ) )
        {
        divAlreadyIncludes = false;
        }
      var selected = "";
      if( obj.value.includes( rightValues[i] ) )
        {
        selected = " selected";
        }
      rightHtml += "  <option value='" + rightValues[i] + "'" + selected + ">" + rightValues[i] + "</option>";
      }
    rightHtml += "</select>";

    if( divAlreadyIncludes )
      { // already got it
      // console.log( "HTML already includes our select" );
      }
    else
      { // the html does not already include what we want to push into it.
      var oldInput = "<input type=.text. id=.value-"+id+"[^/]*>";
      var oldSelect = "<select.*<\/select>";
      var oldInputRe = new RegExp( oldInput, "i" );
      var oldSelectRe = new RegExp( oldSelect, "i" );
      divContents = divContents.replace( oldInputRe, "" );
      divContents = divContents.replace( oldSelectRe, "" );
      divContents += leftHtml + "&nbsp;&hArr;&nbsp;" + rightHtml;
      divRow.innerHTML = divContents;
      delete oldInputRe;
      delete oldSelectRe;
      }
    }
  else if( type == "dateval" )
    {
    if( typeof valField == "undefined" )
      {
      DisplayError( "Cannot locate ID value-"+id );
      return;
      }

    // extract old value, if any, so we can stuff it back into new values.
    var oldValue = valField.value;
    var datePart = "";
    var numPart = "";
    if( typeof oldValue == "string" && "" != oldValue )
      {
      var listOfValues = oldValue.split( " " );
      var l = listOfValues.length;
      if( l==2 )
        {
        datePart = listOfValues[0];
        numPart = listOfValues[1];
        }
      }

    valField.value = datePart;
    valField.type = "date";

    var secondValId = "second-value-" + id;
    var secondValField = document.getElementById( secondValId );
    if( ! secondValField )
      {
      var divField = document.getElementById( id );
      if( divField )
        {
        var oldContents = divField.innerHTML;
        var localMin = definition["MINIMUM"];
        var localMax = definition["MAXIMUM"];
        oldContents += "&nbsp; <input id=\"second-value-" + id + "\" type=\"number\" value=\"" + numPart + "\" onfocus=\"GetVarDefinition('" + id + "', '" + varname + "')\" class=\"value\" oninput=\"MarkEdited(this)\" onkeydown=\"MaybeSaveVariable( event, '" + id + "')\" size=\"10\" maxlength=\"10\"";
        if( typeof localMin == "number" )
          {
          oldContents += "min=\" + localMin + +\" ";
          }
        if( typeof localMax == "number" )
          {
          oldContents += "max=\" + localMax + +\" ";
          }
        oldContents += ">";
        oldContents = oldContents.replace( datePart + " " + numPart, datePart );
        divField.innerHTML = oldContents;
        }
      }
    else
      {
      secondValField.value = numPart;
      }
    }
  }

function VarLine( id, type, vals )
  {
  var variable = vals["VARIABLE"];
  if( typeof variable == "undefined" )
    {
    variable = "API_error_no_VARIABLE";
    }

  var value = vals["VALUE"];
  if( typeof value == "undefined" )
    {
    value = "API error - no VALUE";
    }

  return "<div class='focusable' onkeydown='MaybeInsertDeleteLine( event, \"" + id + "\")' tabindex=0 id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "<input type='text' readonly id='variable-" + id + "' onfocus='GetVarDefinition(\"" + id + "\",\"" + variable + "\")' class='varname' size=40 maxlength=60 value='" + variable + "'/>"
         + " <span id='equals-"+id+"'> = </span> "
         + "<input type='text' id='value-" + id + "' onfocus='GetVarDefinition(\"" + id + "\",\"" + variable + "\")' class='value' oninput='MarkEdited(this)' onkeydown='MaybeSaveVariable( event, \"" + id + "\")' size=40 maxlength=60 value='" + value + "'/>"
//         + "<input type='button' class='save' value='Delete' onclick='DeleteLine(\"" + id + "\")' />"
//         + "<input type='button' class='save' value='Save' onclick='SaveVariable(\"" + id + "\")' />"
         + "\n"
         + "</div>"
         + "\n";
  }

function SaveInclude( id )
  {
  if( typeof id == "undefined" || "" == id )
    {
    DisplayError( "Cannot save without specifying an ID" );
    return;
    }

  if( typeof iniFile == "undefined" || "" == iniFile )
    {
    DisplayError( "Cannot save to empty filename" );
    return;
    }

  var includeId = "include-" + id;

  var includeField = document.getElementById( includeId );
  if( typeof includeField == "undefined" )
    {
    DisplayError( "Cannot locate ID include-"+id );
    return;
    }

  var includeString = includeField.value;
  var patternA = /[0-9a-z_-]+/;
  var patternB = /[0-9a-z_-]+\.ini/;
  if( ! patternA.test( includeString ) && ! patternB.test( includeString ) )
    {
    DisplayError( "Invalid include filename - should be an identifier, optionally with .ini suffix" );
    return;
    }

  var n = Number( id );
  if( ! iniArray[n] )
    {
    DisplayError( "Cannot find line " + n + " in in-memory array" );
    return;
    }

  var obj = iniArray[n];
  if( obj.type != "INCLUDE" )
    {
    DisplayError( "Line " + n + " is not an include statement" );
    return;
    }

  if( obj.filename == includeString )
    { // no change.  that's not an error.
    return;
    }

  var lineString = "#include " + '"' + includeString + '"';

  var args = { "FILENAME": iniFile, "LINENUMBER": Number( id ), "TEXT": lineString };
  var dontCare = CallAPIFunction( "config", "replaceline", args );

  obj.filename = includeString;
  MarkSaved( includeField );
  }

function MaybeSaveInclude( myEvent, id )
  {
  if( myEvent.code=="Enter" )
    {
    SaveInclude( id );
    }
  }

function IncludeLine( id, type, vals )
  {
  var f = vals["FILENAME"];
  if( typeof f == "undefined" )
    {
    f = "api-error.ini";
    }
  f = f.toLowerCase();
  if( ! f.endsWith( ".ini" ) )
    {
    f = "filenames-must-end-with.ini";
    }

  return "<div class='focusable' tabindex=0 onkeydown='MaybeInsertDeleteLine( event, \"" + id + "\")' id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "#include \""
         + "<input id='include-" + id + "' oninput='MarkEdited(this)' onkeydown='MaybeSaveInclude( event, \"" + id + "\")' type='text' class='value' size=20 maxlength=40 value='" + f + "'/>"
         + "\""
//         + "<input type='button' class='save' value='Delete' onclick='DeleteLine(\"" + id + "\")' />"
//         + "<input type='button' class='save' value='Save' onclick='SaveInclude(\"" + id + "\")' />"
         + "<input type='button' class='save' value='Load' onclick='SetFileName(\"" + id + "\")' />"
         + "\n"
         + "</div>"
         + "\n";
  }

function UndefinedLine( id, type, vals )
  {
  return "<div class='focusable' tabindex=0 id='" + id + "'>"
         + "<span class='id'>" + id + ": " + "</span>"
         + "<input class='insert' type='button' onclick='InsertLine(\"" + id + "\")' value='+'/>"
         + "# Undefined type: "
         + type
         + "\n"
         + "</div>"
         + "\n";
  }

async function LoadFile( focusID )
  {
  var iniField = document.getElementById( "ini" );
  if( typeof iniField == "undefined" )
    {
    DisplayError( "Missing ini input field" );
    return;
    }
  var fileName = iniField.value;
  if( typeof iniField == "undefined" )
    {
    DisplayError( "Missing ini input value" );
    return;
    }

  iniFile = fileName.replace( /\.ini$/i, "" );

  var args = { "FILENAME": iniFile };
  contents = CallAPIFunction( "config", "readstr", args );
  if( typeof contents == "undefined" )
    {
    DisplayError( "No such file - " + iniFile );
    return;
    }

  var config = contents["CONFIG"];
  if( typeof config == "undefined" )
    {
    var result = "";
    if( typeof contents["result"] == "string" )
      {
      result = " - " + contents["result"];
      }
    DisplayError( "No CONFIG in API response" + result );
    return;
    }

  var l = Object.keys( config ).length;

  var body = "";
  var i = 0;

  // clear the previous file, if any.
  iniArray = [];
  for( i = 0; i < l; ++i )
    {
    let obj = config[i];
    var id = Object.keys( config )[i];
    var n = Number( id );

    iniArray[n] = new Object();
    iniArray[n].id = id;

    var vals = Object.values( config )[i];
    var type = vals["TYPE"];
    iniArray[n].type = type;
    var div = "";

    if( type == "COMMENT" )
      {
      div = CommentLine( id, type, vals );
      iniArray[n].comment = vals["COMMENT"];
      }
    else if( type == "BLANK" )
      {
      div = BlankLine( id, type, vals );
      }
    else if( type == "VAR" )
      {
      div = VarLine( id, type, vals );
      iniArray[n].variable = vals["VARIABLE"];
      iniArray[n].value = vals["VALUE"];
      }
    else if( type == "INCLUDE" )
      {
      div = IncludeLine( id, type, vals );
      iniArray[n].filename = vals["FILENAME"];
      }
    else
      {
      iniArray[n].type = "UNDEFINED";
      div = UndefinedLine( id, type, vals );
      }

    body += div;
    }

  var lastId = PadDigits( i+1, 5 );
  body += EmptyLine( lastId, "EMPTY", {} );

  var file = document.getElementById( "file" );
  if( typeof file == "undefined" )
    {
    DisplayError( "Missing file document element (div)" );
    return;
    }

  file.innerHTML = body;

  if( "" != focusID )
    {
    await Sleep( 100 ); // can't seem to find a way to avoid this hackery.
    var focusDiv = document.getElementById( focusID );

    if( ! ( typeof focusDiv == "undefined" || focusDiv == null ) )
      {
      focusDiv.focus( { focusVisible: true } );
      }
    }

  }

function CountOccurrences( haystack, needle )
  {
  var l = haystack.length;
  var s = needle.length;
  var n = 0;
  for( let i=0; i<l; ++i )
    {
    if( haystack.substr( i, s )==needle )
      {
      ++n;
      }
    }
  return n;
  }

function BigArrayToString()
  {
  var textBuffer = "";
  var l = iniArray.length;
  var maxWidth = 80;
  var nLinesText = 0;

  for( let i = 1; i<l; i++ )
    {
    var obj = iniArray[i];
    if( ! obj )
      continue;
    var line = "";
    if( obj.type=="VAR"
        && typeof obj["variable"] == "string"
        && typeof obj["value"] == "string" )
      {
      line = obj["variable"] + "=" + obj["value"];
      }
    else if( obj.type=="INCLUDE"
             && typeof obj["filename"] == "string" )
      {
      line = "#include " + "\"" + obj["filename"] + "\"";
      }
    else if( obj.type=="COMMENT"
             && typeof obj["comment"] == "string" )
      {
      line = obj["comment"];
      }
    else if( obj.type=="BLANK" )
      {
      }
    else
      {
      line = "# undefined type - " + i + " - " + JSON.stringify( obj );
      }

    if( line.length > maxWidth )
      {
      maxWidth = line.length;
      }

    textBuffer += line;
    if( i < l-1 )
      {
      textBuffer += "\n";
      }

    ++ nLinesText;
    }

  return { "text": textBuffer, "rows": nLinesText, "cols": maxWidth };
  }

function EditAsText()
  {
  if( textMode )
    {
    DisplayError( "Already in text mode" );
    return;
    }

  var divEdit = document.getElementById( "div-edit" );
  if( divEdit )
    {
    divEdit.style.display = "none";
    }

  var textDiv = document.getElementById( "text" );
  if( ! textDiv )
    {
    DisplayError( "Missing TEXT div in page - aborting" );
    return;
    }

  var fileDiv = document.getElementById( "file" );
  if( ! fileDiv )
    {
    DisplayError( "Missing FILE div in page - aborting" );
    return;
    }

  fileDiv.style.display = "none";

  var fileText = BigArrayToString();

  var html = "";
  html += "<textarea class='textarea' id='edit-buffer'>\n";
  html += fileText["text"];
  html += "</textarea>\n";

  textDiv.innerHTML = html;
  textDiv.style.display = "block";

  var editDiv = document.getElementById( "div-edit" );
  if( editDiv )
    {
    editDiv.style.display = "none";
    }
  var saveDiv = document.getElementById( "div-save" );
  if( saveDiv )
    {
    saveDiv.style.display = "inline";
    }
  var cancelDiv = document.getElementById( "div-cancel" );
  if( cancelDiv )
    {
    cancelDiv.style.display = "inline";
    }
  var editBuf = document.getElementById( "edit-buffer" );
  if( editBuf )
    {
    editBuf.focus( { focusVisible: true } );
    }

  textMode = true;
  }

function ValidateConfig()
  {
  var args = { "FILENAME": iniFile };
  var response = CallAPIFunction( "config", "validate", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get proper API response when asking to validate configuration file." );
    return;
    }

  if( typeof response["code"] == "number"
      && response["code"] == 0 )
    {
    DisplayError( "Valid configuration file." );
    return;
    }

  var errmsg = response["RESULT"];
  if( typeof errmsg == "string" )
    {
    DisplayError( errmsg );
    return;
    }

  DisplayError( "Something strange is going on" );
  }

function FetchOutputFileFromCommand( cmd, serverFileName, localFileName )
  {
  var form = document.createElement("form");

  form.method = "POST";
  form.action = baseURL + "command/fetchoutput";

  var domCmd = document.createElement("input");
  domCmd.type = "hidden";
  domCmd.name = "COMMAND";
  domCmd.value = cmd;
  form.appendChild( domCmd );

  var domSN = document.createElement("input");
  domSN.type = "hidden";
  domSN.name = "FILENAME";
  domSN.value = localFileName;
  form.appendChild( domSN );

  var domCN = document.createElement("input");
  domCN.type = "hidden";
  domCN.name = "DOWNLOAD";
  domCN.value = serverFileName;
  form.appendChild( domCN );

  form.target = "_blank";

  document.body.appendChild( form );
  form.submit();

  form.remove();
  }

function RunCommand( cmdLabel )
  {
  DisplayError( "Running command - " + cmdLabel );

  var args = { "command": cmdLabel, "inifile": iniFile };
  var response = CallAPIFunction( "command", "run", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get proper API response when asking to validate configuration file." );
    return;
    }

  var files = response["files"];
  if( typeof files == "object" )
    {
    for( const property in files )
      {
      var fileType = property;
      var fileName = files[ fileType ];
      FetchOutputFileFromCommand( cmdLabel, fileType, fileName );
      }
    }

  DisplayError( "Ran command - result = " + response["code"] );
  }

function ReallyDeleteConfig()
  {
  var args = { "FILENAME": iniFile };
  var response = CallAPIFunction( "config", "deletefile", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get proper API response when asking to validate configuration file." );
    return;
    }

  if( typeof response["code"] == "number"
      && response["code"] == 0 )
    {
    DisplayError( "File deleted." );
    }

  /* QQQ does this cause a file to be created? */
  LoadFile( iniFile );
  }

function DeleteConfig()
  {
  var len = iniArray.length;

  if( len==0 )
    {
    DisplayError( "Deleting an empty file is considered harmless." );
    ReallyDeleteConfig();
    }
  else
    {
    var html = "";
    html += "<legend>Really delete " + iniFile + " - which contains " + len + " lines of text?</legend>\n"
          + "\n"
          + "<form tabindex=0 class='dialog-form' id='action-form' onkeydown='MaybeCloseForm( event, \"dialog\")'>"
          + "  <div><input id='firstItem' class='dialog-save' type='submit' id='delete' value='Yes, delete it'></div>\n"
          + "  <div><input class='dialog-save' type='submit' id='cancel' value='Cancel'></div>\n"
          + "</form>"
          + "\n"
          ;
    var dialogDiv = document.getElementById( "dialog" );
    var contentDiv = document.getElementById( "content" );
    var deleteItem = document.getElementById( "delete" );
    if( typeof dialogDiv == "object"
        && typeof contentDiv == "object"
        && typeof deleteItem == "object" )
      {
      dialogDiv.innerHTML = html;
      dialogDiv.style.display = "inline-block";

      var firstItem = document.getElementById( "firstItem" );
      if( typeof firstItem != "unknown" )
        {
        firstItem.focus( { focusVisible: true } );
        }

      const form = document.getElementById( "action-form" );
      form.addEventListener(
        'submit', event =>
          {
          var deletedAnything = false;

          dialogDiv.innerHTML = "";
          contentDiv.inert = false;

          event.preventDefault();
          dialogDiv.style.display = "none";

          if( event.submitter.value=="Yes, delete it" )
            { /* QQQ delete the child lines */
            ReallyDeleteConfig();
            }
          }
        );
      }
    }

  /* API call will have already displayed an error message otherwise */
  }

function SaveTextBuffer()
  {
  var buffer = document.getElementById( "edit-buffer" );
  if( ! buffer )
    {
    DisplayError( "Missing edit-buffer page - aborting" );
    return;
    }
  var iniText = BigArrayToString();
  var text = buffer.value;

  if( text == iniText )
    {
    console.log( "No changes detected" );
    return;
    }

  var linesHTML = text.split(/\r\n|\r|\n/);
  var lHTML = linesHTML.length;

  var linesINI = iniText["text"].split(/\r\n|\r|\n/);
  var lINI = iniArray.length - 1;

  console.log( "lINI = " + lINI + ", lHTML = " + lHTML );

  var nChanges = 0;

  for( let i=0; i<lHTML && i<lINI; ++i )
    {
    var lineHTML = linesHTML[i];
    var lineINI = linesINI[i];

    if( lineHTML == lineINI )
      {
      DisplayError( "Nothing to save." );
      continue;
      }

    var args = { "FILENAME": iniFile, "LINENUMBER": i+1, "TEXT": lineHTML };
    var dontCare = CallAPIFunction( "config", "replaceline", args );
    ++nChanges;
    }

  if( lINI < lHTML )
    {
    for( let i=lINI; i<lHTML; ++i )
      { /* Add line from linesHTML to server */
      console.log( "Trying to insert line + " + (i+1) );
      var args = { "FILENAME": iniFile, "LINENUMBER": i+1, "TEXT": linesHTML[i] };
      var dontCare = CallAPIFunction( "config", "insertline", args );
      ++nChanges;
      }
    }
  else if( lINI > lHTML )
    {
    for( let i=lHTML; i<lINI; ++i )
      { /* Delete line from server */
      console.log( "Trying to delete line + " + (i+1) );
      var args = { "FILENAME": iniFile, "LINENUMBER": i+1 };
      var dontCare = CallAPIFunction( "config", "deleteline", args );
      ++nChanges;
      }
    }

  if( nChanges==0 )
    {
    DisplayError( "No changes - nothing to save." );
    }
  else
    {
    DisplayError( "Saved " + nChanges + " changed lines." );
    }

  // reload file on the other, 'structured' side of the display.
  LoadFile( "" );
  }

function CloseTextBuffer()
  {
  if( ! textMode )
    {
    DisplayError( "Not in text mode" );
    return;
    }

  var editDiv = document.getElementById( "div-edit" );
  if( editDiv )
    {
    editDiv.style.display = "inline";
    }
  var saveDiv = document.getElementById( "div-save" );
  if( saveDiv )
    {
    saveDiv.style.display = "none";
    }
  var cancelDiv = document.getElementById( "div-cancel" );
  if( cancelDiv )
    {
    cancelDiv.style.display = "none";
    }

  var textDiv = document.getElementById( "text" );
  if( ! textDiv )
    {
    DisplayError( "Missing TEXT div in page - aborting" );
    return;
    }

  var fileDiv = document.getElementById( "file" );
  if( ! fileDiv )
    {
    DisplayError( "Missing FILE div in page - aborting" );
    return;
    }

  fileDiv.style.display = "block";
  textDiv.style.display = "none";

  var divEdit = document.getElementById( "div-edit" );
  if( divEdit )
    {
    divEdit.style.display = "inline";
    }

  textMode = false;
  }

function ListOfCommands()
  {
  if( gotCommands )
    return commands;

  var args = {};
  var response = CallAPIFunction( "command", "list", args );
  if( typeof response == "undefined" )
    {
    DisplayError( "Can't get a list of available commands" );
    return {};
    }

  var cmdList = response["commands"];
  if( typeof cmdList != "object" || ! Array.isArray( cmdList ) )
    {
    var msg = "Got API response but no command list.";
    if( typeof response["result"] == "string" )
      {
      msg += "  " +  response["result"];
      }
    DisplayError( msg );
    return {};
    }

  commands = cmdList;
  gotCommands = 1;

  return commands;
  }

function LogoutURL()
  {
  var html = "";

  var args = { };
  var responseObj = CallAPIFunction( "logout", "get-url", args );
  if( typeof responseObj == "undefined"
      || typeof responseObj.code == "undefined"
      || responseObj.code!=0 )
    return "logout/get-url failed (A) - " + ProtectString(responseObj["result"]);

  if( typeof responseObj.url == "undefined" )
    return "logout/get-url failed (B) - " + ProtectString(responseObj["result"]);

  var url = responseObj.url;

  // html += "<div class='break'></div>\n";

  html += "<a class='logout' href='" + url + "'>Logout</a>\n";

  return html;
  }

function MyIdentity()
  {
  var html = "";

  var args = { };
  var responseObj = CallAPIFunction( "user", "whoami", args );
  var id = "Unauthorized";

  if( typeof responseObj == "undefined"
      || typeof responseObj.code == "undefined"
      || responseObj.code!=0 )
    DisplayError( "user/whoami failed (A) - " + ProtectString(responseObj["result"]) );
  else
    {
    if( typeof responseObj.id == "undefined" )
      {
      DisplayError( "user/whoami failed (B)" + ProtectString(responseObj["result"]) );
      }
    else
      {
      id = responseObj.id;
      }
    }

  html += "<div class='break'></div>\n";

  html += "<p class='userid'>You are logged in as " + id + ".  &nbsp;" + LogoutURL() + "</p>\n";

  return html;
  }


// main function to render the UI.
function DrawUI()
  {
  if( ! tabIsVisible )
    return;

  if( ! onLineNow )
    {
    console.log( "Cannot DrawUI() while offline" );
    return;
    }

  var htmlContainer = document.getElementById( "content" );
  if( typeof htmlContainer == "undefined" )
    {
    DisplayError( "Missing HTML container - content" );
    return;
    }

  var textDiv = document.getElementById( "text" );
  if( typeof textDiv == "undefined" )
    {
    DisplayError( "Missing HTML DIV tag for page content" );
    return;
    }

  var myID = MyIdentity();

  var page = ""
    + "<div class='header'>"
    + "<form id='loadform' onsubmit='LoadFile(\"00001\")'>"
    + "<label for='ini'>Filename:</label>"
    + "<input class='value' type='text' id='ini' name='ini' value='config.ini'/>"
    + "<input class='load' type='button' onclick='LoadFile(\"00001\")' value='Load'/>"
    + "</form>"
    + "</div>"
    + "<div id='div-validate' class='header-link'><a id='a-validate' class='header-link' onclick='ValidateConfig();'>Validate</a></div>"
    ;

  var cmdList = ListOfCommands();
  for( var i=0; i<cmdList.length; ++i )
    {
    var thisCmd = cmdList[i];
    page += "<div id='div-run' class='header-link'><a id='a-run-"
          + thisCmd
          + "' class='header-link' onclick='RunCommand(\""
          + thisCmd
          + "\");'>"
          + thisCmd
          + "</a></div>"
         ;
    }

  if( ! textMode )
    {
    page = page
      + "<div id='div-edit' class='header-link'><a id='a-edit' class='header-link' onclick='EditAsText();'>Edit as text</a></div>"
    }

  page = page
    + "<div id='div-save' class='header-link'><a id='a-save' class='header-link' onclick='SaveTextBuffer();'>Save</a></div>"
    + "<div id='div-cancel' class='header-link'><a id='a-cancel' class='header-link' onclick='CloseTextBuffer();'>Cancel/Close</a></div>"
    + "<div id='div-delete' class='header-link'><a id='a-delete' class='header-link' onclick='DeleteConfig();'>Delete file</a></div>"
    + "<div id='div-help' class='header-link'><a id='a-help' class='header-link' target='_blank' href='/confedit/schema.pdf'>Help?</a></div>"
    + "<div class='file-editor' id='file'></div>"
    + "<div class='text-editor' id='text'></div>"
    ;

  page += myID;

  htmlContainer.innerHTML = page;

  var saveDiv = document.getElementById( "div-save" );
  if( saveDiv )
    {
    saveDiv.style.display = "none";
    }
  var cancelDiv = document.getElementById( "div-cancel" );
  if( cancelDiv )
    {
    cancelDiv.style.display = "none";
    }
  var textDiv = document.getElementById( "text" );
  if( textDiv )
    {
    textDiv.style.display = "none";
    }

  const loadForm = document.getElementById( "loadform" );
  loadForm.addEventListener(
    'submit', event =>
      {
      event.preventDefault();
      // actual logic, e.g. validate the form
      // console.log('Form submission cancelled.');
      }
    );

  LoadFile( "" );
  }

function InitializeUI()
  {
  window.addEventListener('online', Online );
  window.addEventListener('offline', Offline );

  tabIsVisible = true;
  DrawUI();
  }

