//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef smtk_view_ComponentPhraseModel_h
#define smtk_view_ComponentPhraseModel_h

#include "smtk/view/Configuration.h"
#include "smtk/view/PhraseModel.h"

#include <functional>
#include <map>

namespace smtk
{
namespace view
{
/**\brief Present phrases describing a set of acceptable components held by a single resource.
  *
  * This model maintains the list of acceptable components by
  * asking the resource for all matching components each time
  * an operation runs.
  *
  * The list is flat (i.e., no subphrase generator is provided by default).
  * The model provides access to a user-selected subset as an smtk::view::Selection.
  */
class SMTKCORE_EXPORT ComponentPhraseModel : public PhraseModel
{
public:
  using Observer = std::function<void(DescriptivePhrasePtr, PhraseModelEvent, int, int)>;
  using Operation = smtk::operation::Operation;

  smtkTypeMacro(smtk::view::ComponentPhraseModel);
  smtkSuperclassMacro(smtk::view::PhraseModel);
  smtkSharedPtrCreateMacro(smtk::view::PhraseModel);

  ComponentPhraseModel();
  ComponentPhraseModel(const Configuration*, Manager* mgr);
  virtual ~ComponentPhraseModel();

  /// Return the root phrase of the hierarchy.
  DescriptivePhrasePtr root() const override;

  /**\brief Set the specification for what components are allowed to be \a src.
    *
    * Note that entries in \a src are tuples of (unique name, filter);
    * the unique name specifies a resource type (and all its subclasses) deemed acceptable sources
    * while the filter is a string that can be passed to any acceptable resource to get a
    * set of components it owns that are acceptable.
    * In other words, both the resource and a subset of its components must be acceptable.
    */
  bool setComponentFilters(const std::multimap<std::string, std::string>& src);

  /**\brief Visit all of the filters that define what components are acceptable for display.
    *
    * The function \a fn is called with each entry in m_componentFilters, with a resource's
    * unique name as the first argument and the component filter as the second argument.
    * If \a fn returns a non-zero value, then the visitation terminates early and \a fn will
    * not be called again.
    * Otherwise, iteration continues.
    */
  void visitComponentFilters(std::function<int(const std::string&, const std::string&)> fn) const;

protected:
  /*
  virtual void handleSelectionEvent(const std::string& src, Selection::Ptr seln);
  virtual void handleResourceEvent(Resource::Ptr rsrc, smtk::resource::Event event);
  virtual int handleOperationEvent(Operation::Ptr op, Operator::EventType event, Operator::Result res);

  virtual void handleExpunged(Operation::Ptr op, Operation::Result res, ComponentItemPtr data);
  virtual void handleModified(Operation::Ptr op, Operation::Result res, ComponentItemPtr data);
  */
  void handleResourceEvent(const Resource& rsrc, smtk::resource::EventType event) override;
  void handleCreated(const smtk::resource::PersistentObjectSet& createdObjects) override;

  virtual void processResource(const Resource::Ptr& rsrc, bool adding);
  virtual void populateRoot();

  smtk::view::DescriptivePhrasePtr m_root;
  std::set<std::weak_ptr<smtk::resource::Resource>,
    std::owner_less<std::weak_ptr<smtk::resource::Resource> > >
    m_resources;
  std::multimap<std::string, std::string> m_componentFilters;
};
}
}

#endif
