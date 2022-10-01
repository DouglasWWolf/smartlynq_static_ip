//=========================================================================================================
// tokenizer.h - Defines A class that tokenizes strings
//=========================================================================================================
#pragma once
#include <string>
#include <vector>


class CTokenizer
{
public:
    std::vector<std::string> parse(const std::string& input);
};