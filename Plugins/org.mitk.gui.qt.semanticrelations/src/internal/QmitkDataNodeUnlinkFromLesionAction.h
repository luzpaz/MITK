/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#ifndef QMITKDATANODEUNLINKFROMLESIONACTION_H
#define QMITKDATANODEUNLINKFROMLESIONACTION_H

#include <org_mitk_gui_qt_semanticrelations_Export.h>

// semantic relations module
#include <mitkSemanticRelations.h>

// mitk gui qt application plugin
#include <QmitkAbstractDataNodeAction.h>

// qt
#include <QAction.h>

namespace UnlinkFromLesionAction
{
  MITK_GUI_SEMANTICRELATIONS_EXPORT void Run(mitk::SemanticRelations* semanticRelations, const mitk::DataStorage* dataStorage, const mitk::DataNode* dataNode);

  void UnlinkSegmentation(mitk::SemanticRelations* semanticRelations, const mitk::DataNode* segmentation);
}

class MITK_GUI_SEMANTICRELATIONS_EXPORT QmitkDataNodeUnlinkFromLesionAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeUnlinkFromLesionAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeUnlinkFromLesionAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite);

  virtual ~QmitkDataNodeUnlinkFromLesionAction() override;

  void SetDataStorage(mitk::DataStorage* dataStorage);

private Q_SLOTS:

  void OnActionTriggered(bool);

protected:

  virtual void InitializeAction() override;

  std::unique_ptr<mitk::SemanticRelations> m_SemanticRelations;

};

#endif // QMITKDATANODEUNLINKFROMLESIONACTION_H
