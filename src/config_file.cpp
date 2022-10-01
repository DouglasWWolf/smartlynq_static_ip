//==========================================================================================================
// config_file.cpp - Implements a parser for configuration/settings files
//==========================================================================================================
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include "config_file.h"
#include "tokenizer.h"

using namespace std;

static double s_to_d(const string& s)  {return strtod(s.c_str(), NULL   );}

static int s_to_i(const string& s)
{
    char buffer[100], *out = buffer;
    int remaining = sizeof(buffer) - 1;
    const char* in = s.c_str();
    while (*in && remaining)
    {
        if (*in != '_')
        {
            *out++ = *in;
            --remaining;
        }
        ++in;
    }
    *out = 0;
    return strtol(buffer, NULL, 0);
}

static CTokenizer tokenizer;

//==========================================================================================================
// make_lower() - Converts a std::string to lower-case
//==========================================================================================================
static void make_lower(string& s)
{
    char* in = (char*)s.c_str();
    while (*in)
    {
        if (*in >= 'A' && *in <= 'Z') *in |= 32;
        ++in;
    }
}
//==========================================================================================================


//==========================================================================================================
// parse_bool() - Returns true if the indicated string is a non-zero number or the string "true"
//==========================================================================================================
static bool parse_bool(const char* in)
{
    // A non-zero numeric value always means 'true'
    if (*in >= '1' && *in <= '9') return true;

    // Get a lower-case version of the input string
    string s = in;
    make_lower(s);

    // The word "true" always means 'true'
    if (s == "true") return true;

    // The word "on" always means 'true'
    if (s == "on") return true;

    // Anything else means 'false'
    return false;
}
//==========================================================================================================


//==========================================================================================================
// decode() - Converts a std::string into some other type
//==========================================================================================================
static void decode(const string& s, int32_t *p_result) {*p_result = (int32_t)s_to_i(s);}
static void decode(const string& s, double  *p_result) {*p_result = s_to_d(s);}
static void decode(const string& s, string  *p_result) {*p_result = s;}
static void decode(const string& s, bool    *p_result) {*p_result = parse_bool(s.c_str());}
//==========================================================================================================



//==========================================================================================================
// parse_to_delimeter() - Returns a string of characters up to (but not including) a space or a delimeter
//
// Passed: in = Pointer the first character of the string
//
// Returns: The parsed string
//==========================================================================================================
static string parse_to_delimeter(const char* in, char delimeter)
{
    char token[512], *out = token;

    // Skip past any leading spaces
    while (*in == ' ') ++in;

    // Loop through every character in the token...
    while (*in && *in != ' ' && *in != delimeter)
    {
        // Fetch the character
        int c = *in++;

        // Convert the character to lower-case
        if (c >= 'A' && c <= 'Z') c |= 32;

        // Append the character to the output string
        *out++ = c;
    }

    // nul-terminate the output string
    *out = 0;

    // Hand the caller the token
    return token;
}
//==========================================================================================================



//==========================================================================================================
// Call this to read the config file.  Returns 'true' on success, 'false' if file not found
//
// On Exit: m_specs = a container that maps a key-string to a vector of strings.
//                    That vector of strings is either individual tokens, or in the case of a script
//                    spec is a vector of untokenized lines
//==========================================================================================================
bool CConfigFile::read(string filename, bool msg_on_fail)
{
    char     line[1000], *p;
    strvec_t values;
    string   base_key_name, scoped_key_name;
    
    // We are not currently parsing a script
    bool in_script = false;

    // This will contain the current [section_name] being parsed
    string parsing_section;

    // Open the input file
    FILE* ifile = fopen(filename.c_str(), "r");

    // If the input file couldn't be opened, complain about it
    if (ifile == NULL)
    {
        if (msg_on_fail) printf("Failed to open file \"%s\"\n", filename.c_str());
        return false; 
    }       

    // Loop through every line of the input file...
    while (fgets(line, sizeof line, ifile))
    {
        // Chomp any end of line characters off the end of the line
        p = strchr(line, 10); if (p) *p = 0;
        p = strchr(line, 13); if (p) *p = 0;
        
        // Find the first non-space character in the line
        p = line;
        while (*p == ' ') ++p;

        // If the line is blank or is a comment, ignore it
        if (*p == 0 || *p == '#' || (p[0] == '/' && p[1] == '/')) continue;

        // If the line begins with '[', this is a section-name
        if (*p == '[')
        {
            parsing_section = parse_to_delimeter(p+1, ']');
            continue;
        }

        // If this is the beginning of a script, we will start recording entire lines
        if (*p == '{')
        {
            values.clear();
            in_script = true;
            continue;
        }

        // If this is the end of a script, save the list of lines into our specs
        if (*p == '}')
        {
            if (in_script) m_specs[scoped_key_name] = values;
            in_script = false;
            continue;            
        }

        // If we're parsing a script, just save the line
        if (in_script)
        {
            values.push_back(p);
            continue;
        }

        // Fetch the base name of this key 
        base_key_name = parse_to_delimeter(p, '=');

        // Create the fully scoped name of this key
        scoped_key_name = parsing_section + "::" + base_key_name;

        // We start out without a list of values for this key
        values.clear();

        // Find the equal sign on this line
        p = strchr(p, '=');

        // If it exists, parse the rest of the line after an '=' into a vector of string tokens    
        if (p) values = tokenizer.parse(p+1);

        // Add this configuration spec to our master list of config specs
        m_specs[scoped_key_name] = values;
       
    }

    // We're done with the input file
    fclose(ifile);

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================



//==========================================================================================================
// set_current_section() - Sets the section-name to look for keys in
//==========================================================================================================
void  CConfigFile::set_current_section(string section)
{
    m_current_section = section;
    make_lower(m_current_section);
}
//==========================================================================================================


//==========================================================================================================
// dump_specs() - Displays the m_specs map in human-readable form for debugging
//==========================================================================================================
void CConfigFile::dump_specs()
{
    map<string, strvec_t>::iterator it;

    // Loop through every entry in our map....
    for (it=m_specs.begin(); it != m_specs.end(); ++it)
    {
        // Get a convenient reference to string-vector in this entry
        strvec_t& v = it->second;

        // Display this item's key
        printf("Key \"%s\"\n", it->first.c_str());
        
        // Display every value associated with this item
        for (int i=0; i<v.size(); ++i) printf("   \"%s\"\n", v[i].c_str());
    }
}
//==========================================================================================================



//==========================================================================================================
// lookup() - Like "exists()", but can throw a runtime_error exception if the key is not found
//==========================================================================================================
bool CConfigFile::lookup(string key, strvec_t *p_result)
{
    // Find out if this key exists
    bool key_exists = exists(key, p_result);

    // If it exists, we're done
    if (key_exists) return true;

    // If it doesn't exist and this should throw an error, do so
    if (m_throw_on_fail) throw runtime_error("config key '"+key+"' not found");

    // Otherwise, just report the failure via the return value
    return false;
}
//==========================================================================================================



//==========================================================================================================
// exists() - Checks to see if a given key exists in our spec-map and optionally retrieves the values
//
// Passed: key      = Key to look up.   Can optionally be fully scoped
//         p_result = A pointer to the strvec where the specified key's values should be stored
//
// Returns: true if that key exists in our map, otherwise false
//
// This routine will never throw an exception.   If you need a version that throws an exception when
// the key isn't found, try "lookup"
//==========================================================================================================
bool CConfigFile::exists(string key, strvec_t *p_result)
{
    // An iterator to our specs-map
    map<string, strvec_t>::iterator it;

    // Convert the key to lower-case
    make_lower(key);

    // If the caller gave us a pointer to a result vector, clear it
    if (p_result) p_result->clear();

    // If the caller gave us a fully-scoped name...
    if (key.find("::") != string::npos)
    {
        // Do we have a key by that name?
        it = m_specs.find(key);
        
        // If we have the specified key...
        if (it != m_specs.end())
        {
            // If the caller wants the associated values, hand them to him
            if (p_result) *p_result = it->second;
        
            // Tell the caller that his key existed
            return true;
        }

        // If we get here, the fully-scoped key wasn't found in our map
        return false;
    }

    // Does the current section have a key by that name?
    it = m_specs.find(m_current_section + "::" + key);
        
    // If that fully-scoped key exists, tell the caller
    if (it != m_specs.end())
    {
        if (p_result) *p_result = it->second;
        return true;
    }

    // Does the global section have a key by that name?
    it = m_specs.find("::" + key);
        
    // If that globally-scoped key exists, tell the caller
    if (it != m_specs.end())
    {
        if (p_result) *p_result = it->second;
        return true;
    }

    // Tell the caller that we couldn't find that key in our specs
    return false;
}
//==========================================================================================================


//==========================================================================================================
// Call this to fetch a variable-type configuration spec
// 
// If key doesn't exist in our map, this either returns false, or throws a std::runtime_error
//==========================================================================================================
bool CConfigFile::get(string key, string fmt, void* p1, void* p2, void* p3, void* p4, void* p5
                                            , void* p6, void* p7, void* p8, void* p9)
{
    strvec_t  values;
    char      format = 'i';
    const int field_count = 9;

    // Convert the caller's output pointers to an array
    void* output[] = {p1, p2, p3, p4, p5, p6, p7, p8, p9};

    // How many format-specifiers are there?
    int format_count = fmt.size();

    // This is the current index into 'fmt'
    int format_index = -1;

    // Fetch the values assocated with this key
    if (!lookup(key, &values)) return false;

    // Loop through each value associated with this key
    for (int i=0; i<field_count; ++i)
    {
        // Fetch a pointer to the field where we should store this value
        void* field = output[i];

        // If the caller didn't specify a field for this parameter, we're done
        if (field == NULL) break;

        // Fetch the next available format specifier from 'fmt'
        if (++format_index < format_count) format = fmt[format_index];

        // Fetch the next value for this key, being sure to not run off the end of the vector
        string value = (i >= values.size()) ? "" : values[i].c_str();

        // Parse this value into the appropriate data type in the caller's output field 
        switch(format)
        {
            case 'i':   decode(value, (int32_t*)field);
                        break;

            case 'f':   decode(value, (double*)field);
                        break;

            case 's':   decode(value, (string*)field);
                        break;

            case 'b':   decode(value, (bool*)field);
                        break;
        }

    }

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================



//==========================================================================================================
// get() - These fetch up to 9 parameters of a given type
//
// If key doesn't exist in our map, these either return false, or throw a std::runtime_error
//==========================================================================================================
bool CConfigFile::get(string key, int32_t* p1, int32_t* p2, int32_t* p3, int32_t* p4, int32_t* p5,
                                  int32_t* p6, int32_t* p7, int32_t* p8, int32_t* p9)
{
    return get(key, "i", p1, p2, p3, p4, p5, p6, p7, p8, p9);
}



bool CConfigFile::get(string key, double* p1, double* p2, double* p3, double* p4, double* p5,
                                  double* p6, double* p7, double* p8, double* p9)
{
    return get(key, "f", p1, p2, p3, p4, p5, p6, p7, p8, p9);
}


bool CConfigFile::get(string key, string* p1, string* p2, string* p3, string* p4, string* p5,
                                  string* p6, string* p7, string* p8, string* p9)
{
    return get(key, "s", p1, p2, p3, p4, p5, p6, p7, p8, p9);
}


bool CConfigFile::get(string key, bool* p1, bool* p2, bool* p3, bool* p4, bool* p5,
                                  bool* p6, bool* p7, bool* p8, bool* p9)
{
    return get(key, "b", p1, p2, p3, p4, p5, p6, p7, p8, p9);
}
//==========================================================================================================



//==========================================================================================================
// get() - Fetches a vector of values associated with the specified key
//
// If key doesn't exist in our map, these either return false, or throw a std::runtime_error
//==========================================================================================================
bool CConfigFile::get(string key, vector<double> *p_result)
{
    double      value;
    strvec_t    values;

    // Clear the caller's result vector
    p_result->clear();

    // Fetch the values assocated with this key
    if (!lookup(key, &values)) return false;

    // For each string value that is associated with this key...
    for (int i=0; i<values.size(); ++i)
    {
        // Get a handy reference to this entry
        string& s = values[i];

        // Decode the string into a native value
        decode(s, &value);

        // Add append that value to the caller's result vector
        p_result->push_back(value);
    }

    // Tell the caller that all is well
    return true;
}

bool CConfigFile::get(string key, vector<int32_t> *p_result)
{
    int32_t      value;
    strvec_t    values;

    // Clear the caller's result vector
    p_result->clear();

    // Fetch the values assocated with this key
    if (!lookup(key, &values)) return false;

    // For each string value that is associated with this key...
    for (int i=0; i<values.size(); ++i)
    {
        // Get a handy reference to this entry
        string& s = values[i];

        // Decode the string into a native value
        decode(s, &value);

        // Add append that value to the caller's result vector
        p_result->push_back(value);
    }

    // Tell the caller that all is well
    return true;
}

bool CConfigFile::get(string key, vector<string> *p_result)
{
    string      value;
    strvec_t    values;

    // Clear the caller's result vector
    p_result->clear();

    // Fetch the values assocated with this key
    if (!lookup(key, &values)) return false;

    // For each string value that is associated with this key...
    for (int i=0; i<values.size(); ++i)
    {
        // Get a handy reference to this entry
        string& s = values[i];

        // Decode the string into a native value
        decode(s, &value);

        // Add append that value to the caller's result vector
        p_result->push_back(value);
    }

    // Tell the caller that all is well
    return true;
}

bool CConfigFile::get(string key, vector<bool> *p_result)
{
    bool        value;
    strvec_t    values;

    // Clear the caller's result vector
    p_result->clear();

    // Fetch the values assocated with this key
    if (!lookup(key, &values)) return false;

    // For each string value that is associated with this key...
    for (int i=0; i<values.size(); ++i)
    {
        // Get a handy reference to this entry
        string& s = values[i];

        // Decode the string into a native value
        decode(s, &value);

        // Add append that value to the caller's result vector
        p_result->push_back(value);
    }

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================


//==========================================================================================================
// get() - Fetches the script-spec that is associated with the specified key
//
// If key doesn't exist in our map, this either returns false, or throws a std::runtime_error
//==========================================================================================================
bool CConfigFile::get(string key, CConfigScript* p_script)
{
    strvec_t script_lines;

    // Make the caller's script empty for the moment
    p_script->make_empty();

    // Fetch the values assocated with this key
    if (!lookup(key, &script_lines)) return false;

    // Fill in the caller's script
    *p_script = script_lines;

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================


//==========================================================================================================
// get_script_vector() - Fetches the script-spec that is associated with the specified key
//
// If key doesn't exist in our map, this either returns false, or throws a std::runtime_error
//==========================================================================================================
bool CConfigFile::get_script_vector(string key, vector<string>* p_script)
{
    strvec_t script_lines;

    // Make the caller's script empty for the moment
    p_script->clear();

    // Fetch the values assocated with this key
    if (!lookup(key, &script_lines)) return false;

    // Fill in the caller's script
    *p_script = script_lines;

    // Tell the caller that all is well
    return true;
}
//==========================================================================================================





//==========================================================================================================
// make_empty() - empties the script object of all data
//==========================================================================================================
void CConfigScript::make_empty()
{
    m_script.clear();
    m_tokens.clear();
    m_line_index = m_token_index = 0;
}
//==========================================================================================================




//==========================================================================================================
// get_next_line() - Fetches the next line of the script for processing
//==========================================================================================================
bool CConfigScript::get_next_line(int *p_token_count, string *p_text)
{
    // If we're out of script lines, tell the caller
    if (m_line_index >= m_script.size())
    {
        if (p_text) *p_text = "";
        return false;
    }

    // If the caller wants the script line, fill in the caller's field
    if (p_text) *p_text = m_script[m_line_index];

    // Parse this line into tokens
    m_tokens = tokenizer.parse(m_script[m_line_index++]);

    // If the caller wants to know how many tokens there are, fill in their field
    if (p_token_count) *p_token_count = m_tokens.size();

    // The next call to "get_next_<token|int|float>" will start at the first token
    m_token_index = 0;

    // Tell the caller that their script line is available
    return true;
}
//==========================================================================================================


//==========================================================================================================
// get_next_token() - Fetches the next token from the current line
//==========================================================================================================
string CConfigScript::get_next_token(bool force_lowercase)
{
    // If there are no more tokens, return an empty string
    if (m_token_index >= m_tokens.size()) return "";

    // Fetch the result string
    string token = m_tokens[m_token_index++];

    // If this caller wants this token in all lowercase, make it so
    if (force_lowercase) make_lower(token);

    // Hand the token to the caller
    return token;
}
//==========================================================================================================



//==========================================================================================================
// get_next_int() - Fetches the next integer from the current line
//==========================================================================================================
int32_t CConfigScript::get_next_int()
{
    int32_t result;

    // If there are no more tokens, return an empty string
    if (m_token_index >= m_tokens.size()) return 0;

    // Fetch the result string
    string token = m_tokens[m_token_index++];

    // Decode the token into an integer
    decode(token, &result);

    // Hand the result to the caller
    return result;
}
//==========================================================================================================



//==========================================================================================================
// get_next_float() - Fetches the next double from the current line
//==========================================================================================================
double CConfigScript::get_next_float()
{
    double result;

    // If there are no more tokens, return an empty string
    if (m_token_index >= m_tokens.size()) return 0;

    // Fetch the result string
    string token = m_tokens[m_token_index++];

    // Decode the token into an double
    decode(token, &result);

    // Hand the result to the caller
    return result;
}
//==========================================================================================================


