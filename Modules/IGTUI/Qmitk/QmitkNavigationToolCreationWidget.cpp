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

#include "QmitkNavigationToolCreationWidget.h"

//mitk headers
#include "mitkTrackingTypes.h"
#include <mitkSTLFileReader.h>
#include <mitkSurface.h>

//qt headers
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <QDialog>

//poco headers
#include <Poco/Path.h>

// vtk
#include <vtkSphereSource.h>
#include "vtkConeSource.h"

const std::string QmitkNavigationToolCreationWidget::VIEW_ID = "org.mitk.views.navigationtoolcreationwizardwidget";

QmitkNavigationToolCreationWidget::QmitkNavigationToolCreationWidget(QWidget* parent, Qt::WindowFlags f)
: QWidget(parent, f)
{
  m_Controls = NULL;
  m_AdvancedWidget = new QmitkNavigationToolCreationAdvancedWidget(this);
  m_AdvancedWidget->setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
  m_AdvancedWidget->setWindowTitle("Tool Creation Advanced Options");
  m_AdvancedWidget->setModal(false);
  CreateQtPartControl(this);
  CreateConnections();
}

QmitkNavigationToolCreationWidget::~QmitkNavigationToolCreationWidget()
{
  delete m_AdvancedWidget;
}

void QmitkNavigationToolCreationWidget::CreateQtPartControl(QWidget *parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkNavigationToolCreationWidgetControls;
    m_Controls->setupUi(parent);
  }
}

void QmitkNavigationToolCreationWidget::CreateConnections()
{
  if ( m_Controls )
  {
    connect( (QObject*)(m_Controls->m_cancel), SIGNAL(clicked()), this, SLOT(OnCancel()) );
    connect( (QObject*)(m_Controls->m_finished), SIGNAL(clicked()), this, SLOT(OnFinished()) );
    connect( (QObject*)(m_Controls->m_LoadSurface), SIGNAL(clicked()), this, SLOT(OnLoadSurface()) );
    connect( (QObject*)(m_Controls->m_LoadCalibrationFile), SIGNAL(clicked()), this, SLOT(OnLoadCalibrationFile()) );
    connect( (QObject*)(m_Controls->m_ShowAdvancedOptionsPB), SIGNAL(toggled(bool)), this, SLOT(OnShowAdvancedOptions(bool)) );
    connect( (QObject*)(m_AdvancedWidget), SIGNAL(DialogCloseRequested()), this, SLOT(OnProcessDialogCloseRequest()) );
    connect( (QObject*)(m_AdvancedWidget), SIGNAL(RetrieveDataForManualToolTipManipulation()), this, SLOT(OnRetrieveDataForManualTooltipManipulation()) );
  }
}

void QmitkNavigationToolCreationWidget::Initialize(mitk::DataStorage* dataStorage, std::string supposedIdentifier)
{
  m_DataStorage = dataStorage;
  //m_Controls->m_IdentifierEdit->setText(QString(supposedIdentifier.c_str()));
  m_AdvancedWidget->SetToolIdentifier(supposedIdentifier);
  //initialize UI components
  m_Controls->m_SurfaceChooser->SetDataStorage(m_DataStorage);
  m_Controls->m_SurfaceChooser->SetAutoSelectNewItems(true);
  m_Controls->m_SurfaceChooser->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

  //set default data
  m_Controls->m_ToolNameEdit->setText("NewTool");
  m_Controls->m_CalibrationFileName->setText("<not given>");
  m_AdvancedWidget->SetSerialNumber("<not given>");
  m_Controls->m_Surface_Use_Sphere->setChecked(true);
  m_AdvancedWidget->SetToolType(0);
  m_AdvancedWidget->SetDataStorage(m_DataStorage);

}

void QmitkNavigationToolCreationWidget::SetTrackingDeviceType(mitk::TrackingDeviceType type, bool changeable)
{
  switch(type)
  {
  case mitk::NDIAurora:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(0);break;
  case mitk::NDIPolaris:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(1);break;
  case mitk::ClaronMicron:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(2);break;
  default:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(0);
  }
  m_Controls->m_TrackingDeviceTypeChooser->setEnabled(changeable);
}

mitk::NavigationTool::Pointer QmitkNavigationToolCreationWidget::GetCreatedTool()
{
  return m_CreatedTool;
}

//##################################################################################
//############################## slots                  ############################
//##################################################################################

void QmitkNavigationToolCreationWidget::OnFinished()
{
  //here we create a new tool
  m_CreatedTool = mitk::NavigationTool::New();

  if(m_AdvancedWidget->GetManipulatedToolTip().IsNotNull())
  {
    mitk::Surface::Pointer manipulatedSurface = dynamic_cast<mitk::Surface*>(
      m_AdvancedWidget->GetManipulatedToolTip()->GetData());

    mitk::Geometry3D::Pointer geo = manipulatedSurface->GetGeometry();
    ;
    geo->GetParametricTransform();
    m_CreatedTool->SetToolTipOrientation();
    m_CreatedTool->SetToolTipPosition(geo->GetCenter());
  }

  //create DataNode...
  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  if(m_Controls->m_Surface_Use_Sphere->isChecked())
  {
    //create small sphere and use it as surface
    mitk::Surface::Pointer mySphere = mitk::Surface::New();
    vtkConeSource *vtkData = vtkConeSource::New();
    vtkData->SetAngle(5.0);
    vtkData->SetResolution(50);
    vtkData->SetHeight(6.0f);
    vtkData->SetRadius(2.0f);
    vtkData->SetCenter(0.0, 0.0, 0.0);
    vtkData->Update();
    mySphere->SetVtkPolyData(vtkData->GetOutput());
    vtkData->Delete();
    newNode->SetData(mySphere);
  }
  else
  {
    newNode->SetData(m_Controls->m_SurfaceChooser->GetSelectedNode()->GetData());
  }
  newNode->SetName(m_Controls->m_ToolNameEdit->text().toLatin1());

  m_CreatedTool->SetDataNode(newNode);

  //fill NavigationTool object
  m_CreatedTool->SetCalibrationFile(m_Controls->m_CalibrationFileName->text().toAscii().data());
  m_CreatedTool->SetIdentifier(m_AdvancedWidget->GetToolIdentifier().c_str());
  m_CreatedTool->SetSerialNumber(m_AdvancedWidget->GetSerialNumber().c_str());

  //Tracking Device
  if (m_Controls->m_TrackingDeviceTypeChooser->currentText()=="NDI Aurora") m_CreatedTool->SetTrackingDeviceType(mitk::NDIAurora);
  else if (m_Controls->m_TrackingDeviceTypeChooser->currentText()=="NDI Polaris") m_CreatedTool->SetTrackingDeviceType(mitk::NDIPolaris);
  else if (m_Controls->m_TrackingDeviceTypeChooser->currentText()=="Claron Technology Micron Tracker") m_CreatedTool->SetTrackingDeviceType(mitk::ClaronMicron);
  else m_CreatedTool->SetTrackingDeviceType(mitk::TrackingSystemNotSpecified);

  //ToolType
  if (m_AdvancedWidget->GetToolType() ==
    QmitkNavigationToolCreationAdvancedWidget::Instrument)
  {
    m_CreatedTool->SetType(mitk::NavigationTool::Instrument);
  }
  else if (m_AdvancedWidget->GetToolType() ==
    QmitkNavigationToolCreationAdvancedWidget::Fiducial)
  {
    m_CreatedTool->SetType(mitk::NavigationTool::Fiducial);
  }
  else if (m_AdvancedWidget->GetToolType() ==
    QmitkNavigationToolCreationAdvancedWidget::Skinmarker)
  {
    m_CreatedTool->SetType(mitk::NavigationTool::Skinmarker);
  }
  else
  {
    m_CreatedTool->SetType(mitk::NavigationTool::Unknown);
  }
  emit NavigationToolFinished();
}

void QmitkNavigationToolCreationWidget::OnCancel()
{
  m_CreatedTool = NULL;
  if(m_AdvancedWidget->GetManipulatedToolTip().IsNotNull())
  {
    m_DataStorage->Remove(m_AdvancedWidget->GetManipulatedToolTip());
  }
  emit Canceled();
}

void QmitkNavigationToolCreationWidget::OnLoadSurface()
{
  std::string filename = QFileDialog::getOpenFileName(NULL,tr("Open Surface"), "/", "*.stl").toLatin1().data();
  mitk::STLFileReader::Pointer stlReader = mitk::STLFileReader::New();
  try
  {
    stlReader->SetFileName( filename.c_str() );
    stlReader->Update();
  }
  catch (...)
  {
  }

  if ( stlReader->GetOutput() == NULL );
  else
  {
    mitk::DataNode::Pointer newNode = mitk::DataNode::New();
    newNode->SetName(filename);
    newNode->SetData(stlReader->GetOutput());
    m_DataStorage->Add(newNode);
  }
}

void QmitkNavigationToolCreationWidget::OnLoadCalibrationFile()
{
  m_Controls->m_CalibrationFileName->setText(QFileDialog::getOpenFileName(NULL,tr("Open Calibration File"), "/", "*.*"));
}

void QmitkNavigationToolCreationWidget::SetDefaultData(mitk::NavigationTool::Pointer DefaultTool)
{
  m_Controls->m_ToolNameEdit->setText(QString(DefaultTool->GetDataNode()->GetName().c_str()));
  m_AdvancedWidget->SetToolIdentifier( DefaultTool->GetIdentifier() );
  m_AdvancedWidget->SetSerialNumber( DefaultTool->GetSerialNumber() );
  switch(DefaultTool->GetTrackingDeviceType())
  {
  case mitk::NDIAurora:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(0);break;
  case mitk::NDIPolaris:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(1);break;
  case mitk::ClaronMicron:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(2);break;
  default:
    m_Controls->m_TrackingDeviceTypeChooser->setCurrentIndex(0);
  }
  m_Controls->m_CalibrationFileName->setText(QString(DefaultTool->GetCalibrationFile().c_str()));
  m_Controls->m_Surface_Use_Other->setChecked(true);
  switch(DefaultTool->GetType())
  {
  case mitk::NavigationTool::Instrument:
    m_AdvancedWidget->SetToolType(0); break;
  case mitk::NavigationTool::Fiducial:
    m_AdvancedWidget->SetToolType(1); break;
  case mitk::NavigationTool::Skinmarker:
    m_AdvancedWidget->SetToolType(2); break;
  case mitk::NavigationTool::Unknown:
    m_AdvancedWidget->SetToolType(3); break;
  }

  m_Controls->m_SurfaceChooser->SetSelectedNode(DefaultTool->GetDataNode());

}



//##################################################################################
//############################## internal help methods #############################
//##################################################################################
void QmitkNavigationToolCreationWidget::MessageBox(std::string s)
{
  QMessageBox msgBox;
  msgBox.setText(s.c_str());
  msgBox.exec();
}

void QmitkNavigationToolCreationWidget::OnShowAdvancedOptions(bool state)
{
  if(state)
  {
    m_AdvancedWidget->show();
  }
  else
  {
    m_AdvancedWidget->hide();
  }
}

void QmitkNavigationToolCreationWidget::OnProcessDialogCloseRequest()
{
  m_AdvancedWidget->hide();
  m_Controls->m_ShowAdvancedOptionsPB->setChecked(false);
}

void QmitkNavigationToolCreationWidget::OnRetrieveDataForManualTooltipManipulation()
{
  if(m_Controls->m_Surface_Use_Sphere->isChecked())
  {
    m_AdvancedWidget->SetToolTipSurface(true);
  }
  else
  {
    m_AdvancedWidget->SetToolTipSurface(false,
      dynamic_cast<mitk::DataNode*>(m_Controls->m_SurfaceChooser->GetSelectedNode().GetPointer()));
  }
}