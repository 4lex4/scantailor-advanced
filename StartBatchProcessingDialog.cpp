#include "StartBatchProcessingDialog.h"
#include "ui_StartBatchProcessingDialog.h"


StartBatchProcessingDialog::StartBatchProcessingDialog(QWidget* parent) :
        QDialog(parent),
        ui(new Ui::StartBatchProcessingDialog)
{
    ui->setupUi(this);

    ui->allPages->setChecked(true);
    ui->fromSelected->setChecked(false);
}

StartBatchProcessingDialog::~StartBatchProcessingDialog()
{
    delete ui;
}

bool StartBatchProcessingDialog::isAllPagesChecked() const
{
    return ui->allPages->isChecked();
}
