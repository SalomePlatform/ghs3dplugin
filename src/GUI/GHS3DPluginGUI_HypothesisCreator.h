//  Copyright (C) 2004-2010  CEA/DEN, EDF R&D
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

//  GHS3DPlugin GUI: GUI for plugged-in mesher GHS3DPlugin
//  File   : GHS3DPluginGUI_HypothesisCreator.h
//  Author : Michael Zorin
//  Module : GHS3DPlugin
//
#ifndef GHS3DPLUGINGUI_HypothesisCreator_HeaderFile
#define GHS3DPLUGINGUI_HypothesisCreator_HeaderFile

#ifdef WIN32
  #if defined GHS3DPluginGUI_EXPORTS
    #define GHS3DPLUGINGUI_EXPORT __declspec( dllexport )
  #else
    #define GHS3DPLUGINGUI_EXPORT __declspec( dllimport )
  #endif
#else
  #define GHS3DPLUGINGUI_EXPORT
#endif

#include <SMESHGUI_Hypotheses.h>
// #include <SalomeApp_DoubleSpinBox.h>

#include <QItemDelegate>
#include <map>
#include <vector>
#include CORBA_SERVER_HEADER(GHS3DPlugin_Algorithm)

class QWidget;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QStandardItemModel;
class QTableView;
class QHeaderView;
class QDoubleSpinBox;

class LightApp_SelectionMgr;

typedef std::vector<double> GHS3DEnforcedVertex;
typedef std::vector<GHS3DEnforcedVertex> TEnforcedVertexValues;

typedef struct
{
  bool    myToMeshHoles,myKeepFiles,myToCreateNewNodes,myBoundaryRecovery,myFEMCorrection,myRemoveInitialCentralPoint;
  int     myMaximumMemory,myInitialMemory,myOptimizationLevel;
  QString myName,myWorkingDir,myTextOption;
  short   myVerboseLevel;
  TEnforcedVertexValues myEnforcedVertices;
} GHS3DHypothesisData;

/*!
  \brief Class for creation of GHS3D2D and GHS3D3D hypotheses
*/
class GHS3DPLUGINGUI_EXPORT GHS3DPluginGUI_HypothesisCreator : public SMESHGUI_GenericHypothesisCreator
{
  Q_OBJECT

public:
  GHS3DPluginGUI_HypothesisCreator( const QString& );
  virtual ~GHS3DPluginGUI_HypothesisCreator();

  virtual bool     checkParams(QString& msg) const;
  virtual QString  helpPage() const;

protected:
  virtual QFrame*  buildFrame    ();
  virtual void     retrieveParams() const;
  virtual QString  storeParams   () const;

  virtual QString  caption() const;
  virtual QPixmap  icon() const;
  virtual QString  type() const;

protected slots:
  void                onDirBtnClicked();
  void                updateWidgets();
  void                onVertexBtnClicked();
  void                onRemoveVertexBtnClicked();
  bool                checkVertexIsDefined();

signals:
  void                vertexDefined(bool);

private:
  bool                readParamsFromHypo( GHS3DHypothesisData& ) const;
  bool                readParamsFromWidgets( GHS3DHypothesisData& ) const;
  bool                storeParamsToHypo( const GHS3DHypothesisData& ) const;
  bool                smpVertexExists(double, double, double) const;

private:
  QWidget*            myStdGroup;
  QLineEdit*          myName;
  QCheckBox*          myToMeshHolesCheck;
  QComboBox*          myOptimizationLevelCombo;

  QWidget*            myAdvGroup;
  QCheckBox*          myMaximumMemoryCheck;
  QSpinBox*           myMaximumMemorySpin;
  QCheckBox*          myInitialMemoryCheck;
  QSpinBox*           myInitialMemorySpin;
  QLineEdit*          myWorkingDir;
  QCheckBox*          myKeepFiles;
  QSpinBox*           myVerboseLevelSpin;
  QCheckBox*          myToCreateNewNodesCheck;
  QCheckBox*          myRemoveInitialCentralPointCheck;
  QCheckBox*          myBoundaryRecoveryCheck;
  QCheckBox*          myFEMCorrectionCheck;
QLineEdit*            myTextOption;
  
  QWidget*            myEnfGroup;
  QStandardItemModel* mySmpModel;
  QTableView*         myEnforcedTableView;
  QLineEdit*          myXCoord;
  QLineEdit*          myYCoord;
  QLineEdit*          myZCoord;
  QLineEdit*          mySizeValue;
  QPushButton*        addVertexButton;
  QPushButton*        removeVertexButton;
  
  LightApp_SelectionMgr*  mySelectionMgr;          /* User shape selection */
//   SVTK_Selector*          mySelector;
};

class DoubleLineEditDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    DoubleLineEditDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif
