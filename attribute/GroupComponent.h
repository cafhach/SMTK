/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/
// .NAME GroupComponent.h -
// .SECTION Description
// .SECTION See Also

#ifndef __slctk_attribute_GroupComponent_h
#define __slctk_attribute_GroupComponent_h

#include "AttributeExports.h"
#include "attribute/Component.h"
#include <vector>
namespace slctk
{
  namespace attribute
  {
    class GroupComponentDefinition;
    class SLCTKATTRIBUTE_EXPORT GroupComponent : public Component
    {
    public:
      GroupComponent();
      virtual ~GroupComponent();
      virtual Component::Type type() const;
      virtual bool setDefinition(slctk::ConstAttributeComponentDefinitionPtr def);
      std::size_t numberOfComponentsPerGroup() const;
      std::size_t numberOfGroups() const
      {return this->m_components.size();}
      bool appendGroup();
      bool removeGroup(int element);

      slctk::AttributeComponentPtr component(int ith) const
      {return this->component(0, ith);}
      slctk::AttributeComponentPtr component(int element, int ith) const
        {return this->m_components[element][ith];}

      slctk::AttributeComponentPtr find(const std::string &name)
        {return this->find(0, name);}
      slctk::AttributeComponentPtr find(int element, const std::string &name) ;
      slctk::ConstAttributeComponentPtr find(const std::string &name) const
        {return this->find(0, name);}
      slctk::ConstAttributeComponentPtr find(int element, const std::string &name) const;

    protected:
      std::vector<std::vector<slctk::AttributeComponentPtr> >m_components;
  
    private:
    };
  };
};


#endif /* __GroupComponent_h */
