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

struct IUnknown;
#include <Unknwnbase.h>

#include <unordered_map>

#include <locale> // tolower (...)
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
                      const SK_IVariable*  var    = nullptr,
                      const SK_ICommand*   cmd    = nullptr ) : word_   (word),
                                                                args_   (arguments),
                                                                result_ (result) {
    var_    = var;
    cmd_    = cmd;
    status_ = status;
  }

  const char*          getWord     (void) const noexcept { return word_.c_str   (); }
  const char*          getArgs     (void) const noexcept { return args_.c_str   (); }
  const char*          getResult   (void) const noexcept { return result_.c_str (); }

  const SK_IVariable*  getVariable (void) const noexcept { return var_;    }
  const SK_ICommand*   getCommand  (void) const noexcept { return cmd_;    }

  int                  getStatus   (void) const noexcept { return status_; }

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
  virtual  SK_ICommandResult execute (const char* szArgs) = 0;
  virtual ~SK_ICommand               (      void        ) noexcept { };

  virtual const char* getHelp            (void) noexcept { return "No Help Available"; }

  virtual int         getNumArgs         (void) noexcept { return 0; }
  virtual int         getNumOptionalArgs (void) noexcept { return 0; }
  virtual int         getNumRequiredArgs (void) noexcept {
    return getNumArgs () - getNumOptionalArgs ();
  }
};

interface SK_IVariable
{
  friend interface SK_IVariableListener;

  virtual ~SK_IVariable (void) noexcept { };

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
  virtual bool OnVarChange (SK_IVariable* var, void* val = nullptr) = 0;
};

template <typename T>
interface SK_IVarStub : public SK_IVariable
{
  friend interface SK_IVariableListener;
  friend interface SK_ICommandProcessor;

  SK_IVarStub (void) : type_     (Unknown),
                       var_      (nullptr),
                       listener_ (nullptr) { };

  SK_IVarStub ( T*                    var,
                SK_IVariableListener* pListener = nullptr );

  SK_IVariable::VariableType getType (void) const noexcept override
  {
    return type_;
  }

  virtual ~SK_IVarStub (void) noexcept
  {
    delete min_;
    delete max_;
  }

  void getValueString ( _Out_opt_ char*     szOut,
                          _Inout_ uint32_t* dwLen ) const override
  {
    SK_ReleaseAssert (dwLen != nullptr);
    if (              dwLen == nullptr) return;

    if (szOut != nullptr)
    {
      ZeroMemory ( szOut,                 *dwLen );
      if (         szOut != nullptr    && *dwLen >= 7)
        strncpy_s (szOut, 7, "(null)",    *dwLen);
    }

    *dwLen = std::min (
    *dwLen, static_cast <uint32_t> (strlen ("(null)") + 1)
                      );
  }

  const T& getValue (void) const   { return *var_; }
  const T& setValue (const T& val) {
    if (min_ != nullptr && val < *min_) return *var_;
    if (max_ != nullptr && val > *max_) return *var_;

    if (listener_ != NULL)
      listener_->OnVarChange (this, (void *)&val);
    else
      *var_ = val;

    return *var_;
  }

  // Public interface, the other one is not visible outside this DLL.
  void* getValuePointer (void) const override {
    return getValuePtr ();
  }

  /// NOTE: Avoid doing this, as much as possible...
  T* getValuePtr (void) const noexcept { return var_; }

  SK_IVarStub <T>& setMinValue (const T& val)
  {
    if (min_ == nullptr)
        min_  = new T;

    *min_ = val;

    return *this;
  }

  SK_IVarStub <T>& setMaxValue (const T& val)
  {
    if (max_ == nullptr)
        max_  = new T;

    *max_ = val;

    return *this;
  }

  SK_IVarStub <T>& setRange (const T& min, const T& max)
  {
    setMinValue (std::forward <const T> (min)).
    setMaxValue (std::forward <const T> (max));

    return *this;
  }

  void getRange (T** min, T** max) {
    if (min != nullptr) *min = min_;
    if (max != nullptr) *max = max_;
  }

  using _Tp =  T;

protected:
  typename SK_IVarStub::_Tp* var_      = nullptr;

  typename SK_IVarStub::_Tp* min_      = nullptr;
  typename SK_IVarStub::_Tp* max_      = nullptr;

private:
  SK_IVariableListener*      listener_ = nullptr;
};

#define SK_CaseAdjust(ch,lower) ((lower) ? ::tolower ((int)(ch)) : (ch))

// Hash function for UTF8 strings
template < class _Kty, class _Pr = std::less <> >
class str_hash_compare
{
public:
  using value_type = typename _Kty::value_type;
  using size_type  = typename _Kty::size_type;  /* Was originally size_t ... */

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

using SK_CommandRecord  = std::pair <std::string, std::unique_ptr <SK_ICommand>>;
using SK_VariableRecord = std::pair <std::string, std::unique_ptr <SK_IVariable>>;


interface SK_ICommandProcessor
{
  SK_ICommandProcessor (void);

  virtual ~SK_ICommandProcessor (void);

  virtual SK_ICommand*       FindCommand   (const char* szCommand);

  virtual const SK_ICommand* AddCommand    ( const char*  szCommand,
                                             SK_ICommand* pCommand );
  virtual bool               RemoveCommand ( const char* szCommand );


  virtual const SK_IVariable* FindVariable  (const char* szVariable);

  virtual const SK_IVariable* AddVariable    ( const char*   szVariable,
                                               SK_IVariable* pVariable  );
  virtual bool                RemoveVariable ( const char*   szVariable );


  virtual SK_ICommandResult ProcessCommandLine      (const char* szCommandLine);
  virtual SK_ICommandResult ProcessCommandFormatted (const char* szCommandFormat, ...);


protected:
private:
  std::unordered_map < std::string, std::unique_ptr <SK_ICommand>,
    str_hash_compare <std::string> > commands_;
  std::unordered_map < std::string, std::unique_ptr <SK_IVariable>,
    str_hash_compare <std::string> > variables_;

  std::unique_ptr <SK_Thread_HybridSpinlock> process_cmd_lock    = nullptr;
  std::unique_ptr <SK_Thread_HybridSpinlock> add_remove_var_lock = nullptr;
  std::unique_ptr <SK_Thread_HybridSpinlock> add_remove_cmd_lock = nullptr;
};


SK_ICommandProcessor*
__stdcall
SK_GetCommandProcessor (void) noexcept;

SK_IVariable*
__stdcall
SK_CreateVar ( SK_IVariable::VariableType type,
               void*                      var,
               SK_IVariableListener      *pListener = nullptr );

#endif /* __SK__COMMAND_H__ */