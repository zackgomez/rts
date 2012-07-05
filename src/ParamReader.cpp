#include "ParamReader.h"
#include <cassert>

ParamReader::ParamReader()
{
    logger_ = Logger::getLogger("ParamReader");
}

ParamReader * ParamReader::get()
{
    static ParamReader pr;
    return &pr;
}

void ParamReader::loadFile(const char *filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        logger_->fatal() << "UNABLE TO OPEN " << filename << " to read params\n";
        assert(false);
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::string key;
        float fval;
        std::string sval;

        std::stringstream ss(line);
        ss >> key;
        
        assert(!key.empty());
        // Ignore comments that start with #
        if (key[0] == '#')
            continue;

        // Fail on double key read
        if (floatParams_.count(key) || stringParams_.count(key))
        {
            logger_->fatal() << "Overwritting param " << key << '\n';
            assert(false);
        }


        // Try to read as float first
        ss >> fval;
        if (ss)
        {
            floatParams_[key] = fval;
        }
        // if it doesn't work, read as a string parameter
        // NOTE this only reads single word values
        else
        {
            // You MUST clear the fail bit or receive no input
            ss.clear();
            std::getline(ss, sval);
            //logger_->debug() << "Read [string] param '" << key << "' : '" << sval << "'\n";
            stringParams_[key] = sval;
        }
    }
}

float ParamReader::getFloat(const std::string &key) const
{
    std::map<std::string, float>::const_iterator it = floatParams_.find(key);
    if (it == floatParams_.end())
    {
        logger_->fatal() << "Unable to find [float] key " << key << '\n';
        assert(false && "Key not found in params");
    }
    else
        return it->second;
}

const std::string & ParamReader::getString(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = stringParams_.find(key);
    if (it == stringParams_.end())
    {
        logger_->fatal() << "Unable to find [string] key " << key << '\n';
        assert(false && "Key not found in params");
    }
    else
        return it->second;
}

void ParamReader::setParam(const std::string &key, float value)
{
    floatParams_[key] = value;
}

void ParamReader::setParam(const std::string &key, const std::string &value)
{
    stringParams_[key] = value;
}

bool ParamReader::hasFloat(const std::string &key) const
{
    return floatParams_.find(key) != floatParams_.end();
}

bool ParamReader::hasString(const std::string &key) const
{
    return stringParams_.find(key) != stringParams_.end();
}

void ParamReader::printParams() const
{
    logger_->info() << "Params:\n";
    std::map<std::string, float>::const_iterator it;
    for (it = floatParams_.begin(); it != floatParams_.end(); it++)
    {
        logger_->info() << it->first << ' ' << it->second << '\n';
    }
}

float getParam(const std::string &param)
{
    return ParamReader::get()->getFloat(param);
}

const std::string& strParam(const std::string &param)
{
    return ParamReader::get()->getString(param);
}

void setParam(const std::string &key, float value)
{
    ParamReader::get()->setParam(key, value);
}

void setParam(const std::string &key, const std::string &value)
{
    ParamReader::get()->setParam(key, value);
}

