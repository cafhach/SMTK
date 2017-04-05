//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/extension/qt/qtFileItem.h"

#include "smtk/extension/qt/qtBaseView.h"
#include "smtk/extension/qt/qtOverlay.h"
#include "smtk/extension/qt/qtUIManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSignalMapper>
#include <QSizePolicy>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/DirectoryItem.h"
#include "smtk/attribute/DirectoryItemDefinition.h"
#include "smtk/attribute/FileItem.h"
#include "smtk/attribute/FileItemDefinition.h"
#include "smtk/attribute/FileSystemItem.h"
#include "smtk/attribute/FileSystemItemDefinition.h"
#include "smtk/attribute/System.h"

//#include "pqApplicationCore.h"

using namespace smtk::attribute;
using namespace smtk::extension;

//----------------------------------------------------------------------------
class qtFileItemInternals
{
public:
  qtFileItemInternals() : FileBrowser(NULL) {}
  ~qtFileItemInternals() {}

  bool IsDirectory;
  QFileDialog *FileBrowser;
  QPointer<QComboBox> fileCombo;

  QPointer<QGridLayout> EntryLayout;
  QPointer<QLabel> theLabel;
  Qt::Orientation VectorItemOrient;

  // for discrete items that with potential child widget
  // <Enum-Combo, child-layout >
  QMap<QWidget*, QPointer<QLayout> >ChildrenMap;

  // for extensible items
  QMap<QToolButton*, QPair<QPointer<QLayout>, QPointer<QWidget> > > ExtensibleMap;
  QPointer<QSignalMapper> SignalMapper;
  QList<QToolButton*> MinusButtonIndices;
  QPointer<QToolButton> AddItemButton;
};

//----------------------------------------------------------------------------
qtFileItem::qtFileItem(
  smtk::attribute::FileSystemItemPtr dataObj, QWidget* p, qtBaseView* bview,
   Qt::Orientation enVectorItemOrient) : qtItem(dataObj, p, bview)
{
  this->Internals = new qtFileItemInternals;
  this->Internals->IsDirectory =
    (dataObj->type() == smtk::attribute::Item::DIRECTORY);
  this->Internals->SignalMapper = new QSignalMapper();
  this->IsLeafItem = true;
  this->Internals->VectorItemOrient = enVectorItemOrient;
  this->createWidget();
  if (bview)
    {
    bview->uiManager()->onFileItemCreated(this);
    }
}

//----------------------------------------------------------------------------
qtFileItem::~qtFileItem()
{
  delete this->Internals;
}
//----------------------------------------------------------------------------
void qtFileItem::setLabelVisible(bool visible)
{
  this->Internals->theLabel->setVisible(visible);
}

//----------------------------------------------------------------------------
bool qtFileItem::isDirectory()
{
  return this->Internals->IsDirectory;
}

//----------------------------------------------------------------------------
// Although you *can* disable this feature, it is not recommended.
// Behavior is not defined if this method is called after
// the ancestor qtUIManager::initializeUI() method is called.
void qtFileItem::enableFileBrowser(bool state)
{
  if (!state)
    {
    this->Internals->FileBrowser->setParent(NULL);
    delete this->Internals->FileBrowser;
    this->Internals->FileBrowser = NULL;
    }
  else if (NULL == this->Internals->FileBrowser)
    {
    this->Internals->FileBrowser = new QFileDialog(this->Widget);
    this->Internals->FileBrowser->setObjectName("Select File Dialog");
    this->Internals->FileBrowser->setDirectory(QDir::currentPath());
    }
}

//----------------------------------------------------------------------------
QWidget* qtFileItem::createFileBrowseWidget(int elementIdx)
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<attribute::FileSystemItem>(this->getObject());

  QWidget* fileTextWidget = NULL;
  QComboBox* fileCombo = NULL;
  QLineEdit* lineEdit = NULL;
  QFrame *frame = new QFrame(this->parentWidget());
  //frame->setStyleSheet("QFrame { background-color: yellow; }");
  QString defaultText;
  if (item->type() == smtk::attribute::Item::FILE)
    {
    const smtk::attribute::FileItemDefinition *fDef =
      dynamic_cast<const attribute::FileItemDefinition*>(item->definition().get());
    if (fDef->hasDefault())
      defaultText = fDef->defaultValue().c_str();
    // For open Files, we use a combobox to show the recent file list
    if(fDef->shouldExist() && !item->isExtensible())
      {
      fileCombo = new QComboBox(frame);
      fileCombo->setEditable(true);
      fileTextWidget = fileCombo;
      this->Internals->fileCombo = fileCombo;
      }
    }

  if(fileTextWidget == NULL)
    {
    lineEdit = new QLineEdit(frame);
    fileTextWidget = lineEdit;
    }

  // As a file input, if the name is too long lets favor
  // the file name over the path
  if(lineEdit)
    lineEdit->setAlignment(Qt::AlignRight);
  else if(fileCombo)
    fileCombo->lineEdit()->setAlignment(Qt::AlignRight);

  frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  fileTextWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QPushButton* fileBrowserButton = new QPushButton("Browse", frame);
  fileBrowserButton->setMinimumHeight(fileTextWidget->height());
  fileBrowserButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QHBoxLayout* layout = new QHBoxLayout(frame);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(fileTextWidget);
  layout->addWidget(fileBrowserButton);
  layout->setAlignment(Qt::AlignCenter);

  QString valText =
    item ? item->valueAsString(elementIdx).c_str() : defaultText;

  QVariant vdata;
  vdata.setValue(static_cast<void*>(fileTextWidget));
  this->setProperty("DataItem", vdata);
  QVariant vdata1(elementIdx);
  fileTextWidget->setProperty("ElementIndex", vdata1);

  this->updateFileComboList(valText);

  if(fileCombo)
    fileCombo->setCurrentIndex(fileCombo->findText(valText));
  else if(lineEdit)
    lineEdit->setText(valText);

  QObject::connect(fileBrowserButton, SIGNAL(clicked()),
                   this->Internals->SignalMapper, SLOT(map()));
  this->Internals->SignalMapper->setMapping(fileBrowserButton,
                                            fileBrowserButton);

  // We use a QSignalMapper here to connect our signal, which comes from one of
  // potentially several lineEdits, fileCombos or fileBrowserButtons, to our
  // slot, which is the method setActiveField(QWidget*). This way,
  // setActiveField can tag the appropriate field to be used within
  // onInputValueChanged(). When we depricate Qt4 in favor of Qt5, this may be
  // handled more elegantly using lambda expressions as slots.

  if(lineEdit)
    {
    QObject::connect(lineEdit, SIGNAL(textChanged(const QString &)),
                     this->Internals->SignalMapper, SLOT(map()));
    this->Internals->SignalMapper->setMapping(lineEdit, lineEdit);
    QObject::connect(this->Internals->SignalMapper, SIGNAL(mapped(QWidget*)),
                     this, SLOT(setActiveField(QWidget*)));
    QObject::connect(lineEdit, SIGNAL(textChanged(const QString &)),
      this, SLOT(onInputValueChanged()));
    this->Internals->SignalMapper->setMapping(fileBrowserButton, lineEdit);
    QObject::connect(lineEdit, SIGNAL(editingFinished()),
                     this, SLOT(onEditingFinished()));
    }
  else if(fileCombo)
    {
    QObject::connect(fileCombo, SIGNAL(textChanged(const QString &)),
                     this->Internals->SignalMapper, SLOT(map()));
    QObject::connect(fileCombo, SIGNAL(currentIndexChanged(int)),
                     this->Internals->SignalMapper, SLOT(map()));
    this->Internals->SignalMapper->setMapping(fileCombo, fileCombo);
    QObject::connect(this->Internals->SignalMapper, SIGNAL(mapped(QWidget*)),
                     this, SLOT(setActiveField(QWidget*)));

    QObject::connect(fileCombo, SIGNAL(editTextChanged(const QString &)),
      this, SLOT(onInputValueChanged()));
    QObject::connect(fileCombo, SIGNAL(currentIndexChanged(int)),
      this, SLOT(onInputValueChanged()));
    this->Internals->SignalMapper->setMapping(fileBrowserButton, fileCombo);
    }

  QObject::connect(this->Internals->SignalMapper, SIGNAL(mapped(QWidget*)),
                   this, SLOT(setActiveField(QWidget*)));
  QObject::connect(fileBrowserButton, SIGNAL(clicked()),
                   this, SLOT(onLaunchFileBrowser()));

  return frame;
}

//----------------------------------------------------------------------------
void qtFileItem::onInputValueChanged()
{
  QLineEdit* editBox = NULL;
  if(this->Internals->fileCombo)
    {
    editBox = this->Internals->fileCombo->lineEdit();
    }
  if(!editBox)
    {
    editBox = static_cast<QLineEdit*>(
      this->property("DataItem").value<void *>());
    }

  if(!editBox)
    {
    return;
    }

  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<attribute::FileSystemItem>(this->getObject());
  int elementIdx = editBox->property("ElementIndex").toInt();

  if((item && item->isSet(elementIdx) &&
      item->value(elementIdx) == editBox->text().toStdString()))
    {
    return;
    }
  if(!editBox->text().isEmpty())
    {
    item->setValue(elementIdx, editBox->text().toStdString());
    emit this->modified();
    if (!this->isDirectory())
      {
      this->updateFileComboList(editBox->text());
      }
    this->baseView()->valueChanged(this->getObject());
    }
  else
    {
    item->unset(elementIdx);
    emit(modified());
   }
}


//----------------------------------------------------------------------------
void qtFileItem::onEditingFinished()
{
  // For files, check if the extension in the input is valid. If it is not,
  // append a valid extension to it.

  if(this->Internals->fileCombo || this->Internals->IsDirectory)
    {
    return;
    }

  // The right line edit should be associated with the "DataItem" property,
  // since this call will come after onInputValueChanged()
  QLineEdit* lineEdit = static_cast<QLineEdit*>(
    this->property("DataItem").value<void *>());

  QString value = lineEdit->text();

  if (!value.isEmpty())
    {
    smtk::attribute::FileItemPtr fItem;
    smtk::attribute::ItemPtr item =
      smtk::dynamic_pointer_cast<smtk::attribute::Item>(this->getObject());

    fItem = smtk::dynamic_pointer_cast<smtk::attribute::FileItem>(item);
    const smtk::attribute::FileItemDefinition *fItemDef =
      dynamic_cast<const smtk::attribute::FileItemDefinition*>(fItem->definition().get());
    if (fItemDef->isValueValid(value.toStdString()) == false)
      {
      QFileInfo fi(value);

      std::string filters = fItemDef->getFileFilters();
      std::size_t begin = filters.find_first_of("*",
                                                filters.find_first_of("(")) + 1;
      std::size_t end = filters.find_first_of(" \n\r\t)", begin);
      QString acceptableSuffix(filters.substr(begin, end - begin).c_str());

      value = fi.absolutePath() + QString("/") + fi.baseName() + acceptableSuffix;
      lineEdit->setText(value);
      }
    }
}

//----------------------------------------------------------------------------
void qtFileItem::onLaunchFileBrowser()
{
  // If we are not using local file browser, just emit signal and return
  if (!this->Internals->FileBrowser)
    {
    emit this->launchFileBrowser();
    return;
    }

  // If local file browser instantiated, get data from it
  smtk::attribute::DirectoryItemPtr dItem;
  smtk::attribute::FileItemPtr fItem;
  smtk::attribute::ItemPtr item =
    smtk::dynamic_pointer_cast<smtk::attribute::Item>(this->getObject());

  QString filters;
  QFileDialog::FileMode mode = QFileDialog::AnyFile;
  if (this->Internals->IsDirectory)
    {
    dItem = smtk::dynamic_pointer_cast<smtk::attribute::DirectoryItem>(item);
    const smtk::attribute::DirectoryItemDefinition *dItemDef =
      dynamic_cast<const smtk::attribute::DirectoryItemDefinition*>(dItem->definition().get());
    mode = QFileDialog::Directory;
    this->Internals->FileBrowser->setOption(QFileDialog::ShowDirsOnly,
      dItemDef->shouldExist());
    }
  else
    {
    fItem = smtk::dynamic_pointer_cast<smtk::attribute::FileItem>(item);
    const smtk::attribute::FileItemDefinition *fItemDef =
      dynamic_cast<const smtk::attribute::FileItemDefinition*>(fItem->definition().get());
    filters = fItemDef->getFileFilters().c_str();
    mode = fItemDef->shouldExist() ? QFileDialog::ExistingFile :
      QFileDialog::AnyFile;
    }
  this->Internals->FileBrowser->setFileMode(mode);
#if QT_VERSION >= 0x050000
  QStringList name_filters = filters.split(";;");
  this->Internals->FileBrowser->setNameFilters(name_filters);
#else
  this->Internals->FileBrowser->setFilter(filters);
#endif

  this->Internals->FileBrowser->setWindowModality(Qt::WindowModal);
  if (this->Internals->FileBrowser->exec() == QDialog::Accepted)
    {
    QStringList files = this->Internals->FileBrowser->selectedFiles();
    this->setInputValue(files[0]);
    }
}

//----------------------------------------------------------------------------
void qtFileItem::updateFileComboList(const QString& newFile)
{
  if (this->Internals->fileCombo)
    {
    this->Internals->fileCombo->blockSignals(true);
    QString currentFile = this->Internals->fileCombo->currentText();
    this->Internals->fileCombo->clear();
    smtk::attribute::FileItemPtr fItem =dynamic_pointer_cast<attribute::FileItem>(this->getObject());
    fItem->addRecentValue(newFile.toStdString());
    std::vector<std::string>::const_iterator it;
    for(it = fItem->recentValues().begin();
        it != fItem->recentValues().end(); ++it)
      {
      this->Internals->fileCombo->addItem((*it).c_str());
      }
    this->Internals->fileCombo->setCurrentIndex(
      this->Internals->fileCombo->findText(currentFile));
    this->Internals->fileCombo->blockSignals(false);
    }
}

//----------------------------------------------------------------------------
void qtFileItem::setInputValue(const QString& val)
{
  QLineEdit* lineEdit =  NULL;
  if(this->Internals->fileCombo)
    {
    lineEdit = this->Internals->fileCombo->lineEdit();
    }
  else
    {
    lineEdit = static_cast<QLineEdit*>(
      this->property("DataItem").value<void *>());
    }
  if(!lineEdit)
    {
    return;
    }

  // this itself will not trigger onInputValueChanged
  lineEdit->setText(val);
  this->onInputValueChanged();
}

//----------------------------------------------------------------------------
void qtFileItem::createWidget()
{
  smtk::attribute::ItemPtr dataObj = this->getObject();
  if(!dataObj || !this->passAdvancedCheck() || (this->baseView() &&
    !this->baseView()->uiManager()->passItemCategoryCheck(
      dataObj->definition())))
    {
    return;
    }

  this->clearChildWidgets();
  this->updateItemData();
}

//----------------------------------------------------------------------------
void qtFileItem::updateItemData()
{
  this->updateUI();
  this->qtItem::updateItemData();
}

//----------------------------------------------------------------------------
void qtFileItem::addInputEditor(int i)
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item)
    {
    return;
    }

  int n = static_cast<int>(item->numberOfValues());
  if (!n)
    {
    return;
    }
  QBoxLayout* childLayout = NULL;
  childLayout = new QVBoxLayout;
  childLayout->setContentsMargins(12, 3, 3, 0);
  childLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  QWidget* editBox = this->createFileBrowseWidget(i);
  if(!editBox)
    {
    return;
    }

  const FileSystemItemDefinition *itemDef =
    dynamic_cast<const FileSystemItemDefinition*>(item->definition().get());
  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  QBoxLayout* editorLayout = new QHBoxLayout;
  editorLayout->setMargin(0);
  editorLayout->setSpacing(3);
  if(item->isExtensible())
    {
    QToolButton* minusButton = new QToolButton(this->Widget);
    QString iconName(":/icons/attribute/minus.png");
    minusButton->setFixedSize(QSize(12, 12));
    minusButton->setIcon(QIcon(iconName));
    minusButton->setSizePolicy(sizeFixedPolicy);
    minusButton->setToolTip("Remove value");
    editorLayout->addWidget(minusButton);
    connect(minusButton, SIGNAL(clicked()),
      this, SLOT(onRemoveValue()));
    QPair<QPointer<QLayout>, QPointer<QWidget> > pair;
    pair.first = editorLayout;
    pair.second = editBox;
    this->Internals->ExtensibleMap[minusButton] = pair;
    this->Internals->MinusButtonIndices.push_back(minusButton);
    }

  if(n!=1 && itemDef->hasValueLabels())
    {
    std::string componentLabel = itemDef->valueLabel(i);
    if(!componentLabel.empty())
      {
      // acbauer -- this should probably be improved to look nicer
      QString labelText = componentLabel.c_str();
      QLabel* label = new QLabel(labelText, editBox);
      label->setSizePolicy(sizeFixedPolicy);
      editorLayout->addWidget(label);
      }
    }
  editorLayout->addWidget(editBox);

  // always going vertical for discrete and extensible items
  if(this->Internals->VectorItemOrient == Qt::Vertical || item->isExtensible())
    {
    int row = 2*i;
    // The "Add New Value" button is in first row, so take that into account
    row = item->isExtensible() ? row+1 : row;
    this->Internals->EntryLayout->addLayout(editorLayout, row, 1);

    // there could be conditional children, so we need another layout
    // so that the combobox will stay TOP-left when there are multiple
    // combo boxes.
    if(childLayout)
      {
      this->Internals->EntryLayout->addLayout(childLayout, row+1, 0, 1, 2);
      }
    }
  else // going horizontal
    {
    this->Internals->EntryLayout->addLayout(editorLayout, 0, i+1);
    }

  this->Internals->ChildrenMap[editBox] = childLayout;
  this->updateExtensibleState();
}

//----------------------------------------------------------------------------
void qtFileItem::loadInputValues()
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item)
    {
    return;
    }

  int n = static_cast<int>(item->numberOfValues());
  if (!n && !item->isExtensible())
    {
    return;
    }

  if(item->isExtensible())
    {
    if(!this->Internals->AddItemButton)
      {
      QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      this->Internals->AddItemButton = new QToolButton(this->Widget);
      QString iconName(":/icons/attribute/plus.png");
      this->Internals->AddItemButton->setText("Add New Value");
      this->Internals->AddItemButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

//      this->Internals->AddItemButton->setFixedSize(QSize(12, 12));
      this->Internals->AddItemButton->setIcon(QIcon(iconName));
      this->Internals->AddItemButton->setSizePolicy(sizeFixedPolicy);
      connect(this->Internals->AddItemButton, SIGNAL(clicked()),
        this, SLOT(onAddNewValue()));
      this->Internals->EntryLayout->addWidget(this->Internals->AddItemButton, 0, 1);
      }
    }

  for(int i = 0; i < n; i++)
    {
    this->addInputEditor(i);
    }
}

//----------------------------------------------------------------------------
void qtFileItem::updateUI()
{
  //smtk::attribute::ItemPtr dataObj = this->getObject();
  smtk::attribute::FileSystemItemPtr dataObj =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!dataObj || !this->passAdvancedCheck() || (this->baseView() &&
    !this->baseView()->uiManager()->passItemCategoryCheck(
      dataObj->definition())))
    {
    return;
    }

  this->Widget = new QFrame(this->parentWidget());
  this->Internals->EntryLayout = new QGridLayout(this->Widget);
  this->Internals->EntryLayout->setMargin(0);
  this->Internals->EntryLayout->setSpacing(0);
  this->Internals->EntryLayout->setAlignment( Qt::AlignLeft | Qt::AlignTop );

  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout* labelLayout = new QHBoxLayout();
  labelLayout->setMargin(0);
  labelLayout->setSpacing(0);
  labelLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  int padding = 0;
  if(dataObj->isOptional())
    {
    QCheckBox* optionalCheck = new QCheckBox(this->parentWidget());
    optionalCheck->setChecked(dataObj->isEnabled());
    optionalCheck->setText(" ");
    optionalCheck->setSizePolicy(sizeFixedPolicy);
    padding = optionalCheck->iconSize().width() + 3; // 6 is for layout spacing
    QObject::connect(optionalCheck, SIGNAL(stateChanged(int)),
      this, SLOT(setOutputOptional(int)));
    labelLayout->addWidget(optionalCheck);
    }
  const FileSystemItemDefinition *itemDef =
    dynamic_cast<const FileSystemItemDefinition*>(dataObj->definition().get());

  QString labelText;
  if(!dataObj->label().empty())
    {
    labelText = dataObj->label().c_str();
    }
  else
    {
    labelText = dataObj->name().c_str();
    }
  QLabel* label = new QLabel(labelText, this->Widget);
  label->setSizePolicy(sizeFixedPolicy);
  if(this->baseView())
    {
    label->setFixedWidth(this->baseView()->fixedLabelWidth() - padding);
    }
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignLeft | Qt::AlignTop);

//  qtOverlayFilter *filter = new qtOverlayFilter(this);
//  label->installEventFilter(filter);

  // add in BriefDescription as tooltip if available
  const std::string strBriefDescription = itemDef->briefDescription();
  if(!strBriefDescription.empty())
    {
    label->setToolTip(strBriefDescription.c_str());
    }

  if(itemDef->advanceLevel() && this->baseView())
    {
    label->setFont(this->baseView()->uiManager()->advancedFont());
    }
  labelLayout->addWidget(label);
  this->Internals->theLabel = label;

  this->loadInputValues();

  // we need this layout so that for items with conditionan children,
  // the label will line up at Top-left against the chilren's widgets.
//  QVBoxLayout* vTLlayout = new QVBoxLayout;
//  vTLlayout->setMargin(0);
//  vTLlayout->setSpacing(0);
//  vTLlayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//  vTLlayout->addLayout(labelLayout);
  this->Internals->EntryLayout->addLayout(labelLayout, 0, 0);
//  layout->addWidget(this->Internals->EntryFrame, 0, 1);
  if(this->parentWidget() && this->parentWidget()->layout())
    {
    this->parentWidget()->layout()->addWidget(this->Widget);
    }
  if(dataObj->isOptional())
    {
    this->setOutputOptional(dataObj->isEnabled() ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void qtFileItem::setOutputOptional(int state)
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item)
    {
    return;
    }
  bool enable = state ? true : false;
  if(item->isExtensible())
    {
    if(this->Internals->AddItemButton)
      {
      this->Internals->AddItemButton->setVisible(enable);
      }
    foreach(QToolButton* tButton, this->Internals->ExtensibleMap.keys())
      {
      tButton->setVisible(enable);
      }
   }

  foreach(QWidget* cwidget, this->Internals->ChildrenMap.keys())
    {
    QLayout* childLayout = this->Internals->ChildrenMap.value(cwidget);
    if(childLayout)
      {
      for (int i = 0; i < childLayout->count(); ++i)
        childLayout->itemAt(i)->widget()->setVisible(enable);
      }
    cwidget->setVisible(enable);
    }

//  this->Internals->EntryFrame->setEnabled(enable);
  if(enable != this->getObject()->isEnabled())
    {
    this->getObject()->setIsEnabled(enable);
    emit this->modified();
     if(this->baseView())
       {
       this->baseView()->valueChanged(this->getObject());
       }
    }
}

//----------------------------------------------------------------------------
void qtFileItem::onAddNewValue()
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item)
    {
    return;
    }
  if(item->setNumberOfValues(item->numberOfValues() + 1))
    {
//    QBoxLayout* entryLayout = qobject_cast<QBoxLayout*>(
//      this->Internals->EntryFrame->layout());
    this->addInputEditor(static_cast<int>(item->numberOfValues()) - 1);
    emit this->modified();

    }
}

//----------------------------------------------------------------------------
void qtFileItem::onRemoveValue()
{
  QToolButton* const minusButton = qobject_cast<QToolButton*>(
    QObject::sender());
  if(!minusButton || !this->Internals->ExtensibleMap.contains(minusButton))
    {
    return;
    }

  int gIdx = this->Internals->MinusButtonIndices.indexOf(minusButton);//minusButton->property("SubgroupIndex").toInt();
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item || gIdx < 0 || gIdx >= static_cast<int>(item->numberOfValues()))
    {
    return;
    }

  QWidget* childwidget = this->Internals->ExtensibleMap.value(minusButton).second;
  QLayout* childLayout = this->Internals->ChildrenMap.value(childwidget);
  if(childLayout)
    {
    QLayoutItem *child;
    while ((child = childLayout->takeAt(0)) != 0)
      {
      delete child;
      }
    delete childLayout;
    }
  delete childwidget;
  delete this->Internals->ExtensibleMap.value(minusButton).first;
  this->Internals->ExtensibleMap.remove(minusButton);
  this->Internals->MinusButtonIndices.removeOne(minusButton);
  delete minusButton;

  item->removeValue(gIdx);
  emit this->modified();

  this->updateExtensibleState();
}

//----------------------------------------------------------------------------
void qtFileItem::setActiveField(QWidget* activeField)
{
  QVariant vdata;
  vdata.setValue(static_cast<void*>(activeField));
  this->setProperty("DataItem", vdata);
}

//----------------------------------------------------------------------------
void qtFileItem::updateExtensibleState()
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item || !item->isExtensible())
    {
    return;
    }
  bool maxReached = (item->maxNumberOfValues() > 0) &&
    (item->maxNumberOfValues() == item->numberOfValues());
  this->Internals->AddItemButton->setEnabled(!maxReached);

  bool minReached = (item->numberOfRequiredValues() > 0) &&
    (item->numberOfRequiredValues() == item->numberOfValues());
  foreach(QToolButton* tButton, this->Internals->ExtensibleMap.keys())
    {
    tButton->setEnabled(!minReached);
    }
}

//----------------------------------------------------------------------------
void qtFileItem::clearChildWidgets()
{
  smtk::attribute::FileSystemItemPtr item =
    dynamic_pointer_cast<FileSystemItem>(this->getObject());
  if(!item)
    {
    return;
    }

  if(item->isExtensible())
    {
    //clear mapping
    foreach(QToolButton* tButton, this->Internals->ExtensibleMap.keys())
      {
// will delete later from this->Internals->ChildrenMap
//      delete this->Internals->ExtensibleMap.value(tButton).second;
      delete this->Internals->ExtensibleMap.value(tButton).first;
      delete tButton;
      }
    this->Internals->ExtensibleMap.clear();
    this->Internals->MinusButtonIndices.clear();
    }

  foreach(QWidget* cwidget, this->Internals->ChildrenMap.keys())
    {
    QLayout* childLayout = this->Internals->ChildrenMap.value(cwidget);
    if(childLayout)
      {
      QLayoutItem *child;
      while ((child = childLayout->takeAt(0)) != 0)
        {
        delete child;
        }
      delete childLayout;
      }
    delete cwidget;
    }
  this->Internals->ChildrenMap.clear();
}
