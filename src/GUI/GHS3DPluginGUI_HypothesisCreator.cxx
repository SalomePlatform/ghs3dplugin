// Copyright (C) 2004-2013  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
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

#include <GeometryGUI.h>

#include <SMESHGUI_Utils.h>
#include <SMESHGUI_SpinBox.h>
#include <SMESHGUI_HypothesesUtils.h>
#include <SMESH_NumberFilter.hxx>
#include <SMESH_TypeFilter.hxx>
#include <StdMeshersGUI_ObjectReferenceParamWdg.h>

#include <LightApp_SelectionMgr.h>
#include <SUIT_Session.h>
#include <SUIT_MessageBox.h>
#include <SUIT_ResourceMgr.h>
#include <SUIT_FileDlg.h>
#include <SalomeApp_Tools.h>
#include <SalomeApp_TypeFilter.h>

#include <TopoDS_Iterator.hxx>

#include <QComboBox>
#include <QPalette>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QFileInfo>
#include <QGroupBox>

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

#include <stdexcept>
#include <utilities.h>

#include <boost/algorithm/string.hpp>

namespace {

#ifdef WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

  int maxAvailableMemory()
  {
#ifdef WIN32
    // See http://msdn.microsoft.com/en-us/library/aa366589.aspx
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    int err = GlobalMemoryStatusEx (&statex);
    if (err != 0) {
      int totMB = 
        statex.ullTotalPhys / 1024 / 1024 +
        statex.ullTotalPageFile / 1024 / 1024 +
        statex.ullTotalVirtual / 1024 / 1024;
      return (int) ( 0.7 * totMB );
    }
#else
    struct sysinfo si;
    int err = sysinfo( &si );
    if ( err == 0 ) {
      int totMB =
        si.totalram * si.mem_unit / 1024 / 1024 +
        si.totalswap * si.mem_unit / 1024 / 1024 ;
      return (int) ( 0.7 * totMB );
    }
#endif
    return 0;
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
//   else if (index.column() == ENF_VER_COMPOUND_COLUMN) {
//     QCheckBox *editor = new QCheckBox(parent);
//     editor->setDisabled(!entry.isEmpty());
//     return editor;
//   }
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
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
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
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

// bool EnforcedMeshTableWidgetDelegate::enfMeshExists(QAbstractItemModel *model,
//                                               const QModelIndex &index, 
//                                               QString value) const
// {
//   bool exists = false;
//   QModelIndex parent = index.parent();
//   int row = index.row();
//   int col = index.column();
//   return exists;
// }

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
  if( isCreation() )
  {
    aStdLayout->addWidget( new QLabel( tr( "SMESH_NAME" ), myStdGroup ), row, 0, 1, 1 );
    myName = new QLineEdit( myStdGroup );
    aStdLayout->addWidget( myName, row++, 1, 1, 1 );
  }

  myToMeshHolesCheck = new QCheckBox( tr( "GHS3D_TO_MESH_HOLES" ), myStdGroup );
  aStdLayout->addWidget( myToMeshHolesCheck, row, 0, 1, 1 );
  myToMakeGroupsOfDomains = new QCheckBox( tr( "GHS3D_TO_MAKE_DOMAIN_GROUPS" ), myStdGroup );
  aStdLayout->addWidget( myToMakeGroupsOfDomains, row++, 1, 1, 1 );

  aStdLayout->addWidget( new QLabel( tr( "GHS3D_OPTIMIZATIOL_LEVEL" ), myStdGroup ), row, 0, 1, 1 );
  myOptimizationLevelCombo = new QComboBox( myStdGroup );
  aStdLayout->addWidget( myOptimizationLevelCombo, row++, 1, 1, 1 );

  QStringList types;
  types << tr( "LEVEL_NONE" ) << tr( "LEVEL_LIGHT" ) << tr( "LEVEL_MEDIUM" ) << tr( "LEVEL_STANDARDPLUS" ) << tr( "LEVEL_STRONG" );
  myOptimizationLevelCombo->addItems( types );

  aStdLayout->setRowStretch( row, 10 );

  // advanced parameters
  myAdvGroup = new QWidget();
  QGridLayout* anAdvLayout = new QGridLayout( myAdvGroup );
  anAdvLayout->setSpacing( 6 );
  anAdvLayout->setMargin( 11 );
  
  myMaximumMemoryCheck = new QCheckBox( tr( "MAX_MEMORY_SIZE" ), myAdvGroup );
  myMaximumMemorySpin = new QSpinBox( myAdvGroup );
  myMaximumMemorySpin->setMinimum( 1 );
  myMaximumMemorySpin->setMaximum( maxAvailableMemory() );
  myMaximumMemorySpin->setSingleStep( 10 );
  QLabel* aMegabyteLabel = new QLabel( tr( "MEGABYTE" ), myAdvGroup );

  myInitialMemoryCheck = new QCheckBox( tr( "INIT_MEMORY_SIZE" ), myAdvGroup );
  myInitialMemorySpin = new QSpinBox( myAdvGroup );
  myInitialMemorySpin->setMinimum( 1 );
  myInitialMemorySpin->setMaximum( maxAvailableMemory() );
  myInitialMemorySpin->setSingleStep( 10 );
  QLabel* aMegabyteLabel2 = new QLabel( tr( "MEGABYTE" ), myAdvGroup );

  QLabel* aWorkinDirLabel = new QLabel( tr( "WORKING_DIR" ), myAdvGroup );
  myWorkingDir = new QLineEdit( myAdvGroup );
  //myWorkingDir->setReadOnly( true );
  QPushButton* dirBtn = new QPushButton( tr( "SELECT_DIR" ), myAdvGroup );
  dirBtn->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
  
  myKeepFiles = new QCheckBox( tr( "KEEP_WORKING_FILES" ), myAdvGroup );

  QLabel* aVerboseLevelLabel = new QLabel( tr( "VERBOSE_LEVEL" ), myAdvGroup );
  myVerboseLevelSpin = new QSpinBox( myAdvGroup );
  myVerboseLevelSpin->setMinimum( 0 );
  myVerboseLevelSpin->setMaximum( 10 );
  myVerboseLevelSpin->setSingleStep( 1 );

  myToCreateNewNodesCheck = new QCheckBox( tr( "TO_ADD_NODES" ), myAdvGroup );
  
  myRemoveInitialCentralPointCheck = new QCheckBox( tr( "NO_INITIAL_CENTRAL_POINT" ), myAdvGroup );
  
  myBoundaryRecoveryCheck = new QCheckBox( tr( "RECOVERY_VERSION" ), myAdvGroup );
  
  myFEMCorrectionCheck = new QCheckBox( tr( "FEM_CORRECTION" ), myAdvGroup );
  
  QLabel* myGradationLabel = new QLabel( tr( "GHS3D_GRADATION" ), myAdvGroup );
  myGradation = new SMESHGUI_SpinBox(myAdvGroup);
  myGradation->RangeStepAndValidator(1.05, 5.0, 0.05, "length_precision");

  QLabel* aTextOptionLabel = new QLabel( tr( "TEXT_OPTION" ), myAdvGroup );
  myTextOption = new QLineEdit( myAdvGroup );

  row = 0;
  anAdvLayout->addWidget( myMaximumMemoryCheck,             row, 0, 1, 1 );
  anAdvLayout->addWidget( myMaximumMemorySpin,              row, 1, 1, 1 );
  anAdvLayout->addWidget( aMegabyteLabel,                   row++, 2, 1, 1 );
  anAdvLayout->addWidget( myInitialMemoryCheck,             row, 0, 1, 1 );
  anAdvLayout->addWidget( myInitialMemorySpin,              row, 1, 1, 1 );
  anAdvLayout->addWidget( aMegabyteLabel2,                  row++, 2, 1, 1 );
  anAdvLayout->addWidget( aWorkinDirLabel,                  row, 0, 1, 1 );
  anAdvLayout->addWidget( myWorkingDir,                     row, 1, 1, 2 );
  anAdvLayout->addWidget( dirBtn,                           row++, 3, 1, 1 );
  anAdvLayout->addWidget( myKeepFiles,                      row++, 0, 1, 4 );
  anAdvLayout->addWidget( aVerboseLevelLabel,               row, 0, 1, 1 );
  anAdvLayout->addWidget( myVerboseLevelSpin,               row++, 1, 1, 2 );
  anAdvLayout->addWidget( myToCreateNewNodesCheck,          row++, 0, 1, 4 );
  anAdvLayout->addWidget( myRemoveInitialCentralPointCheck, row++, 0, 1, 4 );
  anAdvLayout->addWidget( myBoundaryRecoveryCheck,          row++, 0, 1, 4 );
  anAdvLayout->addWidget( myFEMCorrectionCheck,             row++, 0, 1, 4 );
  anAdvLayout->addWidget( myGradationLabel,                 row, 0, 1, 1 );
  anAdvLayout->addWidget( myGradation,                      row++, 1, 1, 2 );
  anAdvLayout->addWidget( aTextOptionLabel,                 row, 0, 1, 1 );
  anAdvLayout->addWidget( myTextOption,                     row++, 1, 1, 2 );

  // Enforced vertices parameters
  myEnfGroup = new QWidget();
  QGridLayout* anEnfLayout = new QGridLayout(myEnfGroup);
  
  myEnforcedTableWidget = new QTableWidget(myEnfGroup);
  myEnforcedTableWidget ->setMinimumWidth(300);
  myEnforcedTableWidget->setRowCount( 0 );
  myEnforcedTableWidget->setColumnCount( ENF_VER_NB_COLUMNS );
  myEnforcedTableWidget->setSortingEnabled(true);
  QStringList enforcedHeaders;
  enforcedHeaders << tr( "GHS3D_ENF_NAME_COLUMN" )
                  << tr( "GHS3D_ENF_VER_X_COLUMN" )<< tr( "GHS3D_ENF_VER_Y_COLUMN" ) << tr( "GHS3D_ENF_VER_Z_COLUMN" )
                  << tr( "GHS3D_ENF_SIZE_COLUMN" ) << tr("GHS3D_ENF_ENTRY_COLUMN") << tr("GHS3D_ENF_VER_COMPOUND_COLUMN") << tr( "GHS3D_ENF_GROUP_COLUMN" );

  myEnforcedTableWidget->setHorizontalHeaderLabels(enforcedHeaders);
  myEnforcedTableWidget->verticalHeader()->hide();
  myEnforcedTableWidget->horizontalHeader()->setStretchLastSection(true);
  myEnforcedTableWidget->setAlternatingRowColors(true);
  myEnforcedTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  myEnforcedTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
  myEnforcedTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  myEnforcedTableWidget->resizeColumnsToContents();
  myEnforcedTableWidget->hideColumn(ENF_VER_ENTRY_COLUMN);
  myEnforcedTableWidget->hideColumn(ENF_VER_COMPOUND_COLUMN);
  
  myEnforcedTableWidget->setItemDelegate(new EnforcedVertexTableWidgetDelegate());
  
// VERTEX SELECTION
  TColStd_MapOfInteger shapeTypes;
  shapeTypes.Add( TopAbs_VERTEX );
  shapeTypes.Add( TopAbs_COMPOUND );

  SMESH_NumberFilter* vertexFilter = new SMESH_NumberFilter("GEOM", TopAbs_SHAPE, 1, shapeTypes);
  myEnfVertexWdg = new StdMeshersGUI_ObjectReferenceParamWdg( vertexFilter, 0, /*multiSel=*/true, /*stretch=*/false);
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
//   myGlobalGroupName = new QCheckBox(tr("GHS3D_ENF_VER_GROUPS"), myEnfGroup);
//   myGlobalGroupName->setChecked(false);
  
  QGroupBox* GroupBox = new QGroupBox( myEnfGroup );
  QLabel* info = new QLabel( GroupBox );
  info->setText( tr( "GHS3D_ENF_VER_INFO" ) );
  info->setWordWrap( true );
  QVBoxLayout* GroupBoxVLayout = new QVBoxLayout( GroupBox );
  GroupBoxVLayout->setSpacing( 6 );
  GroupBoxVLayout->setMargin( 11 );
  GroupBoxVLayout->addWidget( info );
  

  anEnfLayout->addWidget(GroupBox,                  ENF_VER_WARNING, 0, 1, 2 );
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
  myEnforcedMeshTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  myEnforcedMeshTableWidget->setAlternatingRowColors(true);
  myEnforcedMeshTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  myEnforcedMeshTableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
  myEnforcedMeshTableWidget->resizeColumnsToContents();
  myEnforcedMeshTableWidget->hideColumn(ENF_MESH_ENTRY_COLUMN);
  
  myEnforcedMeshTableWidget->setItemDelegate(new EnforcedMeshTableWidgetDelegate());
  
//   myEnfMesh = SMESH::SMESH_Mesh::_nil();
//   myEnfMeshArray = new SMESH::mesh_array();

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
//   addEnfMeshButton->setEnabled(false);
  removeEnfMeshButton = new QPushButton(tr("GHS3D_ENF_REMOVE"),myEnfMeshGroup);
    
  QGroupBox* GroupBox2 = new QGroupBox( myEnfMeshGroup );
  QLabel* info2 = new QLabel( GroupBox2 );
  info2->setText( tr( "GHS3D_ENF_MESH_INFO" ) );
  info2->setWordWrap( true );
  QVBoxLayout* GroupBox2VLayout = new QVBoxLayout( GroupBox2 );
  GroupBox2VLayout->setSpacing( 6 );
  GroupBox2VLayout->setMargin( 11 );
  GroupBox2VLayout->addWidget( info2 );
  
  anEnfMeshLayout->addWidget( GroupBox2,                ENF_MESH_WARNING, 0, 1, 2 );
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
  tab->insertTab( ENF_VER_TAB, myEnfGroup, tr( "GHS3D_ENFORCED_VERTICES" ) );
  tab->insertTab( ENF_MESH_TAB, myEnfMeshGroup, tr( "GHS3D_ENFORCED_MESHES" ) );
  tab->setCurrentIndex( STD_TAB );

  // connections
  //connect( myToMeshHolesCheck,      SIGNAL( toggled( bool ) ), this, SLOT( onToMeshHoles(bool)));
  connect( myMaximumMemoryCheck,    SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myInitialMemoryCheck,    SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( myBoundaryRecoveryCheck, SIGNAL( toggled( bool ) ), this, SLOT( updateWidgets() ) );
  connect( dirBtn,                  SIGNAL( clicked() ),       this, SLOT( onDirBtnClicked() ) );
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
//   connect( myEnfMeshWdg,            SIGNAL( contentModified()),              this,  SLOT( checkEnfMeshIsDefined() ) );
//   connect( myEnfMeshConstraint,     SIGNAL( currentIndexChanged(int) ),      this,  SLOT( checkEnfMeshIsDefined() ) );
//   connect( this,                    SIGNAL( enfMeshDefined(bool) ), addEnfMeshButton, SLOT( setEnabled(bool) ) );
  
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
//   mySizeValue->setCleared(true);
  myXCoord->setText("");
  myYCoord->setText("");
  myZCoord->setText("");
//   mySizeValue->setText("");
//   myGroupName->setText("");
  addVertexButton->setEnabled(false);
}

/** GHS3DPluginGUI_HypothesisCreator::updateEnforcedVertexValues(item)
This method updates the tooltip of a modified item. The QLineEdit widgets content
is synchronized with the coordinates of the enforced vertex clicked in the tree widget.
*/
void GHS3DPluginGUI_HypothesisCreator::updateEnforcedVertexValues(QTableWidgetItem* item) {
//   MESSAGE("GHS3DPluginGUI_HypothesisCreator::updateEnforcedVertexValues");
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

//     MESSAGE("Tooltip: " << toolTip.toStdString());
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

void GHS3DPluginGUI_HypothesisCreator::onSelectEnforcedVertex() {
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
void GHS3DPluginGUI_HypothesisCreator::synchronizeCoords() {
  clearEnforcedVertexWidgets();
  QList<QTableWidgetItem *> items = myEnforcedTableWidget->selectedItems();
//   myEnfVertexWdg->disconnect(SIGNAL(contentModified()));
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
void GHS3DPluginGUI_HypothesisCreator::addEnforcedMesh(std::string name, std::string entry, int elementType, std::string groupName)
{
  MESSAGE("addEnforcedMesh(\"" << name << ", \"" << entry << "\", " << elementType << ", \"" << groupName << "\")");
  bool okToCreate = true;
  QString itemEntry = "";
  int itemElementType = 0;
  int rowCount = myEnforcedMeshTableWidget->rowCount();
  bool allColumns = true;
  for (int row = 0;row<rowCount;row++) {
    for (int col = 0 ; col < ENF_MESH_NB_COLUMNS ; col++) {
      MESSAGE("col: " << col);
      if (col == ENF_MESH_CONSTRAINT_COLUMN){
        if (qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row, col)) == 0) {
          allColumns = false;
          MESSAGE("allColumns = false");
          break;
        }
      }
      else if (myEnforcedMeshTableWidget->item(row, col) == 0) {
        allColumns = false;
        MESSAGE("allColumns = false");
        break;
      }
      if (col == ENF_MESH_CONSTRAINT_COLUMN) {
        QComboBox* itemComboBox = qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row, col));
        itemElementType = itemComboBox->currentIndex();
        MESSAGE("itemElementType: " << itemElementType);
      }
      else if (col == ENF_MESH_ENTRY_COLUMN)
        itemEntry = myEnforcedMeshTableWidget->item(row, col)->data(Qt::EditRole).toString();
    }
    
    if (!allColumns)
      break;
  
    if (itemEntry == QString(entry.c_str()) && itemElementType == elementType) { 
//       // update group name
//       if (itemGroupName.toStdString() != groupName) {
//         MESSAGE("Group is updated from \"" << itemGroupName.toStdString() << "\" to \"" << groupName << "\"");
//         myEnforcedMeshTableWidget->item(row, ENF_MESH_GROUP_COLUMN)->setData( Qt::EditRole, QVariant(groupName.c_str()));
//       }
      okToCreate = false;
      break;
    } // if
  } // for

    
  if (!okToCreate)
    return;
  
  MESSAGE("Creation of enforced mesh");

  myEnforcedMeshTableWidget->setRowCount(rowCount+1);
  myEnforcedMeshTableWidget->setSortingEnabled(false);
  
  for (int col=0;col<ENF_MESH_NB_COLUMNS;col++) {
    MESSAGE("Column: " << col);
    if (col == ENF_MESH_CONSTRAINT_COLUMN) {
      QComboBox* comboBox = new QComboBox();
      QPalette pal = comboBox->palette();
      pal.setColor(QPalette::Button, Qt::white);
      comboBox->setPalette(pal);
      comboBox->insertItems(0,myEnfMeshConstraintLabels);
      comboBox->setEditable(false);
      comboBox->setCurrentIndex(elementType);
      MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << comboBox->currentText().toStdString());
      myEnforcedMeshTableWidget->setCellWidget(rowCount,col,comboBox);
    }
    else {
      QTableWidgetItem* item = new QTableWidgetItem();
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      switch (col) {
        case ENF_MESH_NAME_COLUMN:
          item->setData( 0, name.c_str() );
          item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        case ENF_MESH_ENTRY_COLUMN:
          item->setData( 0, entry.c_str() );
          item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        case ENF_MESH_GROUP_COLUMN:
          item->setData( 0, groupName.c_str() );
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          myEnforcedMeshTableWidget->setItem(rowCount,col,item);
          break;
        default:
          break;
      }
    }
    MESSAGE("Done");
  }

//   connect( myEnforcedMeshTableWidget,SIGNAL( itemChanged(QTableWidgetItem *)), this,  SLOT( updateEnforcedVertexValues(QTableWidgetItem *) ) );
  
  myEnforcedMeshTableWidget->setSortingEnabled(true);
//   myEnforcedTableWidget->setCurrentItem(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
//   updateEnforcedVertexValues(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
    
}

/** GHS3DPluginGUI_HypothesisCreator::addEnforcedVertex( x, y, z, size, vertexName, geomEntry, groupName)
This method adds in the tree widget an enforced vertex with given size and coords (x,y,z) or GEOM vertex or compound and with optionally groupName.
*/
void GHS3DPluginGUI_HypothesisCreator::addEnforcedVertex(double x, double y, double z, double size, std::string vertexName, std::string geomEntry, std::string groupName, bool isCompound)
{
  MESSAGE("addEnforcedVertex(" << x << ", " << y << ", " << z << ", " << size << ", \"" << vertexName << ", \"" << geomEntry << "\", \"" << groupName << "\", " << isCompound << ")");
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
//           case ENF_VER_COMPOUND_COLUMN:
//             itemIsCompound = data.toBool();
//             break;
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


    if (( !isCompound && ((itemX == x) && (itemY == y) && (itemZ == z))) || /*( (itemEntry.toStdString() != "") && */ (itemEntry.toStdString() == geomEntry)/*)*/) {
      // update size
      if (itemSize != size) {
        MESSAGE("Size is updated from \"" << itemSize << "\" to \"" << size << "\"");
        myEnforcedTableWidget->item(row, ENF_VER_SIZE_COLUMN)->setData( Qt::EditRole, QVariant(size));
      }
      // update group name
      if (itemGroupName.toStdString() != groupName) {
        MESSAGE("Group is updated from \"" << itemGroupName.toStdString() << "\" to \"" << groupName << "\"");
        myEnforcedTableWidget->item(row, ENF_VER_GROUP_COLUMN)->setData( Qt::EditRole, QVariant(groupName.c_str()));
      }
      okToCreate = false;
      break;
    } // if
  } // for
  if (!okToCreate) {
    if (geomEntry.empty()) {
      MESSAGE("Vertex with coords " << x << ", " << y << ", " << z << " already exist: dont create again");
    }
    else {
      MESSAGE("Vertex with entry " << geomEntry << " already exist: dont create again");
    }
    return;
  }
    
  if (geomEntry.empty()) {
    MESSAGE("Vertex with coords " << x << ", " << y << ", " << z<< " is created");
  }
  else {
    MESSAGE("Vertex with geom entry " << geomEntry << " is created");
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
  
  MESSAGE("myVertexName is \"" << myVertexName.toStdString() << "\"");
  myEnforcedTableWidget->setRowCount(rowCount+1);
  myEnforcedTableWidget->setSortingEnabled(false);
  for (int col=0;col<ENF_VER_NB_COLUMNS;col++) {
    MESSAGE("Column: " << col);
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
    
    MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
    myEnforcedTableWidget->setItem(rowCount,col,item);
    MESSAGE("Done");
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
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::onAddEnforcedMesh()");

  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  
  that->getGeomSelectionTool()->selectionMgr()->clearFilters();
  myEnfMeshWdg->deactivateSelection();

  for (int column = 0; column < myEnforcedMeshTableWidget->columnCount(); ++column)
    myEnforcedMeshTableWidget->resizeColumnToContents(column);

  // Vertex selection
  int selEnfMeshes = myEnfMeshWdg->NbObjects();
  if (selEnfMeshes == 0)
    return;

  std::string groupName = myMeshGroupName->text().toStdString();
//   if (myGlobalGroupName->isChecked())
//     groupName = myGlobalGroupName->text().toStdString();

  if (boost::trim_copy(groupName).empty())
    groupName = "";

  
  int elementType = myEnfMeshConstraint->currentIndex();
  
  
  _PTR(Study) aStudy = SMESH::GetActiveStudyDocument();
  _PTR(SObject) aSObj; //SMESH::SMESH_IDSource::_nil;
  QString meshEntry = myEnfMeshWdg->GetValue();
  MESSAGE("myEnfMeshWdg->GetValue()" << meshEntry.toStdString());
  
  if (selEnfMeshes == 1)
  {
    MESSAGE("1 SMESH object selected");
//     myEnfMesh = myEnfMeshWdg->GetObject< SMESH::SMESH_IDSource >();
//     std::string entry = myEnfMeshWdg->GetValue();
    aSObj = aStudy->FindObjectID(meshEntry.toStdString().c_str());
    CORBA::Object_var anObj = SMESH::SObjectToObject(aSObj,aStudy);
    if (!CORBA::is_nil(anObj)) {
//       SMESH::SMESH_IDSource_var theSource = SMESH::SObjectToInterface<SMESH::SMESH_IDSource>( aSObj );
      addEnforcedMesh( aSObj->GetName(), aSObj->GetID(), elementType, groupName);
    }
  }
  else
  {
    MESSAGE(selEnfMeshes << " SMESH objects selected");
    QStringList meshEntries = meshEntry.split(" ", QString::SkipEmptyParts);
    QStringListIterator meshEntriesIt (meshEntries);
    while (meshEntriesIt.hasNext()) {
      aSObj = aStudy->FindObjectID(meshEntriesIt.next().toStdString().c_str());
      CORBA::Object_var anObj = SMESH::SObjectToObject(aSObj,aStudy);
      if (!CORBA::is_nil(anObj)) {
//         SMESH::SMESH_IDSource_var theSource = SMESH::SObjectToInterface<SMESH::SMESH_IDSource>( aSObj );
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
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::onAddEnforcedVertex()");

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

  std::string groupName = myGroupName->text().toStdString();
//   if (myGlobalGroupName->isChecked())
//     groupName = myGlobalGroupName->text().toStdString();

  if (boost::trim_copy(groupName).empty())
    groupName = "";

  double size = mySizeValue->GetValue();
  
  if (selEnfVertex <= 1)
  {
    MESSAGE("0 or 1 GEOM object selected");
    double x = 0, y = 0, z=0;
    if (myXCoord->GetString() != "") {
      x = myXCoord->GetValue();
      y = myYCoord->GetValue();
      z = myZCoord->GetValue();
    }
    if (selEnfVertex == 1) {
      MESSAGE("1 GEOM object selected");
      myEnfVertex = myEnfVertexWdg->GetObject< GEOM::GEOM_Object >();
      std::string entry = "";
      if (myEnfVertex != GEOM::GEOM_Object::_nil())
        entry = myEnfVertex->GetStudyEntry();
      addEnforcedVertex(x, y, z, size, myEnfVertex->GetName(),entry, groupName, myEnfVertex->GetShapeType() == GEOM::COMPOUND);
    }
    else {
      MESSAGE("0 GEOM object selected");
      MESSAGE("Coords: ("<<x<<","<<y<<","<<z<<")");
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
      MESSAGE("delete row #"<< row);
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
      MESSAGE("delete row #"<< row);
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
  QString dir = SUIT_FileDlg::getExistingDirectory( dlg(), myWorkingDir->text(), QString() );
  if ( !dir.isEmpty() )
    myWorkingDir->setText( dir );
}

void GHS3DPluginGUI_HypothesisCreator::updateWidgets()
{
  //myToMakeGroupsOfDomains->setEnabled( myToMeshHolesCheck->isChecked() );
  myMaximumMemorySpin->setEnabled( myMaximumMemoryCheck->isChecked() );
  myInitialMemoryCheck->setEnabled( !myBoundaryRecoveryCheck->isChecked() );
  myInitialMemorySpin->setEnabled( myInitialMemoryCheck->isChecked() && !myBoundaryRecoveryCheck->isChecked() );
  myOptimizationLevelCombo->setEnabled( !myBoundaryRecoveryCheck->isChecked() );
}

bool GHS3DPluginGUI_HypothesisCreator::checkParams(QString& msg) const
{
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::checkParams");

  if ( !QFileInfo( myWorkingDir->text().trimmed() ).isWritable() ) {
    SUIT_MessageBox::warning( dlg(),
                              tr( "SMESH_WRN_WARNING" ),
                              tr( "GHS3D_PERMISSION_DENIED" ) );
    return false;
  }

  return true;
}

void GHS3DPluginGUI_HypothesisCreator::retrieveParams() const
{
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::retrieveParams");
  GHS3DPluginGUI_HypothesisCreator* that = (GHS3DPluginGUI_HypothesisCreator*)this;
  GHS3DHypothesisData data;
  readParamsFromHypo( data );

  if ( myName )
    myName->setText( data.myName );
  
  myToMeshHolesCheck               ->setChecked    ( data.myToMeshHoles );
  myToMakeGroupsOfDomains          ->setChecked    ( data.myToMakeGroupsOfDomains );
  myOptimizationLevelCombo         ->setCurrentIndex( data.myOptimizationLevel );
  myMaximumMemoryCheck             ->setChecked    ( data.myMaximumMemory > 0 );
  myMaximumMemorySpin              ->setValue      ( qMax( data.myMaximumMemory,
                                                           myMaximumMemorySpin->minimum() ));
  myInitialMemoryCheck             ->setChecked    ( data.myInitialMemory > 0 );
  myInitialMemorySpin              ->setValue      ( qMax( data.myInitialMemory,
                                                           myInitialMemorySpin->minimum() ));
  myWorkingDir                     ->setText       ( data.myWorkingDir );
  myKeepFiles                      ->setChecked    ( data.myKeepFiles );
  myVerboseLevelSpin               ->setValue      ( data.myVerboseLevel );
  myToCreateNewNodesCheck          ->setChecked    ( data.myToCreateNewNodes );
  myRemoveInitialCentralPointCheck ->setChecked    ( data.myRemoveInitialCentralPoint );
  myBoundaryRecoveryCheck          ->setChecked    ( data.myBoundaryRecovery );
  myFEMCorrectionCheck             ->setChecked    ( data.myFEMCorrection );
  myGradation                      ->setValue      ( data.myGradation );
  myTextOption                     ->setText       ( data.myTextOption );

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
      MESSAGE("Column: " << col);
//       MESSAGE("enfVertex->isCompound: " << enfVertex->isCompound);
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
            
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          }
          break;
        case ENF_VER_X_COLUMN:
          if (!enfVertex->isCompound) {
            item->setData( 0, enfVertex->coords.at(0) );
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          }
          break;
        case ENF_VER_Y_COLUMN:
          if (!enfVertex->isCompound) {
            item->setData( 0, enfVertex->coords.at(1) );
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          }
          break;
        case ENF_VER_Z_COLUMN:
          if (!enfVertex->isCompound) {
            item->setData( 0, enfVertex->coords.at(2) );
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          }
          break;
        case ENF_VER_SIZE_COLUMN:
          item->setData( 0, enfVertex->size );
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          break;
        case ENF_VER_ENTRY_COLUMN:
          item->setData( 0, enfVertex->geomEntry.c_str() );
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          break;
        case ENF_VER_COMPOUND_COLUMN:
          item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
          item->setData( Qt::CheckStateRole, enfVertex->isCompound );
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << enfVertex->isCompound);
          break;
        case ENF_VER_GROUP_COLUMN:
          item->setData( 0, enfVertex->groupName.c_str() );
          MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
          break;
        default:
          break;
      }
      
      myEnforcedTableWidget->setItem(rowCount,col,item);
      MESSAGE("Done");
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
      MESSAGE("Column: " << col);
      if (col == ENF_MESH_CONSTRAINT_COLUMN) {
        QComboBox* comboBox = new QComboBox();
        QPalette pal = comboBox->palette();
        pal.setColor(QPalette::Button, Qt::white);
        comboBox->setPalette(pal);
        comboBox->insertItems(0,myEnfMeshConstraintLabels);
        comboBox->setEditable(false);
        comboBox->setCurrentIndex(enfMesh->elementType);
        MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << comboBox->currentText().toStdString());
        myEnforcedMeshTableWidget->setCellWidget(rowCount,col,comboBox);
      }
      else {
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        switch (col) {
          case ENF_MESH_NAME_COLUMN:
            item->setData( 0, enfMesh->name.c_str() );
            item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
            myEnforcedMeshTableWidget->setItem(rowCount,col,item);
            break;
          case ENF_MESH_ENTRY_COLUMN:
            item->setData( 0, enfMesh->entry.c_str() );
            item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
            myEnforcedMeshTableWidget->setItem(rowCount,col,item);
            break;
          case ENF_MESH_GROUP_COLUMN:
            item->setData( 0, enfMesh->groupName.c_str() );
            MESSAGE("Add item in table at (" << rowCount << "," << col << "): " << item->text().toStdString());
            myEnforcedMeshTableWidget->setItem(rowCount,col,item);
            break;
          default:
            break;
        }
      }
      
//       myEnforcedMeshTableWidget->setItem(rowCount,col,item);
      MESSAGE("Done");
    }
//     that->updateEnforcedVertexValues(myEnforcedTableWidget->item(rowCount,ENF_VER_NAME_COLUMN));
    rowCount++;
  }

//   connect( myEnforcedMeshTableWidget,SIGNAL( itemChanged(QTableWidgetItem *)), this,  SLOT( updateEnforcedVertexValues(QTableWidgetItem *) ) );
  myEnforcedMeshTableWidget->setSortingEnabled(true);
  
  for (int col=0;col<ENF_MESH_NB_COLUMNS;col++)
    myEnforcedMeshTableWidget->resizeColumnToContents(col);
  
  that->updateWidgets();
  that->checkVertexIsDefined();
}

QString GHS3DPluginGUI_HypothesisCreator::storeParams() const
{
    MESSAGE("GHS3DPluginGUI_HypothesisCreator::storeParams");
    GHS3DHypothesisData data;
    readParamsFromWidgets( data );
    storeParamsToHypo( data );
    
    QString valStr = "";
    
    if ( !data.myBoundaryRecovery )
        valStr = "-c " + QString::number( !data.myToMeshHoles );
    
    if ( data.myOptimizationLevel >= 0 && data.myOptimizationLevel < 5 && !data.myBoundaryRecovery) {
        const char* level[] = { "none" , "light" , "standard" , "standard+" , "strong" };
        valStr += " -o ";
        valStr += level[ data.myOptimizationLevel ];
    }
    if ( data.myMaximumMemory > 0 ) {
        valStr += " -m ";
        valStr += QString::number( data.myMaximumMemory );
    }
    if ( data.myInitialMemory > 0 && !data.myBoundaryRecovery ) {
        valStr += " -M ";
        valStr += QString::number( data.myInitialMemory );
    }
    valStr += " -v ";
    valStr += QString::number( data.myVerboseLevel );
    
    if ( !data.myToCreateNewNodes )
        valStr += " -p0";
    
    if ( data.myRemoveInitialCentralPoint )
        valStr += " -no_initial_central_point";
    
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
    
//     valStr += " #BEGIN ENFORCED VERTICES#";
//     // Add size map parameters storage
//     for (int i=0 ; i<mySmpModel->rowCount() ; i++) {
//         valStr += " (";
//         double x = mySmpModel->data(mySmpModel->index(i,ENF_VER_X_COLUMN)).toDouble();
//         double y = mySmpModel->data(mySmpModel->index(i,ENF_VER_Y_COLUMN)).toDouble();
//         double z = mySmpModel->data(mySmpModel->index(i,ENF_VER_Z_COLUMN)).toDouble();
//         double size = mySmpModel->data(mySmpModel->index(i,ENF_VER_SIZE_COLUMN)).toDouble();
//         valStr += QString::number( x );
//         valStr += ",";
//         valStr += QString::number( y );
//         valStr += ",";
//         valStr += QString::number( z );
//         valStr += ")=";
//         valStr += QString::number( size );
//         if (i!=mySmpModel->rowCount()-1)
//             valStr += ";";
//     }
//     valStr += " #END ENFORCED VERTICES#";
//     MESSAGE(valStr.toStdString());
  return valStr;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromHypo( GHS3DHypothesisData& h_data ) const
{
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::readParamsFromHypo");
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( initParamsHypothesis() );

  HypothesisData* data = SMESH::GetHypothesisData( hypType() );
  h_data.myName = isCreation() && data ? hypName() : "";

  h_data.myToMeshHoles                = h->GetToMeshHoles();
  h_data.myToMakeGroupsOfDomains      = /*h->GetToMeshHoles() &&*/ h->GetToMakeGroupsOfDomains();
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
  h_data.myTextOption                 = h->GetTextOption();
  
  GHS3DPlugin::GHS3DEnforcedVertexList_var vertices = h->GetEnforcedVertices();
  MESSAGE("vertices->length(): " << vertices->length());
  h_data.myEnforcedVertices.clear();
  for (int i=0 ; i<vertices->length() ; i++) {
    TEnfVertex* myVertex = new TEnfVertex();
    myVertex->name = CORBA::string_dup(vertices[i].name.in());
    myVertex->geomEntry = CORBA::string_dup(vertices[i].geomEntry.in());
    myVertex->groupName = CORBA::string_dup(vertices[i].groupName.in());
    myVertex->size = vertices[i].size;
    myVertex->isCompound = vertices[i].isCompound;
    if (vertices[i].coords.length()) {
      for (int c = 0; c < vertices[i].coords.length() ; c++)
        myVertex->coords.push_back(vertices[i].coords[c]);
      MESSAGE("Add enforced vertex ("<< myVertex->coords.at(0) << ","<< myVertex->coords.at(1) << ","<< myVertex->coords.at(2) << ") ="<< myVertex->size);
    }
    h_data.myEnforcedVertices.insert(myVertex);
  }
  
  GHS3DPlugin::GHS3DEnforcedMeshList_var enfMeshes = h->GetEnforcedMeshes();
  MESSAGE("enfMeshes->length(): " << enfMeshes->length());
  h_data.myEnforcedMeshes.clear();
  for (int i=0 ; i<enfMeshes->length() ; i++) {
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
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::storeParamsToHypo");
  GHS3DPlugin::GHS3DPlugin_Hypothesis_var h =
    GHS3DPlugin::GHS3DPlugin_Hypothesis::_narrow( hypothesis() );

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
    if ( h->GetKeepFiles() != h_data.myKeepFiles )
      h->SetKeepFiles        ( h_data.myKeepFiles         );
    if ( h->GetWorkingDirectory() != h_data.myWorkingDir )
      h->SetWorkingDirectory ( h_data.myWorkingDir.toLatin1().constData() );
    if ( h->GetVerboseLevel() != h_data.myVerboseLevel )
      h->SetVerboseLevel     ( h_data.myVerboseLevel );
    if ( h->GetToCreateNewNodes() != h_data.myToCreateNewNodes )
      h->SetToCreateNewNodes( h_data.myToCreateNewNodes );
    if ( h->GetToRemoveCentralPoint() != h_data.myRemoveInitialCentralPoint )
      h->SetToRemoveCentralPoint( h_data.myRemoveInitialCentralPoint );
    if ( h->GetToUseBoundaryRecoveryVersion() != h_data.myBoundaryRecovery )
      h->SetToUseBoundaryRecoveryVersion( h_data.myBoundaryRecovery );
    if ( h->GetFEMCorrection() != h_data.myFEMCorrection )
      h->SetFEMCorrection( h_data.myFEMCorrection );
    if ( h->GetGradation() != h_data.myGradation )
      h->SetGradation     ( h_data.myGradation );
    if ( h->GetTextOption() != h_data.myTextOption )
      h->SetTextOption       ( h_data.myTextOption.toLatin1().constData() );
    
    // Enforced vertices
    int nbVertex = (int) h_data.myEnforcedVertices.size();
    GHS3DPlugin::GHS3DEnforcedVertexList_var vertexHyp = h->GetEnforcedVertices();
    int nbVertexHyp = vertexHyp->length();
    
    MESSAGE("Store params for size maps: " << nbVertex << " enforced vertices");
    MESSAGE("h->GetEnforcedVertices()->length(): " << nbVertexHyp);
    
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
    int nbEnfMeshes = (int) h_data.myEnforcedMeshes.size();
    GHS3DPlugin::GHS3DEnforcedMeshList_var enfMeshListHyp = h->GetEnforcedMeshes();
    int nbEnfMeshListHyp = enfMeshListHyp->length();
    
    MESSAGE("Store params for size maps: " << nbEnfMeshes << " enforced meshes");
    MESSAGE("h->GetEnforcedMeshes()->length(): " << nbEnfMeshListHyp);
    
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

      MESSAGE("enfMesh->elementType: " << enfMesh->elementType);
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
    
      std::cout << "h->p_SetEnforcedMesh(theSource, "<< elementType <<", \""<< enfMesh->name << "\", \"" << enfMesh->groupName.c_str() <<"\")"<<std::endl;
      ok = h->p_SetEnforcedMesh(theSource, elementType, enfMesh->name.c_str(), enfMesh->groupName.c_str());
    } // for
  } // try
//   catch(const std::exception& ex) {
//     std::cout << "Exception: " << ex.what() << std::endl;
//     throw ex;
//   }
  catch ( const SALOME::SALOME_Exception& ex )
  {
    SalomeApp_Tools::QtCatchCorbaException( ex );
    ok = false;
  }
  return ok;
}

bool GHS3DPluginGUI_HypothesisCreator::readParamsFromWidgets( GHS3DHypothesisData& h_data ) const
{
  MESSAGE("GHS3DPluginGUI_HypothesisCreator::readParamsFromWidgets");
  h_data.myName                       = myName ? myName->text() : "";
  h_data.myToMeshHoles                = myToMeshHolesCheck->isChecked();
  h_data.myToMakeGroupsOfDomains      = myToMakeGroupsOfDomains->isChecked();
  h_data.myMaximumMemory              = myMaximumMemoryCheck->isChecked() ? myMaximumMemorySpin->value() : -1;
  h_data.myInitialMemory              = myInitialMemoryCheck->isChecked() ? myInitialMemorySpin->value() : -1;
  h_data.myOptimizationLevel          = myOptimizationLevelCombo->currentIndex();
  h_data.myKeepFiles                  = myKeepFiles->isChecked();
  h_data.myWorkingDir                 = myWorkingDir->text().trimmed();
  h_data.myVerboseLevel               = myVerboseLevelSpin->value();
  h_data.myToCreateNewNodes           = myToCreateNewNodesCheck->isChecked();
  h_data.myRemoveInitialCentralPoint  = myRemoveInitialCentralPointCheck->isChecked();
  h_data.myBoundaryRecovery           = myBoundaryRecoveryCheck->isChecked();
  h_data.myFEMCorrection              = myFEMCorrectionCheck->isChecked();
  h_data.myGradation                  = myGradation->value();
  h_data.myTextOption                 = myTextOption->text();
  
  // Enforced vertices
  h_data.myEnforcedVertices.clear();
  QVariant valueX, valueY, valueZ;
  for (int row=0 ; row<myEnforcedTableWidget->rowCount() ; row++) {
    
    TEnfVertex *myVertex = new TEnfVertex();
    myVertex->name = myEnforcedTableWidget->item(row,ENF_VER_NAME_COLUMN)->data(Qt::EditRole).toString().toStdString();
    MESSAGE("Add new enforced vertex \"" << myVertex->name << "\"" );
    myVertex->geomEntry = myEnforcedTableWidget->item(row,ENF_VER_ENTRY_COLUMN)->data(Qt::EditRole).toString().toStdString();
    if (myVertex->geomEntry.size())
      MESSAGE("Geom entry is \"" << myVertex->geomEntry << "\"" );
    myVertex->groupName = myEnforcedTableWidget->item(row,ENF_VER_GROUP_COLUMN)->data(Qt::EditRole).toString().toStdString();
    if (myVertex->groupName.size())
      MESSAGE("Group name is \"" << myVertex->groupName << "\"" );
    valueX = myEnforcedTableWidget->item(row,ENF_VER_X_COLUMN)->data(Qt::EditRole);
    valueY = myEnforcedTableWidget->item(row,ENF_VER_Y_COLUMN)->data(Qt::EditRole);
    valueZ = myEnforcedTableWidget->item(row,ENF_VER_Z_COLUMN)->data(Qt::EditRole);
    if (!valueX.isNull() && !valueY.isNull() && !valueZ.isNull()) {
      myVertex->coords.push_back(valueX.toDouble());
      myVertex->coords.push_back(valueY.toDouble());
      myVertex->coords.push_back(valueZ.toDouble());
      MESSAGE("Coords are (" << myVertex->coords.at(0) << ", "
                             << myVertex->coords.at(1) << ", "
                             << myVertex->coords.at(2) << ")");
    }
    myVertex->size = myEnforcedTableWidget->item(row,ENF_VER_SIZE_COLUMN)->data(Qt::EditRole).toDouble();
    MESSAGE("Size is " << myVertex->size);
    myVertex->isCompound = myEnforcedTableWidget->item(row,ENF_VER_COMPOUND_COLUMN)->data(Qt::CheckStateRole).toBool();
    MESSAGE("Is compound ? " << myVertex->isCompound);
    h_data.myEnforcedVertices.insert(myVertex);
  }
  
  // Enforced meshes
  h_data.myEnforcedMeshes.clear();

  for (int row=0 ; row<myEnforcedMeshTableWidget->rowCount() ; row++) {
    
    TEnfMesh *myEnfMesh = new TEnfMesh();
    myEnfMesh->name = myEnforcedMeshTableWidget->item(row,ENF_MESH_NAME_COLUMN)->data(Qt::EditRole).toString().toStdString();
    MESSAGE("Add new enforced mesh \"" << myEnfMesh->name << "\"" );
    myEnfMesh->entry = myEnforcedMeshTableWidget->item(row,ENF_MESH_ENTRY_COLUMN)->data(Qt::EditRole).toString().toStdString();
    MESSAGE("Entry is \"" << myEnfMesh->entry << "\"" );
    myEnfMesh->groupName = myEnforcedMeshTableWidget->item(row,ENF_MESH_GROUP_COLUMN)->data(Qt::EditRole).toString().toStdString();
    MESSAGE("Group name is \"" << myEnfMesh->groupName << "\"" );
    QComboBox* combo = qobject_cast<QComboBox*>(myEnforcedMeshTableWidget->cellWidget(row,ENF_MESH_CONSTRAINT_COLUMN));
    myEnfMesh->elementType = combo->currentIndex();
    MESSAGE("Element type: " << myEnfMesh->elementType);
    h_data.myEnforcedMeshes.insert(myEnfMesh);
    std::cout << "h_data.myEnforcedMeshes.size(): " << h_data.myEnforcedMeshes.size() << std::endl;
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
  return tr( "GHS3D_HYPOTHESIS" );
}

QString GHS3DPluginGUI_HypothesisCreator::helpPage() const
{
  return "ghs3d_hypo_page.html";
}
