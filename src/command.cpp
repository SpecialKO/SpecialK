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

#include <SpecialK/stdafx.h>

SK_ICommandProcessor*
__stdcall
SK_GetCommandProcessor (void) noexcept
{
  static SK_ICommandProcessor command;
  return                     &command;
}


template <>
str_hash_compare <std::string, std::less <> >::size_type
str_hash_compare <std::string, std::less <> >::hash_string (const std::string& _Keyval) const
{
  constexpr bool case_insensitive = true;

        size_type   __h    = 0;
  const size_type   __len  = _Keyval.size ();
  const value_type* __data = _Keyval.data ();

  if (__data == nullptr)
    return __h;

  for (size_type __i = 0; __i < __len; ++__i)
  {
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
str_hash_compare <std::string, std::less <> >::size_type
str_hash_compare <std::string, std::less <> >::operator() (const std::string& _Keyval) const
{
  return hash_string (_Keyval);
}

template <>
bool
str_hash_compare <std::string, std::less <> >::operator() (const std::string& _lhs, const std::string& _rhs) const
{
  return hash_string (_lhs) < hash_string (_rhs);
}

class SK_SourceCmd : public SK_ICommand
{
public:
  explicit SK_SourceCmd (SK_ICommandProcessor* cmd_proc)
  {
    processor_ = cmd_proc;
  }

  SK_ICommandResult execute (const char* szArgs) override
  {
    /* TODO: Replace with a special tokenizer / parser... */
    FILE* src =
      fopen (szArgs, "r");

    if (src == nullptr)
    {
      return
        SK_ICommandResult ( "source", szArgs,
                              "Could not open file!",
                                false, nullptr,
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
                                     nullptr,
                                       this );
  }

  int getNumArgs (void) noexcept override {
    return 1;
  }

  int getNumOptionalArgs (void) noexcept override {
    return 0;
  }

  const char* getHelp (void) noexcept override {
    return "Load and execute a file containing multiple commands "
      "(such as a config file).";
  }

private:
  SK_ICommandProcessor* processor_;
};

SK_ICommandProcessor::SK_ICommandProcessor (void)
{
  process_cmd_lock =
    std::make_unique <SK_Thread_HybridSpinlock> (1024);

  add_remove_cmd_lock =
    std::make_unique <SK_Thread_HybridSpinlock> (128);

  add_remove_var_lock =
    std::make_unique <SK_Thread_HybridSpinlock> (128);

  AddCommand ("source", new SK_SourceCmd (this));
}

SK_ICommandProcessor::~SK_ICommandProcessor (void)
{
}

const SK_ICommand*
SK_ICommandProcessor::AddCommand (const char* szCommand, SK_ICommand* pCommand)
{
  if (szCommand == nullptr || strlen (szCommand) < 1)
    return nullptr;

  if (pCommand == nullptr)
    return nullptr;

  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*add_remove_cmd_lock);

  /* Command already exists, what should we do?! */
  if (FindCommand (szCommand) != nullptr)
    return nullptr;

  commands_.insert (SK_CommandRecord (szCommand, std::unique_ptr <SK_ICommand> (pCommand)));

  return pCommand;
}

bool
SK_ICommandProcessor::RemoveCommand (const char* szCommand)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
      scope_lock (*add_remove_cmd_lock);

  if (FindCommand (szCommand) != nullptr)
  {
    const auto command =
      commands_.find (szCommand);

    commands_.erase (command);
    return true;
  }

  return false;
}

SK_ICommand*
SK_ICommandProcessor::FindCommand (const char* szCommand)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*add_remove_cmd_lock);

  const auto command =
    commands_.find (szCommand);

  if (command != commands_.cend ())
    return (command)->second.get ();

  return nullptr;
}



const SK_IVariable*
SK_ICommandProcessor::AddVariable (const char* szVariable, SK_IVariable* pVariable)
{
  if (pVariable == nullptr)
    return nullptr;

  if (szVariable == nullptr || strlen (szVariable) < 1)
    return nullptr;

  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*add_remove_var_lock);

  /* Variable already exists, what should we do?! */
  if (FindVariable (szVariable) != nullptr)
    return nullptr;

  variables_.emplace (
    SK_VariableRecord (szVariable, std::unique_ptr <SK_IVariable> (pVariable))
  );

  return pVariable;
}

bool
SK_ICommandProcessor::RemoveVariable (const char* szVariable)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*add_remove_var_lock);

  if (FindVariable (szVariable) != nullptr)
  {
    const auto variable =
      variables_.find (szVariable);

    variables_.erase (variable);
    return true;
  }

  return false;
}

const SK_IVariable*
SK_ICommandProcessor::FindVariable (const char* szVariable)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*add_remove_var_lock);

  const auto variable =
    variables_.find (szVariable);

  if (variable != variables_.cend ())
    return (variable)->second.get ();

  return nullptr;
}

SK_ICommandResult
SK_ICommandProcessor::ProcessCommandLine (const char* szCommandLine)
{
  ////std::scoped_lock <SK_Thread_HybridSpinlock>
  ////      scope_lock (*process_cmd_lock);

  if (szCommandLine != nullptr && *szCommandLine != '\0')
  {
    char* command_word  = _strdup     (szCommandLine);
    if (  command_word == nullptr) return
                    SK_ICommandResult (szCommandLine);
    size_t command_word_len =  strlen (command_word);

    char*  command_args     = command_word;
    size_t command_args_len = 0;

    /* Terminate the command word on the first space... */
    for (size_t i = 0; i < command_word_len; i++)
    {
      if (command_word [i] == ' ')
      {   command_word [i] = '\0';

        if (i < (command_word_len - 1))
        {
          command_args     = &command_word [i + 1];
          command_args_len = strlen (command_args);

          /* Eliminate trailing spaces */
          for (unsigned int j = 0; j < command_args_len; j++)
          {
            if (command_word [i + j + 1] != ' ')
            {
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

    if (cmd != nullptr)
    {
      return cmd->execute (cmd_args.c_str ());
    }

    /* No command found, perhaps the word was a variable? */

    const SK_IVariable* var = FindVariable (cmd_word.c_str ());

    if (var != nullptr)
    {
      if (var->getType () == SK_IVariable::Boolean)
      {
        if (command_args_len > 0)
        {
          auto* bool_var = (SK_IVarStub <bool>*) var;
          bool  bool_val = false;

          /* False */
          if (! (0 != _stricmp (cmd_args.c_str (), "false") &&
                 0 != _stricmp (cmd_args.c_str (),     "0") &&
                 0 != _stricmp (cmd_args.c_str (),   "off")) )
          {
            bool_val = false;
            bool_var->setValue (bool_val);
          }

          /* True */
          else if (! (0 != _stricmp (cmd_args.c_str (), "true") &&
                      0 != _stricmp (cmd_args.c_str (),    "1") &&
                      0 != _stricmp (cmd_args.c_str (),   "on")) )
          {
            bool_val = true;
            bool_var->setValue (bool_val);
          }

          /* Toggle */
          else if (! (0 != _stricmp (cmd_args.c_str (), "toggle") &&
                      0 != _stricmp (cmd_args.c_str (),      "~") &&
                      0 != _stricmp (cmd_args.c_str (),      "!")) )
          {
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
        if (command_args_len > 0)
        {
          SK_IVarStub <int>* pVar =
            (SK_IVarStub <int>*) var;

          const int original_val = pVar->getValue ();
                     int int_val = 0;

          // Wraparound
          if (! (0 != _stricmp (cmd_args.c_str (), "cycle") &&
                 0 != _stricmp (cmd_args.c_str (),   "+++")) )
          {
            int_val = original_val + 1;

            int             *min, *max;
            pVar->getRange (&min, &max);

            if (        max != nullptr && int_val > *max)
              int_val = min == nullptr ? INT_MIN  : *min;
          }
          /* Increment */
          else if (! (0 != _stricmp (cmd_args.c_str (),   "++") &&
                      0 != _stricmp (cmd_args.c_str (),  "inc") &&
                      0 != _stricmp (cmd_args.c_str (), "next")) )
          {
            int_val = original_val + 1;
          } else if (! (0 != _stricmp (cmd_args.c_str (),   "--") &&
                        0 != _stricmp (cmd_args.c_str (),  "dec") &&
                        0 != _stricmp (cmd_args.c_str (), "prev")) )
          {
            int_val = original_val - 1;
          }
          /* Negate */
          else if (0 == _stricmp (cmd_args.c_str (), "~"))
          {
            int_val = -original_val;
          }
          else if (     StrStrIA (cmd_args.c_str (), "+="))
          {
            int_val = original_val +
                      strtol (&cmd_args.c_str ()[2], nullptr, 0);
          }
          else if (     StrStrIA (cmd_args.c_str (), "-="))
          {
            int_val = original_val -
                      strtol (&cmd_args.c_str ()[2], nullptr, 0);
          }
          else
            int_val = strtol (cmd_args.c_str (), nullptr, 0);

          pVar->setValue (int_val);
        }
      }

      else if (var->getType () == SK_IVariable::Short)
      {
        if (command_args_len > 0)
        {
          SK_IVarStub <short>* pVar =
            (SK_IVarStub <short>*) var;

          const short original_val = pVar->getValue ();
                   short short_val = 0;

          // Wraparound
          if (! (0 != _stricmp (cmd_args.c_str (), "cycle") &&
                 0 != _stricmp (cmd_args.c_str (),   "+++")) )
          {
            short_val = original_val + 1;

            short           *min, *max;
            pVar->getRange (&min, &max);

            if (          max != nullptr && short_val > *max)
              short_val = min == nullptr ? SHORT_MIN  : *min;
          }
          /* Increment */
          else if (! (0 != _stricmp (cmd_args.c_str (),   "++") &&
                      0 != _stricmp (cmd_args.c_str (),  "inc") &&
                      0 != _stricmp (cmd_args.c_str (), "next")) )
          {
            short_val = original_val + 1;
          } else if (! (0 != _stricmp (cmd_args.c_str (),   "--") &&
                        0 != _stricmp (cmd_args.c_str (),  "dec") &&
                        0 != _stricmp (cmd_args.c_str (), "prev")) )
          {
            short_val = original_val - 1;
          }
          /* Negate */
          else if (0 == _stricmp (cmd_args.c_str (), "~"))
          {
            short_val = -original_val;
          }
          else if (     StrStrIA (cmd_args.c_str (), "-="))
          {
            short_val = original_val -
              (short)strtol (&cmd_args.c_str ()[2], nullptr, 0);
          }
          else if (     StrStrIA (cmd_args.c_str (), "+="))
          {
            short_val = original_val +
              (short)strtol (&cmd_args.c_str ()[2], nullptr, 0);
          }
          else
            short_val = (short)strtol (cmd_args.c_str (), nullptr, 0);

          pVar->setValue (short_val);
        }
      }

      else if (var->getType () == SK_IVariable::Float)
      {
        if (command_args_len > 0)
        {
          float float_val    = 0.0f;
          float original_val =
            ((SK_IVarStub <float>*) var)->getValue ();

          /* Increment */
          if (     0 == _stricmp (cmd_args.c_str (), "++"))
            float_val = original_val + 1.0f;
          /* Decrement */
          else if (0 == _stricmp (cmd_args.c_str (), "--"))
            float_val = original_val - 1.0f;
          /* Negate */
          else if (0 == _stricmp (cmd_args.c_str (), "~"))
            float_val = -original_val;
          else if (     StrStrIA (cmd_args.c_str (), "-="))
          {
            float_val = original_val -
              (float)strtof (&cmd_args.c_str ()[2], nullptr);
          }
          else if (     StrStrIA (cmd_args.c_str (), "+="))
          {
            float_val = original_val +
              (float)strtof (&cmd_args.c_str ()[2], nullptr);
          }
          /* Assign */
          else
            float_val = (float)strtof (cmd_args.c_str (), nullptr);

          ((SK_IVarStub <float>*) var)->setValue (float_val);
        }
      }

      else if (var->getType () == SK_IVariable::String)
      {
        if (command_args_len > 0)
        {
          const char* args = cmd_args.c_str ();

          auto* var_stub =
            (SK_IVarStub <char *>*) var;

          if (var_stub->listener_ != nullptr)
            var_stub->listener_->OnVarChange (var_stub, &args);
          else
            strcpy ((char *)*var_stub->var_, args);
        }
      }

      uint32_t                       len = 256UL;
      var->getValueString (nullptr, &len);

      auto* pszNew =
        SK_TLS_Bottom ()->scratch_memory->cmd.alloc (
          static_cast <size_t> (len) + 1UL, true
        );

      ++len;

      var->getValueString (pszNew, &len);

      return
        SK_ICommandResult (
          cmd_word.c_str (),
          cmd_args.c_str (),
            pszNew, 1,
               var, nullptr
        );
    }

    else
    {
      /* Default args --> failure... */
      return SK_ICommandResult (cmd_word.c_str (), cmd_args.c_str ());
    }
  }

  else
  {
    /* Invalid Command Line (not long enough). */
    return SK_ICommandResult (szCommandLine); /* Default args --> failure... */
  }
}


SK_ICommandResult
SK_ICommandProcessor::ProcessCommandFormatted (const char* szCommandFormat, ...)
{
  va_list ap;

  va_start             (ap, szCommandFormat);
  int len = _vscprintf (szCommandFormat, ap);
  va_end               (ap);

  auto* szFormattedCommandLine =
    SK_TLS_Bottom ()->scratch_memory->cmd.alloc (len + 1, true);

  if (szFormattedCommandLine != nullptr)
  {
    *(szFormattedCommandLine + len) = '\0';

    va_start  (ap, szCommandFormat);
    vsnprintf (szFormattedCommandLine, len + 1, szCommandFormat, ap);
    va_end    (ap);

    SK_ICommandResult result =
      ProcessCommandLine (szFormattedCommandLine);

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
  if (! dwLen)
    return;

  uint32_t len = 0;

  if (getValue ())
  {
    len =
      sk::narrow_cast <uint32_t> (strlen ("true"));

    if (szOut != nullptr)
      strncpy (szOut, "true", *dwLen);

    *dwLen = std::min (*dwLen, len);
  }

  else
  {
    len =
      sk::narrow_cast <uint32_t> (strlen ("false"));

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
  if (! dwLen)
    return;

  if (szOut != nullptr)
    strncpy (szOut, reinterpret_cast <char *> (var_), *dwLen);

  *dwLen = std::min (*dwLen, static_cast <uint32_t> (strlen (reinterpret_cast <char *> (var_))));
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
  if (! dwLen)
    return;

  if (szOut != nullptr)
    *dwLen = snprintf (szOut, *dwLen, "%i", getValue ());
  else
    *dwLen = std::min (*dwLen, static_cast <uint32_t> (_scprintf ("%i", getValue ())));
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
  if (! dwLen)
    return;

  if (szOut != nullptr)
    *dwLen = snprintf (szOut, *dwLen, "%i", getValue ());
  else
    *dwLen = std::min (*dwLen, static_cast <uint32_t> (_scprintf ("%i", getValue ())));
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
  if (! dwLen)
    return;

  if (szOut != nullptr)
  {
    *dwLen = snprintf (szOut, *dwLen, "%f", getValue ());

    // Remove trailing 0's after the .
    *dwLen = sk::narrow_cast <uint32_t> (SK_RemoveTrailingDecimalZeros (szOut, *dwLen));
  }

  else
  {
    *dwLen = std::min (*dwLen, static_cast <uint32_t> (_scprintf ("%f", getValue ())));
  }
}

SK_IVariable*
__stdcall
SK_CreateVar ( SK_IVariable::VariableType type,
               void*                      var,
               SK_IVariableListener      *pListener )
{
  switch (type)
  {
    case SK_IVariable::Float:
      return new SK_IVarStub <float>  (static_cast <float *> (var), pListener);
    case SK_IVariable::Double:
      return nullptr;
    case SK_IVariable::Boolean:
      return new SK_IVarStub <bool>   (static_cast <bool *>  (var), pListener);
    case SK_IVariable::Byte:
      return nullptr;
    case SK_IVariable::Short:
      return new SK_IVarStub <short>  (static_cast <short *> (var), pListener);
    case SK_IVariable::Int:
      return new SK_IVarStub <int>    (static_cast <int *>   (var), pListener);
    case SK_IVariable::LongInt:
      return nullptr;
    case SK_IVariable::String:
      return new SK_IVarStub <char *> (static_cast <char **> (var), pListener);
  }

  return nullptr;
}
