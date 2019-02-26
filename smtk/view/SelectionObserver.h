//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef __smtk_view_SelectionObserver_h
#define __smtk_view_SelectionObserver_h

#include "smtk/CoreExports.h"
#include "smtk/PublicPointerDefs.h"

#include "smtk/common/Observers.h"

#include <string>

namespace smtk
{
namespace view
{

/// Events that alter the phrase model trigger callbacks of this type.
typedef std::function<void(const std::string&, SelectionPtr)> SelectionObserver;

/// A class for holding SelectionObserver functors that observe phrase model events.
typedef smtk::common::Observers<SelectionObserver> SelectionObservers;
}
}

#endif // __smtk_view_SelectionObserver_h