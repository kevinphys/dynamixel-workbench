/**
 * @file /src/main_window.cpp
 *
 * @brief Implementation for the qt gui.
 *
 * @date February 2011
 **/
/*****************************************************************************
** Includes
*****************************************************************************/

#include <QtGui>
#include <QMessageBox>
#include <iostream>
#include "../include/dynamixel_workbench_single_manager_gui/main_window.hpp"

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace dynamixel_workbench_single_manager_gui {

using namespace Qt;

/*****************************************************************************
** Implementation [MainWindow]
*****************************************************************************/

MainWindow::MainWindow(int argc, char** argv, QWidget *parent)
	: QMainWindow(parent)
    , qnode_(argc,argv)
{
  ui_.setupUi(this); // Calling this incidentally connects all ui's triggers to on_...() callbacks in this class.
  QObject::connect(ui_.actionAbout_Qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt())); // qApp is a global variable for the application

  setWindowIcon(QIcon(":/images/icon.png"));
  ui_.tab_manager->setCurrentIndex(0); // ensure the first tab is showing - qt-designer should have this already hardwired, but often loses it (settings?).
  QObject::connect(&qnode_, SIGNAL(rosShutdown()), this, SLOT(close()));

  /*********************
  ** Logging
  **********************/
  ui_.view_logging->setModel(qnode_.loggingModel());
  QObject::connect(&qnode_, SIGNAL(loggingUpdated()), this, SLOT(updateLoggingView()));

  /*********************
  ** Connect
  **********************/
  makeConnect();

  /*********************
  ** Auto Start
  **********************/
  qnode_.init();

  makeUI();
}

MainWindow::~MainWindow() {}

/*****************************************************************************
** Implementation [Slots]
*****************************************************************************/

void MainWindow::showNoMasterMessage() {
	QMessageBox msgBox;
	msgBox.setText("Couldn't find the ros master.");
	msgBox.exec();
    close();
}

void MainWindow::on_torque_enable_toggle_button_toggled(bool check)
{
  if(ui_.torque_enable_toggle_button->isChecked())
  {
    ui_.torque_enable_toggle_button->setText("Torque_Disable");
    qnode_.sendTorqueMsg(std::string("torque_enable"), true);

    ui_.set_id_line_edit->setEnabled(false);
    ui_.set_operating_mode_combo_box->setEnabled(false);
    ui_.set_baud_rate_line_edit->setEnabled(false);
    ui_.set_position_zero_push_button->setVisible(false);
  }
  else
  {
    ui_.torque_enable_toggle_button->setText("Torque_Enable");
    qnode_.sendTorqueMsg(std::string("torque_enable"), false);

    ui_.set_id_line_edit->setEnabled(true);
    ui_.set_operating_mode_combo_box->setEnabled(true);
    ui_.set_baud_rate_line_edit->setEnabled(true);
    ui_.set_position_zero_push_button->setVisible(false);
  }

  uint8_t index_num = ui_.set_address_name_combo_box->count();
  for (uint8_t combo_box_index = 0; combo_box_index < index_num; combo_box_index++)
  {
    ui_.set_address_name_combo_box->removeItem(0);
  }

  for (dxl_->it_ctrl_ = dxl_->ctrl_table_.begin(); dxl_->it_ctrl_ != dxl_->ctrl_table_.end(); dxl_->it_ctrl_++)
  {
    dxl_->dxl_item_ = dxl_->ctrl_table_[dxl_->it_ctrl_->first.c_str()];
    if (ui_.torque_enable_toggle_button->isChecked())
    {
      if ((dxl_->dxl_item_->access_type == dynamixel_tool::READ_WRITE) && (dxl_->dxl_item_->memory_type == dynamixel_tool::RAM))
      {
        ui_.set_address_name_combo_box->addItem(QString::fromStdString(dxl_->dxl_item_->item_name));
      }
    }
    else
    {
      if (dxl_->dxl_item_->access_type == dynamixel_tool::READ_WRITE)
      {
        ui_.set_address_name_combo_box->addItem(QString::fromStdString(dxl_->dxl_item_->item_name));
      }
    }
  }
}

void MainWindow::on_reboot_push_button_clicked(bool check)
{
  qnode_.sendRebootMsg();
}

void MainWindow::on_factory_reset_push_button_clicked(bool check)
{
  qnode_.sendResetMsg();
}

void MainWindow::on_set_position_zero_push_button_clicked(bool check)
{
  qnode_.setPositionZeroMsg(dxl_->value_of_0_radian_position_);
  ui_.set_address_value_spin_box->setValue(dxl_->value_of_0_radian_position_);
  ui_.set_address_value_dial->setValue(dxl_->value_of_0_radian_position_);
}

void MainWindow::changeDynamixelID()
{
  qnode_.sendSetIdMsg(ui_.set_id_line_edit->text().toLong());
}

void MainWindow::changeOperatingMode()
{
  qnode_.sendSetOperatingModeMsg(ui_.set_operating_mode_combo_box->currentText().toStdString());
}

void MainWindow::changeBaudrate()
{
  qnode_.sendSetBaudrateMsg(ui_.set_baud_rate_line_edit->text().toFloat());
  ui_.get_baud_rate_line_edit->setText(QString::number(ui_.set_baud_rate_line_edit->text().toLongLong()));
}

void MainWindow::changeControlTableValue()
{
  qnode_.sendControlTableValueMsg(ui_.set_address_name_combo_box->currentText(), ui_.set_address_value_dial->value());
}

void MainWindow::showHideButton(QString index)
{
  if (index.toStdString() == "goal_position")
    ui_.set_position_zero_push_button->setVisible(true);
  else
    ui_.set_position_zero_push_button->setVisible(false);
}

void MainWindow::updateWorkbenchParamLineEdit(dynamixel_workbench_msgs::WorkbenchParam msg)
{
  ui_.get_device_name_line_edit->setText(QString::fromStdString(msg.device_name));
  ui_.get_baud_rate_line_edit->setText(QString::number(msg.baud_rate));
  ui_.get_protocol_version_line_edit->setText(QString::number(msg.protocol_version));
  ui_.get_model_name_line_edit->setText(QString::fromStdString(msg.model_name));

  dxl_ = new dynamixel_tool::DynamixelTool(msg.model_id, msg.model_number, msg.protocol_version);
}

void MainWindow::makeConnect()
{
  qRegisterMetaType<dynamixel_workbench_msgs::WorkbenchParam>("dynamixel_workbench_msgs::WorkbenchParam");
  QObject::connect(&qnode_, SIGNAL(updateWorkbenchParam(dynamixel_workbench_msgs::WorkbenchParam)), this, SLOT(updateWorkbenchParamLineEdit(dynamixel_workbench_msgs::WorkbenchParam)));
  QObject::connect(ui_.set_id_line_edit, SIGNAL(returnPressed()), this, SLOT(changeDynamixelID()));
  QObject::connect(ui_.set_baud_rate_line_edit, SIGNAL(returnPressed()), this, SLOT(changeBaudrate()));
  QObject::connect(ui_.set_operating_mode_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(changeOperatingMode()));
  QObject::connect(ui_.set_address_name_combo_box, SIGNAL(activated(QString)), this, SLOT(showHideButton(QString)));
  QObject::connect(ui_.set_address_value_spin_box, SIGNAL(valueChanged(int)), ui_.set_address_value_dial, SLOT(setValue(int)));
  QObject::connect(ui_.set_address_value_dial, SIGNAL(valueChanged(int)), ui_.set_address_value_spin_box, SLOT(setValue(int)));
  QObject::connect(ui_.set_address_value_spin_box, SIGNAL(valueChanged(int)), this, SLOT(changeControlTableValue()));
  QObject::connect(ui_.set_address_value_dial, SIGNAL(valueChanged(int)), this, SLOT(changeControlTableValue()));
}

void MainWindow::makeUI()
{
  ui_.set_id_line_edit->setText(QString::number(1));
  ui_.set_baud_rate_line_edit->setText(QString::number(1000000));
  ui_.set_operating_mode_combo_box->addItem((QString("Select Mode")));
  ui_.set_address_value_dial->setRange(dxl_->value_of_min_radian_position_, dxl_->value_of_max_radian_position_);
  ui_.set_address_value_spin_box->setRange(dxl_->value_of_min_radian_position_, dxl_->value_of_max_radian_position_);
  ui_.set_position_zero_push_button->setVisible(false);

  if (ui_.get_protocol_version_line_edit->text().toFloat() == 2.0)
  {
    ui_.reboot_push_button->setEnabled(true);
  }
  else
  {
    ui_.reboot_push_button->setEnabled(false);
  }

  for (dxl_->it_ctrl_ = dxl_->ctrl_table_.begin(); dxl_->it_ctrl_ != dxl_->ctrl_table_.end(); dxl_->it_ctrl_++)
  {
    dxl_->dxl_item_ = dxl_->ctrl_table_[dxl_->it_ctrl_->first.c_str()];
    if ((dxl_->dxl_item_->access_type == dynamixel_tool::READ_WRITE))
    {
      ui_.set_address_name_combo_box->addItem(QString::fromStdString(dxl_->dxl_item_->item_name));
    }
  }

  if(!strncmp(dxl_->model_name_.c_str(), "AX", 2))
  {
    ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "MX", 2))
  {
    if (dxl_->model_number_ == 310) // MX-64
    {
      ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("extended_position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("torque_control"));
    }
    else if (dxl_->model_number_ == 320) // MX-106
    {
      ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("extended_position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("torque_control"));
    }
    else
    {
      ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("extended_position_control"));
    }
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "RX", 2))
  {
    ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "EX", 2))
  {
    ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "XL", 2))
  {
    ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "XM", 2))
  {
    ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("current_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("extended_position_control"));
    ui_.set_operating_mode_combo_box->addItem(QString("position_control_based_on_current"));
    ui_.set_operating_mode_combo_box->addItem(QString("pwm_control"));
  }
  else if (!strncmp(dxl_->model_name_.c_str(), "PRO", 3))
  {
    if (dxl_->model_number_ == 35072) // PRO_L42_10_S300_R
    {
      ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
    }
    else
    {
      ui_.set_operating_mode_combo_box->addItem(QString("position_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("velocity_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("torque_control"));
      ui_.set_operating_mode_combo_box->addItem(QString("extended_position_control"));
    }
  }
}

/*****************************************************************************
** Implemenation [Slots][manually connected]
*****************************************************************************/

/**
 * This function is signalled by the underlying model. When the model changes,
 * this will drop the cursor down to the last line in the QListview to ensure
 * the user can always see the latest log message.
 */
void MainWindow::updateLoggingView() {
       // ui_.view_logging->scrollToTop();
}

/*****************************************************************************
** Implementation [Menu]
*****************************************************************************/

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About ..."),tr("<h2>Dynamixel workbench 1.0</h2><p>Copyright Robotis</p>"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //WriteSettings();
	QMainWindow::closeEvent(event);
}

}  // namespace dynamixel_workbench_single_manager_gui
