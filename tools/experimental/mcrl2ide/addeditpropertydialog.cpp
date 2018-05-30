#include "addeditpropertydialog.h"
#include "ui_addeditpropertydialog.h"
#include "propertywidget.h"

AddEditPropertyDialog::AddEditPropertyDialog(bool add, QWidget *parent, QString propertyName, QString propertyText) :
    QDialog(parent),
    ui(new Ui::AddEditPropertyDialog)
{
    ui->setupUi(this);

    /* change the ui depending on whether this should be an add or edit property window */
    if (add){
        this->setWindowTitle("Add Property");
        ui->addEditButton->setText("Add");
    } else {
        this->setWindowTitle("Edit Property");
        ui->addEditButton->setText("Edit");
        ui->propertyNameField->setText(propertyName);
        ui->propertyTextField->setPlainText(propertyText);
    }

    connect(ui->addEditButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

QString AddEditPropertyDialog::getPropertyName()
{
    return ui->propertyNameField->text();
}

QString AddEditPropertyDialog::getPropertyText()
{
    return ui->propertyTextField->toPlainText();
}

AddEditPropertyDialog::~AddEditPropertyDialog()
{
    delete ui;
}