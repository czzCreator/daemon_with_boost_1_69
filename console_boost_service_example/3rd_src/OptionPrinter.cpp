/**********************************************************************************************************************
**         __________              ___                              ________                                         **
**         \______   \_____     __| _/ _____  _____     ____       /  _____/ _____     _____    ____    ______       **
**          |       _/\__  \   / __ | /     \ \__  \   /    \     /   \  ___ \__  \   /     \ _/ __ \  /  ___/       **
**          |    |   \ / __ \_/ /_/ ||  Y Y  \ / __ \_|   |  \    \    \_\  \ / __ \_|  Y Y  \\  ___/  \___ \        **
**          |____|_  /(____  /\____ ||__|_|  /(____  /|___|  /     \______  /(____  /|__|_|  / \___  \/____  \       **
**                 \/      \/      \/      \/      \/      \/             \/      \/       \/      \/      \/        **
**                                                         2012                                                      **
**********************************************************************************************************************/

#include "OptionPrinter.hpp"

#include "boost/algorithm/string/regex.hpp"

namespace rad
{
//---------------------------------------------------------------------------------------------------------------------
  void OptionPrinter::addOption(const CustomOptionDescription& optionDesc)
  {
    optionDesc.isPositional_ ? positionalOptions_.push_back(optionDesc) : options_.push_back(optionDesc);

  }

//---------------------------------------------------------------------------------------------------------------------
  std::string OptionPrinter::usage()
  {
    std::stringstream usageDesc;
    /** simple flags that have a short version
     */
    bool firstShortOption = true;
    usageDesc << "[";
    for (std::vector<rad::CustomOptionDescription>::iterator it = options_.begin();
         it != options_.end();
         ++it)
    {
      if ( it->hasShort_ && ! it->hasArgument_ && ! it->required_ )
      {
        if (firstShortOption)
        {
          usageDesc << "-";
          firstShortOption = false;
        }

        usageDesc << it->optionDisplayName_[1];
      }

    }
    usageDesc << "] ";

    /** simple flags that DO NOT have a short version
     */
    for (std::vector<rad::CustomOptionDescription>::iterator it = options_.begin();
         it != options_.end();
         ++it)
    {
      if ( ! it->hasShort_ && ! it->hasArgument_ && ! it->required_ )
      {
        usageDesc << "[" << it->optionDisplayName_ << "] ";
      }

    }

    /** options with arguments
     */
    for (std::vector<rad::CustomOptionDescription>::iterator it = options_.begin();
         it != options_.end();
         ++it)
    {
      if ( it->hasArgument_ && ! it->required_ )
      {
        usageDesc << "[" << it->optionDisplayName_ << " ARG] ";
      }

    }

    /** required options with arguments
     */
    for (std::vector<rad::CustomOptionDescription>::iterator it = options_.begin();
         it != options_.end();
         ++it)
    {
      if ( it->hasArgument_ && it->required_ )
      {
        usageDesc << it->optionDisplayName_ << " ARG ";
      }

    }

    /** positional option
     */
    for (std::vector<rad::CustomOptionDescription>::iterator it = positionalOptions_.begin();
         it != positionalOptions_.end();
         ++it)
    {
      usageDesc << it->optionDisplayName_ << " ";

    }

    return usageDesc.str();

  }

//---------------------------------------------------------------------------------------------------------------------
  std::string OptionPrinter::positionalOptionDetails()
  {
    std::stringstream output;
    for (std::vector<rad::CustomOptionDescription>::iterator it = positionalOptions_.begin();
         it != positionalOptions_.end();
         ++it)
    {
      output << it->getOptionUsageString() << std::endl;
    }

    return output.str();
  }

//---------------------------------------------------------------------------------------------------------------------
  std::string OptionPrinter::optionDetails()
  {
    std::stringstream output;
    for (std::vector<rad::CustomOptionDescription>::iterator it = options_.begin();
         it != options_.end();
         ++it)
    {
      output << it->getOptionUsageString() << std::endl;

    }

    return output.str();
  }

//---------------------------------------------------------------------------------------------------------------------
  void OptionPrinter::printStandardAppDesc(const std::string& appName,
                                           std::ostream& out,
                                           boost::program_options::options_description desc,
                                           boost::program_options::positional_options_description* positionalDesc)
  {
    rad::OptionPrinter optionPrinter;

    typedef std::vector<boost::shared_ptr<boost::program_options::option_description > > Options;
    Options allOptions = desc.options();
    for (Options::iterator it = allOptions.begin();
         it != allOptions.end();
         ++it)
    {
      rad::CustomOptionDescription currOption(*it);
      if ( positionalDesc )
      {
        currOption.checkIfPositional(*positionalDesc);
      }

      optionPrinter.addOption(currOption);

    } // foreach option



    //跨平台打印信息
#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
   #else
      //define something for Windows (32-bit only)

    out << "USAGE: " << appName << " " << optionPrinter.usage() << "\r\n" <<std::endl
        << "\r\n" <<std::endl
        << "-- Option Descriptions --" << "\r\n" <<std::endl
        << "\r\n" <<std::endl
        << "Positional arguments:" << "\r\n" <<std::endl
        << optionPrinter.positionalOptionDetails()
        << "\r\n" <<std::endl
        << "Option Arguments: " << "\r\n" <<std::endl
        << optionPrinter.optionDetails()
        << "\r\n" <<std::endl;
   #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    // android
#elif __linux__
    out << "USAGE: " << appName << " " << optionPrinter.usage() << std::endl
        << std::endl
        << "-- Option Descriptions --" << std::endl
        << std::endl
        << "Positional arguments:" << std::endl
        << optionPrinter.positionalOptionDetails()
        << std::endl
        << "Option Arguments: " << std::endl
        << optionPrinter.optionDetails()
        << std::endl;
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif
  }

//---------------------------------------------------------------------------------------------------------------------
  void OptionPrinter::formatRequiredOptionError(boost::program_options::required_option& error)
  {
    std::string currOptionName = error.get_option_name();
    boost::algorithm::erase_regex(currOptionName, boost::regex("^-+"));
    error.set_option_name(currOptionName);

  }

//---------------------------------------------------------------------------------------------------------------------

} // namespace
