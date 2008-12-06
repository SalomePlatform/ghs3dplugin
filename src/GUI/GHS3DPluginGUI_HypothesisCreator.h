//  Copyright (C) 2004-2008  CEA/DEN, EDF R&D
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

#include "GHS3DPlugin_Defs.hxx"
#include <SMESHGUI_Hypotheses.h>

class QWidget;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QSpinBox;

typedef struct
{
  bool    myToMeshHoles;
  int     myMaximumMemory;
  int     myInitialMemory;
  int     myOptimizationLevel;
  bool    myKeepFiles;
  QString myWorkingDir;
  QString myName;
  short   myVerboseLevel;
  bool    myToCreateNewNodes;
  bool    myBoundaryRecovery;
  QString myTextOption;

} GHS3DHypothesisData;

/*!
  \brief Class for creation of GHS3D2D and GHS3D3D hypotheses
*/
class GHS3DPLUGIN_EXPORT GHS3DPluginGUI_HypothesisCreator : public SMESHGUI_GenericHypothesisCreator
{
  Q_OBJECT

public:
  GHS3DPluginGUI_HypothesisCreator( const QString& );
  virtual ~GHS3DPluginGUI_HypothesisCreator();

  virtual bool     checkParams() const;
  virtual QString  helpPage() const;

protected:
  virtual QFrame*  buildFrame    ();
  virtual void     retrieveParams() const;
  virtual QString  storeParams   () const;

  virtual QString  caption() const;
  virtual QPixmap  icon() const;
  virtual QString  type() const;

protected slots:
  void             onDirBtnClicked();
  void             updateWidgets();

private:
  bool             readParamsFromHypo( GHS3DHypothesisData& ) const;
  bool             readParamsFromWidgets( GHS3DHypothesisData& ) const;
  bool             storeParamsToHypo( const GHS3DHypothesisData& ) const;

private:
  QWidget*         myStdGroup;
  QLineEdit*       myName;
  QCheckBox*       myToMeshHolesCheck;
  QComboBox*       myOptimizationLevelCombo;

  QWidget*         myAdvGroup;
  QCheckBox*       myMaximumMemoryCheck;
  QSpinBox*        myMaximumMemorySpin;
  QCheckBox*       myInitialMemoryCheck;
  QSpinBox*        myInitialMemorySpin;
  QLineEdit*       myWorkingDir;
  QCheckBox*       myKeepFiles;
  QSpinBox*        myVerboseLevelSpin;
  QCheckBox*       myToCreateNewNodesCheck;
  QCheckBox*       myBoundaryRecoveryCheck;
  QLineEdit*       myTextOption;
};

#endif
