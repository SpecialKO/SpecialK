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

#ifdef _MSC_VER
# include <unordered_map>
//# include <hash_map>
//# define hash_map stdext::hash_map
#else
# include <hash_map.h>
#endif

#include <locale> // tolower (...)

template <typename T>
class SK_VarStub;// : public SK_Variable

class SK_Variable;
class SK_Command;

class SK_CommandResult
{
public:
  SK_CommandResult (       std::string   word,
                           std::string   arguments = "",
                           std::string   result    = "",
                           int           status = false,
                     const SK_Variable*  var    = NULL,
                     const SK_Command*   cmd    = NULL ) : word_   (word),
                                                           args_   (arguments),
                                                           result_ (result) {
    var_    = var;
    cmd_    = cmd;
    status_ = status;
  }

  std::string         getWord     (void) const { return word_;   }
  std::string         getArgs     (void) const { return args_;   }
  std::string         getResult   (void) const { return result_; }

  const SK_Variable*  getVariable (void) const { return var_;    }
  const SK_Command*   getCommand  (void) const { return cmd_;    }

  int                 getStatus   (void) const { return status_; }

protected:

private:
  const SK_Variable*  var_;
  const SK_Command*   cmd_;
        std::string   word_;
        std::string   args_;
        std::string   result_;
        int           status_;
};

class SK_Command {
public:
  virtual SK_CommandResult execute (const char* szArgs) = 0;

  virtual const char* getHelp            (void) { return "No Help Available"; }

  virtual int         getNumArgs         (void) { return 0; }
  virtual int         getNumOptionalArgs (void) { return 0; }
  virtual int         getNumRequiredArgs (void) {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

class SK_Variable
{
  friend class SK_iVariableListener;
public:
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

  virtual VariableType  getType        (void) const = 0;
  virtual std::string   getValueString (void) const = 0;

protected:
  VariableType type_;
};

class SK_iVariableListener
{
public:
  virtual bool OnVarChange (SK_Variable* var, void* val = NULL) = 0;
protected:
};

template <typename T>
class SK_VarStub : public SK_Variable
{
  friend class SK_iVariableListener;
public:
  SK_VarStub (void) : type_     (Unknown),
                      var_      (NULL),
                      listener_ (NULL)     { };

  SK_VarStub ( T*                     var,
                SK_iVariableListener* pListener = NULL );

  SK_Variable::VariableType getType (void) const
  {
    return type_;
  }

  virtual std::string getValueString (void) const { return "(null)"; }

  const T& getValue (void) const { return *var_; }
  void     setValue (T& val)     {
    if (listener_ != NULL)
      listener_->OnVarChange (this, &val);
    else
      *var_ = val;
  }

  /// NOTE: Avoid doing this, as much as possible...
  T* getValuePtr (void) { return var_; }

  typedef  T _Tp;

protected:
  typename SK_VarStub::_Tp* var_;

private:
  SK_iVariableListener*     listener_;
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

typedef std::pair <std::string, SK_Command*>  SK_CommandRecord;
typedef std::pair <std::string, SK_Variable*> SK_VariableRecord;


class SK_CommandProcessor
{
public:
  SK_CommandProcessor (void);

  virtual ~SK_CommandProcessor (void)
  {
  }

  SK_Command* FindCommand   (const char* szCommand) const;

  const SK_Command* AddCommand    ( const char*  szCommand,
                                     SK_Command* pCommand );
  bool               RemoveCommand ( const char* szCommand );


  const SK_Variable* FindVariable  (const char* szVariable) const;

  const SK_Variable* AddVariable    ( const char*   szVariable,
                                       SK_Variable* pVariable  );
  bool                RemoveVariable ( const char*   szVariable );


  SK_CommandResult ProcessCommandLine      (const char* szCommandLine);
  SK_CommandResult ProcessCommandFormatted (const char* szCommandFormat, ...);


protected:
private:
  std::unordered_map < std::string, SK_Command*,
    str_hash_compare <std::string> > commands_;
  std::unordered_map < std::string, SK_Variable*,
    str_hash_compare <std::string> > variables_;
};


__declspec (dllexport)
SK_CommandProcessor*
__stdcall
SK_GetCommandProcessor (void);

#endif /* __SK__COMMAND_H__ */