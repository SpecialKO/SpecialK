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

#ifndef __SK__COMMAND_H__
#define __SK__COMMAND_H__

#include <Unknwn.h>

# include <unordered_map>

#include <locale> // tolower (...)

#undef min
#undef max

#include <algorithm>

template <typename T>
interface SK_IVarStub;

interface SK_IVariable;
interface SK_ICommand;

interface SK_ICommandResult
{
  SK_ICommandResult (       const char*   word,
                            const char*   arguments = "",
                            const char*   result    = "",
                            int           status = false,
                      const SK_IVariable*  var    = NULL,
                      const SK_ICommand*   cmd    = NULL ) : word_   (word),
                                                             args_   (arguments),
                                                             result_ (result) {
    var_    = var;
    cmd_    = cmd;
    status_ = status;
  }

  const char*          getWord     (void) const { return word_.c_str   (); }
  const char*          getArgs     (void) const { return args_.c_str   (); }
  const char*          getResult   (void) const { return result_.c_str (); }

  const SK_IVariable*  getVariable (void) const { return var_;    }
  const SK_ICommand*   getCommand  (void) const { return cmd_;    }

  int                  getStatus   (void) const { return status_; }

protected:

private:
  const SK_IVariable* var_;
  const SK_ICommand*  cmd_;
        std::string   word_;
        std::string   args_;
        std::string   result_;
        int           status_;
};

interface SK_ICommand
{
  virtual SK_ICommandResult execute (const char* szArgs) = 0;

  virtual const char* getHelp            (void) { return "No Help Available"; }

  virtual int         getNumArgs         (void) { return 0; }
  virtual int         getNumOptionalArgs (void) { return 0; }
  virtual int         getNumRequiredArgs (void) {
    return getNumArgs () - getNumOptionalArgs ();
  }
};

interface SK_IVariable
{
  friend interface SK_IVariableListener;

  enum VariableType {
    Float,
    Double,
    Boolean,
    Byte,
    Short,
    Int,
    LongInt,
    String,

    NUM_VAR_TYPES_,

    Unknown
  } VariableTypes;

  virtual VariableType  getType         (void)                        const = 0;
  virtual void          getValueString  ( _Out_opt_     char* szOut,
                                          _Inout_   uint32_t* dwLen ) const = 0;
  virtual void*         getValuePointer (void)                        const = 0;

protected:
  VariableType type_;
};

interface SK_IVariableListener
{
  virtual bool OnVarChange (SK_IVariable* var, void* val = NULL) = 0;
};

template <typename T>
interface SK_IVarStub : public SK_IVariable
{
  friend interface SK_IVariableListener;
  friend interface SK_ICommandProcessor;

  SK_IVarStub (void) : type_     (Unknown),
                       var_      (NULL),
                       listener_ (NULL)     { };

  SK_IVarStub ( T*                    var,
                SK_IVariableListener* pListener = NULL );

  SK_IVariable::VariableType getType (void) const
  {
    return type_;
  }

  virtual void          getValueString  ( _Out_opt_ char*     szOut,
                                          _Inout_   uint32_t* dwLen ) const {
    if (szOut != nullptr)
      strncpy_s (szOut, 7, "(null)", *dwLen);

    *dwLen = std::min (*dwLen, (uint32_t)strlen ("(null)"));
  }

  const T& getValue (void) const { return *var_; }
  void     setValue (T& val)     {
    if (listener_ != NULL)
      listener_->OnVarChange (this, &val);
    else
      *var_ = val;
  }

  // Public interface, the other one is not visible outside this DLL.
  void* getValuePointer (void) const {
    return (void *)getValuePtr ();
  }

  /// NOTE: Avoid doing this, as much as possible...
  T* getValuePtr (void) const { return var_; }

  typedef  T _Tp;

protected:
  typename SK_IVarStub::_Tp* var_;

private:
  SK_IVariableListener*     listener_;
};

#define SK_CaseAdjust(ch,lower) ((lower) ? ::tolower ((int)(ch)) : (ch))

// Hash function for UTF8 strings
template < class _Kty, class _Pr = std::less <_Kty> >
class str_hash_compare
{
public:
  typedef typename _Kty::value_type value_type;
  typedef typename _Kty::size_type  size_type;  /* Was originally size_t ... */

  enum
  {
    bucket_size = 4,
    min_buckets = 8
  };

  str_hash_compare (void)      : comp ()      { };
  str_hash_compare (_Pr _Pred) : comp (_Pred) { };

  size_type operator() (const _Kty& _Keyval) const;
  bool      operator() (const _Kty& _Keyval1, const _Kty& _Keyval2) const;

  size_type hash_string (const _Kty& _Keyval) const;

private:
  _Pr comp;
};

typedef std::pair <std::string, SK_ICommand*>  SK_CommandRecord;
typedef std::pair <std::string, SK_IVariable*> SK_VariableRecord;


interface SK_ICommandProcessor
{
  SK_ICommandProcessor (void);

  virtual ~SK_ICommandProcessor (void)
  {
  }

  virtual SK_ICommand*       FindCommand   (const char* szCommand) const;

  virtual const SK_ICommand* AddCommand    ( const char*  szCommand,
                                             SK_ICommand* pCommand );
  virtual bool               RemoveCommand ( const char* szCommand );


  virtual const SK_IVariable* FindVariable  (const char* szVariable) const;

  virtual const SK_IVariable* AddVariable    ( const char*   szVariable,
                                               SK_IVariable* pVariable  );
  virtual bool                RemoveVariable ( const char*   szVariable );


  virtual SK_ICommandResult ProcessCommandLine      (const char* szCommandLine);
  virtual SK_ICommandResult ProcessCommandFormatted (const char* szCommandFormat, ...);


protected:
private:
  std::unordered_map < std::string, SK_ICommand*,
    str_hash_compare <std::string> > commands_;
  std::unordered_map < std::string, SK_IVariable*,
    str_hash_compare <std::string> > variables_;
};


SK_ICommandProcessor*
__stdcall
SK_GetCommandProcessor (void);

SK_IVariable*
__stdcall
SK_CreateVar ( SK_IVariable::VariableType type,
               void*                      var,
               SK_IVariableListener      *pListener = nullptr );

#endif /* __SK__COMMAND_H__ */