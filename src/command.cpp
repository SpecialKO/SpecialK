/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#define _CRT_SECURE_NO_WARNINGS

#include <memory>

#include <SpecialK/command.h>
#include <SpecialK/utility.h>


static CRITICAL_SECTION cs_process_cmd = { 0 };


SK_ICommandProcessor*
__stdcall
SK_GetCommandProcessor (void)
{
  static SK_ICommandProcessor* command = nullptr;

  if (command == nullptr) {
    command = new SK_ICommandProcessor ();
    InitializeCriticalSectionAndSpinCount (&cs_process_cmd, 104858);
  }

  return command;
}


template <>
str_hash_compare <std::string, std::less <std::string> >::size_type
str_hash_compare <std::string, std::less <std::string> >::hash_string (const std::string& _Keyval) const
{
  const bool case_insensitive = true;

  size_type   __h    = 0;
  const size_type   __len  = _Keyval.size ();
  const value_type* __data = _Keyval.data ();

  for (size_type __i = 0; __i < __len; ++__i) {
    /* Hash Collision Discovered: "r_window_res_x" vs. "r_window_pos_x" */
    //__h = 5 * __h + SK_CaseAdjust (__data [__i], case_insensitive);

    /* New Hash: sdbm   -  Collision Free (08/04/12) */
    __h = SK_CaseAdjust (__data [__i], case_insensitive) +
      (__h << 06)  +  (__h << 16)      -
      __h;
  }

  return __h;
}


template <>
str_hash_compare <std::string, std::less <std::string> >::size_type
str_hash_compare <std::string, std::less <std::string> >::operator() (const std::string& _Keyval) const
{
  return hash_string (_Keyval);
}

template <>
bool
str_hash_compare <std::string, std::less <std::string> >::operator() (const std::string& _lhs, const std::string& _rhs) const
{
  return hash_string (_lhs) < hash_string (_rhs);
}

class SK_SourceCmd : public SK_ICommand
{
public:
  SK_SourceCmd (SK_ICommandProcessor* cmd_proc) {
    processor_ = cmd_proc;
  }

  SK_ICommandResult execute (const char* szArgs) {
    /* TODO: Replace with a special tokenizer / parser... */
    FILE* src = fopen (szArgs, "r");

    if (! src) {
      return
        SK_ICommandResult ( "source", szArgs,
          "Could not open file!",
          false,
          NULL,
          this );
    }

          char line [1024] = { };
    static int num_lines   =  0;

    while (fgets (line, 1024, src) != nullptr)
    {
      num_lines++;

      /* Remove the newline character... */
      line [strlen (line) - 1] = '\0';

      processor_->ProcessCommandLine (line);

      //printf (" Source Line %d - '%s'\n", num_lines++, line);
    }

    fclose (src);

    return SK_ICommandResult ( "source", szArgs,
                                 "Success",
                                   num_lines,
                                     NULL,
                                       this );
  }

  int getNumArgs (void) {
    return 1;
  }

  int getNumOptionalArgs (void) {
    return 0;
  }

  const char* getHelp (void) {
    return "Load and execute a file containing multiple commands "
      "(such as a config file).";
  }

private:
  SK_ICommandProcessor* processor_;

};

SK_ICommandProcessor::SK_ICommandProcessor (void)
{
  SK_ICommand* src = new SK_SourceCmd (this);

  AddCommand ("source", src);
}

const SK_ICommand*
SK_ICommandProcessor::AddCommand (const char* szCommand, SK_ICommand* pCommand)
{
  if (szCommand == NULL || strlen (szCommand) < 1)
    return NULL;

  if (pCommand == NULL)
    return NULL;

  /* Command already exists, what should we do?! */
  if (FindCommand (szCommand) != NULL)
    return NULL;

  commands_.insert (SK_CommandRecord (szCommand, pCommand));

  return pCommand;
}

bool
SK_ICommandProcessor::RemoveCommand (const char* szCommand)
{
  if (FindCommand (szCommand) != NULL) {
    std::unordered_map < std::string,
                           SK_ICommand*,
                             str_hash_compare <std::string>
                       >::iterator
      command = commands_.find (szCommand);

    commands_.erase (command);
    return true;
  }

  return false;
}

SK_ICommand*
SK_ICommandProcessor::FindCommand (const char* szCommand) const
{
  std::unordered_map < std::string,
                         SK_ICommand*,
                           str_hash_compare <std::string>
                     >::const_iterator
    command = commands_.find (szCommand);

  if (command != commands_.end ())
    return (command)->second;

  return NULL;
}



const SK_IVariable*
SK_ICommandProcessor::AddVariable (const char* szVariable, SK_IVariable* pVariable)
{
  if (szVariable == NULL || strlen (szVariable) < 1)
    return NULL;

  if (pVariable == NULL)
    return NULL;

  /* Variable already exists, what should we do?! */
  if (FindVariable (szVariable) != NULL)
    return NULL;

  variables_.insert (SK_VariableRecord (szVariable, pVariable));

  return pVariable;
}

bool
SK_ICommandProcessor::RemoveVariable (const char* szVariable)
{
  if (FindVariable (szVariable) != NULL) {
    std::unordered_map < std::string,
                           SK_IVariable*,
                             str_hash_compare <std::string>
                       >::iterator
      variable = variables_.find (szVariable);

    variables_.erase (variable);
    return true;
  }

  return false;
}

const SK_IVariable*
SK_ICommandProcessor::FindVariable (const char* szVariable) const
{
  std::unordered_map < std::string,
                         SK_IVariable*,
                           str_hash_compare <std::string>
                     >::const_iterator
    variable = variables_.find (szVariable);

  if (variable != variables_.end ())
    return (variable)->second;

  return NULL;
}

SK_ICommandResult
SK_ICommandProcessor::ProcessCommandLine (const char* szCommandLine)
{
  SK_AutoCriticalSection auto_cs (&cs_process_cmd);

  if (szCommandLine != NULL && strlen (szCommandLine))
  {
    char*  command_word     = _strdup (szCommandLine);
    size_t command_word_len =  strlen (command_word);

    char*  command_args     = command_word;
    size_t command_args_len = 0;

    /* Terminate the command word on the first space... */
    for (size_t i = 0; i < command_word_len; i++) {
      if (command_word [i] == ' ') {
        command_word [i] = '\0';

        if (i < (command_word_len - 1)) {
          command_args     = &command_word [i + 1];
          command_args_len = strlen (command_args);

          /* Eliminate trailing spaces */
          for (unsigned int j = 0; j < command_args_len; j++) {
            if (command_word [i + j + 1] != ' ') {
              command_args = &command_word [i + j + 1];
              break;
            }
          }

          command_args_len = strlen (command_args);
        }

        break;
      }
    }

    std::string cmd_word (command_word);
    std::string cmd_args (command_args_len > 0 ? command_args : "");

    free (command_word);

    /* ^^^ cmd_args is what is passed back to the object that issued
    this command... If no arguments were passed, it MUST be
    an empty string. */

    SK_ICommand* cmd = FindCommand (cmd_word.c_str ());

    if (cmd != NULL) {
      return cmd->execute (cmd_args.c_str ());
    }

    /* No command found, perhaps the word was a variable? */

    const SK_IVariable* var = FindVariable (cmd_word.c_str ());

    if (var != NULL) {
      if (var->getType () == SK_IVariable::Boolean)
      {
        if (command_args_len > 0) {
          SK_IVarStub <bool>* bool_var = (SK_IVarStub <bool>*) var;
          bool                bool_val = false;

          /* False */
          if (! (_stricmp (cmd_args.c_str (), "false") && _stricmp (cmd_args.c_str (), "0") &&
                 _stricmp (cmd_args.c_str (), "off"))) {
            bool_val = false;
            bool_var->setValue (bool_val);
          }

          /* True */
          else if (! (_stricmp (cmd_args.c_str (), "true") && _stricmp (cmd_args.c_str (), "1") &&
                      _stricmp (cmd_args.c_str (), "on"))) {
            bool_val = true;
            bool_var->setValue (bool_val);
          }

          /* Toggle */
          else if (! (_stricmp (cmd_args.c_str (), "toggle") && _stricmp (cmd_args.c_str (), "~") &&
                      _stricmp (cmd_args.c_str (), "!"))) {
            bool_val = ! bool_var->getValue ();
            bool_var->setValue (bool_val);

            /* ^^^ TODO: Consider adding a toggle (...) function to
            the bool specialization of SK_IVarStub... */
          } else {
            // Unknown Trailing Characters
          }
        }
      }

      else if (var->getType () == SK_IVariable::Int)
      {
        if (command_args_len > 0) {
          const int original_val = ((SK_IVarStub <int>*) var)->getValue ();
                     int int_val = 0;

          /* Increment */
          if (! (_stricmp (cmd_args.c_str (), "++") && _stricmp (cmd_args.c_str (), "inc") &&
                 _stricmp (cmd_args.c_str (), "next"))) {
            int_val = original_val + 1;
          } else if (! (_stricmp (cmd_args.c_str (), "--") && _stricmp (cmd_args.c_str (), "dec") &&
                        _stricmp (cmd_args.c_str (), "prev"))) {
            int_val = original_val - 1;
          } else
            int_val = atoi (cmd_args.c_str ());

          ((SK_IVarStub <int>*) var)->setValue (int_val);
        }
      }

      else if (var->getType () == SK_IVariable::Short)
      {
        if (command_args_len > 0) {
          const short original_val = ((SK_IVarStub <short>*) var)->getValue ();
                   short short_val = 0;

          /* Increment */
          if (! (_stricmp (cmd_args.c_str (), "++") && _stricmp (cmd_args.c_str (), "inc") &&
                 _stricmp (cmd_args.c_str (), "next"))) {
            short_val = original_val + 1;
          } else if (! (_stricmp (cmd_args.c_str (), "--") && _stricmp (cmd_args.c_str (), "dec") &&
                        _stricmp (cmd_args.c_str (), "prev"))) {
            short_val = original_val - 1;
          } else
            short_val = (short)atoi (cmd_args.c_str ());

          ((SK_IVarStub <short>*) var)->setValue (short_val);
        }
      }

      else if (var->getType () == SK_IVariable::Float)
      {
        if (command_args_len > 0) {
          //          float original_val = ((SK_IVarStub <float>*) var)->getValue ();
          float float_val = (float)atof (cmd_args.c_str ());

          ((SK_IVarStub <float>*) var)->setValue (float_val);
        }
      }

      else if (var->getType () == SK_IVariable::String)
      {
        if (command_args_len > 0) {
          const char* args = cmd_args.c_str ();

          SK_IVarStub <char *>* var_stub =
            (SK_IVarStub <char *>*) var;

          if (var_stub->listener_ != NULL)
            var_stub->listener_->OnVarChange (var_stub, &args);
          else
            strcpy ((char *)*var_stub->var_, args);
        }
      }

      uint32_t len = 256;
      var->getValueString (nullptr, &len);

      std::unique_ptr <char> pszNew (new char [len + 1]);
                             pszNew.get () [len] = '\0';

      ++len;

      var->getValueString (pszNew.get (), &len);

      return SK_ICommandResult (cmd_word.c_str (), cmd_args.c_str (), pszNew.get (), true, var, NULL);
    }

    else {
      /* Default args --> failure... */
      return SK_ICommandResult (cmd_word.c_str (), cmd_args.c_str ());
    }
  } else {
    /* Invalid Command Line (not long enough). */
    return SK_ICommandResult (szCommandLine); /* Default args --> failure... */
  }
}

#include <cstdarg>

SK_ICommandResult
SK_ICommandProcessor::ProcessCommandFormatted (const char* szCommandFormat, ...)
{
  va_list ap;

  va_start             (ap, szCommandFormat);
  int len = _vscprintf (szCommandFormat, ap);
  va_end               (ap);

  char* szFormattedCommandLine =
    (char *)malloc (sizeof (char) * (len + 1));

  if (szFormattedCommandLine != nullptr) {
    *(szFormattedCommandLine + len) = '\0';

    va_start  (ap, szCommandFormat);
    vsnprintf (szFormattedCommandLine, len, szCommandFormat, ap);
    va_end    (ap);

    SK_ICommandResult result =
      ProcessCommandLine (szFormattedCommandLine);

    free (szFormattedCommandLine);

    return result;
  }

  return SK_ICommandResult ("OUT OF MEMORY");
}

/** Variable Type Support **/


template <>
SK_IVarStub <bool>::SK_IVarStub ( bool*                 var,
                                  SK_IVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Boolean;
}

template <>
void
SK_IVarStub <bool>::getValueString ( _Out_opt_     char* szOut,
                                     _Inout_   uint32_t* dwLen ) const
{
  uint32_t len = 0;

  if (getValue ())
  {
    len = (uint32_t)strlen ("true");

    if (szOut != nullptr)
      strncpy (szOut, "true", *dwLen);

    *dwLen = std::min (*dwLen, len);
  }

  else
  {
    len = (uint32_t)strlen ("false");

    if (szOut != nullptr)
      strncpy (szOut, "false", *dwLen);

    *dwLen = std::min (*dwLen, len);
  }
}

template <>
SK_IVarStub <char*>::SK_IVarStub ( char**                var,
                                   SK_IVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = String;
}

template <>
void
SK_IVarStub <char*>::getValueString ( _Out_opt_     char* szOut,
                                      _Inout_   uint32_t* dwLen ) const
{
  if (szOut != nullptr)
    strncpy (szOut, (char *)var_, *dwLen);

  *dwLen = std::min (*dwLen, (uint32_t)strlen ((char *)var_));
}

template <>
SK_IVarStub <int>::SK_IVarStub ( int*                  var,
                                 SK_IVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Int;
}

template <>
void
SK_IVarStub <int>::getValueString ( _Out_opt_ char*     szOut,
                                    _Inout_   uint32_t* dwLen ) const
{
  if (szOut != nullptr)
    *dwLen = snprintf (szOut, *dwLen, "%li", getValue ());
  else
    *dwLen = std::min (*dwLen, (uint32_t)_scprintf ("%li", getValue ()));
}


template <>
SK_IVarStub <short>::SK_IVarStub ( short*                var,
                                   SK_IVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Short;
}

template <>
void
SK_IVarStub <short>::getValueString ( _Out_opt_ char*     szOut,
                                      _Inout_   uint32_t* dwLen ) const
{
  if (szOut != nullptr)
    *dwLen = snprintf (szOut, *dwLen, "%i", getValue ());
  else
    *dwLen = std::min (*dwLen, (uint32_t)_scprintf ("%i", getValue ()));
}


template <>
SK_IVarStub <float>::SK_IVarStub ( float*                var,
                                   SK_IVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Float;
}

template <>
void
SK_IVarStub <float>::getValueString ( _Out_opt_ char*     szOut,
                                      _Inout_   uint32_t* dwLen ) const
{
  if (szOut != nullptr) {
    *dwLen = snprintf (szOut, *dwLen, "%f", getValue ());

    // Remove trailing 0's after the .
    *dwLen = (uint32_t)SK_RemoveTrailingDecimalZeros (szOut, *dwLen);
  } else {
    *dwLen = std::min (*dwLen, (uint32_t)_scprintf ("%f", getValue ()));
  }
}

SK_IVariable*
__stdcall
SK_CreateVar ( SK_IVariable::VariableType type,
               void*                      var,
               SK_IVariableListener      *pListener )
{
  switch (type) {
    case SK_IVariable::Float:
      return new SK_IVarStub <float> ((float *)var, pListener);
    case SK_IVariable::Double:
      return nullptr;
    case SK_IVariable::Boolean:
      return new SK_IVarStub <bool> ((bool *)var, pListener);
    case SK_IVariable::Byte:
      return nullptr;
    case SK_IVariable::Short:
      return new SK_IVarStub <short> ((short *)var, pListener);
    case SK_IVariable::Int:
      return new SK_IVarStub <int> ((int *)var, pListener);
    case SK_IVariable::LongInt:
      return nullptr;
    case SK_IVariable::String:
      return new SK_IVarStub <char *> ((char **)var, pListener);
  }

  return nullptr;
}
