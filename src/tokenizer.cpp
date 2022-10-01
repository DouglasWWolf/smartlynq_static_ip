
//=========================================================================================================
// tokenizer.cpp - Implements a class that tokenizes strings
//=========================================================================================================
#include "tokenizer.h"
using namespace std;

// is_eol() - checks to for an end-of-line character (nul, LF, or CR)
static bool is_eol(char c) {return c == 0 || c == 10 || c == 13;}

// is_ws() - Checks for a whitespace character (space or tab)
static bool is_ws(char c) {return c == 32 || c == 9;}



//==========================================================================================================
// parse() - Parses an input string into a vector of tokens
//==========================================================================================================
vector<string> CTokenizer::parse(const string& input)
{
    vector<string> result;
    char           token[512];

    // Fetch a const char* to the input string
    const char* in = input.c_str();

    // So long as there are input characters still to be processed...
    while (!is_eol(*in))
    {
        // Point to the output buffer 
        char* out = token;

        // Skip over any leading spaces on the input
        while (is_ws(*in)) in++;

        // If we hit end-of-line, there are no more tokens to parse
        if (is_eol(*in)) break;

        // Assume for the moment that we're not starting a quoted string
        char in_quotes = 0;

        // If this is a single or double quote-mark, remember it and skip past it
        if (*in == '"' || *in == '\'') in_quotes = *in++;

        // Loop until we've parsed this entire token...
        while (!is_eol(*in))
        {
            // If we're parsing a quoted string...
            if (in_quotes)
            {
                // If we've hit the ending quote-mark, we're done parsing this token
                if (*in == in_quotes)
                {
                    ++in;
                    break;
                }
            }

            // Otherwise, we're not parsing a quoted string. A space or comma ends the token
            else if (is_ws(*in) || *in == ',') break;

            // Append this character to the token buffer
            *out++ = *in++;
        }

        // nul-terminate the token string
        *out = 0;

        // Add the token to our result list
        result.push_back(token);

        // Skip over any trailing spaces in the input
        while (is_ws(*in)) ++in;

        // If there is a trailing comma, throw it away
        if (*in == ',') ++in;
    }

    // Hand the caller a vector of tokens
    return result;
}
//==========================================================================================================
