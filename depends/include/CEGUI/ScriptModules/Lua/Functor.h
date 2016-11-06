/***********************************************************************
    created:  Thu Jan 26 2006
    author:   Tomas Lindquist Olsen <tomas@famolsen.dk>

    purpose:  Defines interface for LuaFunctor class
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2008 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUILuaFunctor_h_
#define _CEGUILuaFunctor_h_

#include "CEGUI/EventSet.h"

struct lua_State;

// Start of CEGUI namespace section
namespace CEGUI
{

// forward declaration
class LuaScriptModule;

/*!
\brief
    Functor class used for subscribing Lua functions to CEGUI events
*/
class LuaFunctor
{
public:
    LuaFunctor(lua_State* state, int func, int selfIndex);
    LuaFunctor(lua_State* state, const String& func, int selfIndex);

    LuaFunctor(lua_State* state, const int func, const int selfIndex,
               const String& error_handler);
    LuaFunctor(lua_State* state, const String& func, const int selfIndex,
               const String& error_handler);
    LuaFunctor(lua_State* state, const int func, const int selfIndex,
               const int error_handler);
    LuaFunctor(lua_State* state, const String& func, const int selfIndex,
               const int error_handler);

    LuaFunctor(const LuaFunctor& cp);
    ~LuaFunctor();

    bool operator()(const EventArgs& args) const;

    /*!
    \brief
        function used to subscribe any Lua function as event handler.
        References using the Lua registry.
        To be called from Lua only.
    */
    static Event::Connection SubscribeEvent(EventSet* self,
                                            const String& eventName,
                                            const int funcIndex,
                                            const int selfIndex,
                                            const int error_handler,
                                            lua_State* L);

    /*!
    \brief
        Pushes the Lua function named \param name on top of the Lua stack.
        The name may contain '.' (dots) character for (nested) table values.
    */
    static void pushNamedFunction(lua_State* L, const String& name);

private:
    /*!
    \brief
        Invalidate the registry references.  This is used internally to ensure
        that the references do not get released (for example when we destroy
        a temporary object)
    */
    void invalidateLuaRefs();

    lua_State* L;
    mutable int index;
    int self;
    mutable bool needs_lookup;
    mutable String function_name;

    //! Error handler function to pass to lua_pcall.
    mutable String d_errFuncName;
    //! registry index of the function to pass to lua_pcall.
    mutable int d_errFuncIndex;
    //! signfies whether we made the reference index at d_errFuncIndex.
    mutable bool d_ourErrFuncIndex;

    friend class LuaScriptModule;
};

} // namespace CEGUI

#endif // end of guard _CEGUILuaFunctor_h_
