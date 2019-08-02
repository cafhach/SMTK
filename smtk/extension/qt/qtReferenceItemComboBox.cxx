//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/extension/qt/qtReferenceItemComboBox.h"
#include "smtk/extension/qt/qtActiveObjects.h"

#include "smtk/extension/qt/qtAttribute.h"
#include "smtk/extension/qt/qtBaseAttributeView.h"
#include "smtk/extension/qt/qtItem.h"
#include "smtk/extension/qt/qtSMTKUtilities.h"
#include "smtk/extension/qt/qtTableWidget.h"
#include "smtk/extension/qt/qtTypeDeclarations.h"
#include "smtk/extension/qt/qtUIManager.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/Definition.h"
#include "smtk/attribute/ReferenceItem.h"
#include "smtk/attribute/ReferenceItemDefinition.h"
#include "smtk/attribute/Resource.h"

#include "smtk/operation/Manager.h"
#include "smtk/operation/SpecificationOps.h"

#include "smtk/resource/Manager.h"

#include "smtk/view/Selection.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <sstream>

#define DEBUG_REFERENCEITEM 0

using namespace smtk::attribute;
using namespace smtk::extension;

class qtReferenceItemComboBoxInternals
{
public:
  QPointer<QComboBox> comboBox;
  QPointer<QGridLayout> EntryLayout;
  QPointer<QLabel> theLabel;
  smtk::resource::WeakManagerPtr resourceManager;
  smtk::operation::WeakManagerPtr operationManager;
};

qtItem* qtReferenceItemComboBox::createItemWidget(const qtAttributeItemInfo& info)
{
  // So we support this type of item?
  if (info.itemAs<smtk::attribute::ReferenceItem>() == nullptr)
  {
    return nullptr;
  }
  auto qi = new qtReferenceItemComboBox(info);
  return qi;
}

qtReferenceItemComboBox::qtReferenceItemComboBox(const qtAttributeItemInfo& info)
  : qtItem(info)
  , m_okToCreate(false)
  , m_useAssociations(false)
{
  this->Internals = new qtReferenceItemComboBoxInternals;
  m_useAssociations = m_itemInfo.component().attributeAsBool("UseAssociations");
  if (m_useAssociations)
  {
    QObject::connect(m_itemInfo.baseView(), SIGNAL(modified(smtk::attribute::ItemPtr)), this,
      SLOT(itemChanged(smtk::attribute::ItemPtr)));
  }
  std::ostringstream receiverSource;
  receiverSource << "qtReferenceItemComboBox_" << this;
  m_selectionSourceName = receiverSource.str();
  auto uiManager = this->uiManager();
  if (uiManager == nullptr)
  {
#if !defined(NDEBUG)
    std::cerr << "qtReferenceItemComboBox: Could not find UI Manager!\n";
#endif
    return;
  }

  QObject::connect(
    uiManager, SIGNAL(highlightOnHoverChanged(bool)), this, SLOT(highlightOnHoverChanged(bool)));

  auto opManager = uiManager->operationManager();
  if (opManager != nullptr)
  {
    m_operationObserverKey = opManager->observers().insert(
      [this](smtk::operation::Operation::Ptr oper, smtk::operation::EventType event,
        smtk::operation::Operation::Result result) -> int {
        return this->handleOperationEvent(oper, event, result);
      });
    this->Internals->operationManager = opManager;
  }
  else
  {
#if !defined(NDEBUG) && DEBUG_REFERENCEITEM
    std::cerr << "qtReferenceItemComboBox: Could not find Operation Manager!\n";
#endif
  }
  auto resManager = uiManager->resourceManager();
  if (resManager != nullptr)
  {
    m_resourceObserverKey =
      resManager->observers().insert([this](const smtk::resource::Resource::Ptr& resource,
        smtk::resource::EventType event) { this->handleResourceEvent(resource, event); });
    this->Internals->resourceManager = resManager;
  }
  else
  {
#if !defined(NDEBUG) && DEBUG_REFERENCEITEM
    std::cerr << "qtReferenceItemComboBox: Could not find Resource Manager!\n";
#endif
  }
  this->createWidget();
  this->highlightOnHoverChanged(uiManager->highlightOnHover());
}

qtReferenceItemComboBox::~qtReferenceItemComboBox()
{
  this->removeObservers();
  delete this->Internals;
}

void qtReferenceItemComboBox::createWidget()
{
  auto item = m_itemInfo.itemAs<attribute::ReferenceItem>();
  if (item == nullptr)
  {
    return;
  }
  m_widget = new QFrame(m_itemInfo.parentWidget());
  if (this->isReadOnly())
  {
    m_widget->setEnabled(false);
  }
  this->Internals->EntryLayout = new QGridLayout(m_widget);
  this->Internals->EntryLayout->setMargin(0);
  this->Internals->EntryLayout->setSpacing(0);
  this->Internals->EntryLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout* labelLayout = new QHBoxLayout();
  labelLayout->setMargin(0);
  labelLayout->setSpacing(0);
  labelLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  int padding = 0;
  if (item->isOptional())
  {
    QCheckBox* optionalCheck = new QCheckBox(m_itemInfo.parentWidget());
    optionalCheck->setChecked(item->isEnabled());
    optionalCheck->setText(" ");
    optionalCheck->setSizePolicy(sizeFixedPolicy);
    padding = optionalCheck->iconSize().width() + 3; // 6 is for layout spacing
    QObject::connect(optionalCheck, SIGNAL(stateChanged(int)), this, SLOT(setOutputOptional(int)));
    labelLayout->addWidget(optionalCheck);
  }
  auto itemDef = item->definitionAs<attribute::ReferenceItemDefinition>();

  QString labelText;
  if (!item->label().empty())
  {
    labelText = item->label().c_str();
  }
  else
  {
    labelText = item->name().c_str();
  }
  QLabel* label = new QLabel(labelText, m_widget);
  label->setSizePolicy(sizeFixedPolicy);
  auto iview = dynamic_cast<qtBaseAttributeView*>(m_itemInfo.baseView().data());
  if (iview)
  {
    label->setFixedWidth(iview->fixedLabelWidth() - padding);
  }
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  //  qtOverlayFilter *filter = new qtOverlayFilter(this);
  //  label->installEventFilter(filter);

  // add in BriefDescription as tooltip if available
  const std::string strBriefDescription = itemDef->briefDescription();
  if (!strBriefDescription.empty())
  {
    label->setToolTip(strBriefDescription.c_str());
  }

  if (itemDef->advanceLevel() && m_itemInfo.baseView())
  {
    label->setFont(m_itemInfo.uiManager()->advancedFont());
  }
  labelLayout->addWidget(label);
  this->Internals->theLabel = label;
  this->Internals->comboBox = new QComboBox(m_widget);
  this->Internals->comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  this->Internals->comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  this->updateItemData();
  // signals/slots
  QObject::connect(
    this->Internals->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectItem(int)));
  this->Internals->EntryLayout->addLayout(labelLayout, 0, 0);
  this->Internals->EntryLayout->addWidget(this->Internals->comboBox, 0, 1);
  if (m_itemInfo.parentWidget() && m_itemInfo.parentWidget()->layout())
  {
    m_itemInfo.parentWidget()->layout()->addWidget(m_widget);
  }
  if (item->isOptional())
  {
    this->setOutputOptional(item->isEnabled() ? 1 : 0);
  }
}

void qtReferenceItemComboBox::updateItemData()
{
  this->updateChoices();
}

void qtReferenceItemComboBox::updateChoices(const smtk::common::UUID& ignoreResource)
{
  auto item = m_itemInfo.itemAs<attribute::ReferenceItem>();
  auto itemDef = item->definitionAs<attribute::ReferenceItemDefinition>();
  auto theAttribute = item->attribute();
  this->Internals->comboBox->blockSignals(true);
  this->m_mappedObjects.clear();
  this->Internals->comboBox->clear();
  if (!item)
  {
    this->Internals->comboBox->blockSignals(false);
    return;
  }

  attribute::DefinitionPtr attDef = theAttribute->definition();

  ResourcePtr attResource = attDef->resource();
  // Lets get a set of possible candidates that could be assigned to the item
  auto objSet = this->associatableObjects(ignoreResource);
  std::vector<smtk::resource::PersistentObjectPtr> objects(objSet.begin(), objSet.end());
  smtk::resource::PersistentObjectPtr selectObj = item->objectValue();
  // Lets sort the list
  std::sort(std::begin(objects), std::end(objects),
    [](smtk::resource::PersistentObjectPtr a, smtk::resource::PersistentObjectPtr b) {
      return a->name() < b->name();
    });

  // Lets set the comboBox
  // First add the please select option
  this->Internals->comboBox->addItem("Please Select");
  this->Internals->comboBox->setItemData(0, QBrush(Qt::red), Qt::ForegroundRole);
  // Are we allowed to create an object?
  if (m_okToCreate)
  {
    this->Internals->comboBox->addItem("Create New");
  }

  int count = 0;
  int selectedIndex = 0;
  for (auto& obj : objects)
  {
    QVariant vdata;
    vdata.setValue(count);
    if (obj == selectObj)
    {
      selectedIndex = this->Internals->comboBox->count();
    }
    // TODO: right now the desciptive phrase stuff displays New Resource for
    // unamed resources so we will do the same.  However, the code should be
    // refactored so that the logic for calculating names resides in one place
    // Also - the current approach doesn't work when there are more than 1
    // unamed resources.
    if (obj->name() == "")
    {
      this->Internals->comboBox->addItem("New Resource", vdata);
    }
    else
    {
      this->Internals->comboBox->addItem(obj->name().c_str(), vdata);
    }
    this->m_mappedObjects[count++] = obj;
  }
  this->Internals->comboBox->setCurrentIndex(selectedIndex);
  if (selectedIndex == 0)
  {
    QPalette comboboxPalette = this->Internals->comboBox->palette();
    comboboxPalette.setColor(QPalette::Text, Qt::red);
    comboboxPalette.setColor(QPalette::WindowText, Qt::red);
    this->Internals->comboBox->setPalette(comboboxPalette);
  }
  else
  {
    this->Internals->comboBox->setPalette(this->Internals->comboBox->parentWidget()->palette());
  }
  this->Internals->comboBox->blockSignals(false);
}

smtk::resource::PersistentObjectPtr qtReferenceItemComboBox::object(int index)
{
  // Lets get the right index
  // Are we dealing with the create new option
  if ((index <= 0) || (m_okToCreate && (index == 1)))
  {
    return nullptr; // These are entries without items associated with them
  }
  bool ok;
  int val = this->Internals->comboBox->itemData(index).toInt(&ok);
  if (!ok)
  {
// There is a problem in that the item doesn't have a mapped value for us to look up
#if !defined(NDEBUG)
    std::cerr << "qtReferenceItemComboBox::object - can't get mapped id for index = " << index
              << "\n";
#endif
    return nullptr;
  }
  auto findResult = m_mappedObjects.find(val);
  if (findResult == m_mappedObjects.end())
  {
// There is a problem in that the mapped value can't be found
#if !defined(NDEBUG)
    std::cerr << "qtReferenceItemComboBox::object - can't find mapped id\n";
#endif
    return nullptr;
  }
  auto selectedObj = findResult->second.lock();
  if (selectedObj == nullptr)
  {
// There is a problem in that we can't get a persistent object from the weak pointer
#if !defined(NDEBUG)
    std::cerr << "qtReferenceItemComboBox::object - can't get PersistentObject\n";
#endif
    return nullptr;
  }
  return selectedObj;
}

void qtReferenceItemComboBox::highlightItem(int index)
{
  // Are we dealing with the create new option
  if ((index == 0) || (m_okToCreate && (index == 1)))
  {
    return; // Nothing to highlight
  }
  // If there is no selection manager then there is nothing
  // we need to do
  auto uiManager = this->uiManager();
  if (uiManager == nullptr)
  {
    return;
  }

  auto selection = uiManager->selection();
  if (selection == nullptr)
  {
    return;
  }
  // First lets see if this is something we can highlight
  if ((index <= 0) || ((index == 1) && m_okToCreate))
  {
    // In this case the user has selected something that corresponds
    // to a special action (unset or create) so there is nothing to color
    selection->resetSelectionBits(m_selectionSourceName, uiManager->hoverBit());
  }
  // Discover what is currently hovered
  auto selectedObject = this->object(index);
  if (selectedObject == nullptr)
  {
    // There is a problem in that we can't get a persistent object from the index
    selection->resetSelectionBits(m_selectionSourceName, uiManager->hoverBit());
#if !defined(NDEBUG)
    std::cerr << "qtReferenceItemComboBox::highlightItem - can't get PersistentObject for index: "
              << index << std::endl;
#endif
    return;
  }
  // Add new hover state
  auto hoverMask = uiManager->hoverBit();
  const auto& selnMap = selection->currentSelection();
  auto cvit = selnMap.find(selectedObject);
  int sv = (cvit == selnMap.end() ? 0 : cvit->second) | hoverMask;
  smtk::resource::PersistentObjectSet objs;
  objs.insert(selectedObject);
  selection->modifySelection(
    objs, m_selectionSourceName, sv, smtk::view::SelectionAction::UNFILTERED_REPLACE, true);
}

void qtReferenceItemComboBox::selectItem(int index)
{
  // Lets get the selection manager if possible since
  // we want to clear anything being highlighted

  smtk::extension::qtUIManager* uiManager = this->uiManager();
  smtk::view::SelectionPtr selection;
  if (uiManager)
  {
    selection = uiManager->selection();
  }

  auto item = m_itemInfo.itemAs<attribute::ReferenceItem>();
  if (item == nullptr)
  {
    // Clear the selection if we can
    if (selection)
    {
      selection->resetSelectionBits(m_selectionSourceName, uiManager->hoverBit());
    }
    return;
  }
  // Is the "Please Select" option choosen?  If so then unset the item if needed
  if (index <= 0)
  {
    if (item->isSet())
    {
      item->unset();
      emit this->modified();
    }
  }
  // Are we dealing with the create new option
  else if (m_okToCreate && (index == 1))
  {
    // ToDo:  Implemented!
  }
  else // Will this change the item?
  {
    auto selectedObject = this->object(index);
    if (selectedObject && !(item->isSet() && (item->objectValue() == selectedObject)))
    {
      item->setObjectValue(selectedObject);
      emit this->modified();
    }
  }

  // Clear the selection if we can
  if (selection)
  {
    selection->resetSelectionBits(m_selectionSourceName, uiManager->hoverBit());
  }
}

std::set<smtk::resource::PersistentObjectPtr> qtReferenceItemComboBox::associatableObjects(
  const smtk::common::UUID& ignoreResource) const
{
  std::set<smtk::resource::PersistentObjectPtr> result;
  auto item = m_itemInfo.itemAs<attribute::ReferenceItem>();
  auto theAttribute = item->attribute();
  if (item == nullptr)
  {
    return result;
  }

  // There are 3 possible sources of Persistent Objects:
  // 1. Those associated with the attribute this item is a member of
  // 2. Based on the resources associated with the attribute resource the item's attribute
  // is a component of
  // 3. The resources contained in the resource manager associated with the UIManager
  if (this->m_useAssociations)
  {
    // We must access elements of the association carefully, since this method is called
    // in the middle of a resource's removal logic. By accessing the associations' keys
    // instead of the associations themselves, we avoid triggering the association's
    // resolve() method (which will attempt to read in the resource being removed).
    auto associations = theAttribute->associations();
    for (std::size_t i = 0; i < associations->numberOfValues(); ++i)
    {
      if (!associations->isSet(i))
      {
        continue;
      }
      attribute::ReferenceItem::Key key = theAttribute->associations()->objectKey(i);
      auto& surrogate = theAttribute->resource()->links().data().value(key.first);
      if (surrogate.id() != ignoreResource)
      {
        if (auto object = associations->objectValue(i))
        {
          result.insert(object);
        }
      }
    }
    return result;
  }

  auto itemDef = item->definitionAs<attribute::ReferenceItemDefinition>();
  auto attResource = theAttribute->attributeResource();
  auto assocMap = item->acceptableEntries();

  // Are we dealing with the case the attribute resource has resources directly associated
  // with it
  if (attResource->hasAssociations())
  {
    auto resources = attResource->associations();
    if (resources.empty())
    {
      // Ok there are resources associated with it but they are not in memory
      return result;
    }
    // Iterate over the acceptable entries
    decltype(assocMap.equal_range("")) range;
    for (auto i = assocMap.begin(); i != assocMap.end(); i = range.second)
    {
      // Get the range for the current key
      range = assocMap.equal_range(i->first);

      // Lets see if any of the resources match this type
      for (auto resource : resources)
      {
        if (resource->id() == ignoreResource)
        {
          continue;
        }

        if (resource->isOfType(i->first))
        {
          // We need to find all of the component types for
          // this resource.  If a string is empty then the resource
          // itself can be associated with the attribute
          for (auto j = range.first; j != range.second; ++j)
          {
            if (j->second.empty())
            {
              result.insert(resource);
            }
            else
            {
              auto comps = resource->find(j->second);
              result.insert(comps.begin(), comps.end());
            }
          }
        }
      }
    }
  }
  else // we need to use the resource manager
  {
    // Iterate over the acceptable entries
    auto resManager = this->uiManager()->resourceManager();
    if (resManager == nullptr)
    {
      return result;
    }

    decltype(assocMap.equal_range("")) range;
    for (auto i = assocMap.begin(); i != assocMap.end(); i = range.second)
    {
      // Get the range for the current key
      range = assocMap.equal_range(i->first);

      // As the resource manager to get all appropriate resources
      auto resources = resManager->find(i->first);
      // Need to process all of these resources
      for (auto resource : resources)
      {
        if (resource->id() == ignoreResource)
        {
          continue;
        }
        // We need to find all of the component types for
        // this resource.  If a string is empty then the resource
        // itself can be associated with the attribute
        for (auto j = range.first; j != range.second; ++j)
        {
          if (j->second.empty())
          {
            result.insert(resource);
          }
          else
          {
            auto comps = resource->find(j->second);
            result.insert(comps.begin(), comps.end());
          }
        }
      }
    }
  }
  return result;
}

void qtReferenceItemComboBox::removeObservers()
{
  if (m_operationObserverKey.assigned())
  {
    auto opManager = this->Internals->operationManager.lock();
    if (opManager != nullptr)
    {
      opManager->observers().erase(m_operationObserverKey);
    }
  }
  if (m_resourceObserverKey.assigned())
  {
    auto resManager = this->Internals->resourceManager.lock();
    if (resManager != nullptr)
    {
      resManager->observers().erase(m_resourceObserverKey);
    }
  }
}

int qtReferenceItemComboBox::handleOperationEvent(smtk::operation::OperationPtr,
  smtk::operation::EventType event, smtk::operation::Operation::Result result)
{
  if (event != smtk::operation::EventType::DID_OPERATE)
  {
    return 0;
  }

  std::size_t count = smtk::operation::extractResources(result).size();
  // If nothing has changed then just return
  if (count == 0)
  {
    return 0;
  }

  // The simplest solution is just to refresh the widget
  this->updateChoices();
  return 0;
}

void qtReferenceItemComboBox::handleResourceEvent(
  const smtk::resource::Resource::Ptr& resource, smtk::resource::EventType event)
{

  auto item = m_itemInfo.itemAs<attribute::ReferenceItem>();
  if (item == nullptr)
  {
    return;
  }
  auto theAttribute = item->attribute();
  auto attResource = theAttribute->attributeResource();

  if ((event == smtk::resource::EventType::REMOVED) && (attResource != resource))
  {
    // The simplest solution is just to refresh the widget
    this->updateChoices(resource->id());
  }
}

void qtReferenceItemComboBox::resetHover()
{
  auto uiManager = this->uiManager();
  if (uiManager == nullptr)
  {
    return;
  }

  auto selection = uiManager->selection();
  if (selection == nullptr)
  {
    return;
  }
  selection->resetSelectionBits(m_selectionSourceName, uiManager->hoverBit());
}

void qtReferenceItemComboBox::setOutputOptional(int state)
{
  auto item = m_itemInfo.itemAs<smtk::attribute::ReferenceItem>();
  if (!item)
  {
    return;
  }
  bool enable = state ? true : false;
  this->Internals->comboBox->setVisible(enable);
  if (enable != item->isEnabled())
  {
    item->setIsEnabled(enable);
    auto iview = dynamic_cast<qtBaseAttributeView*>(m_itemInfo.baseView().data());
    if (iview)
    {
      iview->valueChanged(item);
    }
    emit this->modified();
  }
}

void qtReferenceItemComboBox::highlightOnHoverChanged(bool shouldHighlight)
{
  if (shouldHighlight)
  {
    QObject::connect(
      this->Internals->comboBox, SIGNAL(highlighted(int)), this, SLOT(highlightItem(int)));
  }
  else
  {
    QObject::disconnect(
      this->Internals->comboBox, SIGNAL(highlighted(int)), this, SLOT(highlightItem(int)));
    this->resetHover();
  }
}

void qtReferenceItemComboBox::itemChanged(smtk::attribute::ItemPtr modifiedItem)
{
  smtk::attribute::ItemPtr item = m_itemInfo.item();
  if ((!item) || (item == modifiedItem))
  {
    return;
  }

  if (item->attribute() != modifiedItem->attribute())
  {
    return;
  }
  this->updateChoices();
}
