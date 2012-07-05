#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Logger.h"

// Global convenience methods
float getParam(const std::string &param);
const std::string& strParam(const std::string &param);

void setParam(const std::string &key, float value);
void setParam(const std::string &key, const std::string &value);


class ParamReader
{
public:
    static ParamReader *get();

    void loadFile(const char *filename);

    float getFloat(const std::string &key) const;
    const std::string& getString(const std::string &key) const;

    bool hasFloat(const std::string &key) const;
    bool hasString(const std::string &key) const;

    // Sets a given parameter, overwrites if the param already exits
    void setParam(const std::string &key, float value);
    void setParam(const std::string &key, const std::string &value);

    void printParams() const;

private:
    ParamReader();
    LoggerPtr logger_;

    std::map<std::string, float> floatParams_;
    std::map<std::string, std::string> stringParams_;
};

