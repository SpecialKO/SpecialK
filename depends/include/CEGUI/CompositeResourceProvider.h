/*******************************************************************************
Created:  14.07.2011
Author:   Hans Mackowiak (Hanmac) hanmac@gmx.de
*******************************************************************************/
/***************************************************************************
 Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
***************************************************************************/
#ifndef _CEGUICompositeResourceProvider_h_
#define _CEGUICompositeResourceProvider_h_
#include "CEGUI/Base.h"
#include "CEGUI/IteratorBase.h"
#include "CEGUI/MemoryAllocation.h"
#include "CEGUI/ResourceProvider.h"
#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class CEGUIEXPORT CompositeResourceProvider : public ResourceProvider
{
public:
  /*************************************************************************
    Construction and Destruction
  *************************************************************************/
  CompositeResourceProvider();
  ~CompositeResourceProvider(void);
  /*!
      \brief
          adds a ResourceProvider

      \param prov
          A pointer to an ResourceProvider (ownership is taken).

      \param name
          A String thats names the provider inside this holder.

      \return
          Nothing.
      */
  void add(ResourceProvider *prov,const String& name);
  /*!
      \brief
          creates and add a ResourceProvider

      \param name
          A String thats names the provider inside this holder.

      \return
          Nothing.
      */
  template<typename T>void add(const String& name)
  {
    add(CEGUI_NEW_AO T,name);
  }
  /*!
      \brief
          removes a ResourceProvider

      \param prov
          A pointer to an ResourceProvider.

      \return
          Nothing.
      */
  void remove(ResourceProvider *prov);
  /*!
      \brief
          removes a ResourceProvider

      \param name
          A String thats names the provider inside this holder.

      \return
          Nothing.
      */
  void remove(const String& name);
  /*!
      \brief
         get a ResourceProvider

      \param name
          A String thats names the provider inside this holder.

      \return
          ResourceProvider Pointer.
      */      
  ResourceProvider* get(const String& name);

  void loadRawDataContainer(const String& filename,
        RawDataContainer& output,
        const String& resourceGroup);
  size_t getResourceGroupFileNames(std::vector<String>& out_vec,
         const String& file_pattern,
         const String& resource_group);
protected:
  typedef std::map<String, ResourceProvider*, StringFastLessCompare> Providermap;
  Providermap  d_providerlist;
public:
  typedef ConstMapIterator<Providermap> ProviderIterator;
  ProviderIterator  getIterator() const;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUICompositeResourceProvider_h_
