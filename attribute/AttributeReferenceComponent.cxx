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


#include "attribute/AttributeReferenceComponent.h"
#include "attribute/AttributeReferenceComponentDefinition.h"
#include "attribute/Attribute.h"
#include <iostream>

using namespace slctk::attribute; 

//----------------------------------------------------------------------------
AttributeReferenceComponent::AttributeReferenceComponent()
{
}

//----------------------------------------------------------------------------
bool AttributeReferenceComponent::
setDefinition(slctk::ConstAttributeComponentDefinitionPtr adef)
{
  // Note that we do a dynamic cast here since we don't
  // know if the proper definition is being passed
  const AttributeReferenceComponentDefinition *def = 
    dynamic_cast<const AttributeReferenceComponentDefinition *>(adef.get());
  
  // Call the parent's set definition - similar to constructor calls
  // we call from base to derived
  if ((def == NULL) || (!Component::setDefinition(adef)))
    {
    return false;
    }
  int n = def->numberOfValues();
  if (n)
    {
    this->m_values.resize(n);
    }
  return true;
}

//----------------------------------------------------------------------------
AttributeReferenceComponent::~AttributeReferenceComponent()
{
}
//----------------------------------------------------------------------------
Component::Type AttributeReferenceComponent::type() const
{
  return ATTRIBUTE_REFERENCE;
}

//----------------------------------------------------------------------------
bool AttributeReferenceComponent::setValue(int element, slctk::AttributePtr att)
{
  const AttributeReferenceComponentDefinition *def = 
    static_cast<const AttributeReferenceComponentDefinition *>(this->definition().get());
  if (def->isValueValid(att))
    {
    this->m_values[element] = att;
    return true;
    }
  return false;
}
//----------------------------------------------------------------------------
const std::string &
AttributeReferenceComponent::valueAsString(int element, 
                                      const std::string &format) const
{
  // For the initial design we will use sprintf and force a limit of 300 char
  char dummy[300];
  sprintf(dummy, format.c_str(), this->m_values[element].lock()->id());
  this->m_tempString = dummy;
  return this->m_tempString;
}
//----------------------------------------------------------------------------
bool
AttributeReferenceComponent::appendValue(slctk::AttributePtr val)
{
  //First - are we allowed to change the number of values?
  const AttributeReferenceComponentDefinition *def =
    static_cast<const AttributeReferenceComponentDefinition *>(this->definition().get());
  int n = def->numberOfValues();
  if (n)
    {
    return false; // The number of values is fixed
    }
  
  if (def->isValueValid(val))
    {
    this->m_values.push_back(val);
    return true;
    }
  return false;
}
//----------------------------------------------------------------------------
bool
AttributeReferenceComponent::removeValue(int element)
{
  //First - are we allowed to change the number of values?
  const AttributeReferenceComponentDefinition *def =
    static_cast<const AttributeReferenceComponentDefinition *>(this->definition().get());
  int n = def->numberOfValues();
  if (n)
    {
    return false; // The number of values is fixed
    }
  this->m_values.erase(this->m_values.begin()+element);
  return true;
}
//----------------------------------------------------------------------------
void
AttributeReferenceComponent::reset()
{
  const AttributeReferenceComponentDefinition *def
    = static_cast<const AttributeReferenceComponentDefinition *>(this->definition().get());
  // Was the initial size 0?
  int i, n = def->numberOfValues();
  if (!n)
    {
    this->m_values.clear();
    return;
    }
  for (i = 0; i < n; i++)
    {
    this->unset(i);
    }
}
//----------------------------------------------------------------------------
