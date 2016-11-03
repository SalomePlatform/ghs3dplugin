// Copyright (C) 2004-2016  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  GHS3DPlugin GUI: GUI for plugged-in mesher GHS3DPlugin
//  File   : GHS3DPluginGUI_HypothesisCreator.cxx
//  Author : Michael Zorin
//  Module : GHS3DPlugin
//
#include "GHS3DPluginGUI_HypothesisCreator.h"
#include "GHS3DPluginGUI_Enums.h"
#include "GHS3DPluginGUI_Dlg.h"
#include "GHS3DPlugin_OptimizerHypothesis.hxx"

#include <GeometryGUI.h>

#include <SMESHGUI_Utils.h>
#include <SMESHGUI_SpinBox.h>
#include <SMESHGUI_HypothesesUtils.h>
#include <SMESH_NumberFilter.hxx>
#include <SMESH_TypeFilter.hxx>
#include <StdMeshersGUI_ObjectReferenceParamWdg.h>

#include <LightApp_SelectionMgr.h>
#include <SUIT_FileDlg.h>
#include <SUIT_MessageBox.h>
#include <SUIT_ResourceMgr.h>
#include <SUIT_Session.h>
#include <SalomeApp_IntSpinBox.h>
#include <SalomeApp_Tools.h>
#include <SalomeApp_TypeFilter.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <stdexcept>
#include <utilities.h>

namespace
{
  QComboBox* getModeCombo( QWidget* parent, bool isPThreadCombo )
  {
    QComboBox* combo = new QComboBox( parent );
    if ( isPThreadCombo )
    {
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::SAFE, QObject::tr("MODE_SAFE"));
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::AGGRESSIVE, QObject::tr("MODE_AGGRESSIVE"));
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::NONE, QObject::tr("MODE_NONE"));
    }
    else
    {
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::NO, QObject::tr("MODE_NO"));
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::YES, QObject::tr("MODE_YES"));
      combo->insertItem((int) GHS3DPlugin_OptimizerHypothesis::ONLY, QObject::tr("MODE_ONLY"));
    }
    return combo;
  }
}

//
// BEGIN EnforcedVertexTableWidgetDelegate
//

EnforcedVertexTableWidgetDelegate::EnforcedVertexTableWidgetDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

QWidget *EnforcedVertexTableWidgetDelegate::createEditor(QWidget *parent,
                                                         const QStyleOptionViewItem & option ,
                                                         const QModelIndex & index ) const
{
  QModelIndex father = index.parent();
  QString entry = father.child(index.row(), ENF_VER_ENTRY_COLUMN).data().toString();
  
  if (index.column() == ENF_VER_X_COLUMN ||
      index.column() == ENF_VER_Y_COLUMN ||
      index.column() == ENF_VER_Z_COLUMN ||
      index.column() == ENF_VER_SIZE_COLUMN) {
    SMESHGUI_SpinBox *editor = new SMESHGUI_SpinBox(parent);
    if (index.column() == ENF_VER_SIZE_COLUMN)
      editor->RangeStepAndValidator(0, COORD_MAX, 10.0, "length_precision");
    else
      editor->RangeStepAndValidator(COORD_MIN, COORD_MAX, 10.0, "length_precision");
    editor->setReadOnly(!entry.isEmpty());
    editor->setDisabled(!entry.isEmpty());
    return editor;
  }
  else if (index.column() == ENF_VER_GROUP_COLUMN ||
           index.column() == ENF_VER_NAME_COLUMN) {
    //   else {
    QLineEdit *editor = new QLineEdit(parent);
    if (index.column() != ENF_VER_GROUP_COLUMN) {
      editor->setReadOnly(!entry.isEmpty());
      editor->setDisabled(!entry.isEmpty());
    }
    return editor;
  }
  return QItemDelegate::createEditor(parent, option, index);
}

void EnforcedVertexTableWidgetDelegate::setEditorData(QWidget *editor,
                                                      const QModelIndex &index) const
{
  if (index.column() == ENF_VER_X_COLUMN ||
      index.column() == ENF_VER_Y_COLUMN ||
      index.column() == ENF_VER_Z_COLUMN ||
      index.column() == ENF_VER_SIZE_COLUMN)
  {
    SMESHGUI_SpinBox *lineEdit = qobject_cast<SMESHGUI_SpinBox*>(editor);
    lineEdit->SetValue(index.data().toDouble());
  } 
  else if (index.column() == ENF_VER_COMPOUND_COLUMN) {
    QCheckBox *checkBox = qobject_cast<QCheckBox*>(editor);
    checkBox->setChecked(index.data().toBool());
  }
  else {
    QItemDelegate::setEditorData(editor, index);
  }
}

void EnforcedVertexTableWidgetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                                     const QModelIndex &index) const
{
  QModelIndex parent = index.parent();
  
  QString entry = parent.child(index.row(), ENF_VER_ENTRY_COLUMN).data().toString();
  bool isCompound = parent.child(index.row(), ENF_VER_COMPOUND_COLUMN).data(Qt::CheckStateRole).toBool();
  
  if (index.column() == ENF_VER_X_COLUMN || 
      index.column() == ENF_VER_Y_COLUMN || 
      index.column() == ENF_VER_Z_COLUMN) {
    SMESHGUI_SpinBox *lineEdit = qobject_cast<SMESHGUI_SpinBox*>(editor);
    if (!isCompound && !vertexExists(model, index, lineEdit->GetString()))
      model->setData(index, lineEdit->GetValue(), Qt::EditRole);
  } 
  else if (index.column() == ENF_VER_SIZE_COLUMN)
  {
    SMESHGUI_SpinBox *lineEdit = qobject_cast<SMESHGUI_SpinBox*>(editor);
    const double newsize =  lineEdit->GetValue();
    if (newsize > 0)
      model->setData(index, newsize, Qt::EditRole);
  } 
  else if (index.column() == ENF_VER_NAME_COLUMN) {
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
    QString value = lineEdit->text();
    if (entry.isEmpty() && !vertexExists(model, index, value))
      model->setData(index, value, Qt::EditRole);
  } 
  else if (index.column() == ENF_VER_ENTRY_COLUMN) {
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
    QString value = lineEdit->text();
    if (! vertexExists(model, index, value))
      model->setData(index, value, Qt::EditRole);
  }
  else if (index.column() == ENF_VER_COMPOUND_COLUMN) {
    QCheckBox *checkBox = qobject_cast<QCheckBox*>(editor);
    model->setData(index, checkBox->isChecked(), Qt::CheckStateRole);
  }
  else {
    QItemDelegate::setModelData(editor, model, index);
  }
}

void EnforcedVertexTableWidgetDelegate::updateEditorGeometry(QWidget *editor,
                                                             const QStyleOptionViewItem &option,
                                                             const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

bool EnforcedVertexTableWidgetDelegate::vertexExists(QAbstractItemModel *model,
                                                     const QModelIndex &index, 
                                                     QString value) const
{
  bool exists = false;
  QModelIndex parent = index.parent();
  int row = index.row();
  int col = index.column();

  if (parent.isValid() && !value.isEmpty()) {
    if (col == ENF_VER_X_COLUMN || col == ENF_VER_Y_COLUMN || col == ENF_VER_Z_COLUMN) {
      double x, y, z;
      if (col == ENF_VER_X_COLUMN) {
        x = value.toDouble();
        y = parent.child(row, ENF_VER_Y_COLUMN).data().toDouble();
        z = parent.child(row, ENF_VER_Z_COLUMN).data().toDouble();
      }
      if (col == ENF_VER_Y_COLUMN) {
        y = value.toDouble();
        x = parent.child(row, ENF_VER_X_COLUMN).data().toDouble();
        z = parent.child(row, ENF_VER_Z_COLUMN).data().toDouble();
      }
      if (col == ENF_VER_Z_COLUMN) {
        z = value.toDouble();
        x = parent.child(row, ENF_VER_X_COLUMN).data().toDouble();
        y = parent.child(row, ENF_VER_Y_COLUMN).data().toDouble();
      }
      int nbChildren = model->rowCount(parent);
      for (int i = 0 ; i < nbChildren ; i++) {
        if (i != row) {
          double childX = parent.child(i, ENF_VER_X_COLUMN).data().toDouble();
          double childY = parent.child(i, ENF_VER_Y_COLUMN).data().toDouble();
          double childZ = parent.child(i, ENF_VER_Z_COLUMN).data().toDouble();
          if ((childX == x) && (childY == y) && (childZ == z)) {
            exists = true;
            break;
          }
        }
      }
    }
    else if (col == ENF_VER_NAME_COLUMN) {
      QString name = parent.child(row, ENF_VER_NAME_COLUMN).data().toString();
      if (name == value)
        exists = true;
    }
  }

  return exists;
}

//
// END EnforcedVertexTableWidgetDelegate
//

//
// BEGIN EnforcedMeshTableWidgetDelegate
//

EnforcedMeshTableWidgetDelegate::EnforcedMeshTableWidgetDelegate(QObject *parent)
  : QItemDelegate(parent)
{
}

QWidget *EnforcedMeshTableWidgetDelegate::createEditor(QWidget *parent,
                                                       const QStyleOptionViewItem & option ,
                                                       const QModelIndex & index ) const
{
  return QItemDelegate::createEditor(parent, option, index);
}

void EnforcedMeshTableWidgetDelegate::setEditorData(QWidget *editor,
                                                    const QModelIndex &index) const
{
  QItemDelegate::setEditorData(editor, index);
}

void EnforcedMeshTableWidgetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                                   const QModelIndex &index) const
{  
  QItemDelegate::setModelData(editor, model, index);

}

void EnforcedMeshTableWidgetDelegate::updateEditorGeometry(QWidget *editor,
                                                           const QStyleOptionViewItem &option,
                                                           const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

//
// END EnforcedMeshTableWidgetDelegate
//


GHS3DPluginGUI_HypothesisCreator::GHS3DPluginGUI_HypothesisCreator( const QString& theHypType )
  : SMESHGUI_GenericHypothesisCreator( theHypType )
{
  GeomToolSelected = NULL;
  GeomToolSelected = getGeomSelectionTool();

  iconVertex  = QPixmap(SUIT_Session::session()->resourceMgr()->loadPixmap("GEOM", tr("ICON_OBJBROWSER_VERTEX")));
  iconCompound  = QPixmap(SUIT_Session::session()->resourceMgr()->loadPixmap("GEOM", tr("ICON_OBJBROWSER_COMPOUND")));
  //   mySelectionMgr = SMESH::GetSelectionMgr(SMESHGUI::GetSMESHGUI());
  myEnfMeshConstraintLabels << tr( "GHS3D_ENF_MESH_CONSTRAINT_NODE" ) << tr( "GHS3D_ENF_MESH_CONSTRAINT_EDGE" ) << tr("GHS3D_ENF_MESH_CONSTRAINT_FACE");
}

GHS3DPluginGUI_HypothesisCreator::~GHS3DPluginGUI_HypothesisCreator()
{
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  that->getGeomSelectionTool()->selectionMgr()->clearFilters();
  myEnfMeshWdg->deactivateSelection();
}

/**
 * \brief {Get or create the geom selection tool for active study}
 * */
GeomSelectionTools* GHS3DPluginGUI_HypothesisCreator::getGeomSelectionTool()
{
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  _PTR(Study) aStudy = SMESH::GetActiveStudyDocument();
  if (that->GeomToolSelected == NULL || that->GeomToolSelected->getMyStudy() != aStudy) {
    that->GeomToolSelected = new GeomSelectionTools(aStudy);
  }
  return that->GeomToolSelected;
}

GEOM::GEOM_Gen_var GHS3DPluginGUI_HypothesisCreator::getGeomEngine()
{
  return GeometryGUI::GetGeomGen();
}

bool GHS3DPluginGUI_HypothesisCreator::isOptimization() const
{
  return ( hypType() == GHS3DPlugin_OptimizerHypothesis::GetHypType() );
}

QFrame* GHS3DPluginGUI_HypothesisCreator::buildFrame()
{
  QFrame* fr = new QFrame( 0 );
  QVBoxLayout* lay = new QVBoxLayout( fr );
  lay->setMargin( 5 );
  lay->setSpacing( 0 );

  // tab
  QTabWidget* tab = new QTabWidget( fr );
  tab->setTabShape( QTabWidget::Rounded );
  tab->setTabPosition( QTabWidget::North );
  lay->addWidget( tab );

  // basic parameters
  myStdGroup = new QWidget();
  QGridLayout* aStdLayout = new QGridLayout( myStdGroup );
  aStdLayout->setSpacing( 6 );
  aStdLayout->setMargin( 11 );

  int row = 0;
  myName = 0;
  if ( isCreation() )
  {
    aStdLayout->addWidget( new QLabel( tr( "SMESH_NAME" ), myStdGroup ), row, 0, 1, 1 );
    myName = new QLineEdit( myStdGroup );
    aStdLayout->addWidget( myName, row++, 1, 1, 1 );
  }

  myToMeshHolesCheck = new QCheckBox( tr( "GHS3D_TO_MESH_HOLES" ), myStdGroup );
  aStdLayout->addWidget( myToMeshHolesCheck, row, 0, 1, 1 );
  myToMakeGroupsOfDomains = new QCheckBox( tr( "GHS3D_TO_MAKE_DOMAIN_GROUPS" ), myStdGroup );
  aStdLayout->addWidget( myToMakeGroupsOfDomains, row++, 1, 1, 1 );

  QLabel* optimizationLbl = new QLabel( tr( "GHS3D_OPTIMIZATION" ), myStdGroup );
  aStdLayout->addWidget( optimizationLbl, row, 0, 1, 1 );
  myOptimizationCombo = getModeCombo( myStdGroup, false );
  aStdLayout->addWidget( myOptimizationCombo, row++, 1, 1, 1 );

  QLabel* optimizatiolLevelLbl = new QLabel( tr( "GHS3D_OPTIMIZATIOL_LEVEL" ), myStdGroup );
  aStdLayout->addWidget( optimizatiolLevelLbl, row, 0, 1, 1 );
  myOptimizationLevelCombo = new QComboBox( myStdGroup );
  aStdLayout->addWidget( myOptimizationLevelCombo, row++, 1, 1, 1 );

  QLabel* splitOverconstrainedLbl = new QLabel( tr("GHS3D_SPLIT_OVERCONSTRAINED"), myStdGroup );
  aStdLayout->addWidget( splitOverconstrainedLbl, row, 0, 1, 1 );
  mySplitOverConstrainedCombo = getModeCombo( myStdGroup, false );
  aStdLayout->addWidget( mySplitOverConstrainedCombo, row++, 1, 1, 1 );

  QLabel* pthreadsModeLbl = new QLabel( tr( "GHS3D_PTHREADS_MODE" ), myStdGroup);
  aStdLayout->addWidget( pthreadsModeLbl, row, 0, 1, 1 );
  myPThreadsModeCombo = getModeCombo( myStdGroup, true );
  aStdLayout->addWidget( myPThreadsModeCombo, row++, 1, 1, 1 );

  QLabel* nbThreadsLbl = new QLabel( tr( "GHS3D_NB_THREADS" ), myStdGroup);
  aStdLayout->addWidget( nbThreadsLbl, row, 0, 1, 1 );
  myNumberOfThreadsSpin = new SalomeApp_IntSpinBox( 0, 1000, 1, myStdGroup );
  aStdLayout->addWidget( myNumberOfThreadsSpin, row++, 1, 1, 1 );

  mySmoothOffSliversCheck = new QCheckBox( tr( "GHS3D_SMOOTH_OFF_SLIVERS" ), myStdGroup );
  aStdLayout->addWidget( mySmoothOffSliversCheck, row, 0, 1, 1 );
  myCreateNewNodesCheck = new QCheckBox( tr( "TO_ADD_NODES" ), myStdGroup );
  aStdLayout->addWidget( myCreateNewNodesCheck, row++, 1, 1, 1 );

  myOptimizationLevelCombo->addItems( QStringList()
                                      << tr( "LEVEL_NONE" )   << tr( "LEVEL_LIGHT" )
                                      << tr( "LEVEL_MEDIUM" ) << tr( "LEVEL_STANDARDPLUS" )
                                      << tr( "LEVEL_STRONG" ));
  aStdLayout->setRowStretch( row, 10 );

  if ( isOptimization() )
  {
    myToMeshHolesCheck->hide();
    myToMakeGroupsOfDomains->hide();
  }
  else
  {
    optimizationLbl->hide();
    myOptimizationCombo->hide();
    splitOverconstrainedLbl->hide();
    mySplitOverConstrainedCombo->hide();
    pthreadsModeLbl->hide();
    myPThreadsModeCombo->hide();
    nbThreadsLbl->hide();
    myNumberOfThreadsSpin->hide();
    mySmoothOffSliversCheck->hide();
    myCreateNewNodesCheck->hide();
  }

  // advanced parameters
  myAdvGroup = new QWidget();
  QGridLayout* anAdvLayout = new QGridLayout( myAdvGroup );
  anAdvLayout->setSpacing( 6 );
  anAdvLayout->setMargin( 11 );
  myAdvWidget = new GHS3DPluginGUI_AdvWidget(myAdvGroup);
  anAdvLayout->addWidget( myAdvWidget);

  myAdvWidget->maxMemoryCheck->setText(tr( "MAX_MEMORY_SIZE" ));
  myAdvWidget->initialMemoryCheck->setText(tr( "INIT_MEMORY_SIZE" ));

  myAdvWidget->maxMemorySpin->stepBy(10);
  myAdvWidget->maxMemorySpin->setValue( 128 );

  myAdvWidget->initialMemorySpin->stepBy(10);
  myAdvWidget->initialMemorySpin->setValue( 100 );

  myAdvWidget->initialMemoryLabel            ->setText (tr( "MEGABYTE" ));
  myAdvWidget->maxMemoryLabel                ->setText (tr( "MEGABYTE" ));

  myAdvWidget->workingDirectoryLabel         ->setText (tr( "WORKING_DIR" ));
  myAdvWidget->workingDirectoryPushButton    ->setText (tr( "SELECT_DIR" ));
  myAdvWidget->keepWorkingFilesCheck         ->setText (tr( "KEEP_WORKING_FILES" ));
  myAdvWidget->verboseLevelLabel             ->setText (tr( "VERBOSE_LEVEL" ));
  myAdvWidget->removeLogOnSuccessCheck       ->setText (tr( "REMOVE_LOG_ON_SUCCESS" ));
  myAdvWidget->logInFileCheck                ->setText (tr( "LOG_IN_FILE" ));
  
  myAdvWidget->memoryGroupBox                ->setTitle(tr( "MEMORY_GROUP_TITLE" ));
  myAdvWidget->logGroupBox                   ->setTitle(tr( "LOG_GROUP_TITLE" ));
  myAdvWidget->advancedMeshingGroupBox       ->setTitle(tr( "ADVANCED_MESHING_GROUP_TITLE" ));
  
  myAdvWidget->createNewNodesCheck           ->setText (tr( "TO_ADD_NODES" ));
  myAdvWidget->removeInitialCentralPointCheck->setText (tr( "NO_INITIAL_CENTRAL_POINT" ));
  myAdvWidget->boundaryRecoveryCheck         ->setText (tr( "RECOVERY_VERSION" ));
  myAdvWidget->FEMCorrectionCheck            ->setText (tr( "FEM_CORRECTION" ));
  myAdvWidget->gradationLabel                ->setText (tr( "GHS3D_GRADATION" ));
  myAdvWidget->gradationSpinBox->RangeStepAndValidator(0.0, 5.0, 0.05, "length_precision");

  if ( isOptimization() )
  {
    myAdvWidget->createNewNodesCheck->hide();
    myAdvWidget->removeInitialCentralPointCheck->hide();
    myAdvWidget->boundaryRecoveryCheck->hide();
    myAdvWidget->FEMCorrectionCheck->hide();
    myAdvWidget->gradationLabel->hide();
    myAdvWidget->gradationSpinBox->hide();
  }

  // Enforced vertices parameters
  myEnfGroup = new QWidget();
  QGridLayout* anEnfLayout = new QGridLayout(myEnfGroup);
  
  myEnforcedTableWidget = new QTableWidget(myEnfGroup);
  myEnforcedTableWidget ->setMinimumWidth(300);
  myEnforcedTableWidget->setRowCount( 0 );
  myEnforcedTableWidget->setColumnCount( ENF_VER_NB_COLUMNS );
  myEnforcedTableWidget->setSortingEnabled(true);
  myEnforcedTableWidget->setHorizontalHeaderLabels
    ( QStringList()
      << tr( "GHS3D_ENF_NAME_COLUMN" ) << tr( "GHS3D_ENF_VER_X_COLUMN" )
      << tr( "GHS3D_ENF_VER_Y_COLUMN" ) << tr( "GHS3D_ENF_VER_Z_COLUMN" )
      << tr( "GHS3D_ENF_SIZE_COLUMN" ) << tr("GHS3D_ENF_ENTRY_COLUMN")
      << tr("GHS3D_ENF_VER_COMPOUND_COLUMN") << tr( "GHS3D_ENF_GROUP_COLUMN" ));
  myEnforcedTableWidget->verticalHeader()->hide();
  myEnforcedTableWidget->horizontalHeader()->setStretchLastSection(true);
  myEnforcedTableWidget->setAlternatingRowColors(true);
  myEnforcedTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  myEnforcedTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  myEnforcedTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#else
  myEnforcedTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#endif
  myEnforcedTableWidget->resizeColumnsToContents();
  myEnforcedTableWidget->hideColumn(ENF_VER_ENTRY_COLUMN);
  myEnforcedTableWidget->hideColumn(ENF_VER_COMPOUND_COLUMN);
  
  myEnforcedTableWidget->setItemDelegate(new EnforcedVertexTableWidgetDelegate());
  
  // VERTEX SELECTION
  TColStd_MapOfInteger shapeTypes;
  shapeTypes.Add( TopAbs_VERTEX );
  shapeTypes.Add( TopAbs_COMPOUND );

  SMESH_NumberFilter* vertexFilter = new SMESH_NumberFilter("GEOM", TopAbs_SHAPE, 1, shapeTypes);
  myEnfVertexWdg = new StdMeshersGUI_ObjectReferenceParamWdg( vertexFilter, 0, /*multiSel=*/true);
  myEnfVertexWdg->SetDefaultText(tr("GHS3D_ENF_SELECT_VERTEX"), "QLineEdit { color: grey }");
  
  QLabel* myXCoordLabel = new QLabel( tr( "GHS3D_ENF_VER_X_LABEL" ), myEnfGroup );
  myXCoord = new SMESHGUI_SpinBox(myEnfGroup);
  myXCoord->RangeStepAndValidator(COORD_MIN, COORD_MAX, 10.0, "length_precision");
  QLabel* myYCoordLabel = new QLabel( tr( "GHS3D_ENF_VER_Y_LABEL" ), myEnfGroup );
  myYCoord = new SMESHGUI_SpinBox(myEnfGroup);
  myYCoord->RangeStepAndValidator(COORD_MIN, COORD_MAX, 10.0, "length_precision");
  QLabel* myZCoordLabel = new QLabel( tr( "GHS3D_ENF_VER_Z_LABEL" ), myEnfGroup );
  myZCoord = new SMESHGUI_SpinBox(myEnfGroup);
  myZCoord->RangeStepAndValidator(COORD_MIN, COORD_MAX, 10.0, "length_precision");
  QLabel* mySizeLabel = new QLabel( tr( "GHS3D_ENF_SIZE_LABEL" ), myEnfGroup );
  mySizeValue = new SMESHGUI_SpinBox(myEnfGroup);
  mySizeValue->RangeStepAndValidator(COORD_MIN, COORD_MAX, 10.0, "length_precision");

  QLabel* myGroupNameLabel = new QLabel( tr( "GHS3D_ENF_GROUP_LABEL" ), myEnfGroup );
  myGroupName = new QLineEdit(myEnfGroup);

  addVertexButton = new QPushButton(tr("GHS3D_ENF_ADD"),myEnfGroup);
  addVertexButton->setEnabled(false);
  removeVertexButton = new QPushButton(tr("GHS3D_ENF_REMOVE"),myEnfGroup);

  anEnfLayout->addWidget(myEnforcedTableWidget,     ENF_VER_VERTEX, 0, ENF_VER_NB_LINES, 1);
  
  QGridLayout* anEnfLayout2 = new QGridLayout(myEnfGroup);
  anEnfLayout2->addWidget(myEnfVertexWdg,           ENF_VER_VERTEX, 0, 1, 2);
  anEnfLayout2->addWidget(myXCoordLabel,            ENF_VER_X_COORD, 0, 1, 1);
  anEnfLayout2->addWidget(myXCoord,                 ENF_VER_X_COORD, 1, 1, 1);
  anEnfLayout2->addWidget(myYCoordLabel,            ENF_VER_Y_COORD, 0, 1, 1);
  anEnfLayout2->addWidget(myYCoord,                 ENF_VER_Y_COORD, 1, 1, 1);
  anEnfLayout2->addWidget(myZCoordLabel,            ENF_VER_Z_COORD, 0, 1, 1);
  anEnfLayout2->addWidget(myZCoord,                 ENF_VER_Z_COORD, 1, 1, 1);
  anEnfLayout2->addWidget(mySizeLabel,              ENF_VER_SIZE, 0, 1, 1);
  anEnfLayout2->addWidget(mySizeValue,              ENF_VER_SIZE, 1, 1, 1);
  anEnfLayout2->addWidget(myGroupNameLabel,         ENF_VER_GROUP, 0, 1, 1);
  anEnfLayout2->addWidget(myGroupName,              ENF_VER_GROUP, 1, 1, 1);
  anEnfLayout2->addWidget(addVertexButton,          ENF_VER_BTN, 0, 1, 1);
  anEnfLayout2->addWidget(removeVertexButton,       ENF_VER_BTN, 1, 1, 1);
  anEnfLayout2->setRowStretch(ENF_VER_NB_LINES, 1);
  
  anEnfLayout->addLayout(anEnfLayout2,              ENF_VER_VERTEX, 1,ENF_VER_NB_LINES, 1);
  anEnfLayout->setRowStretch(ENF_VER_VERTEX, 10);
  

  // Enforced meshes parameters
  myEnfMeshGroup = new QWidget();
  QGridLayout* anEnfMeshLayout = new QGridLayout(myEnfMeshGroup);
  
  myEnforcedMeshTableWidget = new QTableWidget(myEnfGroup);
  myEnforcedMeshTableWidget->setRowCount( 0 );
  myEnforcedMeshTableWidget->setColumnCount( ENF_MESH_NB_COLUMNS );
  myEnforcedMeshTableWidget->setSortingEnabled(true);
  myEnforcedMeshTableWidget->verticalHeader()->hide();
  QStringList enforcedMeshHeaders;
  enforcedMeshHeaders << tr( "GHS3D_ENF_NAME_COLUMN" ) 
                      << tr( "GHS3D_ENF_ENTRY_COLUMN" ) 
                      << tr( "GHS3D_ENF_MESH_CONSTRAINT_COLUMN" ) 
                      << tr( "GHS3D_ENF_GROUP_COLUMN" );
  myEnforcedMeshTableWidget->setHorizontalHeaderLabels(enforcedMeshHeaders);
  myEnforcedMeshTableWidget->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  myEnforcedMeshTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#else
  myEnforcedMeshTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#endif
  myEnforcedMeshTableWidget->setAlternatingRowColors(true);
  myEnforcedMeshTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  myEnforcedMeshTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
  myEnforcedMeshTableWidget->resizeColumnsToContents();
  myEnforcedMeshTableWidget->hideColumn(ENF_MESH_ENTRY_COLUMN);
  
  myEnforcedMeshTableWidget->setItemDelegate(new EnforcedMeshTableWidgetDelegate());
  
  myEnfMeshWdg = new StdMeshersGUI_ObjectReferenceParamWdg( SMESH::IDSOURCE, myEnfMeshGroup, /*multiSel=*/true);
  myEnfMeshWdg->SetDefaultText(tr("GHS3D_ENF_SELECT_MESH"), "QLineEdit { color: grey }");
  
  myEnfMeshWdg->AvoidSimultaneousSelection(myEnfVertexWdg);
  
  QLabel* myMeshConstraintLabel = new QLabel( tr( "GHS3D_ENF_MESH_CONSTRAINT_LABEL" ), myEnfMeshGroup );
  myEnfMeshConstraint = new QComboBox(myEnfMeshGroup);
  myEnfMeshConstraint->insertItems(0,myEnfMeshConstraintLabels);
  myEnfMeshConstraint->setEditable(false);
  myEnfMeshConstraint->setCurrentIndex(0);

  QLabel* myMeshGroupNameLabel = new QLabel( tr( "GHS3D_ENF_GROUP_LABEL" ), myEnfMeshGroup );
  myMeshGroupName = new QLineEdit(myEnfMeshGroup);

  addEnfMeshButton = new QPushButton(tr("GHS3D_ENF_ADD"),myEnfMeshGroup);
  removeEnfMeshButton = new QPushButton(tr("GHS3D_ENF_REMOVE"),myEnfMeshGroup);
  
  anEnfMeshLayout->addWidget(myEnforcedMeshTableWidget, ENF_MESH_MESH, 0, ENF_MESH_NB_LINES , 1);
  
  QGridLayout* anEnfMeshLayout2 = new QGridLayout(myEnfMeshGroup);
  anEnfMeshLayout2->addWidget(myEnfMeshWdg,             ENF_MESH_MESH, 0, 1, 2);
  anEnfMeshLayout2->addWidget(myMeshConstraintLabel,    ENF_MESH_CONSTRAINT, 0, 1, 1);
  anEnfMeshLayout2->addWidget(myEnfMeshConstraint,      ENF_MESH_CONSTRAINT, 1, 1, 1);
  anEnfMeshLayout2->addWidget(myMeshGroupNameLabel,     ENF_MESH_GROUP, 0, 1, 1);
  anEnfMeshLayout2->addWidget(myMeshGroupName,          ENF_MESH_GROUP, 1, 1, 1);
  anEnfMeshLayout2->addWidget(addEnfMeshButton,         ENF_MESH_BTN, 0, 1, 1);
  anEnfMeshLayout2->addWidget(removeEnfMeshButton,      ENF_MESH_BTN, 1, 1, 1);
  anEnfMeshLayout2->setRowStretch(ENF_MESH_NB_LINES, 1);
  
  anEnfMeshLayout->addLayout(anEnfMeshLayout2,          ENF_MESH_MESH, 1, ENF_MESH_NB_LINES, 1);
  anEnfMeshLayout->setRowStretch(ENF_MESH_MESH, 10);
  
  // add tabs
  tab->insertTab( STD_TAB, myStdGroup, tr( "SMESH_ARGUMENTS" ) );
  tab->insertTab( ADV_TAB, myAdvGroup, tr( "GHS3D_ADV_ARGS" ) );
  if ( !isOptimization() )
  {
    tab->insertTab( ENF_VER_TAB, myEnfGroup, tr( "GHS3D_ENFORCED_VERTICES" ) );
    tab->insertTab( ENF_MESH_TAB, myEnfMeshGroup, tr( "GHS3D_ENFORCED_MESHES" ) );
  }
  tab->setCurrentIndex( STD_TAB );

  connect( myAdvWidget->maxMemoryCheck,             SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myAdvWidget->initialMemoryCheck,         SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myAdvWidget->boundaryRecoveryCheck,      SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myAdvWidget->logInFileCheck,             SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myAdvWidget->keepWorkingFilesCheck,      SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myAdvWidget->workingDirectoryPushButton, SIGNAL( clicked() ),       this, SLOT( onDirBtnClicked() ) );
  
  connect( myEnforcedTableWidget,   SIGNAL( itemClicked(QTableWidgetItem *)), this, SLOT( synchronizeCoords() ) );
  connect( myEnforcedTableWidget,   SIGNAL( itemChanged(QTableWidgetItem *)), this, SLOT( updateEnforcedVertexValues(QTableWidgetItem *) ) );
  connect( myEnforcedTableWidget,   SIGNAL( itemSelectionChanged() ),         this, SLOT( synchronizeCoords() ) );
  connect( addVertexButton,         SIGNAL( clicked()),                       this, SLOT( onAddEnforcedVertex() ) );
  connect( removeVertexButton,      SIGNAL( clicked()),                       this, SLOT( onRemoveEnforcedVertex() ) );
  connect( myEnfVertexWdg,          SIGNAL( contentModified()),               this, SLOT( onSelectEnforcedVertex() ) );
  connect( myEnfVertexWdg,          SIGNAL( contentModified()),              this,  SLOT( checkVertexIsDefined() ) );
  connect( myXCoord,                SIGNAL( textChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( myYCoord,                SIGNAL( textChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( myZCoord,                SIGNAL( textChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( mySizeValue,             SIGNAL( textChanged(const QString&) ),   this,  SLOT( checkVertexIsDefined() ) );
  connect( myXCoord,                SIGNAL( valueChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( myYCoord,                SIGNAL( valueChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( myZCoord,                SIGNAL( valueChanged(const QString&) ),   this,  SLOT( clearEnfVertexSelection() ) );
  connect( mySizeValue,             SIGNAL( valueChanged(const QString&) ),   this,  SLOT( checkVertexIsDefined() ) );
  connect( this,                    SIGNAL( vertexDefined(bool) ), addVertexButton, SLOT( setEnabled(bool) ) );
  
  connect( addEnfMeshButton,        SIGNAL( clicked()),                       this, SLOT( onAddEnforcedMesh() ) );
  connect( removeEnfMeshButton,     SIGNAL( clicked()),                       this, SLOT( onRemoveEnforcedMesh() ) );
  
  return fr;
}

/** 
 * This method checks if an enforced vertex is defined;
 **/
void GHS3DPluginGUI_HypothesisCreator::clearEnfVertexSelection()
{
  if (myEnfVertexWdg->NbObjects() != 0) {
    disconnect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( onSelectEnforcedVertex() ) );
    disconnect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( checkVertexIsDefined() ) );
    myEnfVertexWdg->SetObject(GEOM::GEOM_Object::_nil());
    connect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( onSelectEnforcedVertex() ) );
    connect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( checkVertexIsDefined() ) );
  }
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  that->checkVertexIsDefined();
}

/** 
 * This method checks if an enforced vertex is defined;
 **/
void GHS3DPluginGUI_HypothesisCreator::checkVertexIsDefined()
{
  bool enfVertexIsDefined = false;
  enfVertexIsDefined = (!mySizeValue->GetString().isEmpty() &&
                        (!myEnfVertexWdg->NbObjects() == 0 ||
                         (myEnfVertexWdg->NbObjects() == 0 && !myXCoord->GetString().isEmpty()
                          && !myYCoord->GetString().isEmpty()
                          && !myZCoord->GetString().isEmpty())));
  emit vertexDefined(enfVertexIsDefined);
}

/** 
 * This method checks if an enforced mesh is defined;
 **/
void GHS3DPluginGUI_HypothesisCreator::checkEnfMeshIsDefined()
{
  emit enfMeshDefined( myEnfVertexWdg->NbObjects() != 0);
}

/** 
 * This method resets the content of the X, Y, Z, size and GroupName widgets;
 **/
void GHS3DPluginGUI_HypothesisCreator::clearEnforcedVertexWidgets()
{
  myXCoord->setCleared(true);
  myYCoord->setCleared(true);
  myZCoord->setCleared(true);
  myXCoord->setText("");
  myYCoord->setText("");
  myZCoord->setText("");
  addVertexButton->setEnabled(false);
}

/** GHS3DPluginGUI_HypothesisCreator::updateEnforcedVertexValues(item)
    This method updates the tooltip of a modified item. The QLineEdit widgets content
    is synchronized with the coordinates of the enforced vertex clicked in the tree widget.
*/
void GHS3DPluginGUI_HypothesisCreator::updateEnforcedVertexValues(QTableWidgetItem* item)
{
  int row = myEnforcedTableWidget->row(item);
      
  QVariant vertexName = myEnforcedTableWidget->item(row,ENF_VER_NAME_COLUMN)->data(Qt::EditRole);
  QVariant x = myEnforcedTableWidget->item(row,ENF_VER_X_COLUMN)->data( Qt::EditRole);
  QVariant y = myEnforcedTableWidget->item(row,ENF_VER_Y_COLUMN)->data( Qt::EditRole);
  QVariant z = myEnforcedTableWidget->item(row,ENF_VER_Z_COLUMN)->data( Qt::EditRole);
  QVariant size = myEnforcedTableWidget->item(row,ENF_VER_SIZE_COLUMN)->data( Qt::EditRole);
  QVariant entry = myEnforcedTableWidget->item(row,ENF_VER_ENTRY_COLUMN)->data( Qt::EditRole);
  QString groupName = myEnforcedTableWidget->item(row,ENF_VER_GROUP_COLUMN)->data( Qt::EditRole).toString();
  
  clearEnforcedVertexWidgets();
  
  if ( !x.isNull() || !entry.isNull()) {
    QString toolTip = vertexName.toString();
    toolTip += QString("(");
    if (entry.isNull() || (!entry.isNull() && entry.toString() == "")) {
      toolTip += x.toString();
      toolTip += QString(", ") + y.toString();
      toolTip += QString(", ") + z.toString();
    }
    else
      toolTip += entry.toString();
    toolTip += QString(")");
    
    if (!size.isNull())
      toolTip += QString("=") + size.toString();
    
    if (!groupName.isEmpty())
      toolTip += QString(" [") + groupName + QString("]");

    for (int col=0;col<ENF_VER_NB_COLUMNS;col++)
      myEnforcedTableWidget->item(row,col)->setToolTip(toolTip);

    if (!x.isNull()) {
      disconnect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      disconnect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      disconnect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      myXCoord->SetValue(x.toDouble());
      myYCoord->SetValue(y.toDouble());
      myZCoord->SetValue(z.toDouble());
      connect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      connect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      connect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
    }
    if (!size.isNull())
      mySizeValue->SetValue(size.toDouble());
    
    if (!groupName.isEmpty())
      myGroupName->setText(groupName);
  }
}

void GHS3DPluginGUI_HypothesisCreator::onSelectEnforcedVertex()
{
  int nbSelEnfVertex = myEnfVertexWdg->NbObjects();
  clearEnforcedVertexWidgets();
  if (nbSelEnfVertex == 1)
  {
    if ( CORBA::is_nil( getGeomEngine() ) && !GeometryGUI::InitGeomGen() )
      return ;

    myEnfVertex = myEnfVertexWdg->GetObject< GEOM::GEOM_Object >(nbSelEnfVertex-1);
    if (myEnfVertex == GEOM::GEOM_Object::_nil())
      return;
    if (myEnfVertex->GetShapeType() == GEOM::VERTEX) {
      GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
      GEOM::GEOM_IMeasureOperations_var measureOp = getGeomEngine()->GetIMeasureOperations( that->getGeomSelectionTool()->getMyStudy()->StudyId() );
      if (CORBA::is_nil(measureOp))
        return;
      
      CORBA::Double x,y,z;
      measureOp->PointCoordinates (myEnfVertex, x, y, z);
      if ( measureOp->IsDone() )
      {
        disconnect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        disconnect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        disconnect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        myXCoord->SetValue(x);
        myYCoord->SetValue(y);
        myZCoord->SetValue(z);
        connect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        connect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        connect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      }
    }
  }
}

/** GHS3DPluginGUI_HypothesisCreator::synchronizeCoords()
    This method synchronizes the QLineEdit/SMESHGUI_SpinBox widgets content with the coordinates
    of the enforced vertex clicked in the tree widget.
*/
void GHS3DPluginGUI_HypothesisCreator::synchronizeCoords()
{
  clearEnforcedVertexWidgets();
  QList<QTableWidgetItem *> items = myEnforcedTableWidget->selectedItems();
  disconnect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( onSelectEnforcedVertex() ) );
  if (! items.isEmpty()) {
    QTableWidgetItem *item;
    int row;
    QVariant entry;
    if (items.size() == 1) {
      item = items[0];
      row = myEnforcedTableWidget->row(item);
      QVariant x = myEnforcedTableWidget->item(row,ENF_VER_X_COLUMN)->data( Qt::EditRole);
      QVariant y = myEnforcedTableWidget->item(row,ENF_VER_Y_COLUMN)->data( Qt::EditRole);
      QVariant z = myEnforcedTableWidget->item(row,ENF_VER_Z_COLUMN)->data( Qt::EditRole);
      QVariant size = myEnforcedTableWidget->item(row,ENF_VER_SIZE_COLUMN)->data( Qt::EditRole);
      entry = myEnforcedTableWidget->item(row,ENF_VER_ENTRY_COLUMN)->data( Qt::EditRole);
      if (!entry.isNull()) {
        SMESH::string_array_var objIds = new SMESH::string_array;
        objIds->length(1);
        objIds[0] = entry.toString().toStdString().c_str();
        myEnfVertexWdg->SetObjects(objIds);
      }
      else {
        myEnfVertexWdg->SetObject(GEOM::GEOM_Object::_nil());
      }
      QVariant group = myEnforcedTableWidget->item(row,ENF_VER_GROUP_COLUMN)->data( Qt::EditRole);
      if (!x.isNull()/* && entry.isNull()*/) {
        //         disconnect( myXCoord, SIGNAL( textChanged(const QString &)), this, SLOT( onSelectEnforcedVertex() ) );
        disconnect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        disconnect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        disconnect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        myXCoord->SetValue(x.toDouble());
        myYCoord->SetValue(y.toDouble());
        myZCoord->SetValue(z.toDouble());
        connect( myXCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        connect( myYCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
        connect( myZCoord, SIGNAL( textChanged(const QString&) ), this, SLOT( clearEnfVertexSelection() ) );
      }
      if (!size.isNull())
        mySizeValue->SetValue(size.toDouble());
      
      if (!group.isNull() && (!x.isNull() || !entry.isNull()))
        myGroupName->setText(group.toString());
    }
    else {
      QList<QString> entryList;
      for (int i = 0; i < items.size(); ++i) {
        item = items[i];
        row = myEnforcedTableWidget->row(item);
        entry = myEnforcedTableWidget->item(row,ENF_VER_ENTRY_COLUMN)->data( Qt::EditRole);
        if (!entry.isNull())
          entryList << entry.toString();
      }
      if (entryList.size() > 0) {
        SMESH::string_array_var objIds = new SMESH::string_array;
        objIds->length(entryList.size());
        for (int i = 0; i < entryList.size() ; i++)
          objIds[i] = entryList.at(i).toStdString().c_str();
        myEnfVertexWdg->SetObjects(objIds);
      }
      else {
        myEnfVertexWdg->SetObject(GEOM::GEOM_Object::_nil());
      }
    }
  }
  else {
    myEnfVertexWdg->SetObject(GEOM::GEOM_Object::_nil());
  }
  connect( myEnfVertexWdg, SIGNAL( contentModified()), this, SLOT( onSelectEnforcedVertex() ) );
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  that->checkVertexIsDefined();
}

/** GHS3DPluginGUI_HypothesisCreator::addEnforcedMesh( meshName, geomEntry, elemType, size, groupName)
    This method adds in the tree widget an enforced mesh from mesh, submesh or group with optionally size and and groupName.
*/
void GHS3DPluginGUI_HypothesisCreator::addEnforcedMesh(std::string name,
                                                       std::string entry,
                                                       int elementType,
                                                       std::string groupName)
{
  bool okToCreate = true;
  QString itemEntry = "";
  int itemElementType = 0;
  int rowCount = myEnforcedMeshTableWidget->rowCount();
  bool allColumns = true;
  for (int row = 0;row<rowCount;row++) {
    for (int col = 0 ; col < ENF_MESH_NB_COLUMNS ; col++) {
      if (col == ENF_MESH_CONSTRAINT_COLUMN){
        if (qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row, col)) == 0) {
          allColumns = false;
          break;
        }
      }
      else if (myEnforcedMeshTableWidget->item(row, col) == 0) {
        allColumns = false;
        break;
      }
      if (col == ENF_MESH_CONSTRAINT_COLUMN) {
        QComboBox* itemComboBox = qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row, col));
        itemElementType = itemComboBox->currentIndex();
      }
      else if (col == ENF_MESH_ENTRY_COLUMN)
        itemEntry = myEnforcedMeshTableWidget->item(row, col)->data(Qt::EditRole).toString();
    }
    
    if (!allColumns)
      break;
  
    if (itemEntry == QString(entry.c_str()) && itemElementType == elementType) { 
      okToCreate = false;
      break;
    } // if
  } // for

    
  if (!okToCreate)
    return;
  
  myEnforcedMeshTableWidget->setRowCount(rowCount+1);
  myEnforcedMeshTableWidget->setSortingEnabled(false);
  
  for (int col=0;col<ENF_MESH_NB_COLUMNS;col++) {
    if (col == ENF_MESH_CONSTRAINT_COLUMN) {
      QComboBox* comboBox = new QComboBox();
      QPalette pal = comboBox->palette();
      pal.setColor(QPalette::Button, Qt::white);
      comboBox->setPalette(pal);
      comboBox->insertItems(0,myEnfMeshConstraintLabels);
      comboBox->setEditable(false);
      comboBox->setCurrentIndex(elementType);
      myEnforcedMeshTableWidget->setCellWidget(rowCount,col,comboBox);
    }
    else {
      QTableWidgetItem* item = new QTableWidgetItem();
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      switch (col) {
      case ENF_MESH_NAME_COLUMN:
        item->setData( 0, name.c_str() );
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        myEnforcedMeshTableWidget->setItem(rowCount,col,item);
        break;
      case ENF_MESH_ENTRY_COLUMN:
        item->setData( 0, entry.c_str() );
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        myEnforcedMeshTableWidget->setItem(rowCount,col,item);
        break;
      case ENF_MESH_GROUP_COLUMN:
        item->setData( 0, groupName.c_str() );
        myEnforcedMeshTableWidget->setItem(rowCount,col,item);
        break;
      default:
        break;
      }
    }
  }
  myEnforcedMeshTableWidget->setSortingEnabled(true);
}

/** GHS3DPluginGUI_HypothesisCreator::addEnforcedVertex( x, y, z, size, vertexName, geomEntry, groupName)
    This method adds in the tree widget an enforced vertex with given size and coords (x,y,z) or GEOM vertex or compound and with optionally groupName.
*/
void GHS3DPluginGUI_HypothesisCreator::addEnforcedVertex(double x, double y, double z, double size, std::string vertexName, std::string geomEntry, std::string groupName, bool isCompound)
{
  myEnforcedTableWidget->disconnect(SIGNAL( itemChanged(QTableWidgetItem *)));
  bool okToCreate = true;
  double itemX,itemY,itemZ,itemSize = 0;
  QString itemEntry, itemGroupName = QString("");
  //   bool itemIsCompound;
  int rowCount = myEnforcedTableWidget->rowCount();
  QVariant data;
  bool allColumns;
  for (int row = 0;row<rowCount;row++) {
    allColumns = true;
    for (int col = 0 ; col < ENF_VER_NB_COLUMNS ; col++) {
      if (myEnforcedTableWidget->item(row, col) == 0) {
        allColumns = false;
        break;
      }
      
      data = myEnforcedTableWidget->item(row, col)->data(Qt::EditRole);
      if (!data.isNull()) {
        switch (col) {
        case ENF_VER_GROUP_COLUMN:
          itemGroupName = data.toString();
          break;
        case ENF_VER_ENTRY_COLUMN:
          itemEntry = data.toString();
          break;
        case ENF_VER_X_COLUMN:
          itemX = data.toDouble();
          break;
        case ENF_VER_Y_COLUMN:
          itemY = data.toDouble();
          break;
        case ENF_VER_Z_COLUMN:
          itemZ = data.toDouble();
          break;
        case ENF_VER_SIZE_COLUMN:
          itemSize = data.toDouble();
          break;
        default:
          break;
        }
      }
    }
    
    if (!allColumns)
      break;


    if (( !isCompound && ((itemX == x) && (itemY == y) && (itemZ == z))) ||
        ( !itemEntry.isEmpty() && ( itemEntry == geomEntry.c_str() )))
    {
      // update size
      if (itemSize != size) {
        myEnforcedTableWidget->item(row, ENF_VER_SIZE_COLUMN)->setData( Qt::EditRole, QVariant(size));
      }
      // update group name
      if (itemGroupName.toStdString() != groupName) {
        myEnforcedTableWidget->item(row, ENF_VER_GROUP_COLUMN)->setData( Qt::EditRole, QVariant(groupName.c_str()));
      }
      okToCreate = false;
      break;
    } // if
  } // for
  if (!okToCreate) {
    if (geomEntry.empty()) {
    }
    else {
    }
    return;
  }
    
  int vertexIndex=0;
  int indexRef = -1;
  QString myVertexName;
  while(indexRef != vertexIndex) {
    indexRef = vertexIndex;
    if (vertexName.empty())
      myVertexName = QString("Vertex #%1").arg(vertexIndex);
    else
      myVertexName = QString(vertexName.c_str());

    for (int row = 0;row<rowCount;row++) {
      QString name = myEnforcedTableWidget->item(row,ENF_VER_NAME_COLUMN)->data(Qt::EditRole).toString();
      if (myVertexName == name) {
        vertexIndex++;
        break;
      }
    }
  }
  
  myEnforcedTableWidget->setRowCount(rowCount+1);
  myEnforcedTableWidget->setSortingEnabled(false);
  for (int col=0;col<ENF_VER_NB_COLUMNS;col++) {
    QTableWidgetItem* item = new QTableWidgetItem();
    item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    switch (col) {
    case ENF_VER_NAME_COLUMN:
      item->setData( Qt::EditRole, myVertexName );
      if (!geomEntry.empty()) {
        if (isCompound)
          item->setIcon(QIcon(iconCompound.scaled(iconCompound.size()*0.7,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
        else
          item->setIcon(QIcon(iconVertex.scaled(iconVertex.size()*0.7,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
      }
      break;
    case ENF_VER_X_COLUMN:
      if (!isCompound)
        item->setData( 0, QVariant(x) );
      break;
    case ENF_VER_Y_COLUMN:
      if (!isCompound)
        item->setData( 0, QVariant(y) );
      break;
    case ENF_VER_Z_COLUMN:
      if (!isCompound)
        item->setData( 0, QVariant(z) );
      break;
    case ENF_VER_SIZE_COLUMN:
      item->setData( 0, QVariant(size) );
      break;
    case ENF_VER_ENTRY_COLUMN:
      if (!geomEntry.empty())
        item->setData( 0, QString(geomEntry.c_str()) );
      break;
    case ENF_VER_COMPOUND_COLUMN:
      item->setData( Qt::CheckStateRole, isCompound );
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
      break;
    case ENF_VER_GROUP_COLUMN:
      if (!groupName.empty())
        item->setData( 0, QString(groupName.c_str()) );
      break;
    default:
      break;
    }
    
    myEnforcedTableWidget->setItem(rowCount,col,item);
  }

  connect( myEnforcedTableWidget,SIGNAL( itemChanged(QTableWidgetItem *)), this,  SLOT( updateEnforcedVertexValues(QTableWidgetItem *) ) );
  
  myEnforcedTableWidget->setSortingEnabled(true);
  //   myEnforcedTableWidget->setCurrentItem(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
  updateEnforcedVertexValues(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
}

/** GHS3DPluginGUI_HypothesisCreator::onAddEnforcedMesh()
    This method is called when a item is added into the enforced meshes tree widget
*/
void GHS3DPluginGUI_HypothesisCreator::onAddEnforcedMesh()
{
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  
  that->getGeomSelectionTool()->selectionMgr()->clearFilters();
  myEnfMeshWdg->deactivateSelection();

  for (int column = 0; column < myEnforcedMeshTableWidget->columnCount(); ++column)
    myEnforcedMeshTableWidget->resizeColumnToContents(column);

  // Vertex selection
  int selEnfMeshes = myEnfMeshWdg->NbObjects();
  if (selEnfMeshes == 0)
    return;

  std::string groupName = myMeshGroupName->text().simplified().toStdString();
  
  int elementType = myEnfMeshConstraint->currentIndex();
  
  
  _PTR(Study) aStudy = SMESH::GetActiveStudyDocument();
  _PTR(SObject) aSObj;
  QString meshEntry = myEnfMeshWdg->GetValue();
  
  if (selEnfMeshes == 1)
  {
    aSObj = aStudy->FindObjectID(meshEntry.toStdString().c_str());
    CORBA::Object_var anObj = SMESH::SObjectToObject(aSObj,aStudy);
    if (!CORBA::is_nil(anObj))
      addEnforcedMesh( aSObj->GetName(), aSObj->GetID(), elementType, groupName);
  }
  else
  {
    QStringList meshEntries = meshEntry.split(" ", QString::SkipEmptyParts);
    QStringListIterator meshEntriesIt (meshEntries);
    while (meshEntriesIt.hasNext()) {
      aSObj = aStudy->FindObjectID(meshEntriesIt.next().toStdString().c_str());
      CORBA::Object_var anObj = SMESH::SObjectToObject(aSObj,aStudy);
      if (!CORBA::is_nil(anObj)) {
        addEnforcedMesh( aSObj->GetName(), aSObj->GetID(), elementType, groupName);
      }
    }
  }

  myEnfVertexWdg->SetObject(SMESH::SMESH_IDSource::_nil());
  
  for (int column = 0; column < myEnforcedMeshTableWidget->columnCount(); ++column)
    myEnforcedMeshTableWidget->resizeColumnToContents(column);  
}


/** GHS3DPluginGUI_HypothesisCreator::onAddEnforcedVertex()
    This method is called when a item is added into the enforced vertices tree widget
*/
void GHS3DPluginGUI_HypothesisCreator::onAddEnforcedVertex()
{
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  
  that->getGeomSelectionTool()->selectionMgr()->clearFilters();
  myEnfVertexWdg->deactivateSelection();

  for (int column = 0; column < myEnforcedTableWidget->columnCount(); ++column)
    myEnforcedTableWidget->resizeColumnToContents(column);

  // Vertex selection
  int selEnfVertex = myEnfVertexWdg->NbObjects();
  bool coordsEmpty = (myXCoord->text().isEmpty()) || (myYCoord->text().isEmpty()) || (myZCoord->text().isEmpty());
  if ((selEnfVertex == 0) && coordsEmpty)
    return;

  std::string groupName = myGroupName->text().simplified().toStdString();

  double size = mySizeValue->GetValue();
  
  if (selEnfVertex <= 1)
  {
    double x = 0, y = 0, z=0;
    if (myXCoord->GetString() != "") {
      x = myXCoord->GetValue();
      y = myYCoord->GetValue();
      z = myZCoord->GetValue();
    }
    if (selEnfVertex == 1) {
      myEnfVertex = myEnfVertexWdg->GetObject< GEOM::GEOM_Object >();
      std::string entry = "", name = "";
      bool isCompound = false;
      if ( !myEnfVertex->_is_nil() ) {
        entry = SMESH::toStdStr( myEnfVertex->GetStudyEntry() );
        name  = SMESH::toStdStr( myEnfVertex->GetName() );
        isCompound = ( myEnfVertex->GetShapeType() == GEOM::COMPOUND );
      }
      addEnforcedVertex(x, y, z, size, name, entry, groupName, isCompound);
    }
    else {
      addEnforcedVertex(x, y, z, size, "", "", groupName);
    }
  }
  else
  {
    if ( CORBA::is_nil(getGeomEngine()))
      return;

    GEOM::GEOM_IMeasureOperations_var measureOp = getGeomEngine()->GetIMeasureOperations( that->getGeomSelectionTool()->getMyStudy()->StudyId() );
    if (CORBA::is_nil(measureOp))
      return;

    CORBA::Double x = 0, y = 0,z = 0;
    for (int j = 0 ; j < selEnfVertex ; j++)
    {
      myEnfVertex = myEnfVertexWdg->GetObject< GEOM::GEOM_Object >(j);
      if (myEnfVertex == GEOM::GEOM_Object::_nil())
        continue;
      if (myEnfVertex->GetShapeType() == GEOM::VERTEX) {
        measureOp->PointCoordinates (myEnfVertex, x, y, z);
        if ( measureOp->IsDone() )
          addEnforcedVertex(x, y, z, size, myEnfVertex->GetName(),myEnfVertex->GetStudyEntry(), groupName);
      } else if (myEnfVertex->GetShapeType() == GEOM::COMPOUND) {
        addEnforcedVertex(0., 0., 0., size, myEnfVertex->GetName(),myEnfVertex->GetStudyEntry(), groupName, true);
      }
    }
  }

  myEnfVertexWdg->SetObject(GEOM::GEOM_Object::_nil());
  
  for (int column = 0; column < myEnforcedTableWidget->columnCount(); ++column)
    myEnforcedTableWidget->resizeColumnToContents(column);  
}

/** GHS3DPluginGUI_HypothesisCreator::onRemoveEnforcedMesh()
    This method is called when a item is removed from the enforced meshes tree widget
*/
void GHS3DPluginGUI_HypothesisCreator::onRemoveEnforcedMesh()
{
  QList<int> selectedRows;
  QList<QTableWidgetItem *> selectedItems = myEnforcedMeshTableWidget->selectedItems();
  QTableWidgetItem* item;
  int row;
  foreach( item, selectedItems ) {
    row = item->row();
    if (!selectedRows.contains( row ) )
      selectedRows.append(row);
  }

  qSort( selectedRows );
  QListIterator<int> it( selectedRows );
  it.toBack();
  while ( it.hasPrevious() ) {
    row = it.previous();
    myEnforcedMeshTableWidget->removeRow(row );
  }

  myEnforcedMeshTableWidget->selectionModel()->clearSelection();
}

/** GHS3DPluginGUI_HypothesisCreator::onRemoveEnforcedVertex()
    This method is called when a item is removed from the enforced vertices tree widget
*/
void GHS3DPluginGUI_HypothesisCreator::onRemoveEnforcedVertex()
{
  QList<int> selectedRows;
  QList<QTableWidgetItem *> selectedItems = myEnforcedTableWidget->selectedItems();
  QTableWidgetItem* item;
  int row;
  foreach( item, selectedItems ) {
    row = item->row();
    if (!selectedRows.contains( row ) )
      selectedRows.append(row);
  }
  
  qSort( selectedRows );
  QListIterator<int> it( selectedRows );
  it.toBack();
  while ( it.hasPrevious() ) {
    row = it.previous();
    myEnforcedTableWidget->removeRow(row );
  }

  myEnforcedTableWidget->selectionModel()->clearSelection();
}

void GHS3DPluginGUI_HypothesisCreator::onToMeshHoles(bool isOn)
{
  // myToMakeGroupsOfDomains->setEnabled( isOn );
  // if ( !isOn )
  //   myToMakeGroupsOfDomains->setChecked( false );
}

void GHS3DPluginGUI_HypothesisCreator::onDirBtnClicked()
{
  QString dir = SUIT_FileDlg::getExistingDirectory( dlg(), myAdvWidget->workingDirectoryLineEdit->text(), QString() );
  if ( !dir.isEmpty() )
    myAdvWidget->workingDirectoryLineEdit->setText( dir );
}

void GHS3DPluginGUI_HypothesisCreator::updateWidgets()
{
  //myToMakeGroupsOfDomains->setEnabled( myToMeshHolesCheck->isChecked() );
  myAdvWidget->maxMemorySpin->setEnabled( myAdvWidget->maxMemoryCheck->isChecked() );
  myAdvWidget->initialMemoryCheck->setEnabled( !myAdvWidget->boundaryRecoveryCheck->isChecked() );
  myAdvWidget->initialMemorySpin->setEnabled( myAdvWidget->initialMemoryCheck->isChecked() && !myAdvWidget->boundaryRecoveryCheck->isChecked() );
  myOptimizationLevelCombo->setEnabled( !myAdvWidget->boundaryRecoveryCheck->isChecked() );
  if ( sender() == myAdvWidget->logInFileCheck ||
       sender() == myAdvWidget->keepWorkingFilesCheck )
  {
    bool logFileRemovable = myAdvWidget->logInFileCheck->isChecked() &&
      !myAdvWidget->keepWorkingFilesCheck->isChecked();

    myAdvWidget->removeLogOnSuccessCheck->setEnabled( logFileRemovable );
  }
}

bool GHS3DPluginGUI_HypothesisCreator::checkParams(QString& msg) const
{
  if ( !QFileInfo( myAdvWidget->workingDirectoryLineEdit->text().trimmed() ).isWritable() ) {
    SUIT_MessageBox::warning( dlg(),
                              tr( "SMESH_WRN_WARNING" ),
                              tr( "GHS3D_PERMISSION_DENIED" ) );
    return false;
  }

  return true;
}

void GHS3DPluginGUI_HypothesisCreator::retrieveParams() const
{
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  GHS3DHypothesisData data;
  readParamsFromHypo( data );

  if ( myName )
  {
    myName->setText( data.myName );

    int width = QFontMetrics( myName->font() ).width( data.myName );
    QGridLayout* aStdLayout = (QGridLayout*) myStdGroup->layout();
    aStdLayout->setColumnMinimumWidth( 1, width + 10 );
  }
  myToMeshHolesCheck                          ->setChecked    ( data.myToMeshHoles );
  myToMakeGroupsOfDomains                     ->setChecked    ( data.myToMakeGroupsOfDomains );
  myOptimizationLevelCombo                    ->setCurrentIndex( data.myOptimizationLevel );
  myOptimizationCombo                         ->setCurrentIndex( data.myOptimization );
  mySplitOverConstrainedCombo                 ->setCurrentIndex( data.mySplitOverConstrained );
  myPThreadsModeCombo                         ->setCurrentIndex( data.myPThreadsMode );
  myNumberOfThreadsSpin                       ->setValue      ( data.myNumberOfThreads );
  mySmoothOffSliversCheck                     ->setChecked    ( data.mySmoothOffSlivers );
  myCreateNewNodesCheck                       ->setChecked    ( data.myToCreateNewNodes );

  myAdvWidget->maxMemoryCheck                 ->setChecked    ( data.myMaximumMemory > 0 );
  myAdvWidget->maxMemorySpin                  ->setValue      ( qMax( (int)data.myMaximumMemory,
                                                                      myAdvWidget->maxMemorySpin->minimum() ));
  myAdvWidget->initialMemoryCheck             ->setChecked    ( data.myInitialMemory > 0 );
  myAdvWidget->initialMemorySpin              ->setValue      ( qMax( (int)data.myInitialMemory,
                                                                      myAdvWidget->initialMemorySpin->minimum() ));
  myAdvWidget->workingDirectoryLineEdit       ->setText       ( data.myWorkingDir );
  myAdvWidget->keepWorkingFilesCheck           ->setChecked    ( data.myKeepFiles );
  myAdvWidget->verboseLevelSpin               ->setValue      ( data.myVerboseLevel );
  myAdvWidget->createNewNodesCheck            ->setChecked    ( data.myToCreateNewNodes );
  myAdvWidget->removeInitialCentralPointCheck ->setChecked    ( data.myRemoveInitialCentralPoint );
  myAdvWidget->boundaryRecoveryCheck          ->setChecked    ( data.myBoundaryRecovery );
  myAdvWidget->FEMCorrectionCheck             ->setChecked    ( data.myFEMCorrection );
  myAdvWidget->gradationSpinBox               ->setValue      ( data.myGradation );
  myAdvWidget->advOptionTable                 ->SetCustomOptions( data.myTextOption );
  myAdvWidget->logInFileCheck                 ->setChecked    ( !data.myLogInStandardOutput );
  myAdvWidget->removeLogOnSuccessCheck        ->setChecked    ( data.myRemoveLogOnSuccess );

  TEnfVertexList::const_iterator it;
  int rowCount = 0;
  myEnforcedTableWidget->clearContents();
  myEnforcedTableWidget->setSortingEnabled(false);
  myEnforcedTableWidget->disconnect(SIGNAL( itemChanged(QTableWidgetItem *)));
  for(it = data.myEnforcedVertices.begin() ; it != data.myEnforcedVertices.end(); it++ )
  {
    TEnfVertex* enfVertex = (*it);
    myEnforcedTableWidget->setRowCount(rowCount+1);

    for (int col=0;col<ENF_VER_NB_COLUMNS;col++) {
      QTableWidgetItem* item = new QTableWidgetItem();
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      switch (col) {
      case ENF_VER_NAME_COLUMN:
        item->setData( 0, enfVertex->name.c_str() );
        if (!enfVertex->geomEntry.empty()) {
          if (enfVertex->isCompound)
            item->setIcon(QIcon(iconCompound.scaled(iconCompound.size()*0.7,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
          else
            item->setIcon(QIcon(iconVertex.scaled(iconVertex.size()*0.7,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
            
        }
        break;
      case ENF_VER_X_COLUMN:
        if (!enfVertex->isCompound) {
          item->setData( 0, enfVertex->coords.at(0) );
        }
        break;
      case ENF_VER_Y_COLUMN:
        if (!enfVertex->isCompound) {
          item->setData( 0, enfVertex->coords.at(1) );
        }
        break;
      case ENF_VER_Z_COLUMN:
        if (!enfVertex->isCompound) {
          item->setData( 0, enfVertex->coords.at(2) );
        }
        break;
      case ENF_VER_SIZE_COLUMN:
        item->setData( 0, enfVertex->size );
        break;
      case ENF_VER_ENTRY_COLUMN:
        item->setData( 0, enfVertex->geomEntry.c_str() );
        break;
      case ENF_VER_COMPOUND_COLUMN:
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setData( Qt::CheckStateRole, enfVertex->isCompound );
        break;
      case ENF_VER_GROUP_COLUMN:
        item->setData( 0, enfVertex->groupName.c_str() );
        break;
      default:
        break;
      }
      
      myEnforcedTableWidget->setItem(rowCount,col,item);
    }
    that->updateEnforcedVertexValues(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
    rowCount++;
  }

  connect( myEnforcedTableWidget,SIGNAL( itemChanged(QTableWidgetItem *)), this,  SLOT( updateEnforcedVertexValues(QTableWidgetItem *) ) );
  myEnforcedTableWidget->setSortingEnabled(true);
  
  for (int column = 0; column < myEnforcedTableWidget->columnCount(); ++column)
    myEnforcedTableWidget->resizeColumnToContents(column);

  // Update Enforced meshes QTableWidget
  TEnfMeshList::const_iterator itMesh;
  rowCount = 0;
  myEnforcedMeshTableWidget->clearContents();
  myEnforcedMeshTableWidget->setSortingEnabled(false);
  //   myEnforcedMeshTableWidget->disconnect(SIGNAL( itemChanged(QTableWidgetItem *)));
  for(itMesh = data.myEnforcedMeshes.begin() ; itMesh != data.myEnforcedMeshes.end(); itMesh++ )
  {
    TEnfMesh* enfMesh = (*itMesh);
    myEnforcedMeshTableWidget->setRowCount(rowCount+1);

    for (int col=0;col<ENF_MESH_NB_COLUMNS;col++) {
      if (col == ENF_MESH_CONSTRAINT_COLUMN) {
        QComboBox* comboBox = new QComboBox();
        QPalette pal = comboBox->palette();
        pal.setColor(QPalette::Button, Qt::white);
        comboBox->setPalette(pal);
        comboBox->insertItems(0,myEnfMeshConstraintLabels);
        comboBox->setEditable(false);
        comboBox->setCurrentIndex(enfMesh->elementType);
        myEnforcedMeshTableWidget->setCellWidget(rowCount,col,comboBox);
      }
      else {
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        switch (col) {
        case ENF_MESH_NAME_COLUMN:
          item->setData( 0, enfMesh->name.c_str() );
          item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        case ENF_MESH_ENTRY_COLUMN:
          item->setData( 0, enfMesh->entry.c_str() );
          item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        case ENF_MESH_GROUP_COLUMN:
          item->setData( 0, enfMesh->groupName.c_str() );
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        default:
          break;
        }
      }
      
    }
    rowCount++;
  }

  myEnforcedMeshTableWidget->setSortingEnabled(true);
  
  for (int col=0;col<ENF_MESH_NB_COLUMNS;col++)
    myEnforcedMeshTableWidget->resizeColumnToContents(col);
  
  that->updateWidgets();
  that->checkVertexIsDefined();
}

QString GHS3DPluginGUI_HypothesisCreator::storeParams() const
{
  GHS3DHypothesisData data;
  readParamsFromWidgets( data );
  storeParamsToHypo( data );
    
  QString valStr = "";
    
  if ( !data.myBoundaryRecovery )
    valStr = " --components " + data.myToMeshHoles ? "all" : "outside_components" ;
    
  if ( data.myOptimizationLevel >= 0 && data.myOptimizationLevel < 5 && !data.myBoundaryRecovery) {
    const char* level[] = { "none" , "light" , "standard" , "standard+" , "strong" };
    valStr += " --optimisation_level ";
    valStr += level[ data.myOptimizationLevel ];
  }
  if ( data.myMaximumMemory > 0 ) {
    valStr += " --max_memory ";
    valStr += QString::number( data.myMaximumMemory );
  }
  if ( data.myInitialMemory > 0 && !data.myBoundaryRecovery ) {
    valStr += " --automatic_memory ";
    valStr += QString::number( data.myInitialMemory );
  }
  valStr += " --verbose ";
  valStr += QString::number( data.myVerboseLevel );
    
  if ( !data.myToCreateNewNodes )
    valStr += " --no_internal_points";
    
  if ( data.myRemoveInitialCentralPoint )
    valStr += " --no_initial_central_point";
    
  if ( data.myBoundaryRecovery )
    valStr += " -C";
    
  if ( data.myFEMCorrection )
    valStr += " -FEM";
    
  if ( data.myGradation != 1.05 ) {
    valStr += " -Dcpropa=";
    valStr += QString::number( data.myGradation );
  }
    
  valStr += " ";
  valStr += data.myTextOption;

  return valStr;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromHypo( GHS3DHypothesisData& h_data ) const
{
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( initParamsHypothesis() );
  GHS3DPlugin::GHS3DPlugin_OptimizerHypothesis_var opt =
    GHS3DPlugin::GHS3DPlugin_OptimizerHypothesis::_narrow( initParamsHypothesis() );

  HypothesisData* data = SMESH::GetHypothesisData( hypType() );
  h_data.myName = isCreation() && data ? hypName() : "";

  if ( !opt->_is_nil() )
  {
    h_data.myOptimization         = opt->GetOptimization();
    h_data.mySplitOverConstrained = opt->GetSplitOverConstrained();
    h_data.myPThreadsMode         = opt->GetPThreadsMode();
    h_data.myNumberOfThreads      = opt->GetMaximalNumberOfThreads();
    h_data.mySmoothOffSlivers     = opt->GetSmoothOffSlivers();
  }
  h_data.myToMeshHoles                = h->GetToMeshHoles();
  h_data.myToMakeGroupsOfDomains      = h->GetToMakeGroupsOfDomains();
  h_data.myMaximumMemory              = h->GetMaximumMemory();
  h_data.myInitialMemory              = h->GetInitialMemory();
  h_data.myInitialMemory              = h->GetInitialMemory();
  h_data.myOptimizationLevel          = h->GetOptimizationLevel();
  h_data.myKeepFiles                  = h->GetKeepFiles();
  h_data.myWorkingDir                 = h->GetWorkingDirectory();
  h_data.myVerboseLevel               = h->GetVerboseLevel();
  h_data.myToCreateNewNodes           = h->GetToCreateNewNodes();
  h_data.myRemoveInitialCentralPoint  = h->GetToRemoveCentralPoint();
  h_data.myBoundaryRecovery           = h->GetToUseBoundaryRecoveryVersion();
  h_data.myFEMCorrection              = h->GetFEMCorrection();
  h_data.myGradation                  = h->GetGradation();
  h_data.myTextOption                 = h->GetAdvancedOption();
  h_data.myLogInStandardOutput        = h->GetStandardOutputLog();
  h_data.myRemoveLogOnSuccess         = h->GetRemoveLogOnSuccess();
  
  GHS3DPlugin::GHS3DEnforcedVertexList_var vertices = h->GetEnforcedVertices();
  h_data.myEnforcedVertices.clear();
  for (CORBA::ULong i=0 ; i<vertices->length() ; i++) {
    TEnfVertex* myVertex = new TEnfVertex();
    myVertex->name = CORBA::string_dup(vertices[i].name.in());
    myVertex->geomEntry = CORBA::string_dup(vertices[i].geomEntry.in());
    myVertex->groupName = CORBA::string_dup(vertices[i].groupName.in());
    myVertex->size = vertices[i].size;
    myVertex->isCompound = vertices[i].isCompound;
    if (vertices[i].coords.length()) {
      for (CORBA::ULong c = 0; c < vertices[i].coords.length() ; c++)
        myVertex->coords.push_back(vertices[i].coords[c]);
    }
    h_data.myEnforcedVertices.insert(myVertex);
  }
  
  GHS3DPlugin::GHS3DEnforcedMeshList_var enfMeshes = h->GetEnforcedMeshes();
  h_data.myEnforcedMeshes.clear();
  for (CORBA::ULong i=0 ; i<enfMeshes->length() ; i++) {
    TEnfMesh* myEnfMesh = new TEnfMesh();
    myEnfMesh->name = CORBA::string_dup(enfMeshes[i].name.in());
    myEnfMesh->entry = CORBA::string_dup(enfMeshes[i].entry.in());
    myEnfMesh->groupName = CORBA::string_dup(enfMeshes[i].groupName.in());
    switch (enfMeshes[i].elementType) {
    case SMESH::NODE:
      myEnfMesh->elementType = 0;
      break;
    case SMESH::EDGE:
      myEnfMesh->elementType = 1;
      break;
    case SMESH::FACE:
      myEnfMesh->elementType = 2;
      break;
    default:
      break;
    }
    //     myEnfMesh->elementType = enfMeshes[i].elementType;
    h_data.myEnforcedMeshes.insert(myEnfMesh);
  }
  return true;
}

bool GHS3DPluginGUI_HypothesisCreator::storeParamsToHypo( const GHS3DHypothesisData& h_data ) const
{
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( hypothesis() );
  GHS3DPlugin::GHS3DPlugin_OptimizerHypothesis_var opt =
    GHS3DPlugin::GHS3DPlugin_OptimizerHypothesis::_narrow( initParamsHypothesis() );

  bool ok = true;
  try
  {
    if( isCreation() )
      SMESH::SetName( SMESH::FindSObject( h ), h_data.myName.toLatin1().constData() );

    if ( h->GetToMeshHoles() != h_data.myToMeshHoles ) // avoid duplication of DumpPython commands
      h->SetToMeshHoles      ( h_data.myToMeshHoles       );
    if ( h->GetToMakeGroupsOfDomains() != h_data.myToMakeGroupsOfDomains )
      h->SetToMakeGroupsOfDomains( h_data.myToMakeGroupsOfDomains );
    if ( h->GetMaximumMemory() != h_data.myMaximumMemory )
      h->SetMaximumMemory    ( h_data.myMaximumMemory     );
    if ( h->GetInitialMemory() != h_data.myInitialMemory )
      h->SetInitialMemory    ( h_data.myInitialMemory     );
    if ( h->GetInitialMemory() != h_data.myInitialMemory )
      h->SetInitialMemory    ( h_data.myInitialMemory     );
    if ( h->GetOptimizationLevel() != h_data.myOptimizationLevel )
      h->SetOptimizationLevel( h_data.myOptimizationLevel );
    if ( h->GetKeepFiles() != h_data.myKeepFiles         )
      h->SetKeepFiles        ( h_data.myKeepFiles         );
    if ( h->GetWorkingDirectory() != h_data.myWorkingDir )
      h->SetWorkingDirectory ( h_data.myWorkingDir.toLatin1().constData() );
    if ( h->GetVerboseLevel() != h_data.myVerboseLevel   )
      h->SetVerboseLevel     ( h_data.myVerboseLevel      );
    if ( h->GetToCreateNewNodes() != h_data.myToCreateNewNodes )
      h->SetToCreateNewNodes( h_data.myToCreateNewNodes   );
    if ( h->GetToRemoveCentralPoint() != h_data.myRemoveInitialCentralPoint )
      h->SetToRemoveCentralPoint( h_data.myRemoveInitialCentralPoint );
    if ( h->GetToUseBoundaryRecoveryVersion() != h_data.myBoundaryRecovery )
      h->SetToUseBoundaryRecoveryVersion( h_data.myBoundaryRecovery );
    if ( h->GetFEMCorrection() != h_data.myFEMCorrection )
      h->SetFEMCorrection    ( h_data.myFEMCorrection     );
    if ( h->GetGradation() != h_data.myGradation         )
      h->SetGradation        ( h_data.myGradation         );
    if ( h->GetAdvancedOption() != h_data.myTextOption       )
      h->SetAdvancedOption       ( h_data.myTextOption.toLatin1().constData() );
    if ( h->GetStandardOutputLog() != h_data.myLogInStandardOutput   )
      h->SetStandardOutputLog( h_data.myLogInStandardOutput  );
    if ( h->GetRemoveLogOnSuccess() != h_data.myRemoveLogOnSuccess   )
      h->SetRemoveLogOnSuccess( h_data.myRemoveLogOnSuccess  );

    if ( !opt->_is_nil() )
    {
      opt->SetOptimization          ( (GHS3DPlugin::Mode) h_data.myOptimization );
      opt->SetSplitOverConstrained  ( (GHS3DPlugin::Mode) h_data.mySplitOverConstrained );
      opt->SetPThreadsMode          ( (GHS3DPlugin::PThreadsMode) h_data.myPThreadsMode );
      opt->SetSmoothOffSlivers      ( h_data.mySmoothOffSlivers );
      opt->SetMaximalNumberOfThreads( h_data.myNumberOfThreads );
    }

    // Enforced vertices
    GHS3DPlugin::GHS3DEnforcedVertexList_var vertexHyp = h->GetEnforcedVertices();
    int nbVertexHyp = vertexHyp->length();

    // 1. Clear all enforced vertices in hypothesis
    // 2. Add new enforced vertex according to h_data
    if ( nbVertexHyp > 0)
      h->ClearEnforcedVertices();

    TEnfVertexList::const_iterator it;
    double x = 0, y = 0, z = 0;
    for(it = h_data.myEnforcedVertices.begin() ; it != h_data.myEnforcedVertices.end(); it++ ) {
      TEnfVertex* enfVertex = (*it);
      x =y =z = 0;
      if (enfVertex->coords.size()) {
        x = enfVertex->coords.at(0);
        y = enfVertex->coords.at(1);
        z = enfVertex->coords.at(2);
      }
      ok = h->p_SetEnforcedVertex( enfVertex->size, x, y, z, enfVertex->name.c_str(), enfVertex->geomEntry.c_str(), enfVertex->groupName.c_str(), enfVertex->isCompound);
    } // for

    // Enforced Meshes
    GHS3DPlugin::GHS3DEnforcedMeshList_var enfMeshListHyp = h->GetEnforcedMeshes();
    int nbEnfMeshListHyp = enfMeshListHyp->length();

    // 1. Clear all enforced vertices in hypothesis
    // 2. Add new enforced vertex according to h_data
    if ( nbEnfMeshListHyp > 0)
      h->ClearEnforcedMeshes();

    TEnfMeshList::const_iterator itEnfMesh;

    _PTR(Study) aStudy = SMESH::GetActiveStudyDocument();

    for(itEnfMesh = h_data.myEnforcedMeshes.begin() ; itEnfMesh != h_data.myEnforcedMeshes.end(); itEnfMesh++ ) {
      TEnfMesh* enfMesh = (*itEnfMesh);

      _PTR(SObject) aSObj = aStudy->FindObjectID(enfMesh->entry.c_str());
      SMESH::SMESH_IDSource_var theSource = SMESH::SObjectToInterface<SMESH::SMESH_IDSource>( aSObj );

      SMESH::ElementType elementType;
      switch(enfMesh->elementType) {
      case 0:
        elementType = SMESH::NODE;
        break;
      case 1:
        elementType = SMESH::EDGE;
        break;
      case 2:
        elementType = SMESH::FACE;
        break;
      default:
        break;
      }
    
      ok = h->p_SetEnforcedMesh(theSource, elementType, enfMesh->name.c_str(), enfMesh->groupName.c_str());
    } // for
  } // try
  catch ( const SALOME::SALOME_Exception& ex )
  {
    SalomeApp_Tools::QtCatchCorbaException( ex );
    ok = false;
  }
  return ok;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromWidgets( GHS3DHypothesisData& h_data ) const
{
  h_data.myName                       = myName ? myName->text() : "";
  h_data.myToMeshHoles                = myToMeshHolesCheck->isChecked();
  h_data.myToMakeGroupsOfDomains      = myToMakeGroupsOfDomains->isChecked();
  h_data.myOptimization               = myOptimizationCombo->currentIndex();
  h_data.myOptimizationLevel          = myOptimizationLevelCombo->currentIndex();
  h_data.mySplitOverConstrained       = mySplitOverConstrainedCombo->currentIndex();
  h_data.myPThreadsMode               = myPThreadsModeCombo->currentIndex();
  h_data.myNumberOfThreads            = myNumberOfThreadsSpin->value();
  h_data.mySmoothOffSlivers           = mySmoothOffSliversCheck->isChecked();
  h_data.myMaximumMemory              = myAdvWidget->maxMemoryCheck->isChecked() ? myAdvWidget->maxMemorySpin->value() : -1;
  h_data.myInitialMemory              = myAdvWidget->initialMemoryCheck->isChecked() ? myAdvWidget->initialMemorySpin->value() : -1;
  h_data.myKeepFiles                  = myAdvWidget->keepWorkingFilesCheck->isChecked();
  h_data.myWorkingDir                 = myAdvWidget->workingDirectoryLineEdit->text().trimmed();
  h_data.myVerboseLevel               = myAdvWidget->verboseLevelSpin->value();
  h_data.myRemoveInitialCentralPoint  = myAdvWidget->removeInitialCentralPointCheck->isChecked();
  h_data.myBoundaryRecovery           = myAdvWidget->boundaryRecoveryCheck->isChecked();
  h_data.myFEMCorrection              = myAdvWidget->FEMCorrectionCheck->isChecked();
  h_data.myGradation                  = myAdvWidget->gradationSpinBox->value();
  h_data.myTextOption                 = myAdvWidget->advOptionTable->GetCustomOptions();
  h_data.myLogInStandardOutput        = !myAdvWidget->logInFileCheck->isChecked();
  h_data.myRemoveLogOnSuccess         = myAdvWidget->removeLogOnSuccessCheck->isChecked();
  if ( isOptimization() )
    h_data.myToCreateNewNodes         = myCreateNewNodesCheck->isChecked();
  else
    h_data.myToCreateNewNodes         = myAdvWidget->createNewNodesCheck->isChecked();
  
  // Enforced vertices
  h_data.myEnforcedVertices.clear();
  QVariant valueX, valueY, valueZ;
  for (int row=0 ; row<myEnforcedTableWidget->rowCount() ; row++)
  {
    TEnfVertex *myVertex = new TEnfVertex();
    myVertex->name = myEnforcedTableWidget->item(row,ENF_VER_NAME_COLUMN)->data(Qt::EditRole).toString().toStdString();
    myVertex->geomEntry = myEnforcedTableWidget->item(row,ENF_VER_ENTRY_COLUMN)->data(Qt::EditRole).toString().toStdString();
    myVertex->groupName = myEnforcedTableWidget->item(row,ENF_VER_GROUP_COLUMN)->data(Qt::EditRole).toString().toStdString();
    valueX = myEnforcedTableWidget->item(row,ENF_VER_X_COLUMN)->data(Qt::EditRole);
    valueY = myEnforcedTableWidget->item(row,ENF_VER_Y_COLUMN)->data(Qt::EditRole);
    valueZ = myEnforcedTableWidget->item(row,ENF_VER_Z_COLUMN)->data(Qt::EditRole);
    if (!valueX.isNull() && !valueY.isNull() && !valueZ.isNull()) {
      myVertex->coords.push_back(valueX.toDouble());
      myVertex->coords.push_back(valueY.toDouble());
      myVertex->coords.push_back(valueZ.toDouble());
    }
    myVertex->size = myEnforcedTableWidget->item(row,ENF_VER_SIZE_COLUMN)->data(Qt::EditRole).toDouble();
    myVertex->isCompound = myEnforcedTableWidget->item(row,ENF_VER_COMPOUND_COLUMN)->data(Qt::CheckStateRole).toBool();
    h_data.myEnforcedVertices.insert(myVertex);
  }
  
  // Enforced meshes
  h_data.myEnforcedMeshes.clear();

  for (int row=0 ; row<myEnforcedMeshTableWidget->rowCount() ; row++)
  {
    TEnfMesh *myEnfMesh = new TEnfMesh();
    myEnfMesh->name = myEnforcedMeshTableWidget->item(row,ENF_MESH_NAME_COLUMN)->data(Qt::EditRole).toString().toStdString();
    myEnfMesh->entry = myEnforcedMeshTableWidget->item(row,ENF_MESH_ENTRY_COLUMN)->data(Qt::EditRole).toString().toStdString();
    myEnfMesh->groupName = myEnforcedMeshTableWidget->item(row,ENF_MESH_GROUP_COLUMN)->data(Qt::EditRole).toString().toStdString();
    QComboBox* combo = qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row,ENF_MESH_CONSTRAINT_COLUMN));
    myEnfMesh->elementType = combo->currentIndex();
    h_data.myEnforcedMeshes.insert(myEnfMesh);
  }

  return true;
}

QString GHS3DPluginGUI_HypothesisCreator::caption() const
{
  return tr( "GHS3D_TITLE" );
}

QPixmap GHS3DPluginGUI_HypothesisCreator::icon() const
{
  return SUIT_Session::session()->resourceMgr()->loadPixmap( "GHS3DPlugin", tr( "ICON_DLG_GHS3D_PARAMETERS" ) );
}

QString GHS3DPluginGUI_HypothesisCreator::type() const
{
  return tr( isOptimization() ? "GHS3D_OPTIMIZATIOL_HYPOTHESIS" : "GHS3D_HYPOTHESIS" );
}

QString GHS3DPluginGUI_HypothesisCreator::helpPage() const
{
  return isOptimization() ? "optimization_page.html" : "ghs3d_hypo_page.html";
}
