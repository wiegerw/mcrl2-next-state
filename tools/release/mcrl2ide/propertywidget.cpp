// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "propertywidget.h"

#include <QMessageBox>
#include <QSpacerItem>
#include <QStyleOption>
#include <QPainter>

PropertyWidget::PropertyWidget(Property property, ProcessSystem* processSystem,
                               FileSystem* fileSystem, QWidget* parent)
    : QWidget(parent)
{
  this->property = property;
  this->processSystem = processSystem;
  this->fileSystem = fileSystem;
  this->parent = parent;
  lastRunningProcessId = -1;

  editPropertyDialog =
      new AddEditPropertyDialog(false, processSystem, fileSystem, this);
  connect(editPropertyDialog, SIGNAL(accepted()), this,
          SLOT(actionEditResult()));

  /* create the label for the property name */
  propertyNameLabel = new QLabel(property.name);

  /* create the verify button */
  QPushButton* verifyButton = new QPushButton();
  verifyButton->setIcon(QIcon(":/icons/verify.png"));
  verifyButton->setIconSize(QSize(24, 24));
  verifyButton->setStyleSheet("QPushButton { padding:5px; }");
  verifyButton->setToolTip("Verify");
  connect(verifyButton, SIGNAL(clicked()), this, SLOT(actionVerify()));

  /* create the abort button for when a property is being verified */
  QPushButton* abortButton = new QPushButton();
  abortButton->setIcon(QIcon(":/icons/abort.png"));
  abortButton->setIconSize(QSize(24, 24));
  abortButton->setStyleSheet("QPushButton { padding:5px; }");
  abortButton->setToolTip("Abort verification");
  connect(abortButton, SIGNAL(clicked()), this,
          SLOT(actionAbortVerification()));

  /* create the witness button for when a property is true */
  QPushButton* witnessButton = new QPushButton();
  witnessButton->setIcon(QIcon(":/icons/witness.png"));
  witnessButton->setIconSize(QSize(24, 24));
  witnessButton->setStyleSheet("QPushButton { padding:5px; }");
  witnessButton->setToolTip("Show witness");
  connect(witnessButton, SIGNAL(clicked()), this, SLOT(actionShowEvidence()));

  /* create the counterexample button for when a property is false */
  QPushButton* counterexampleButton = new QPushButton();
  counterexampleButton->setIcon(QIcon(":/icons/counterexample.png"));
  counterexampleButton->setIconSize(QSize(24, 24));
  counterexampleButton->setStyleSheet("QPushButton { padding:5px; }");
  counterexampleButton->setToolTip("Show counterexample");
  connect(counterexampleButton, SIGNAL(clicked()), this,
          SLOT(actionShowEvidence()));

  /* stack the verification widgets */
  verificationWidgets = new QStackedWidget(this);
  verificationWidgets->setMaximumWidth(40);
  verificationWidgets->addWidget(verifyButton);         /* index = 0 */
  verificationWidgets->addWidget(abortButton);          /* index = 1 */
  verificationWidgets->addWidget(witnessButton);        /* index = 2 */
  verificationWidgets->addWidget(counterexampleButton); /* index = 3 */
  verificationWidgets->setCurrentIndex(0);

  /* create the edit button */
  editButton = new QPushButton();
  editButton->setIcon(QIcon(":/icons/edit.png"));
  editButton->setIconSize(QSize(24, 24));
  editButton->setStyleSheet("QPushButton { padding:5px; }");
  editButton->setToolTip("Edit property");
  connect(editButton, SIGNAL(clicked()), this, SLOT(actionEdit()));

  /* create the delete button */
  deleteButton = new QPushButton();
  deleteButton->setIcon(QIcon(":/icons/delete.png"));
  deleteButton->setIconSize(QSize(24, 24));
  deleteButton->setStyleSheet("QPushButton { padding:5px; }");
  deleteButton->setToolTip("Delete property");
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(actionDelete()));

  /* lay them out */
  propertyLayout = new QHBoxLayout();
  propertyLayout->addWidget(propertyNameLabel);
  propertyLayout->addStretch();
  propertyLayout->addWidget(verificationWidgets);
  propertyLayout->addWidget(editButton);
  propertyLayout->addWidget(deleteButton);

  this->setLayout(propertyLayout);

  connect(processSystem, SIGNAL(processFinished(int)), this,
          SLOT(actionVerifyResult(int)));
  connect(processSystem, SIGNAL(processFinished(int)), this,
          SLOT(actionShowEvidenceResult(int)));
}

PropertyWidget::~PropertyWidget()
{
  editPropertyDialog->deleteLater();
  propertyNameLabel->deleteLater();
  for (int i = 0; i < verificationWidgets->count(); i++)
  {
    verificationWidgets->widget(i)->deleteLater();
  }
  verificationWidgets->deleteLater();
  editButton->deleteLater();
  deleteButton->deleteLater();
  propertyLayout->deleteLater();
  editPropertyDialog->deleteLater();
}

void PropertyWidget::paintEvent(QPaintEvent* pe)
{
  Q_UNUSED(pe);
  QStyleOption o;
  o.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

Property PropertyWidget::getProperty()
{
  return this->property;
}

void PropertyWidget::resetWidget()
{
  verificationWidgets->setCurrentIndex(0);
  this->setStyleSheet("");
}

void PropertyWidget::actionVerify()
{
  /* check if the property isn't already being verified */
  if (verificationWidgets->currentIndex() != 1)
  {
    resetWidget();
    /* start the verification process */
    lastRunningProcessId = processSystem->verifyProperty(property);

    /* if starting the process was successful, change the buttons */
    if (lastRunningProcessId >= 0)
    {
      lastProcessIsVerification = true;
      verificationWidgets->setCurrentIndex(1);
      editButton->setEnabled(false);
      deleteButton->setEnabled(false);
    }
  }
}

void PropertyWidget::actionVerifyResult(int processid)
{
  /* check if the process that has finished is the verification process of this
   *   property */
  if (processid == lastRunningProcessId && lastProcessIsVerification)
  {
    /* get the result and apply it to the widget */
    QString result = processSystem->getResult(lastRunningProcessId);
    if (result == "true")
    {
      verificationWidgets->setCurrentIndex(2);
      this->setStyleSheet("PropertyWidget{background-color:rgb(153,255,153)}");
    }
    else if (result == "false")
    {
      verificationWidgets->setCurrentIndex(3);
      this->setStyleSheet("PropertyWidget{background-color:rgb(255,153,153)}");
    }
    else
    {
      verificationWidgets->setCurrentIndex(0);
    }
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
  }
}

void PropertyWidget::actionShowEvidence()
{
  /* check if the property has veen verified */
  if (verificationWidgets->currentIndex() > 1)
  {
    /* start the evidence creation process */
    lastRunningProcessId = processSystem->showEvidence(property);
    evidenceIsWitness = verificationWidgets->currentIndex() == 2;

    /* if starting the process was successful, change the buttons */
    if (lastRunningProcessId >= 0)
    {
      lastProcessIsVerification = false;
      verificationWidgets->setCurrentIndex(1);
      editButton->setEnabled(false);
      deleteButton->setEnabled(false);
    }
  }
}

void PropertyWidget::actionShowEvidenceResult(int processid)
{
  /* check if the process that has finished is the verification process of this
   *   property */
  if (processid == lastRunningProcessId && !lastProcessIsVerification)
  {
    verificationWidgets->setCurrentIndex(evidenceIsWitness ? 2 : 3);
    editButton->setEnabled(true);
    deleteButton->setEnabled(true);
  }
}

void PropertyWidget::actionAbortVerification()
{
  processSystem->abortProcess(lastRunningProcessId);
}

void PropertyWidget::actionEdit()
{
  editPropertyDialog->setProperty(property);
  editPropertyDialog->setOldProperty(property);
  editPropertyDialog->resetFocus();
  if (editPropertyDialog->isVisible())
  {
    editPropertyDialog->activateWindow();
    editPropertyDialog->setFocus();
  }
  else
  {
    editPropertyDialog->show();
  }
}

void PropertyWidget::actionEditResult()
{
  /* if editing was successful (Edit button was pressed), change the property
   * we don't need to save to file as this is already done by the dialog */
  Property newProperty = editPropertyDialog->getProperty();
  fileSystem->editProperty(property, newProperty);
  /* only make changes if the property changed */
  if (property != newProperty)
  {
    property = newProperty;
    propertyNameLabel->setText(property.name);
    resetWidget();
  }
}

void PropertyWidget::actionDelete()
{
  if (fileSystem->deleteProperty(property))
  {
    emit deleteMe(this);
  }
}
