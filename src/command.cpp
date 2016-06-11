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

#include "command.h"

SK_CommandProcessor*
__stdcall
SK_GetCommandProcessor (void)
{
  static SK_CommandProcessor* command = nullptr;

  if (command == nullptr) {
    command = new SK_CommandProcessor ();
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

class SK_SourceCmd : public SK_Command
{
public:
  SK_SourceCmd (SK_CommandProcessor* cmd_proc) {
    processor_ = cmd_proc;
  }

  SK_CommandResult execute (const char* szArgs) {
    /* TODO: Replace with a special tokenizer / parser... */
    FILE* src = fopen (szArgs, "r");

    if (! src) {
      return
        SK_CommandResult ( "source", szArgs,
          "Could not open file!",
          false,
          NULL,
          this );
    }

    char line [1024];

    static int num_lines = 0;

    while (fgets (line, 1024, src) != NULL) {
      num_lines++;

      /* Remove the newline character... */
      line [strlen (line) - 1] = '\0';

      processor_->ProcessCommandLine (line);

      //printf (" Source Line %d - '%s'\n", num_lines++, line);
    }

    fclose (src);

    return SK_CommandResult ( "source", szArgs,
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
  SK_CommandProcessor* processor_;

};

SK_CommandProcessor::SK_CommandProcessor (void)
{
  SK_Command* src = new SK_SourceCmd (this);

  AddCommand ("source", src);
}

const SK_Command*
SK_CommandProcessor::AddCommand (const char* szCommand, SK_Command* pCommand)
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
SK_CommandProcessor::RemoveCommand (const char* szCommand)
{
  if (FindCommand (szCommand) != NULL) {
    std::unordered_map <std::string, SK_Command*, str_hash_compare <std::string> >::iterator
      command = commands_.find (szCommand);

    commands_.erase (command);
    return true;
  }

  return false;
}

SK_Command*
SK_CommandProcessor::FindCommand (const char* szCommand) const
{
  std::unordered_map <std::string, SK_Command*, str_hash_compare <std::string> >::const_iterator
    command = commands_.find (szCommand);

  if (command != commands_.end ())
    return (command)->second;

  return NULL;
}



const SK_Variable*
SK_CommandProcessor::AddVariable (const char* szVariable, SK_Variable* pVariable)
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
SK_CommandProcessor::RemoveVariable (const char* szVariable)
{
  if (FindVariable (szVariable) != NULL) {
    std::unordered_map <std::string, SK_Variable*, str_hash_compare <std::string> >::iterator
      variable = variables_.find (szVariable);

    variables_.erase (variable);
    return true;
  }

  return false;
}

const SK_Variable*
SK_CommandProcessor::FindVariable (const char* szVariable) const
{
  std::unordered_map <std::string, SK_Variable*, str_hash_compare <std::string> >::const_iterator
    variable = variables_.find (szVariable);

  if (variable != variables_.end ())
    return (variable)->second;

  return NULL;
}



SK_CommandResult
SK_CommandProcessor::ProcessCommandLine (const char* szCommandLine)
{
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

    SK_Command* cmd = FindCommand (cmd_word.c_str ());

    if (cmd != NULL) {
      return cmd->execute (command_args);
    }

    /* No command found, perhaps the word was a variable? */

    const SK_Variable* var = FindVariable (cmd_word.c_str ());

    if (var != NULL) {
      if (var->getType () == SK_Variable::Boolean)
      {
        if (command_args_len > 0) {
          SK_VarStub <bool>* bool_var = (SK_VarStub <bool>*) var;
          bool               bool_val = false;

          /* False */
          if (! (_stricmp (command_args, "false") && _stricmp (command_args, "0") &&
                 _stricmp (command_args, "off"))) {
            bool_val = false;
            bool_var->setValue (bool_val);
          }

          /* True */
          else if (! (_stricmp (command_args, "true") && _stricmp (command_args, "1") &&
                      _stricmp (command_args, "on"))) {
            bool_val = true;
            bool_var->setValue (bool_val);
          }

          /* Toggle */
          else if (! (_stricmp (command_args, "toggle") && _stricmp (command_args, "~") &&
                      _stricmp (command_args, "!"))) {
            bool_val = ! bool_var->getValue ();
            bool_var->setValue (bool_val);

            /* ^^^ TODO: Consider adding a toggle (...) function to
            the bool specialization of SK_VarStub... */
          } else {
            // Unknown Trailing Characters
          }
        }
      }

      else if (var->getType () == SK_Variable::Int)
      {
        if (command_args_len > 0) {
          int original_val = ((SK_VarStub <int>*) var)->getValue ();
          int int_val = 0;

          /* Increment */
          if (! (_stricmp (command_args, "++") && _stricmp (command_args, "inc") &&
                 _stricmp (command_args, "next"))) {
            int_val = original_val + 1;
          } else if (! (_stricmp (command_args, "--") && _stricmp (command_args, "dec") &&
                        _stricmp (command_args, "prev"))) {
            int_val = original_val - 1;
          } else
            int_val = atoi (command_args);

          ((SK_VarStub <int>*) var)->setValue (int_val);
        }
      }

      else if (var->getType () == SK_Variable::Short)
      {
        if (command_args_len > 0) {
          short original_val = ((SK_VarStub <short>*) var)->getValue ();
          short short_val    = 0;

          /* Increment */
          if (! (_stricmp (command_args, "++") && _stricmp (command_args, "inc") &&
                 _stricmp (command_args, "next"))) {
            short_val = original_val + 1;
          } else if (! (_stricmp (command_args, "--") && _stricmp (command_args, "dec") &&
                        _stricmp (command_args, "prev"))) {
            short_val = original_val - 1;
          } else
            short_val = (short)atoi (command_args);

          ((SK_VarStub <short>*) var)->setValue (short_val);
        }
      }

      else if (var->getType () == SK_Variable::Float)
      {
        if (command_args_len > 0) {
          //          float original_val = ((SK_VarStub <float>*) var)->getValue ();
          float float_val = (float)atof (command_args);

          ((SK_VarStub <float>*) var)->setValue (float_val);
        }
      }

      return SK_CommandResult (cmd_word, cmd_args, var->getValueString (), true, var, NULL);
    } else {
      /* Default args --> failure... */
      return SK_CommandResult (cmd_word, cmd_args); 
    }
  } else {
    /* Invalid Command Line (not long enough). */
    return SK_CommandResult (szCommandLine); /* Default args --> failure... */
  }
}

#include <cstdarg>

SK_CommandResult
SK_CommandProcessor::ProcessCommandFormatted (const char* szCommandFormat, ...)
{
  va_list ap;
  int     len;

  va_start         (ap, szCommandFormat);
  len = _vscprintf (szCommandFormat, ap);
  va_end           (ap);

  char* szFormattedCommandLine =
    (char *)malloc (sizeof (char) * (len + 1));

  *(szFormattedCommandLine + len) = '\0';

  va_start (ap, szCommandFormat);
  vsprintf (szFormattedCommandLine, szCommandFormat, ap);
  va_end   (ap);

  SK_CommandResult result =
    ProcessCommandLine (szFormattedCommandLine);

  free (szFormattedCommandLine);

  return result;
}

/** Variable Type Support **/


template <>
SK_VarStub <bool>::SK_VarStub ( bool*                 var,
                                SK_iVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Boolean;
}

template <>
std::string
SK_VarStub <bool>::getValueString (void) const
{
  if (getValue ())
    return std::string ("true");
  else
    return std::string ("false");
}

template <>
SK_VarStub <const char*>::SK_VarStub ( const char**          var,
                                       SK_iVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = String;
}

template <>
SK_VarStub <int>::SK_VarStub ( int*                  var,
                               SK_iVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Int;
}

template <>
std::string
SK_VarStub <int>::getValueString (void) const
{
  char szIntString [32];
  snprintf (szIntString, 32, "%d", getValue ());

  return std::string (szIntString);
}


template <>
SK_VarStub <short>::SK_VarStub ( short*                var,
                                 SK_iVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Short;
}

template <>
std::string
SK_VarStub <short>::getValueString (void) const
{
  char szShortString [32];
  snprintf (szShortString, 32, "%d", getValue ());

  return std::string (szShortString);
}


template <>
SK_VarStub <float>::SK_VarStub ( float*                var,
                                 SK_iVariableListener* pListener ) :
  var_ (var)
{
  listener_ = pListener;
  type_     = Float;
}

template <>
std::string
SK_VarStub <float>::getValueString (void) const
{
  char szFloatString [32];
  snprintf (szFloatString, 32, "%f", getValue ());

  // Remove trailing 0's after the .
  int len = strlen (szFloatString);
  for (int i = (len - 1); i > 1; i--) {
    if (szFloatString [i] == '0' && szFloatString [i - 1] != '.')
      len--;
    if (szFloatString [i] != '0' && szFloatString [i] != '\0')
      break;
  }

  szFloatString [len] = '\0';

  return std::string (szFloatString);
}