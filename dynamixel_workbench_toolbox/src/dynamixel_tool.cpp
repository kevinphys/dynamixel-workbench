/*******************************************************************************
* Copyright (c) 2016, ROBOTIS CO., LTD.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* * Neither the name of ROBOTIS nor the names of its
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/* Author: Taehoon Lim (Darby), zerom */

#include "dynamixel_workbench_toolbox/dynamixel_tool.h"

using namespace dynamixel_tool;

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}
static inline std::vector<std::string> split(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while((end = text.find(sep, start)) != (std::string::npos)) {
        tokens.push_back(text.substr(start, end - start));
        trim(tokens.back());
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    trim(tokens.back());
    return tokens;
}

DynamixelTool::DynamixelTool(uint8_t id, uint16_t model_number, float protocol_version)
    :id_(0),
     model_number_(0),
     protocol_version_(0.0),
     model_name_(""),
     item_path_(""),
     dynamixel_name_path_("")
{
  id_ = id;
  protocol_version_ = protocol_version;

  getModelName(model_number);
  getModelItem();
}

DynamixelTool::DynamixelTool(uint8_t id, std::string model_name, float protocol_version)
    :id_(0),
     model_number_(0),
     protocol_version_(0.0),
     model_name_(""),
     item_path_(""),
     dynamixel_name_path_("")
{
  id_ = id;
  model_name_ = model_name;
  protocol_version_ = protocol_version;

  getModelItem();
}

DynamixelTool::~DynamixelTool(){}

bool DynamixelTool::getModelName(uint16_t model_number)
{
  dynamixel_name_path_  = ros::package::getPath("dynamixel_workbench_toolbox") + "/dynamixel/model_info.list";

  std::ifstream file(dynamixel_name_path_.c_str());
  if (file.is_open())
  {
    std::string input_str;

    while (!file.eof())
    {
      std::getline(file, input_str);

      // remove comment ( # )
      std::size_t pos = input_str.find("#");
      if (pos != std::string::npos)
      {
        input_str = input_str.substr(0,pos);
      }

      std::vector<std::string> tokens = split(input_str, '|');
      if (tokens.size() != 2)
        continue;

      if (model_number == std::atoi(tokens[0].c_str()))
      {
        model_number_ = model_number;
        model_name_ = tokens[1];
      }
    }
    file.close();
  }
  else
  {
    ROS_ERROR("Unable to open model_info file : %s", dynamixel_name_path_.c_str());
    ros::shutdown();
  }
}

bool DynamixelTool::getModelPath()
{
  std::string dynamixel_series = "";
  dynamixel_series = model_name_.substr(0,2);

  item_path_  = ros::package::getPath("dynamixel_workbench_toolbox") + "/dynamixel";

  item_path_ = item_path_ + "/models" + "/" + dynamixel_series + "/" + model_name_ + ".device";
}

bool DynamixelTool::getModelItem()
{
  getModelPath();

  std::ifstream file(item_path_.c_str());
  if (file.is_open())
  {
    std::string session = "";
    std::string input_str;

    while (!file.eof())
    {
      std::getline(file, input_str);

      // remove comment ( # )
      std::size_t pos = input_str.find("#");
      if (pos != std::string::npos)
      {
        input_str = input_str.substr(0,pos);
      }

      // trim
      input_str = trim(input_str);
      if (input_str == "")
        continue;

      // find session;
      if (!input_str.compare(0, 1, "[") && !input_str.compare(input_str.size()-1, 1, "]"))
      {
        input_str = input_str.substr(1, input_str.size()-2);
        std::transform(input_str.begin(), input_str.end(), input_str.begin(), ::tolower);
        session = trim(input_str);
        continue;
      }

      if (session == "type info")
      {
        std::vector<std::string> tokens = split(input_str, '=');
        if (tokens.size() != 2)
          continue;

        if (tokens[0] == "torque_to_current_value_ratio")
          torque_to_current_value_ratio_ = std::atof(tokens[1].c_str());
        else if (tokens[0] == "velocity_to_value_ratio")
          velocity_to_value_ratio_ = std::atof(tokens[1].c_str());
        else if (tokens[0] == "value_of_0_radian_position")
          value_of_0_radian_position_ = std::atoi(tokens[1].c_str());
        else if (tokens[0] == "value_of_min_radian_position")
          value_of_min_radian_position_ = std::atoi(tokens[1].c_str());
        else if (tokens[0] == "value_of_max_radian_position")
          value_of_max_radian_position_ = std::atoi(tokens[1].c_str());
        else if (tokens[0] == "min_radian")
          min_radian_ = std::atof(tokens[1].c_str());
        else if (tokens[0] == "max_radian")
          max_radian_ = std::atof(tokens[1].c_str());
      }
      else if (session == "baud rate")
      {
        std::vector<std::string> tokens = split(input_str, '|');
        if(tokens.size() != 2)
          continue;

        baud_rate_table_[std::atoi(tokens[0].c_str())] = std::atoi(tokens[1].c_str());
      }
      else if (session == "control table")
      {
        std::vector<std::string> tokens = split(input_str, '|');
        if(tokens.size() != 5)
          continue;

        ControlTableItem *item = new ControlTableItem;
        item->item_name = tokens[1];
        item->address = std::atoi(tokens[0].c_str());
        item->data_length = std::atoi(tokens[2].c_str());
        if(tokens[3] == "R")
            item->access_type = READ;
        else if(tokens[3] == "RW")
            item->access_type = READ_WRITE;
        if(tokens[4] == "EEPROM")
            item->memory_type = EEPROM;
        else if(tokens[4] == "RAM")
            item->memory_type = RAM;

        ctrl_table_[item->item_name] = item;
      }
    }
    file.close();
  }
  else
  {
    ROS_ERROR("Unable to open .device file : %s", item_path_.c_str());
    ros::shutdown();
  }
}
