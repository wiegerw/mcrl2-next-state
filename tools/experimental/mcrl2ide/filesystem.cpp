// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "filesystem.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QDirIterator>
#include <QMessageBox>
#include <QInputDialog>

Property::Property(QString name, QString text)
{
  this->name = name;
  this->text = text;
}

bool Property::equals(Property* property)
{
  return this->name == property->name && this->text == property->text;
}

FileSystem::FileSystem(CodeEditor* specificationEditor, QWidget* parent)
{
  this->specificationEditor = specificationEditor;
  this->parent = parent;
  specificationModified = false;
  projectOpen = false;

  if (!temporaryFolder.isValid())
  {
    qDebug("Warning: could not create temporary folder");
  }

  connect(specificationEditor, SIGNAL(modificationChanged(bool)), this,
          SLOT(setSpecificationModified(bool)));
}

void FileSystem::makeSurePropertiesFolderExists()
{
  if (!QDir(propertiesFolderPath()).exists())
  {
    QDir().mkpath(propertiesFolderPath());
  }
}

QString FileSystem::projectFilePath()
{
  return projectFolderPath + QDir::separator() + projectName +
         projectFileExtension;
}

QString FileSystem::propertiesFolderPath()
{
  return projectFolderPath + QDir::separator() + propertiesFolderName;
}

QString FileSystem::defaultSpecificationFilePath()
{
  return projectFolderPath + QDir::separator() + projectName + "_spec.mcrl2";
}

QString FileSystem::specificationFilePath()
{
  if (specFilePath.isEmpty())
  {
    return defaultSpecificationFilePath();
  }
  else
  {
    return specFilePath;
  }
}

QString FileSystem::lpsFilePath(bool evidence, QString propertyName)
{
  return temporaryFolder.path() + QDir::separator() + projectName +
         (evidence ? "_" + propertyName + "_evidence" : "") + "_lps.lps";
}

QString FileSystem::ltsFilePath(LtsReduction reduction, bool evidence,
                                QString propertyName)
{
  return temporaryFolder.path() + QDir::separator() + projectName +
         (evidence ? "_" + propertyName + "_evidence" : "") + "_lts_" +
         LTSREDUCTIONNAMES.at(reduction) + ".lts";
}

QString FileSystem::propertyFilePath(QString propertyName)
{
  return propertiesFolderPath() + QDir::separator() + propertyName + ".mcf";
}

QString FileSystem::pbesFilePath(QString propertyName, bool evidence)
{
  return temporaryFolder.path() + QDir::separator() + projectName + "_" +
         propertyName + (evidence ? "_evidence" : "") + "_pbes.pbes";
}

bool FileSystem::projectOpened()
{
  return projectOpen;
}

bool FileSystem::isSpecificationModified()
{
  return specificationModified;
}

void FileSystem::setSpecificationModified(bool modified)
{
  specificationModified = modified;
}

void FileSystem::clearProperties()
{
  for (Property* property : properties)
  {
    delete property;
  }
  properties.clear();
}

bool FileSystem::propertyNameExists(QString propertyName)
{
  for (Property* property : properties)
  {
    if (property->name == propertyName)
    {
      return true;
    }
  }
  return false;
}

bool FileSystem::upToDateLpsFileExists(bool evidence, QString propertyName)
{
  /* in case not evidence lps, an lps file is up to date if the lps file exists
   *   and the lps file is created after the last time the specification file
   *   was modified
   * in case evidence lps, an lps file is up to date if the lps file exists and
   *   the lps file is created after the the last time the evidence pbes was
   *   modified */
  if (!evidence)
  {
    return QFile(lpsFilePath(evidence, propertyName)).exists() &&
           QFileInfo(specificationFilePath()).lastModified() <=
               QFileInfo(lpsFilePath(evidence, propertyName)).lastModified();
  }
  else
  {
    return QFile(lpsFilePath(evidence, propertyName)).exists() &&
           QFileInfo(pbesFilePath(propertyName, evidence)).lastModified() <=
               QFileInfo(lpsFilePath(evidence, propertyName)).lastModified();
  }
}

bool FileSystem::upToDateLtsFileExists(LtsReduction reduction, bool evidence,
                                       QString propertyName)
{
  /* in case not reduced lts, an lts file is up to date if the lts file exists
   *   and the lts file is created after the last time the lps file was modified
   * in case reduced lts, an lts file is up to date if the lts file exists and
   *   the lts file is created after the last time the unreduced lts file was
   *   modified */
  if (reduction == LtsReduction::None)
  {
    return QFile(ltsFilePath(reduction, evidence, propertyName)).exists() &&
           QFileInfo(lpsFilePath(evidence, propertyName)).lastModified() <=
               QFileInfo(ltsFilePath(reduction, evidence, propertyName))
                   .lastModified();
  }
  else
  {
    return QFile(ltsFilePath(reduction)).exists() &&
           QFileInfo(ltsFilePath(LtsReduction::None)).lastModified() <=
               QFileInfo(ltsFilePath(reduction)).lastModified();
  }
}

bool FileSystem::upToDatePbesFileExists(QString propertyName, bool evidence)
{
  /* a pbes file is up to date if the pbes file exists and the pbes file is
   *   created after the last time both the lps and the property files were
   *   modified */
  return QFile(pbesFilePath(propertyName, evidence)).exists() &&
         QFileInfo(lpsFilePath()).lastModified() <=
             QFileInfo(pbesFilePath(propertyName, evidence)).lastModified() &&
         QFileInfo(propertyFilePath(propertyName)).lastModified() <=
             QFileInfo(pbesFilePath(propertyName, evidence)).lastModified();
}

void FileSystem::setSpecificationEditorCursor(int row, int column)
{
  QTextCursor cursor = specificationEditor->textCursor();
  cursor.movePosition(QTextCursor::Start);
  cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row - 1);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
  specificationEditor->setTextCursor(cursor);
}

QFileDialog* FileSystem::createFileDialog(int type)
{
  QFileDialog* fileDialog = new QFileDialog(parent, Qt::WindowCloseButtonHint);
  fileDialog->setDirectory(
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
  fileDialog->setOption(QFileDialog::DontUseNativeDialog);
  fileDialog->setLabelText(QFileDialog::FileName, "Project name:");
  switch (type)
  {
  case 0:
    fileDialog->setNameFilters(QStringList({"Directory"}));
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setWindowTitle("New Project");
    fileDialog->setLabelText(QFileDialog::Accept, "Create");
    break;
  case 1:
    fileDialog->setNameFilters(QStringList({"Directory"}));
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setWindowTitle("Save Project As");
    fileDialog->setLabelText(QFileDialog::Accept, "Save as");
    break;
  case 2:
    fileDialog->setNameFilters(
        QStringList({"mCRL2 project file (*" + projectFileExtension + ")"}));
    fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog->setWindowTitle("Open Project");
    fileDialog->setLabelText(QFileDialog::Accept, "Open");
  }
  return fileDialog;
}

void FileSystem::createProjectFile()
{
  QFile projectFile(projectFilePath());
  projectFile.open(QIODevice::WriteOnly);
  QTextStream saveStream(&projectFile);
  saveStream << "SPEC " + defaultSpecificationFilePath();
  projectFile.close();
}

QString FileSystem::newProject(bool askToSave, bool forNewProject)
{
  QString error = "";
  QString context = forNewProject ? "New Project" : "Save Project As";

  /* if there are changes in the current project, ask to save first */
  if (askToSave && isSpecificationModified())
  {
    QMessageBox::StandardButton result = QMessageBox::question(
        parent, "New Project",
        "There are changes in the current project, do you want to save?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    switch (result)
    {
    case QMessageBox::Yes:
      saveProject();
      break;
    case QMessageBox::Cancel:
      return "";
    default:
      break;
    }
  }

  /* ask the user for a project folder */
  QFileDialog* newProjectDialog = createFileDialog(forNewProject ? 0 : 1);
  if (newProjectDialog->exec() == QDialog::Accepted)
  {
    /* if successful, create the new project */
    QString newProjectFolderPath = newProjectDialog->selectedFiles().first();
    QString newProjectName = QFileInfo(newProjectFolderPath).fileName();

    /* create the folder for this project */
    if (QDir().mkpath(newProjectFolderPath))
    {
      /* if successful, set the current projectFolder and create the project
       *   file and properties folder */
      projectFolderPath = newProjectFolderPath;
      projectName = newProjectName;
      specFilePath = defaultSpecificationFilePath();

      createProjectFile();
      QDir(projectFolderPath).mkdir(propertiesFolderName);

      /* also empty the editor and properties list if we are not saving as and
       *   if there was already a project open */
      if (forNewProject && projectOpened())
      {
        specificationEditor->clear();
        clearProperties();
      }

      projectOpen = true;

      /* if we are not saving as, create an empty specification file by saving
       *   the project */
      if (forNewProject)
      {
        saveProject(true);
      }
    }
    else
    {
      /* if unsuccessful, tell the user */
      error = "Could not create project";
    }
  }
  delete newProjectDialog;

  if (error.isEmpty())
  {
    return projectName;
  }
  else
  {
    /* if there was an error, tell the user */
    QMessageBox msgBox(QMessageBox::Information, context, error,
                       QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox.exec();
    return "";
  }
}

bool FileSystem::deletePropertyFile(QString propertyName, bool showIfFailed)
{
  QFile* propertyFile = new QFile(propertyFilePath(propertyName));
  bool deleteSucceeded = true;
  if (propertyFile->exists() && !propertyFile->remove())
  {
    /* if deleting the file failed, tell the user */
    QMessageBox msgBox(QMessageBox::Information, "Delete property",
                       "Could not delete property file: " +
                           propertyFile->errorString(),
                       QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox.exec();
    deleteSucceeded = false;
  }

  delete propertyFile;
  return deleteSucceeded;
}

void FileSystem::deleteUnlistedPropertyFiles()
{
  QDir propertiesFolder = QDir(propertiesFolderPath());
  QStringList fileNames = propertiesFolder.entryList();
  for (QString fileName : fileNames)
  {
    if (fileName.endsWith(".mcf"))
    {
      QString propertyName = fileName.left(fileName.length() - 4);
      if (!propertyNameExists(propertyName))
      {
        deletePropertyFile(propertyName, false);
      }
    }
  }
}

void FileSystem::newProperty(Property* property)
{
  /* add to the properties list */
  properties.push_back(property);
  /* delete any obsolete property files generated by the dialog */
  deleteUnlistedPropertyFiles();
}

void FileSystem::editProperty(Property* oldProperty, Property* newProperty)
{
  /* alter the properties list */
  std::list<Property*>::iterator it;
  for (it = properties.begin(); it != properties.end(); it++)
  {
    if ((*it)->equals(oldProperty))
    {
      (*it) = newProperty;
      break;
    }
  }

  /* delete any obsolete property files generated by the dialog
   * this also deletes the original property file if the property name has
   *   changed */
  deleteUnlistedPropertyFiles();
}

bool FileSystem::deleteProperty(Property* oldProperty)
{
  /* show a message box to ask the user whether he is sure to delete the
   *   property */
  bool deleteIt =
      QMessageBox::question(parent, "Delete Property",
                            "Are you sure you want to delete the property " +
                                oldProperty->name + "?",
                            QMessageBox::Yes | QMessageBox::Cancel) ==
      QMessageBox::Yes;

  /* if the user agrees, delete the property */
  if (deleteIt)
  {
    Property* toRemove = nullptr;
    for (Property* property : properties)
    {
      if (property->equals(oldProperty))
      {
        toRemove = property;
        break;
      }
    }
    if (toRemove != nullptr)
    {
      properties.remove(toRemove);
    }
    delete toRemove;

    /* also delete the file if it exists */
    if (!deletePropertyFile(oldProperty->name))
    {
      deleteIt = false;
    }
  }

  return deleteIt;
}

void FileSystem::openProjectFromFile(QString newProjectFilePath,
                                     QString* newProjectName,
                                     std::list<Property*>* newProperties)
{
  QString error = "";
  QFile projectFile(newProjectFilePath);

  if (!projectFile.exists())
  {
    error = "Could not find provided project file";
  }
  else if (!newProjectFilePath.endsWith(projectFileExtension))
  {
    error =
        "Provided file is not a mcrl2 project file (does not have extension " +
        projectFileExtension + ")";
  }
  else
  {
    /* read the project file to get the specification path */
    projectFile.open(QIODevice::ReadOnly);
    QTextStream projectOpenStream(&projectFile);
    QString projectInfo = projectOpenStream.readAll();
    int specLineIndex = projectInfo.lastIndexOf("SPEC");
    if (specLineIndex < 0)
    {
      error = "Provided project file does not contain the specification "
              "(should contain SPEC <path_to_spec>).";
    }
    else
    {
      specFilePath =
          projectInfo.right(projectInfo.length() - specLineIndex - 5);
      std::string test = specFilePath.toStdString();
      projectFile.close();

      /* read the specification and put it in the specification editor */
      QFile specificationFile(specFilePath);
      if (!specificationFile.exists())
      {
        error = "Specification file given in the provided project file does "
                "not exist";
      }
      else
      {
        specificationFile.open(QIODevice::ReadOnly);
        QTextStream specificationOpenStream(&specificationFile);
        QString spec = specificationOpenStream.readAll();
        specificationEditor->setPlainText(spec);
        specificationFile.close();
        specificationModified = false;

        /* set propject folder and project name */
        projectFolderPath = QFileInfo(newProjectFilePath).path();
        this->projectName = QFileInfo(newProjectFilePath).baseName();

        /* read all properties */
        clearProperties();
        QDirIterator* dirIterator =
            new QDirIterator(QDir(propertiesFolderPath()));

        while (dirIterator->hasNext())
        {
          QFile propertyFile(dirIterator->next());
          QFileInfo propertyFileInfo(propertyFile);
          QString fileName = propertyFileInfo.fileName();

          if (propertyFileInfo.isFile() && fileName.endsWith(".mcf"))
          {
            fileName.chop(4);
            propertyFile.open(QIODevice::ReadOnly);
            QTextStream propertyOpenStream(&propertyFile);
            QString propertyText = propertyOpenStream.readAll();
            properties.push_back(new Property(fileName, propertyText));
            propertyFile.close();
          }
        }
        delete dirIterator;
      }
    }
  }

  /* if successful, set variables to the opened project, else tell the user */
  if (error.isEmpty())
  {
    projectOpen = true;
    this->properties = properties;
    *newProjectName = projectName;
    *newProperties = properties;
  }
  else
  {
    QMessageBox msgBox(QMessageBox::Information, "Open Project", error,
                       QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox.exec();
  }
}

void FileSystem::openProject(QString* newProjectName,
                             std::list<Property*>* newProperties)
{
  /* ask the user for a project */
  QFileDialog* openProjectDialog = createFileDialog(2);
  if (openProjectDialog->exec() == QDialog::Accepted)
  {
    QString newProjectFilePath = openProjectDialog->selectedFiles().first();
    openProjectFromFile(newProjectFilePath, newProjectName, newProperties);
  }
  delete openProjectDialog;
}

QString FileSystem::saveProject(bool forceSave)
{
  makeSurePropertiesFolderExists();

  /* if we have a project open, we have a location to save in so we can simply
   *   save, else use save as */
  if (projectOpen)
  {
    /* save the specification (when there are changes) */
    if (isSpecificationModified() || forceSave)
    {
      QFile specificationFile(specFilePath);
      specificationFile.open(QIODevice::WriteOnly);
      QTextStream saveStream(&specificationFile);
      saveStream << specificationEditor->toPlainText();
      specificationFile.close();
      specificationModified = false;
      emit changesSaved();
    }

    /* save all properties if necessary */
    if (forceSave)
    {
      for (Property* property : properties)
      {
        saveProperty(property);
      }
    }

    specificationEditor->document()->setModified(false);
    return projectName;
  }
  else
  {
    return saveProjectAs();
  }
}

QString FileSystem::saveProjectAs()
{
  /* ask the user for a new project name */
  QString projectName = newProject(false, false);

  /* save if successful, else return the empty string */
  if (projectName.isEmpty())
  {
    return "";
  }
  else
  {
    saveProject(true);
    return projectName;
  }
}

void FileSystem::saveProperty(Property* property)
{
  makeSurePropertiesFolderExists();

  QFile propertyFile(propertyFilePath(property->name));
  propertyFile.open(QIODevice::WriteOnly);
  QTextStream saveStream(&propertyFile);
  saveStream << property->text;
  propertyFile.close();
  propertyModified[property->name] = false;
}

void FileSystem::removeTemporaryFolder()
{
  temporaryFolder.remove();
}
